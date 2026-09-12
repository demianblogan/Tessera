#include "LanguagePickerState.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../input/MenuInput.h"
#include "../localization/LanguageAccent.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/SettingsManager.h"
#include "../ui/ColourUtils.h"
#include "../ui/Easing.h"
#include "../ui/TextLayout.h"
#include "MenuShell.h"

namespace
{
	constexpr unsigned int PromptSize = 46;
	constexpr sf::Vector2f PromptCentre{ 960.f, 190.f };
	constexpr float PromptDropHeight = 220.f;
	constexpr float PromptFallSpread = 0.9f;   // total stagger across every letter

	constexpr unsigned int ButtonTextSize = 46;
	constexpr sf::Vector2f ColumnTopLeft{ 800.f, 360.f };
	constexpr float RowGap = 110.f;

	constexpr float HighlightSpeed = 8.f;   // 1/seconds to reach the target highlight
	constexpr float HighlightScale = 1.15f;
	constexpr float RestDesaturate = 0.35f;
	constexpr float RestDarken = 0.55f;

	const sf::Color DividerColour(150, 156, 168);

	[[nodiscard]] sf::Color LerpColour(sf::Color a, sf::Color b, float t)
	{
		return {
			UI::ToByte(static_cast<float>(a.r) + (static_cast<float>(b.r) - a.r) * t),
			UI::ToByte(static_cast<float>(a.g) + (static_cast<float>(b.g) - a.g) * t),
			UI::ToByte(static_cast<float>(a.b) + (static_cast<float>(b.b) - a.b) * t),
			UI::ToByte(static_cast<float>(a.a) + (static_cast<float>(b.a) - a.a) * t) };
	}

	[[nodiscard]] sf::Color RestColourFor(Language language)
	{
		return UI::Darken(UI::Desaturate(LanguageAccent(language), RestDesaturate), RestDarken);
	}

	[[nodiscard]] std::string_view LanguageNameKey(Language language)
	{
		switch (language)
		{
		case Language::English:   return TextKey::Options::LanguageEnglish;
		case Language::Spanish:   return TextKey::Options::LanguageSpanish;
		case Language::German:    return TextKey::Options::LanguageGerman;
		case Language::Russian:   return TextKey::Options::LanguageRussian;
		case Language::Ukrainian: return TextKey::Options::LanguageUkrainian;
		}
		return TextKey::Options::LanguageEnglish;
	}

	// Reads the picker's multi-language prompt: one line per language, in
	// AllLanguages order. Not routed through the per-language catalogs -- it
	// has to read before any language is chosen. A plain UTF-8 text file (not
	// a C++ string literal), decoded with fromUtf8 like every other catalog
	// the game reads, instead of the compiler's narrow-literal execution
	// charset. Falls back to English for any line the file is missing.
	[[nodiscard]] std::array<sf::String, LanguageCount> LoadPromptLines()
	{
		std::array<sf::String, LanguageCount> lines;
		lines.fill("Choose your language");

		std::ifstream file(Assets::Paths::Data::LanguagePickerPrompt);
		if (!file.is_open())
		{
			return lines;
		}

		std::string rawLine;
		for (std::size_t i = 0; i < LanguageCount && std::getline(file, rawLine); ++i)
		{
			while (!rawLine.empty() && (rawLine.back() == '\r' || rawLine.back() == '\n'))
			{
				rawLine.pop_back();
			}
			if (!rawLine.empty())
			{
				lines[i] = sf::String::fromUtf8(rawLine.begin(), rawLine.end());
			}
		}

		return lines;
	}
}

