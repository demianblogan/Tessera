#include "LanguagePickerState.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/SettingsManager.h"
#include "../ui/TextLayout.h"
#include "MenuShell.h"

namespace
{
	constexpr unsigned int PromptSize = 30;
	constexpr float PromptMaxWidth = 1700.f;
	constexpr sf::Vector2f PromptCentre{ 960.f, 260.f };

	constexpr unsigned int ButtonTextSize = 46;
	constexpr sf::Vector2f ColumnTopLeft{ 800.f, 420.f };
	constexpr float RowGap = 110.f;

	// Not routed through the per-language catalogs -- it has to read before any
	// language is chosen, so it carries one line per language instead of one
	// translation. Lives in its own plain UTF-8 text file (not a C++ string
	// literal) so it goes through the same byte-exact fromUtf8 decode as every
	// other piece of non-ASCII text in the game, instead of the compiler's
	// narrow-literal execution charset.
	[[nodiscard]] sf::String LoadPrompt()
	{
		std::ifstream file(Assets::Paths::Data::LanguagePickerPrompt);
		if (!file.is_open())
		{
			return "Choose your language";
		}

		std::ostringstream contents;
		contents << file.rdbuf();
		std::string text = contents.str();

		while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
		{
			text.pop_back();
		}

		return sf::String::fromUtf8(text.begin(), text.end());
	}
}

LanguagePickerState::LanguagePickerState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, prompt(context.fonts.Get(Assets::FontID::Main), LoadPrompt(), PromptSize)
	, column(context.fonts.Get(Assets::FontID::MenuList), ButtonTextSize,
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{
	UI::TextLayout::FitWidth(prompt, PromptMaxWidth);
	UI::TextLayout::CentreOrigin(prompt);
	prompt.setPosition(PromptCentre);
	prompt.setFillColor(sf::Color(220, 226, 236));

	column.AddButton(context.localization.GetText(TextKey::Options::LanguageEnglish),
		[this] { Choose(Language::English); });
	column.AddButton(context.localization.GetText(TextKey::Options::LanguageSpanish),
		[this] { Choose(Language::Spanish); });
	column.AddButton(context.localization.GetText(TextKey::Options::LanguageGerman),
		[this] { Choose(Language::German); });
	column.AddButton(context.localization.GetText(TextKey::Options::LanguageRussian),
		[this] { Choose(Language::Russian); });
	column.AddButton(context.localization.GetText(TextKey::Options::LanguageUkrainian),
		[this] { Choose(Language::Ukrainian); });

	column.SetLayout(ColumnTopLeft, RowGap);
	column.SetSelectionChangedCallback([this](std::size_t)
		{
			this->context.audioPlayer.Play(Assets::SoundID::MenuItemSelected);
		});
	column.Begin();
}

void LanguagePickerState::Choose(Language language)
{
	if (leaving)
	{
		return;
	}

	context.localization.SetLanguage(language);
	context.settings.GetSettings().language = language;
	context.settings.GetSettings().languageChosen = true;
	context.settings.Save();

	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	leaving = true;
	column.PlayExit();
}

void LanguagePickerState::Finish()
{
	RequestChange(std::make_unique<MenuShell>(context));
}

void LanguagePickerState::HandleEvent(const sf::Event& event)
{
	if (leaving)
	{
		return;
	}

	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Up:      column.SelectPrevious(); return;
	case MenuInput::Action::Down:    column.SelectNext();     return;
	case MenuInput::Action::Confirm: column.Activate();       return;
	default:                                                  break;
	}

	if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		column.PointerMoved(context.window.mapPixelToCoords(moved->position));
	}
	else if (const auto* clicked = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (clicked->button == sf::Mouse::Button::Left)
		{
			column.PointerPressed(context.window.mapPixelToCoords(clicked->position));
		}
	}
}

void LanguagePickerState::Update(float deltaTime)
{
	column.Update(deltaTime);

	if (leaving)
	{
		fade = std::min(1.f, fade + deltaTime / FadeDuration);
		if (fade >= 1.f)
		{
			Finish();
		}
	}
	else
	{
		fade = std::max(0.f, fade - deltaTime / FadeDuration);
	}
}

void LanguagePickerState::Render(sf::RenderTarget& target)
{
	target.draw(prompt);
	column.Render(target);

	if (fade > 0.f)
	{
		sf::RectangleShape blackout(target.getView().getSize());
		blackout.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(std::clamp(fade, 0.f, 1.f) * 255.f)));
		target.draw(blackout);
	}
}
