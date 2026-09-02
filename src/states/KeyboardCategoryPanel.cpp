#include "KeyboardCategoryPanel.h"

#include <array>
#include <string_view>

#include <SFML/Window/Event.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../input/KeyName.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/SettingsManager.h"
#include "../ui/OptionRow.h"

namespace
{
	constexpr sf::FloatRect PanelBounds{ { 600.f, 210.f }, { 1240.f, 760.f } };
	constexpr float RowsTop = PanelBounds.position.y + 56.f;
	constexpr float RowMargin = 96.f;
	constexpr float RowHeight = 74.f;
	constexpr float RowGap = 8.f;

	constexpr sf::Color FlashAccept{ 90, 220, 130 };   // green
	constexpr sf::Color FlashInvalid{ 255, 170, 60 };  // amber -- reserved / menu key
	constexpr sf::Color FlashClash{ 240, 80, 80 };     // red -- already bound elsewhere
}

KeyboardCategoryPanel::KeyboardCategoryPanel(Context& context, sf::Color accent)
	: SettingsCategoryPanel(context, accent, PanelBounds)
{
	fields = {
		&ControlSettings::moveLeft,
		&ControlSettings::moveRight,
		&ControlSettings::softDrop,
		&ControlSettings::hardDrop,
		&ControlSettings::rotateClockwise,
		&ControlSettings::rotateCounterClockwise };

	BuildRows();
}

void KeyboardCategoryPanel::BuildRows()
{
	const LocalizationManager& text = context.localization;
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	const std::array<std::string_view, ActionCount> labels{
		TextKey::Options::KeyMoveLeft, TextKey::Options::KeyMoveRight,
		TextKey::Options::KeySoftDrop, TextKey::Options::KeyHardDrop,
		TextKey::Options::KeyRotateCw, TextKey::Options::KeyRotateCcw };

	rows.clear();
	for (std::size_t i = 0; i < ActionCount; ++i)
	{
		const sf::Keyboard::Scancode key = working.controls.*fields[i];
		auto row = std::make_unique<UI::KeyBindRow>(font, text.GetText(labels[i]), Input::KeyName(key));
		rowPtrs[i] = row.get();
		rows.push_back(std::move(row));
	}

	LayOutRows(RowsTop, RowMargin, RowHeight, RowGap);
	selectedRow = 0;
	capturingRow.reset();
	captureIgnore.reset();
}

void KeyboardCategoryPanel::SyncRows()
{
	for (std::size_t i = 0; i < ActionCount; ++i)
	{
		rowPtrs[i]->SetKeyLabel(Input::KeyName(working.controls.*fields[i]));
	}
}

bool KeyboardCategoryPanel::SettingsEqual(const GameSettings& a, const GameSettings& b) const
{
	for (const Field field : fields)
	{
		if (a.controls.*field != b.controls.*field)
		{
			return false;
		}
	}
	return true;
}

GameSettings KeyboardCategoryPanel::DefaultSettings() const
{
	return GameSettings{};   // ControlSettings' member defaults are the stock bindings
}

void KeyboardCategoryPanel::ApplyWorking()
{
	GameSettings& saved = context.settings.GetSettings();
	for (const Field field : fields)
	{
		saved.controls.*field = working.controls.*field;
	}
	context.settings.Save();
}

void KeyboardCategoryPanel::ResetWorking()
{
	const ControlSettings defaults;
	for (const Field field : fields)
	{
		working.controls.*field = defaults.*field;
	}
	SyncRows();
}

void KeyboardCategoryPanel::AdjustRow(std::size_t /*index*/, int /*direction*/)
{
	// Key rows have no left / right control.
}

void KeyboardCategoryPanel::ActivateRow(std::size_t index)
{
	BeginCapture(index, true);
}

void KeyboardCategoryPanel::RowClicked(std::size_t index)
{
	BeginCapture(index, false);
}

