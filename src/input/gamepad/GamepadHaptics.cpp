#include "GamepadHaptics.h"

#include <algorithm>

// NOMINMAX: stops Windows.h from defining its own min/max macros, which
// would otherwise shadow std::min/std::max (used below) and silently break
// them anywhere this header is included.
#ifndef NOMINMAX
#define NOMINMAX
#endif

// WIN32_LEAN_AND_MEAN: excludes the rarely-used parts of Windows.h (WinSock
// 1, GDI, shell, RPC, cryptography...) that this file never touches --
// keeps the include lightweight and avoids the classic WinSock1/WinSock2
// macro collisions some of those unused headers are prone to.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <Xinput.h>

#include <DualSenseWindows/DSW_Api.h>
#include <DualSenseWindows/IO.h>

namespace Haptics
{
	namespace
	{
		// How often (in seconds) to scan for a newly connected controller
		// while none is currently active. Disconnects of an already-active
		// controller are instead caught reactively, the moment a send to it
		// fails -- re-enumerating/reopening a DualSense's HID handle every
		// second while nothing has changed would be wasteful (and could
		// visibly flicker its lightbar), and XInput slots are just as cheap
		// to notice as "gone" on their next failed send.
		constexpr float ConnectionRecheckInterval = 1.f;

		// XInput exposes controllers on 4 fixed slots; this project has no
		// local multiplayer, so the first connected slot found is always
		// the one to drive.
		constexpr unsigned long MaximumXInputUserIndex = 3u;

		constexpr unsigned int MaximumDualSenseDevices = 4u;

		constexpr float MotorSpeedScale = 65535.f;

		// How long a single regular-shot recoil kick lasts before releasing
		// back to whatever the sustained state is.
		constexpr float RightTriggerRecoilDuration = 0.08f;

		// A real trigger motor can't shove a finger backwards -- it can only
		// resist -- so the "kick" AAA shooters feel on every shot isn't a
		// resistance band, it's a short, sharp burst of vibration right
		// under the fingertip (EffectEx's frequency parameter). Force is
		// applied at both the begin (>=127) and middle (<=127) bands so it
		// still buzzes regardless of exactly how far the trigger is held in
		// while firing.
		constexpr unsigned char RightTriggerRecoilStartPosition = 0u;
		constexpr unsigned char RightTriggerRecoilForce = 255u;
		constexpr unsigned char RightTriggerRecoilFrequency = 220u;

		// Constant resistance felt across the whole pull while the laser is
		// held -- heavy, but not so stiff it fights the trigger being held
		// down for a sustained beam.
		constexpr unsigned char RightTriggerSustainedForce = 140u;

		[[nodiscard]] unsigned char ToByte(float normalizedValue)
		{
			return static_cast<unsigned char>(std::clamp(normalizedValue, 0.f, 1.f) * 255.f);
		}

		[[nodiscard]] DWORD SendXInputVibration(unsigned long userIndex, float lowFrequencyMotor, float highFrequencyMotor)
		{
			XINPUT_VIBRATION vibration{};
			vibration.wLeftMotorSpeed = static_cast<WORD>(std::clamp(lowFrequencyMotor, 0.f, 1.f) * MotorSpeedScale);
			vibration.wRightMotorSpeed = static_cast<WORD>(std::clamp(highFrequencyMotor, 0.f, 1.f) * MotorSpeedScale);

			return XInputSetState(userIndex, &vibration);
		}
	}

	GamepadHaptics::GamepadHaptics()
	{
		RefreshConnection();
	}

	GamepadHaptics::~GamepadHaptics()
	{
		if (connectedType == ConnectedControllerType::Xbox)
		{
			static_cast<void>(SendXInputVibration(xboxUserIndex, 0.f, 0.f));
		}
		else if (connectedType == ConnectedControllerType::DualSense)
		{
			DS5W::DS5OutputState offState{};
			DS5W::setDeviceOutputState(&dualSenseContext, &offState);
			DS5W::freeDeviceContext(&dualSenseContext);
		}
	}

	void GamepadHaptics::SetVibrationEnabled(bool newIsVibrationEnabled) noexcept
	{
		isVibrationEnabled = newIsVibrationEnabled;
	}

	bool GamepadHaptics::IsVibrationEnabled() const noexcept
	{
		return isVibrationEnabled;
	}

	void GamepadHaptics::SetLightbarEnabled(bool newIsLightbarEnabled) noexcept
	{
		isLightbarEnabled = newIsLightbarEnabled;
	}

	bool GamepadHaptics::IsLightbarEnabled() const noexcept
	{
		return isLightbarEnabled;
	}

	void GamepadHaptics::SetAdaptiveTriggersEnabled(bool newIsAdaptiveTriggersEnabled) noexcept
	{
		isAdaptiveTriggersEnabled = newIsAdaptiveTriggersEnabled;
	}

	bool GamepadHaptics::IsAdaptiveTriggersEnabled() const noexcept
	{
		return isAdaptiveTriggersEnabled;
	}

	void GamepadHaptics::PulseVibration(float lowFrequencyMotor, float highFrequencyMotor, float durationSeconds)
	{
		if (durationSeconds <= 0.f)
			return;

		pulseDuration = std::max(pulseDuration, durationSeconds);
		pulseRemaining = std::max(pulseRemaining, durationSeconds);
		pulseLowMotor = std::max(pulseLowMotor, std::clamp(lowFrequencyMotor, 0.f, 1.f));
		pulseHighMotor = std::max(pulseHighMotor, std::clamp(highFrequencyMotor, 0.f, 1.f));
	}

