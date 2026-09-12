#include "OptionsScreen.h"

#include <algorithm>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../config/HapticSettings.h"
#include "../core/Context.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../settings/SettingsManager.h"
#include "../ui/Easing.h"
#include "AudioCategoryPanel.h"
#include "GameplayCategoryPanel.h"
#include "GamepadCategoryPanel.h"
#include "GraphicsCategoryPanel.h"
#include "HudCategoryPanel.h"
#include "KeyboardCategoryPanel.h"
#include "ScreenHost.h"

namespace
{
	constexpr unsigned int ButtonTextSize = 38;
	constexpr sf::Vector2f ColumnTopLeft{ 130.f, 288.f };
	constexpr float RowGap = 104.f;

	constexpr float SubRowGap = 96.f;      // tighter spacing for a sub-list column
	constexpr float FlyoutX = 470.f;       // the sub-list's x while it is a hover flyout
	constexpr float FlyoutMinTop = 150.f;  // never let a flyout ride higher than this
	constexpr float FlyoutDim = 0.42f;

	constexpr float PreviewFadeDuration = 0.18f;
	constexpr float SlideDuration = 0.34f;
	constexpr float ColumnExitShiftX = -1500.f;

	// Row order in the category column.
	enum Row : std::size_t { Gameplay = 0, Hud = 1, Graphics = 2, Audio = 3, Controls = 4, Language = 5, Back = 6 };

	// Item order in the Controls sub-column.
	enum ControlsItem : std::size_t { CtrlKeyboard = 0, CtrlGamepad = 1, CtrlBack = 2 };

	constexpr sf::Color GameplayColour{ 80, 210, 195 };    // teal
	constexpr sf::Color HudColour{ 241, 89, 123 };         // watermelon
	constexpr sf::Color GraphicsColour{ 90, 200, 255 };    // sky blue
	constexpr sf::Color AudioColour{ 120, 220, 130 };      // green
	constexpr sf::Color ControlsColour{ 190, 130, 240 };   // violet
	constexpr sf::Color LanguageColour{ 235, 110, 175 };   // rose

	using UI::Easing::Lerp;
	using UI::Easing::SmoothStep;

	// The render shift that places a sub-list as a flyout beside `categoryRow`:
	// roughly centred on that row, then pulled up to stay on screen and clear of
	// the category column's own "Back to Main Menu" entry.
	[[nodiscard]] sf::Vector2f FlyoutShift(std::size_t categoryRow, std::size_t itemCount) noexcept
	{
		const float listHeight = static_cast<float>(itemCount) * SubRowGap;
		const float rowY = ColumnTopLeft.y + static_cast<float>(categoryRow) * RowGap;
		const float centredTop = rowY - listHeight * 0.5f + SubRowGap * 0.5f;

		const float lastCategoryRowY = ColumnTopLeft.y + static_cast<float>(Row::Back) * RowGap;   // "Back to Main Menu"
		const float maxTop = std::max(FlyoutMinTop, lastCategoryRowY - listHeight - 20.f);
		const float top = std::clamp(centredTop, FlyoutMinTop, maxTop);

		return { FlyoutX - ColumnTopLeft.x, top - ColumnTopLeft.y };
	}
}

