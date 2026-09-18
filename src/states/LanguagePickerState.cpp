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
#include "../localization/LanguageColors.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/SettingsManager.h"
#include "../ui/ColorUtils.h"
#include "../utils/Easing.h"
#include "../ui/TextLayout.h"
#include "MenuShellState.h"

namespace
{
	constexpr unsigned int PromptSize = 46;
	constexpr sf::Vector2f PromptCenter{ 960.f, 190.f };
	constexpr float PromptDropHeight = 220.f;
	constexpr float PromptFallSpread = 0.9f;   // total stagger across every letter

	constexpr unsigned int ButtonTextSize = 46;
	constexpr sf::Vector2f ColumnTopLeft{ 800.f, 360.f };
	constexpr float RowGap = 110.f;

	constexpr float HighlightSpeed = 8.f;   // 1/seconds to reach the target highlight
	constexpr float HighlightScale = 1.15f;
	constexpr float RestDesaturate = 0.35f;
	constexpr float RestDarken = 0.55f;

	// Navigation ticks: higher when moving forward / down, lower backward / up --
	// mirrors MainMenuScreen's NavPitchLow / NavPitchHigh.
	constexpr float NavPitchLow = 0.9f;
	constexpr float NavPitchHigh = 1.14f;

	const sf::Color DividerColor(150, 156, 168);

	[[nodiscard]] sf::Color LerpColor(sf::Color from, sf::Color to, float mixFactor)
	{
		return {
			UI::ToByte(static_cast<float>(from.r) + (static_cast<float>(to.r) - from.r) * mixFactor),
			UI::ToByte(static_cast<float>(from.g) + (static_cast<float>(to.g) - from.g) * mixFactor),
			UI::ToByte(static_cast<float>(from.b) + (static_cast<float>(to.b) - from.b) * mixFactor),
			UI::ToByte(static_cast<float>(from.a) + (static_cast<float>(to.a) - from.a) * mixFactor) };
	}

	[[nodiscard]] sf::Color RestColorFor(Language language)
	{
		return UI::Darken(UI::Desaturate(LanguageColor(language), RestDesaturate), RestDarken);
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
	// and color for every entry; this only exists to give sf::Text (which
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
			[this, language] { Choose(language); }, true, LanguageColor(language));
	}

	column.SetLayout(ColumnTopLeft, RowGap);
	column.SetSelectionChangedCallback([this](std::size_t index, int direction)
		{
			this->context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, direction >= 0 ? NavPitchHigh : NavPitchLow);
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
				IntroGlyph{ sf::Text(font, single, PromptSize), i, charX, PromptCenter.y, 0.f });
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

	const float startXShift = PromptCenter.x - x * 0.5f;

	// Now that the row's total width is known, place the steady-state phrases
	// (origin at their own center, so a later setScale grows them in place)
	// and the dividers between them.
	float cursor = 0.f;
	std::size_t dividerIndex = 0;
	for (std::size_t i = 0; i < LanguageCount; ++i)
	{
		// The intro glyphs are drawn with the default (top-left) origin at
		// {cursor + startXShift, PromptCenter.y}; setting origin to the ink
		// center (so scale grows the phrase in place) shifts what "position"
		// means, so the *position* has to move by that same origin offset to
		// keep the rendered top-left exactly where the intro left it -- with
		// origin O, world(bounds.position) = position - size*0.5 = position -
		// (O - bounds.position), so position = topLeft + O reproduces the old
		// origin-(0,0) placement exactly at scale 1.
		const sf::FloatRect bounds = segments[i].text.getLocalBounds();
		const sf::Vector2f origin{ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f };
		const sf::Vector2f topLeft{ cursor + startXShift, PromptCenter.y };

		segments[i].text.setOrigin(origin);
		segments[i].text.setPosition(topLeft + origin);
		cursor += bounds.size.x;

		if (i + 1 < LanguageCount)
		{
			sf::Text& divider = dividers[dividerIndex++];
			const sf::FloatRect divBounds = divider.getLocalBounds();
			UI::TextLayout::CenterOrigin(divider);
			divider.setFillColor(DividerColor);
			divider.setPosition({ cursor + divBounds.size.x * 0.5f + startXShift, PromptCenter.y });
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
		: static_cast<float>(introGlyphs.size() - 1) * fallStagger + FallDuration;
}

bool LanguagePickerState::IsIntroDone() const
{
	return introElapsed >= introTotalDuration;
}

void LanguagePickerState::SetHovered(Language language)
{
	hoveredLanguage = language;
}

void LanguagePickerState::Choose(Language language)
{
	if (isLeaving)
	{
		return;
	}

	context.localization.SetLanguage(language);
	context.settings.GetSettings().language = language;
	context.settings.GetSettings().isLanguageChosen = true;
	context.settings.Save();

	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	isLeaving = true;
	column.PlayExit();
}

void LanguagePickerState::Finish()
{
	RequestChange(std::make_unique<MenuShellState>(context));
}

void LanguagePickerState::HandleEvent(const sf::Event& event)
{
	if (isLeaving)
	{
		return;
	}

	switch (MenuInput::ResolveAction(event, context.gamepad))
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
		segment.highlight = Easing::Lerp(segment.highlight, target, std::min(1.f, deltaTime * HighlightSpeed));
	}

	aurora.Update(deltaTime);
	backdrop.Update(deltaTime);
	sparks.Update(deltaTime);
	column.Update(deltaTime);

	if (isLeaving)
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

	if (!IsIntroDone())
	{
		for (IntroGlyph& glyph : introGlyphs)
		{
			if (introElapsed <= glyph.startDelay)
			{
				continue;   // hasn't started falling yet -- not drawn at all
			}

			const float fallFraction = (introElapsed - glyph.startDelay) / FallDuration;
			const float eased = Easing::EaseInCubic(fallFraction);

			glyph.text.setFillColor(RestColorFor(segments[glyph.segmentIndex].language));
			glyph.text.setPosition({ glyph.restX, glyph.restY - PromptDropHeight * (1.f - eased) });
			target.draw(glyph.text);
		}
	}
	else
	{
		for (PromptSegment& segment : segments)
		{
			const sf::Color accent = LanguageColor(segment.language);
			const float scale = Easing::Lerp(1.f, HighlightScale, segment.highlight);

			segment.text.setFillColor(LerpColor(RestColorFor(segment.language), accent, segment.highlight));
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
