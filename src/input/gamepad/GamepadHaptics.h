#pragma once

#include <DualSenseWindows/DS5State.h>
#include <DualSenseWindows/Device.h>

// DualSense/DualShock support (rumble, lightbar, adaptive triggers) is built
// on top of Ohjurot/DualSense-Windows (MIT), vendored in full under
// libs/DualSenseWindows/. Windows exposes no public vendor API for these
// controllers -- XInput doesn't cover them at all -- so this library talks
// to the hardware directly over raw HID (SetupAPI/hid.lib), matching it by
// its USB vendor/product ID. See libs/DualSenseWindows/LICENSE.

namespace Haptics
{
	// Plain 0..255 RGB. This module is deliberately SFML-free -- it's meant
	// to be copied wholesale into another project that might not even use
	// SFML -- so it can't reach for sf::Color the way the rest of this
	// project's code does; this is its own minimal stand-in.
	struct RGBColor
	{
		unsigned char r = 0u;
		unsigned char g = 0u;
		unsigned char b = 0u;
	};

	// Cross-controller vibration/lightbar/adaptive-trigger layer, independent
	// of SFML's own joystick polling -- detects an Xbox controller through
	// its native XInput API, or a DualSense/DualShock controller through the
	// vendored DualSenseWindows library (raw HID), and turns high-level
	// "make it rumble"/"set this color"/"resist this trigger" calls into
	// whatever that hardware actually expects.
	//
	// Self-contained: only depends on the Windows SDK and the vendored
	// DualSenseWindows library -- nothing from this project's own game
	// code. Meant to be copyable wholesale into another project.
	class GamepadHaptics
	{
	public:
		GamepadHaptics();
		~GamepadHaptics();

		GamepadHaptics(const GamepadHaptics&) = delete;
		GamepadHaptics& operator=(const GamepadHaptics&) = delete;

		void SetVibrationEnabled(bool isVibrationEnabled) noexcept;
		[[nodiscard]] bool IsVibrationEnabled() const noexcept;

		void SetLightbarEnabled(bool isLightbarEnabled) noexcept;
		[[nodiscard]] bool IsLightbarEnabled() const noexcept;

		void SetAdaptiveTriggersEnabled(bool isAdaptiveTriggersEnabled) noexcept;
		[[nodiscard]] bool IsAdaptiveTriggersEnabled() const noexcept;

		// Starts (or strengthens) a vibration burst that fades linearly to
		// zero over durationSeconds. Motor strengths are 0..1:
		// lowFrequencyMotor is the heavier/rumblier motor, highFrequencyMotor
		// the sharper/buzzier one (XInput's two-motor convention; DualSense's
		// single pair of rumble motors map onto the same two values).
		// Calling this again while a pulse is still fading only ever makes
		// the result stronger/longer, never weaker or restarted from scratch
		// -- the same merge rule GameplayEffects::StartCameraShake uses for
		// overlapping shake requests, so a rapid string of small pulses (a
		// sustained effect re-armed every frame) reads as one continuous
		// vibration instead of stuttering.
		void PulseVibration(float lowFrequencyMotor, float highFrequencyMotor, float durationSeconds);

		// DualSense/DualShock only -- no-op on Xbox (that hardware has no
		// lightbar). Sets the resting lightbar color shown whenever no pulse
		// is running. Takes effect on the next Update(); safe to call every
		// frame with the same color.
		void SetLightbarColor(RGBColor color) noexcept;

		// DualSense/DualShock only. Flashes `color` on the lightbar over
		// durationSeconds. With blinks == 1 it fades back to the resting
		// color with a faint throb, and re-arming every frame holds it lit
		// (a sustained glow). With blinks > 1 it does that many clean
		// on-off flashes across the duration instead. Merges toward the
		// stronger / longer request, like PulseVibration.
		void PulseLightbar(RGBColor color, float durationSeconds, int blinks = 1) noexcept;

		// --- Adaptive triggers -------------------------------------------------
		// Ported from Until Last Asteroid and kept intact although nothing in
		// Tessera drives them yet -- a future version (harder drop, hold-to-
		// charge, ...) is expected to. Do not "tidy away".

		// DualSense/DualShock only -- no-op on Xbox (that hardware has no
		// adaptive triggers). A brief, sharp vibration burst under the
		// fingertip on the right trigger for one regular shot's recoil --
		// the trigger motor can only resist, not push back, so this is a
		// buzz-kick (EffectEx), not a resistance band. Each call plays
		// independently and just restarts the burst, unlike PulseVibration
		// -- there's no meaningful "stronger" version of a single kick, so
		// firing rapidly just retriggers it cleanly on every shot.
		void PulseRightTriggerRecoil();

		// DualSense/DualShock only -- no-op on Xbox. Holds a constant, heavy
		// resistance on the right trigger for as long as active is true
		// (e.g. while the laser beam is being held down); pass false to
		// release it back to no resistance. Takes effect on the next
		// Update() call; safe to call every frame with the same value.
		void SetRightTriggerSustainedResistance(bool active) noexcept;

		// Advances the current pulse's fade-out and re-sends the resulting
		// motor speeds (and, on DualSense, the lightbar color) to the
		// connected controller. Call once per frame, regardless of which
		// part of the game is currently active.
		void Update(float deltaTime);

	private:
		// No Generic/other case: XInput is already the generic path here --
		// on Windows it covers the overwhelming majority of controllers
		// people actually plug in, Xbox pads included -- so anything this
		// enum doesn't name is a controller neither backend recognizes at
		// all, meaning there is nothing to drive rumble on regardless of
		// what we'd call it. That's exactly what None already means: no
		// controller, or one connected that neither backend can talk to.
		enum class ConnectedControllerType
		{
			None,
			Xbox,
			DualSense
		};

		void RefreshConnection();
		[[nodiscard]] bool RefreshXboxConnection();
		[[nodiscard]] bool RefreshDualSenseConnection();
		void DisconnectDualSense();

		void ApplyVibration(float lowFrequencyMotor, float highFrequencyMotor);
		[[nodiscard]] DS5W::TriggerEffect BuildRightTriggerEffect() const noexcept;

		bool isVibrationEnabled = true;
		bool isLightbarEnabled = true;
		bool isAdaptiveTriggersEnabled = true;

		ConnectedControllerType connectedType = ConnectedControllerType::None;
		unsigned long xboxUserIndex = 0u;
		DS5W::DeviceContext dualSenseContext{};

		float connectionRecheckRemaining = 0.f;

		float pulseRemaining = 0.f;
		float pulseDuration = 0.f;
		float pulseLowMotor = 0.f;
		float pulseHighMotor = 0.f;

		RGBColor lightbarColor{};

		RGBColor lightbarPulseColor{};
		float lightbarPulseRemaining = 0.f;
		float lightbarPulseDuration = 0.f;
		int lightbarPulseBlinks = 1;
		float lightbarThrobTime = 0.f;
		RGBColor currentLightbar{};

		float rightTriggerRecoilRemaining = 0.f;
		bool isRightTriggerSustainedResistanceActive = false;
	};
}