	void GamepadHaptics::SetLightbarColor(RGBColor color) noexcept
	{
		lightbarColor = color;
	}

	void GamepadHaptics::PulseRightTriggerRecoil()
	{
		rightTriggerRecoilRemaining = RightTriggerRecoilDuration;
	}

	void GamepadHaptics::SetRightTriggerSustainedResistance(bool active) noexcept
	{
		isRightTriggerSustainedResistanceActive = active;
	}

	void GamepadHaptics::Update(float deltaTime)
	{
		connectionRecheckRemaining -= deltaTime;

		if (connectionRecheckRemaining <= 0.f)
		{
			RefreshConnection();
			connectionRecheckRemaining = ConnectionRecheckInterval;
		}

		if (pulseRemaining > 0.f)
			pulseRemaining = std::max(0.f, pulseRemaining - deltaTime);

		if (rightTriggerRecoilRemaining > 0.f)
			rightTriggerRecoilRemaining = std::max(0.f, rightTriggerRecoilRemaining - deltaTime);

		const float FallOff = (pulseDuration > 0.f && pulseRemaining > 0.f) ? pulseRemaining / pulseDuration : 0.f;

		if (FallOff <= 0.f)
		{
			pulseDuration = 0.f;
			pulseLowMotor = 0.f;
			pulseHighMotor = 0.f;
		}

		const float lowMotor = isVibrationEnabled ? pulseLowMotor * FallOff : 0.f;
		const float highMotor = isVibrationEnabled ? pulseHighMotor * FallOff : 0.f;

		ApplyVibration(lowMotor, highMotor);
	}

	void GamepadHaptics::RefreshConnection()
	{
		// Disconnects of an already-active controller surface reactively in
		// ApplyVibration (see the comment on ConnectionRecheckInterval), so
		// there's nothing to re-verify here while one is still marked connected.
		if (connectedType != ConnectedControllerType::None)
			return;

		if (RefreshXboxConnection())
		{
			connectedType = ConnectedControllerType::Xbox;
			return;
		}

		if (RefreshDualSenseConnection())
			connectedType = ConnectedControllerType::DualSense;
	}

	bool GamepadHaptics::RefreshXboxConnection()
	{
		for (unsigned long userIndex = 0u; userIndex <= MaximumXInputUserIndex; userIndex++)
		{
			XINPUT_STATE state{};
			if (XInputGetState(userIndex, &state) == ERROR_SUCCESS)
			{
				xboxUserIndex = userIndex;
				return true;
			}
		}

		return false;
	}

	bool GamepadHaptics::RefreshDualSenseConnection()
	{
		DS5W::DeviceEnumInfo devices[MaximumDualSenseDevices]{};
		unsigned int deviceCount = 0u;

		if (DS5W_FAILED(DS5W::enumDevices(devices, MaximumDualSenseDevices, &deviceCount)) || deviceCount == 0u)
			return false;

		return DS5W_SUCCESS(DS5W::initDeviceContext(&devices[0], &dualSenseContext));
	}

	void GamepadHaptics::DisconnectDualSense()
	{
		DS5W::freeDeviceContext(&dualSenseContext);
		dualSenseContext = DS5W::DeviceContext{};
		connectedType = ConnectedControllerType::None;
	}

	void GamepadHaptics::ApplyVibration(float lowFrequencyMotor, float highFrequencyMotor)
	{
		switch (connectedType)
		{
		case ConnectedControllerType::Xbox:
			if (SendXInputVibration(xboxUserIndex, lowFrequencyMotor, highFrequencyMotor) != ERROR_SUCCESS)
				connectedType = ConnectedControllerType::None;
			break;

		case ConnectedControllerType::DualSense:
		{
			DS5W::DS5OutputState outputState{};
			outputState.leftRumble = ToByte(lowFrequencyMotor);
			outputState.rightRumble = ToByte(highFrequencyMotor);

			if (isLightbarEnabled)
				outputState.lightbar = { lightbarColor.r, lightbarColor.g, lightbarColor.b };

			outputState.rightTriggerEffect = BuildRightTriggerEffect();

			if (DS5W_FAILED(DS5W::setDeviceOutputState(&dualSenseContext, &outputState)))
				DisconnectDualSense();

			break;
		}

		// Deliberately listed rather than left to a default: -- with every
		// enumerator handled explicitly, the compiler warns if this enum
		// ever gains a value and this switch isn't updated for it.
		case ConnectedControllerType::None:
			break;
		}
	}

	DS5W::TriggerEffect GamepadHaptics::BuildRightTriggerEffect() const noexcept
	{
		DS5W::TriggerEffect effect{};

		if (!isAdaptiveTriggersEnabled)
		{
			effect.effectType = DS5W::TriggerEffectType::NoResitance;
		}
		else if (rightTriggerRecoilRemaining > 0.f)
		{
			effect.effectType = DS5W::TriggerEffectType::EffectEx;
			effect.EffectEx.startPosition = RightTriggerRecoilStartPosition;
			effect.EffectEx.keepEffect = true;
			effect.EffectEx.beginForce = RightTriggerRecoilForce;
			effect.EffectEx.middleForce = RightTriggerRecoilForce;
			effect.EffectEx.endForce = 0u;
			effect.EffectEx.frequency = RightTriggerRecoilFrequency;
		}
		else if (isRightTriggerSustainedResistanceActive)
		{
			effect.effectType = DS5W::TriggerEffectType::ContinuousResitance;
			effect.Continuous.startPosition = 0u;
			effect.Continuous.force = RightTriggerSustainedForce;
		}
		else
		{
			effect.effectType = DS5W::TriggerEffectType::NoResitance;
		}

		return effect;
	}
}