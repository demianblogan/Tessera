#include "LanguagePickerState.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
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
#include "MenuShell.h"

namespace
{
	constexpr unsigned int PromptSize = 34;
	constexpr sf::Vector2f PromptCentre{ 960.f, 260.f };
	constexpr float PromptDropHeight = 260.f;
	constexpr float PromptFallSpread = 0.9f;   // total stagger across every glyph

	constexpr unsigned int ButtonTextSize = 46;
	constexpr sf::Vector2f ColumnTopLeft{ 800.f, 420.f };
	constexpr float RowGap = 110.f;

	constexpr float HighlightSpeed = 8.f;       // 1/seconds to reach the target highlight
	constexpr float HighlightScale = 1.15f;
	constexpr float HighlightHaloScale = 1.12f; // extra, on top of HighlightScale
	constexpr float HighlightHaloAlpha = 90.f;
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

	sf::String divider;
	divider += U' ';
	divider += U' ';
	divider += static_cast<char32_t>(0x00b7);
	divider += U' ';
	divider += U' ';

	promptGlyphs.clear();
	float x = 0.f;

	const auto appendCodepoint = [&](char32_t codepoint, int segmentIndex)
	{
		sf::String single;
		single += codepoint;

		promptGlyphs.push_back(PromptGlyph{ sf::Text(font, single, PromptSize), segmentIndex, x, PromptCentre.y, 0.f });
		x += font.getGlyph(codepoint, PromptSize, false).advance;
	};

	for (std::size_t i = 0; i < LanguageCount; ++i)
	{
		const float segmentStartX = x;
		for (std::size_t c = 0; c < lines[i].getSize(); ++c)
		{
			appendCodepoint(lines[i][c], static_cast<int>(i));
		}

		segments[i].language = AllLanguages[i];
		segments[i].pivotX = (segmentStartX + x) * 0.5f;
		segments[i].highlight = 0.f;

		if (i + 1 < LanguageCount)
		{
			for (std::size_t c = 0; c < divider.getSize(); ++c)
			{
				appendCodepoint(divider[c], -1);
			}
		}
	}

	const float startX = PromptCentre.x - x * 0.5f;
	fallStagger = promptGlyphs.empty() ? 0.f : PromptFallSpread / static_cast<float>(promptGlyphs.size());

	for (std::size_t i = 0; i < promptGlyphs.size(); ++i)
	{
		promptGlyphs[i].restX += startX;
		promptGlyphs[i].startDelay = static_cast<float>(i) * fallStagger;
	}
	for (PromptSegment& segment : segments)
	{
		segment.pivotX += startX;
	}
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
	promptElapsed += deltaTime;

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

	for (const PromptGlyph& glyph : promptGlyphs)
	{
		if (promptElapsed <= glyph.startDelay)
		{
			continue;   // hasn't started falling yet -- not drawn at all
		}

		const float rawT = (promptElapsed - glyph.startDelay) / fallDuration;
		const float eased = UI::Easing::EaseInCubic(rawT);
		const sf::Vector2f fallenPosition{ glyph.restX, glyph.restY - PromptDropHeight * (1.f - eased) };

		sf::Color colour = DividerColour;
		float scale = 1.f;
		sf::Vector2f position = fallenPosition;

		if (glyph.segmentIndex >= 0)
		{
			const PromptSegment& segment = segments[static_cast<std::size_t>(glyph.segmentIndex)];
			const sf::Color accent = LanguageAccent(segment.language);
			const sf::Color restColour = UI::Darken(UI::Desaturate(accent, RestDesaturate), RestDarken);

			colour = LerpColour(restColour, accent, segment.highlight);
			scale = UI::Easing::Lerp(1.f, HighlightScale, segment.highlight);

			const sf::Vector2f pivot{ segment.pivotX, PromptCentre.y };
			position = pivot + (fallenPosition - pivot) * scale;

			if (segment.highlight > 0.02f)
			{
				const float haloScale = scale * HighlightHaloScale;
				sf::Text halo = glyph.text;
				halo.setScale({ haloScale, haloScale });
				sf::Color haloColour = accent;
				haloColour.a = static_cast<std::uint8_t>(HighlightHaloAlpha * segment.highlight);
				halo.setFillColor(haloColour);
				halo.setPosition(pivot + (fallenPosition - pivot) * haloScale);
				target.draw(halo, sf::RenderStates(sf::BlendAdd));
			}
		}

		sf::Text glyphText = glyph.text;
		glyphText.setScale({ scale, scale });
		glyphText.setFillColor(colour);
		glyphText.setPosition(position);
		target.draw(glyphText);
	}

	column.Render(target);

	if (fade > 0.f)
	{
		sf::RectangleShape blackout(target.getView().getSize());
		blackout.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(std::clamp(fade, 0.f, 1.f) * 255.f)));
		target.draw(blackout);
	}
}