OptionsScreen::OptionsScreen(ScreenHost& host, sf::Color accent)
	: MenuScreen(host)
	, accent(accent)
	, column(context.fonts.Get(Assets::FontID::MenuList), ButtonTextSize,
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, controlsColumn(context.fonts.Get(Assets::FontID::MenuList), ButtonTextSize,
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, languageColumn(context.fonts.Get(Assets::FontID::MenuList), ButtonTextSize,
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, seenLocalizationRevision(context.localization.Revision())
{
	const LocalizationManager& text = context.localization;

	column.AddButton(text.GetText(TextKey::Options::Gameplay), [this] { OpenCategory(Row::Gameplay); }, true, GameplayColour);
	column.AddButton(text.GetText(TextKey::Options::Hud), [this] { OpenCategory(Row::Hud); }, true, HudColour);
	column.AddButton(text.GetText(TextKey::Options::Graphics), [this] { OpenCategory(Row::Graphics); }, true, GraphicsColour);
	column.AddButton(text.GetText(TextKey::Options::Audio), [this] { OpenCategory(Row::Audio); }, true, AudioColour);
	column.AddButton(text.GetText(TextKey::Options::Controls), [this] { OpenSub(Row::Controls); }, true, ControlsColour);
	column.AddButton(text.GetText(TextKey::Options::Language), [this] { OpenSub(Row::Language); }, true, LanguageColour);
	column.AddButton(text.GetText(TextKey::Options::Back), [this] { Leave(); }, true);   // white

	column.SetLayout(ColumnTopLeft, RowGap);
	column.SetSelectionChangedCallback([this](std::size_t)
		{
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		});
	column.SetSwooshCallback([this](std::size_t index)
		{
			// Same fly-in swoosh as the main-menu ring, pitched a little higher.
			context.audioPlayer.Play(Assets::SoundID::MenuItemAppeared, 1.02f + 0.05f * static_cast<float>(index));
		});

	const auto subSelectionSound = [this](std::size_t)
		{
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		};

	controlsColumn.AddButton(text.GetText(TextKey::Options::ControlsKeyboard), [this] { OpenControlsItem(CtrlKeyboard); }, true, ControlsColour);
	controlsColumn.AddButton(text.GetText(TextKey::Options::ControlsGamepad), [this] { OpenControlsItem(CtrlGamepad); }, true, ControlsColour);
	controlsColumn.AddButton(text.GetText(TextKey::Options::ControlsBack), [this] { CloseSub(); }, true);   // plain white
	controlsColumn.SetLayout(ColumnTopLeft, SubRowGap);
	controlsColumn.SetSelectionChangedCallback(subSelectionSound);
	controlsColumn.AppearInstantly();

	languageColumn.AddButton(LanguageButtonLabel(Language::English, TextKey::Options::LanguageEnglish),
		[this] { SelectLanguage(Language::English); }, true, LanguageColour);
	languageColumn.AddButton(LanguageButtonLabel(Language::Spanish, TextKey::Options::LanguageSpanish),
		[this] { SelectLanguage(Language::Spanish); }, true, LanguageColour);
	languageColumn.AddButton(LanguageButtonLabel(Language::German, TextKey::Options::LanguageGerman),
		[this] { SelectLanguage(Language::German); }, true, LanguageColour);
	languageColumn.AddButton(LanguageButtonLabel(Language::Russian, TextKey::Options::LanguageRussian),
		[this] { SelectLanguage(Language::Russian); }, true, LanguageColour);
	languageColumn.AddButton(LanguageButtonLabel(Language::Ukrainian, TextKey::Options::LanguageUkrainian),
		[this] { SelectLanguage(Language::Ukrainian); }, true, LanguageColour);
	languageColumn.AddButton(text.GetText(TextKey::Options::ControlsBack), [this] { CloseSub(); }, true);   // plain white
	languageColumn.SetLayout(ColumnTopLeft, SubRowGap);
	languageColumn.SetSelectionChangedCallback(subSelectionSound);
	languageColumn.AppearInstantly();

	panels[Row::Gameplay] = std::make_unique<GameplayCategoryPanel>(context, GameplayColour);
	panels[Row::Hud] = std::make_unique<HudCategoryPanel>(context, HudColour);
	panels[Row::Graphics] = std::make_unique<GraphicsCategoryPanel>(context, GraphicsColour);
	panels[Row::Audio] = std::make_unique<AudioCategoryPanel>(context, AudioColour);
	controlsPanels[CtrlKeyboard] = std::make_unique<KeyboardCategoryPanel>(context, ControlsColour);
	controlsPanels[CtrlGamepad] = std::make_unique<GamepadCategoryPanel>(context, ControlsColour);

	previewIndex = Row::Graphics;   // the column starts focused on the first enabled row

	ApplyColumnShifts();
}

UI::MenuButtonColumn& OptionsScreen::SubColumnFor(std::size_t categoryRow)
{
	return categoryRow == Row::Language ? languageColumn : controlsColumn;
}

sf::String OptionsScreen::LanguageButtonLabel(::Language language, std::string_view key) const
{
	sf::String label = context.localization.GetText(key);
	if (language == context.localization.GetLanguage())
	{
		label += " *";
	}
	return label;
}

void OptionsScreen::SelectLanguage(::Language language)
{
	if (language != context.localization.GetLanguage())
	{
		context.localization.SetLanguage(language);
		context.settings.GetSettings().language = language;
		context.settings.GetSettings().languageChosen = true;
		context.settings.Save();
	}

	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
}

void OptionsScreen::RefreshText()
{
	const LocalizationManager& text = context.localization;

	column.SetButtonText(Row::Gameplay, text.GetText(TextKey::Options::Gameplay));
	column.SetButtonText(Row::Hud, text.GetText(TextKey::Options::Hud));
	column.SetButtonText(Row::Graphics, text.GetText(TextKey::Options::Graphics));
	column.SetButtonText(Row::Audio, text.GetText(TextKey::Options::Audio));
	column.SetButtonText(Row::Controls, text.GetText(TextKey::Options::Controls));
	column.SetButtonText(Row::Language, text.GetText(TextKey::Options::Language));
	column.SetButtonText(Row::Back, text.GetText(TextKey::Options::Back));

	controlsColumn.SetButtonText(CtrlKeyboard, text.GetText(TextKey::Options::ControlsKeyboard));
	controlsColumn.SetButtonText(CtrlGamepad, text.GetText(TextKey::Options::ControlsGamepad));
	controlsColumn.SetButtonText(CtrlBack, text.GetText(TextKey::Options::ControlsBack));

	languageColumn.SetButtonText(0, LanguageButtonLabel(Language::English, TextKey::Options::LanguageEnglish));
	languageColumn.SetButtonText(1, LanguageButtonLabel(Language::Spanish, TextKey::Options::LanguageSpanish));
	languageColumn.SetButtonText(2, LanguageButtonLabel(Language::German, TextKey::Options::LanguageGerman));
	languageColumn.SetButtonText(3, LanguageButtonLabel(Language::Russian, TextKey::Options::LanguageRussian));
	languageColumn.SetButtonText(4, LanguageButtonLabel(Language::Ukrainian, TextKey::Options::LanguageUkrainian));
	languageColumn.SetButtonText(5, text.GetText(TextKey::Options::ControlsBack));

	for (const std::unique_ptr<OptionsCategoryPanel>& panel : panels)
	{
		if (panel) { panel->RefreshText(); }
	}
	for (const std::unique_ptr<OptionsCategoryPanel>& panel : controlsPanels)
	{
		if (panel) { panel->RefreshText(); }
	}

	host.SetHeaderText(text.GetText(TextKey::Options::Title));
}

void OptionsScreen::PlayIntro()
{
	column.Begin();
}

void OptionsScreen::StartExit()
{
	column.PlayExit();
}

bool OptionsScreen::ExitFinished() const
{
	return column.IsExitDone();
}

std::pair<std::string_view, sf::Color> OptionsScreen::CurrentLightbar() const
{
	if (openIndex == static_cast<std::size_t>(Row::Gameplay)) { return { "options_gameplay", GameplayColour }; }
	if (openIndex == static_cast<std::size_t>(Row::Hud)) { return { "options_hud", HudColour }; }
	if (openIndex == static_cast<std::size_t>(Row::Graphics)) { return { "options_graphics", GraphicsColour }; }
	if (openIndex == static_cast<std::size_t>(Row::Audio)) { return { "options_audio", AudioColour }; }

	if (page != Page::Categories)
	{
		return subRow == Row::Language
			? std::pair<std::string_view, sf::Color>{ "options_language", LanguageColour }
			: std::pair<std::string_view, sf::Color>{ "options_controls", ControlsColour };
	}

	switch (column.SelectedIndex())
	{
	case Row::Gameplay: return { "options_gameplay", GameplayColour };
	case Row::Hud:      return { "options_hud", HudColour };
	case Row::Graphics: return { "options_graphics", GraphicsColour };
	case Row::Audio:    return { "options_audio", AudioColour };
	case Row::Controls: return { "options_controls", ControlsColour };
	case Row::Language: return { "options_language", LanguageColour };
	default:            return { "menu_options", accent };
	}
}

std::optional<sf::Color> OptionsScreen::LightbarColour() const
{
	const auto [key, fallback] = CurrentLightbar();
	const HapticSettings::Colour resolved =
		context.hapticSettings.LightbarFor(key, { fallback.r, fallback.g, fallback.b });
	return sf::Color(resolved.r, resolved.g, resolved.b);
}

void OptionsScreen::Leave()
{
	if (leaving)
	{
		return;
	}

	leaving = true;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	host.BeginBack();
}

void OptionsScreen::OpenCategory(std::size_t index)
{
	if (index >= RowCount || !panels[index])
	{
		return;
	}

	openIndex = index;
	column.SetCompact(true, index);
	panels[index]->Open();
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 1.05f);
}

void OptionsScreen::CloseCategory()
{
	if (!openIndex)
	{
		return;
	}

	panels[*openIndex]->Close();
	openIndex.reset();
	column.SetCompact(false, 0);
	context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.78f);
}

void OptionsScreen::OpenSub(std::size_t categoryRow)
{
	if (page != Page::Categories)
	{
		return;
	}

	subRow = categoryRow;
	page = Page::ToSub;
	pageT = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 1.05f);
}