void KeyboardCategoryPanel::BeginCapture(std::size_t index, bool fromKeyboard)
{
	if (index >= ActionCount)
	{
		return;
	}

	EndCapture();
	capturingRow = index;
	selectedRow = index;
	rowPtrs[index]->SetCapturing(true);

	// Started with Enter: ignore that key until it is released, so its own
	// press (and any auto-repeat) is not captured as the new binding.
	captureIgnore = fromKeyboard
		? std::optional<sf::Keyboard::Scancode>(sf::Keyboard::Scancode::Enter)
		: std::nullopt;

	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 1.1f);
}

void KeyboardCategoryPanel::EndCapture()
{
	if (capturingRow)
	{
		rowPtrs[*capturingRow]->SetCapturing(false);
	}
	capturingRow.reset();
	captureIgnore.reset();
}

bool KeyboardCategoryPanel::IsReserved(sf::Keyboard::Scancode key)
{
	using S = sf::Keyboard::Scancode;
	switch (key)
	{
	case S::Unknown:
	case S::Escape:
	case S::Enter:
	case S::NumpadEnter:
	case S::LShift:  case S::RShift:
	case S::LControl: case S::RControl:
	case S::LAlt:    case S::RAlt:
	case S::LSystem: case S::RSystem:
	case S::Menu:
	case S::CapsLock: case S::NumLock: case S::ScrollLock:
	case S::PrintScreen:
	case S::Pause:
		return true;
	default:
		return false;
	}
}

std::optional<std::size_t> KeyboardCategoryPanel::ClashingRow(std::size_t index, sf::Keyboard::Scancode key) const
{
	for (std::size_t i = 0; i < ActionCount; ++i)
	{
		if (i != index && working.controls.*fields[i] == key)
		{
			return i;
		}
	}
	return std::nullopt;
}

void KeyboardCategoryPanel::TryAssign(std::size_t index, sf::Keyboard::Scancode key)
{
	AudioPlayer& audio = context.audioPlayer;

	if (working.controls.*fields[index] == key)
	{
		EndCapture();   // unchanged -- quietly accept
		return;
	}

	if (IsReserved(key))
	{
		rowPtrs[index]->Flash(FlashInvalid);
		audio.Play(Assets::SoundID::MenuItemPressed, 0.55f);
		EndCapture();
		return;
	}

	if (const std::optional<std::size_t> other = ClashingRow(index, key))
	{
		rowPtrs[index]->Flash(FlashClash);
		rowPtrs[*other]->Flash(FlashClash);
		audio.Play(Assets::SoundID::MenuItemPressed, 0.5f);
		EndCapture();
		return;
	}

	working.controls.*fields[index] = key;
	rowPtrs[index]->SetKeyLabel(Input::KeyName(key));
	rowPtrs[index]->Flash(FlashAccept);
	audio.Play(Assets::SoundID::MenuItemSelected, 1.15f);
	EndCapture();
}

bool KeyboardCategoryPanel::HandleEvent(const sf::Event& event)
{
	if (capturingRow)
	{
		if (const auto* released = event.getIf<sf::Event::KeyReleased>())
		{
			if (captureIgnore && released->scancode == *captureIgnore)
			{
				captureIgnore.reset();
			}
			return true;
		}

		if (const auto* pressed = event.getIf<sf::Event::KeyPressed>())
		{
			if (captureIgnore && pressed->scancode == *captureIgnore)
			{
				return true;
			}
			if (pressed->scancode == sf::Keyboard::Scancode::Escape)
			{
				context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.7f);
				EndCapture();
				return true;
			}
			TryAssign(*capturingRow, pressed->scancode);
			return true;
		}

		if (event.getIf<sf::Event::MouseButtonPressed>())
		{
			context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.7f);
			EndCapture();
			return true;
		}

		return true;   // swallow everything else while capturing a key
	}

	return SettingsCategoryPanel::HandleEvent(event);
}

void KeyboardCategoryPanel::Close()
{
	EndCapture();
	SettingsCategoryPanel::Close();
}

bool KeyboardCategoryPanel::WantsToStayOpen() const
{
	return capturingRow.has_value() || SettingsCategoryPanel::WantsToStayOpen();
}