LanguagePickerState::LanguagePickerState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, backgroundSprite(context.textures.Get(Assets::TextureID::MenuBackground))
	, aurora(context.shaders.Get(Assets::ShaderID::MenuAurora))
	, backdrop(context.textures.Get(Assets::TextureID::BlockSpritesheetWithOutline))
	// Placeholder text -- BuildPrompt() below fully rebuilds string, layout
	// and colour for every entry; this only exists to give sf::Text (which
	// has no default constructor) something to hold until then.
	, segments{ {
		{ Language::English, sf::Text(context.fonts.Get(Assets::FontID::Main), "", PromptSize), 0.f },
		{ Language::Spanish, sf::Text(context.fonts.Get(Assets::FontID::Main), "", PromptSize), 0.f },
		{ Language::German, sf::Text(context.fonts.Get(Assets::FontID::Main), "", PromptSize), 0.f },
		{ Language::Russian, sf::Text(context.fonts.Get(Assets::FontID::Main), "", PromptSize), 0.f },
		{ Language::Ukrainian, sf::Text(context.fonts.Get(Assets::FontID::Main), "", PromptSize), 0.f } } }
	, column(context.fonts.Get(Assets::FontID::MenuList), ButtonTextSize,
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{
	BuildPrompt();

	for (const Language language : AllLanguages)
	{
		column.AddButton(context.localization.GetText(LanguageNameKey(language)),
			[this, language] { Choose(language); }, true, LanguageAccent(language));
	}

	column.SetLayout(ColumnTopLeft, RowGap);
	column.SetSelectionChangedCallback([this](std::size_t index)
		{
			this->context.audioPlayer.Play(Assets::SoundID::MenuItemSelected);
			if (index < AllLanguages.size())
			{
				SetHovered(AllLanguages[index]);
			}
		});
	column.Begin();

	SetHovered(Language::English);
}

void LanguagePickerState::BuildPrompt()
{
	const std::array<sf::String, LanguageCount> lines = LoadPromptLines();
	const sf::Font& font = context.fonts.Get(Assets::FontID::Main);

	sf::String dividerString;
	dividerString += U' ';
	dividerString += U' ';
	dividerString += static_cast<char32_t>(0x00b7);
	dividerString += U' ';
	dividerString += U' ';

	introGlyphs.clear();
	dividers.clear();

	float x = 0.f;

	for (std::size_t i = 0; i < LanguageCount; ++i)
	{
		segments[i].language = AllLanguages[i];
		segments[i].text = sf::Text(font, lines[i], PromptSize);
		segments[i].highlight = 0.f;

		// Per-letter breakdown for the intro fall, starting at the same x the
		// combined phrase below will occupy.
		float charX = x;
		for (std::size_t c = 0; c < lines[i].getSize(); ++c)
		{
			const char32_t codepoint = lines[i][c];
			sf::String single;
			single += codepoint;

			introGlyphs.push_back(
				IntroGlyph{ sf::Text(font, single, PromptSize), i, charX, PromptCentre.y, 0.f });
			charX += font.getGlyph(codepoint, PromptSize, false).advance;
		}

		x += segments[i].text.getLocalBounds().size.x;

		if (i + 1 < LanguageCount)
		{
			sf::Text divider(font, dividerString, PromptSize);
			x += divider.getLocalBounds().size.x;
			dividers.push_back(std::move(divider));
		}
	}

	const float startXShift = PromptCentre.x - x * 0.5f;

	// Now that the row's total width is known, place the steady-state phrases
	// (origin at their own centre, so a later setScale grows them in place)
	// and the dividers between them.
	float cursor = 0.f;
	std::size_t dividerIndex = 0;
	for (std::size_t i = 0; i < LanguageCount; ++i)
	{
		const sf::FloatRect bounds = segments[i].text.getLocalBounds();
		UI::TextLayout::CentreOrigin(segments[i].text);
		segments[i].text.setPosition({ cursor + bounds.size.x * 0.5f + startXShift, PromptCentre.y });
		cursor += bounds.size.x;

		if (i + 1 < LanguageCount)
		{
			sf::Text& divider = dividers[dividerIndex++];
			const sf::FloatRect divBounds = divider.getLocalBounds();
			UI::TextLayout::CentreOrigin(divider);
			divider.setFillColor(DividerColour);
			divider.setPosition({ cursor + divBounds.size.x * 0.5f + startXShift, PromptCentre.y });
			cursor += divBounds.size.x;
		}
	}

	fallStagger = introGlyphs.empty() ? 0.f : PromptFallSpread / static_cast<float>(introGlyphs.size());
	for (std::size_t i = 0; i < introGlyphs.size(); ++i)
	{
		introGlyphs[i].restX += startXShift;
		introGlyphs[i].startDelay = static_cast<float>(i) * fallStagger;
	}

	introTotalDuration = introGlyphs.empty()
		? 0.f
		: static_cast<float>(introGlyphs.size() - 1) * fallStagger + fallDuration;
}

bool LanguagePickerState::IntroDone() const
{
	return introElapsed >= introTotalDuration;
}

void LanguagePickerState::SetHovered(Language language)
{
	hoveredLanguage = language;
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
	introElapsed += deltaTime;

	for (PromptSegment& segment : segments)
	{
		const float target = segment.language == hoveredLanguage ? 1.f : 0.f;
		segment.highlight = UI::Easing::Lerp(segment.highlight, target, std::min(1.f, deltaTime * HighlightSpeed));
	}

	aurora.Update(deltaTime);
	backdrop.Update(deltaTime);
	sparks.Update(deltaTime);
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
	target.clear(sf::Color::Black);
	target.draw(backgroundSprite);
	aurora.Render(target);
	backdrop.Render(target);
	sparks.Render(target);

	if (!IntroDone())
	{
		for (IntroGlyph& glyph : introGlyphs)
		{
			if (introElapsed <= glyph.startDelay)
			{
				continue;   // hasn't started falling yet -- not drawn at all
			}

			const float rawT = (introElapsed - glyph.startDelay) / fallDuration;
			const float eased = UI::Easing::EaseInCubic(rawT);

			glyph.text.setFillColor(RestColourFor(segments[glyph.segmentIndex].language));
			glyph.text.setPosition({ glyph.restX, glyph.restY - PromptDropHeight * (1.f - eased) });
			target.draw(glyph.text);
		}
	}
	else
	{
		for (PromptSegment& segment : segments)
		{
			const sf::Color accent = LanguageAccent(segment.language);
			const float scale = UI::Easing::Lerp(1.f, HighlightScale, segment.highlight);

			segment.text.setFillColor(LerpColour(RestColourFor(segment.language), accent, segment.highlight));
			segment.text.setScale({ scale, scale });
			target.draw(segment.text);
		}

		for (const sf::Text& divider : dividers)
		{
			target.draw(divider);
		}
	}

	column.Render(target);

	if (fade > 0.f)
	{
		sf::RectangleShape blackout(target.getView().getSize());
		blackout.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(std::clamp(fade, 0.f, 1.f) * 255.f)));
		target.draw(blackout);
	}
}