void OptionsScreen::CloseSub()
{
	if (page != Page::Sub || openControlsItem)
	{
		return;
	}

	page = Page::ToCategories;
	pageT = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.78f);
}

void OptionsScreen::OpenControlsItem(std::size_t item)
{
	if (page != Page::Sub || subRow != Row::Controls || openControlsItem
		|| item >= controlsPanels.size() || !controlsPanels[item])
	{
		return;
	}

	openControlsItem = item;
	controlsColumn.SetCompact(true, item);
	controlsPanels[item]->Open();
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 1.05f);
}

void OptionsScreen::CloseControlsItem()
{
	if (!openControlsItem)
	{
		return;
	}

	controlsPanels[*openControlsItem]->Close();
	openControlsItem.reset();
	controlsColumn.SetCompact(false, 0);
	context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.78f);
}

void OptionsScreen::ApplyColumnShifts()
{
	float slide = 0.f;   // 0 = category column fully in, 1 = the sub-list fully in
	switch (page)
	{
	case Page::Categories:   slide = 0.f; break;
	case Page::ToSub:        slide = SmoothStep(pageT); break;
	case Page::Sub:          slide = 1.f; break;
	case Page::ToCategories: slide = 1.f - SmoothStep(pageT); break;
	}

	column.SetRenderShift({ ColumnExitShiftX * slide, 0.f });

	const bool onCategories = page == Page::Categories;
	const auto configure = [&](UI::MenuButtonColumn& sub, std::size_t categoryRow)
	{
		const sf::Vector2f flyout = FlyoutShift(categoryRow, sub.ButtonCount());
		const bool isActive = !onCategories && subRow == categoryRow;
		const bool isFlyout = onCategories && column.SelectedIndex() == categoryRow;

		if (isActive)
		{
			sub.SetRenderShift(Lerp(flyout, { 0.f, 0.f }, slide));
			sub.SetRenderDim(FlyoutDim + (1.f - FlyoutDim) * slide);
		}
		else
		{
			sub.SetRenderShift(flyout);
			sub.SetRenderDim(isFlyout ? FlyoutDim : 0.f);
		}

		// Focused look only once it is the live page, not as a flyout / mid-slide.
		sub.SetSelectionHighlight(isActive && (page == Page::Sub || page == Page::ToSub));
	};
	configure(controlsColumn, Row::Controls);
	configure(languageColumn, Row::Language);
}

void OptionsScreen::HandleEvent(const sf::Event& event)
{
	if (leaving)
	{
		return;
	}

	if (page == Page::ToSub || page == Page::ToCategories)
	{
		return;   // no input mid-slide
	}

	if (page == Page::Sub)
	{
		if (openControlsItem)
		{
			OptionsCategoryPanel& panel = *controlsPanels[*openControlsItem];
			if (panel.HandleEvent(event))
			{
				return;
			}
			if (MenuInput::Resolve(event, context.gamepad) == MenuInput::Action::Back && !panel.WantsToStayOpen())
			{
				CloseControlsItem();
			}
			return;
		}

		UI::MenuButtonColumn& sub = ActiveSubColumn();
		switch (MenuInput::Resolve(event, context.gamepad))
		{
		case MenuInput::Action::Up:      sub.SelectPrevious(); return;
		case MenuInput::Action::Down:    sub.SelectNext();     return;
		case MenuInput::Action::Confirm: sub.Activate();       return;
		case MenuInput::Action::Back:    CloseSub();           return;
		default:                                               break;
		}

		if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
		{
			sub.PointerMoved(context.window.mapPixelToCoords(moved->position));
		}
		else if (const auto* clicked = event.getIf<sf::Event::MouseButtonPressed>())
		{
			if (clicked->button == sf::Mouse::Button::Left)
			{
				sub.PointerPressed(context.window.mapPixelToCoords(clicked->position));
			}
		}
		return;
	}

	if (openIndex)
	{
		OptionsCategoryPanel& panel = *panels[*openIndex];
		if (panel.HandleEvent(event))
		{
			return;
		}

		if (MenuInput::Resolve(event, context.gamepad) == MenuInput::Action::Back && !panel.WantsToStayOpen())
		{
			CloseCategory();
		}
		return;
	}

	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Up:      column.SelectPrevious(); return;
	case MenuInput::Action::Down:    column.SelectNext();     return;
	case MenuInput::Action::Confirm: column.Activate();       return;
	case MenuInput::Action::Back:    Leave();                 return;
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

void OptionsScreen::Update(float deltaTime)
{
	if (seenLocalizationRevision != context.localization.Revision())
	{
		seenLocalizationRevision = context.localization.Revision();
		RefreshText();
	}

	column.Update(deltaTime);
	controlsColumn.Update(deltaTime);
	languageColumn.Update(deltaTime);

	if (page == Page::ToSub || page == Page::ToCategories)
	{
		pageT += deltaTime / SlideDuration;
		if (pageT >= 1.f)
		{
			pageT = 0.f;
			page = (page == Page::ToSub) ? Page::Sub : Page::Categories;
		}
	}

	ApplyColumnShifts();

	const bool categoriesActive = page == Page::Categories;

	if (categoriesActive && !openIndex && column.SelectedIndex() != previewIndex)
	{
		previewIndex = column.SelectedIndex();
		previewFade = 0.f;
	}

	previewFade = std::min(1.f, previewFade + deltaTime / PreviewFadeDuration);

	for (std::size_t i = 0; i < RowCount; ++i)
	{
		if (!panels[i])
		{
			continue;
		}

		OptionsCategoryPanel::Visibility visibility = OptionsCategoryPanel::Visibility::Hidden;
		if (categoriesActive && openIndex && *openIndex == i)
		{
			visibility = OptionsCategoryPanel::Visibility::Open;
		}
		else if (categoriesActive && !openIndex && previewIndex == i)
		{
			visibility = OptionsCategoryPanel::Visibility::Preview;
		}

		panels[i]->SetVisibility(visibility, previewFade);
		panels[i]->Update(deltaTime);
	}

	if (openIndex && panels[*openIndex] && panels[*openIndex]->WantsToClose())
	{
		CloseCategory();
	}

	const bool controlsLive = page == Page::Sub && subRow == Row::Controls;
	for (std::size_t i = 0; i < controlsPanels.size(); ++i)
	{
		if (!controlsPanels[i])
		{
			continue;
		}

		OptionsCategoryPanel::Visibility visibility = OptionsCategoryPanel::Visibility::Hidden;
		if (openControlsItem && *openControlsItem == i)
		{
			visibility = OptionsCategoryPanel::Visibility::Open;
		}
		else if (controlsLive && !openControlsItem && controlsColumn.SelectedIndex() == i)
		{
			visibility = OptionsCategoryPanel::Visibility::Preview;
		}

		controlsPanels[i]->SetVisibility(visibility, previewFade);
		controlsPanels[i]->Update(deltaTime);
	}

	if (openControlsItem && controlsPanels[*openControlsItem]->WantsToClose())
	{
		CloseControlsItem();
	}
}

void OptionsScreen::Render(sf::RenderTarget& target)
{
	column.Render(target);

	const auto renderSub = [&](UI::MenuButtonColumn& sub, std::size_t categoryRow)
	{
		const bool asPage = page != Page::Categories && subRow == categoryRow;
		const bool asFlyout = page == Page::Categories && column.SelectedIndex() == categoryRow;
		if (asPage || asFlyout)
		{
			sub.Render(target);
		}
	};
	renderSub(controlsColumn, Row::Controls);
	renderSub(languageColumn, Row::Language);

	for (const std::unique_ptr<OptionsCategoryPanel>& panel : panels)
	{
		if (panel)
		{
			panel->Render(target);
		}
	}

	for (const std::unique_ptr<OptionsCategoryPanel>& panel : controlsPanels)
	{
		if (panel)
		{
			panel->Render(target);
		}
	}
}
