#include "OptionsScreen.h"

#include <algorithm>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../../audio/AudioPlayer.h"
#include "../../core/Context.h"
#include "../../haptics/HapticSettings.h"
#include "../../input/MenuInput.h"
#include "../../localization/LanguageColors.h"
#include "../../localization/LocalizationManager.h"
#include "../../localization/TextKeys.h"
#include "../../resources/Assets.h"
#include "../../settings/SettingsManager.h"
#include "../../utils/Easing.h"
#include "../panels/AudioCategoryPanel.h"
#include "../panels/GameplayCategoryPanel.h"
#include "../panels/GamepadCategoryPanel.h"
#include "../panels/GraphicsCategoryPanel.h"
#include "../panels/HUDCategoryPanel.h"
#include "../panels/KeyboardCategoryPanel.h"
#include "../../states/ScreenHost.h"

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

	// Clearance kept between a flyout sub-list's bottom and the category
	// column's "Back to Main Menu" entry.
	constexpr float FlyoutBackClearance = 20.f;

	// Navigation ticks: higher when moving forward / down, lower backward / up --
	// mirrors MainMenuScreen's NavPitchLow / NavPitchHigh.
	constexpr float NavPitchLow = 0.9f;
	constexpr float NavPitchHigh = 1.14f;

	// Pitch for opening / closing a category, sub-page or controls item.
	constexpr float OpenPitch = 1.05f;
	constexpr float ClosePitch = 0.78f;

	// Fly-in swoosh pitch for the category column, mirrors MainMenuScreen's
	// SwooshBasePitch / SwooshPitchStep.
	constexpr float SwooshBasePitch = 1.02f;
	constexpr float SwooshPitchStep = 0.05f;

	// Row order in the category column.
	enum Row : std::size_t { Gameplay = 0, HUD = 1, Graphics = 2, Audio = 3, Controls = 4, Language = 5, Back = 6 };

	// Item order in the Controls sub-column.
	enum ControlsItem : std::size_t { CtrlKeyboard = 0, CtrlGamepad = 1, CtrlBack = 2 };

	constexpr sf::Color GameplayColor{ 80, 210, 195 };    // teal
	constexpr sf::Color HUDColor{ 241, 89, 123 };         // watermelon
	constexpr sf::Color GraphicsColor{ 90, 200, 255 };    // sky blue
	constexpr sf::Color AudioColor{ 120, 220, 130 };      // green
	constexpr sf::Color ControlsColor{ 190, 130, 240 };   // violet
	constexpr sf::Color LanguageRowColor{ 235, 110, 175 };   // rose

	using Easing::Lerp;
	using Easing::SmoothStep;

	// The render shift that places a sub-list as a flyout beside `categoryRow`:
	// roughly centered on that row, then pulled up to stay on screen and clear of
	// the category column's own "Back to Main Menu" entry.
	[[nodiscard]] sf::Vector2f FlyoutShift(std::size_t categoryRow, std::size_t itemCount) noexcept
	{
		const float listHeight = static_cast<float>(itemCount) * SubRowGap;
		const float rowY = ColumnTopLeft.y + static_cast<float>(categoryRow) * RowGap;
		const float centeredTop = rowY - listHeight * 0.5f + SubRowGap * 0.5f;

		const float lastCategoryRowY = ColumnTopLeft.y + static_cast<float>(Row::Back) * RowGap;   // "Back to Main Menu"
		const float maxTop = std::max(FlyoutMinTop, lastCategoryRowY - listHeight - FlyoutBackClearance);
		const float top = std::clamp(centeredTop, FlyoutMinTop, maxTop);

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
	, seenLocalizationRevision(context.localization.GetRevision())
{
	const LocalizationManager& text = context.localization;

	column.AddButton(text.GetText(TextKey::Options::Gameplay), [this] { OpenCategory(Row::Gameplay); }, true, GameplayColor);
	column.AddButton(text.GetText(TextKey::Options::HUD), [this] { OpenCategory(Row::HUD); }, true, HUDColor);
	column.AddButton(text.GetText(TextKey::Options::Graphics), [this] { OpenCategory(Row::Graphics); }, true, GraphicsColor);
	column.AddButton(text.GetText(TextKey::Options::Audio), [this] { OpenCategory(Row::Audio); }, true, AudioColor);
	column.AddButton(text.GetText(TextKey::Options::Controls), [this] { OpenSub(Row::Controls); }, true, ControlsColor);
	column.AddButton(text.GetText(TextKey::Options::Language), [this] { OpenSub(Row::Language); }, true, LanguageRowColor);
	column.AddButton(text.GetText(TextKey::Options::Back), [this] { Leave(); }, true);   // white

	column.SetLayout(ColumnTopLeft, RowGap);
	column.SetSelectionChangedCallback([this](std::size_t, int direction)
		{
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected, direction >= 0 ? NavPitchHigh : NavPitchLow);
		});
	column.SetSwooshCallback([this](std::size_t index)
		{
			// Same fly-in swoosh as the main-menu ring, pitched a little higher.
			context.audioPlayer.Play(Assets::SoundID::MenuItemAppeared, SwooshBasePitch + SwooshPitchStep * static_cast<float>(index));
		});

	const auto subSelectionSound = [this](std::size_t, int direction)
		{
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected, direction >= 0 ? NavPitchHigh : NavPitchLow);
		};

	controlsColumn.AddButton(text.GetText(TextKey::Options::ControlsKeyboard), [this] { OpenControlsItem(CtrlKeyboard); }, true, ControlsColor);
	controlsColumn.AddButton(text.GetText(TextKey::Options::ControlsGamepad), [this] { OpenControlsItem(CtrlGamepad); }, true, ControlsColor);
	controlsColumn.AddButton(text.GetText(TextKey::Options::ControlsBack), [this] { CloseSub(); }, true);   // plain white
	controlsColumn.SetLayout(ColumnTopLeft, SubRowGap);
	controlsColumn.SetSelectionChangedCallback(subSelectionSound);
	controlsColumn.AppearInstantly();

	languageColumn.AddButton(GetLanguageButtonLabel(Language::English, TextKey::Options::LanguageEnglish),
		[this] { SelectLanguage(Language::English); }, true, LanguageColor(Language::English));
	languageColumn.AddButton(GetLanguageButtonLabel(Language::Spanish, TextKey::Options::LanguageSpanish),
		[this] { SelectLanguage(Language::Spanish); }, true, LanguageColor(Language::Spanish));
	languageColumn.AddButton(GetLanguageButtonLabel(Language::German, TextKey::Options::LanguageGerman),
		[this] { SelectLanguage(Language::German); }, true, LanguageColor(Language::German));
	languageColumn.AddButton(GetLanguageButtonLabel(Language::Russian, TextKey::Options::LanguageRussian),
		[this] { SelectLanguage(Language::Russian); }, true, LanguageColor(Language::Russian));
	languageColumn.AddButton(GetLanguageButtonLabel(Language::Ukrainian, TextKey::Options::LanguageUkrainian),
		[this] { SelectLanguage(Language::Ukrainian); }, true, LanguageColor(Language::Ukrainian));
	languageColumn.AddButton(text.GetText(TextKey::Options::ControlsBack), [this] { CloseSub(); }, true);   // plain white
	languageColumn.SetLayout(ColumnTopLeft, SubRowGap);
	languageColumn.SetSelectionChangedCallback(subSelectionSound);
	languageColumn.AppearInstantly();

	panels[Row::Gameplay] = std::make_unique<GameplayCategoryPanel>(context, GameplayColor);
	panels[Row::HUD] = std::make_unique<HUDCategoryPanel>(context, HUDColor);
	panels[Row::Graphics] = std::make_unique<GraphicsCategoryPanel>(context, GraphicsColor);
	panels[Row::Audio] = std::make_unique<AudioCategoryPanel>(context, AudioColor);
	controlsPanels[CtrlKeyboard] = std::make_unique<KeyboardCategoryPanel>(context, ControlsColor);
	controlsPanels[CtrlGamepad] = std::make_unique<GamepadCategoryPanel>(context, ControlsColor);

	previewIndex = Row::Graphics;   // the column starts focused on the first enabled row

	ApplyColumnShifts();
}

UI::MenuButtonColumn& OptionsScreen::GetSubColumn(std::size_t categoryRow)
{
	return categoryRow == Row::Language ? languageColumn : controlsColumn;
}

UI::MenuButtonColumn& OptionsScreen::GetActiveSubColumn()
{
	return GetSubColumn(subRow);
}

sf::String OptionsScreen::GetLanguageButtonLabel(::Language language, std::string_view key) const
{
	sf::String label = context.localization.GetText(key);
	if (language == context.localization.GetLanguage())
	{
		label += " [";
		label += context.localization.GetText(TextKey::Options::LanguageSelected);
		label += "]";
	}
	return label;
}

void OptionsScreen::SelectLanguage(::Language language)
{
	if (language != context.localization.GetLanguage())
	{
		context.localization.SetLanguage(language);
		context.settings.GetSettings().language = language;
		context.settings.GetSettings().isLanguageChosen = true;
		context.settings.Save();
	}

	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
}

void OptionsScreen::RefreshText()
{
	const LocalizationManager& text = context.localization;

	column.SetButtonText(Row::Gameplay, text.GetText(TextKey::Options::Gameplay));
	column.SetButtonText(Row::HUD, text.GetText(TextKey::Options::HUD));
	column.SetButtonText(Row::Graphics, text.GetText(TextKey::Options::Graphics));
	column.SetButtonText(Row::Audio, text.GetText(TextKey::Options::Audio));
	column.SetButtonText(Row::Controls, text.GetText(TextKey::Options::Controls));
	column.SetButtonText(Row::Language, text.GetText(TextKey::Options::Language));
	column.SetButtonText(Row::Back, text.GetText(TextKey::Options::Back));

	controlsColumn.SetButtonText(CtrlKeyboard, text.GetText(TextKey::Options::ControlsKeyboard));
	controlsColumn.SetButtonText(CtrlGamepad, text.GetText(TextKey::Options::ControlsGamepad));
	controlsColumn.SetButtonText(CtrlBack, text.GetText(TextKey::Options::ControlsBack));

	languageColumn.SetButtonText(0, GetLanguageButtonLabel(Language::English, TextKey::Options::LanguageEnglish));
	languageColumn.SetButtonText(1, GetLanguageButtonLabel(Language::Spanish, TextKey::Options::LanguageSpanish));
	languageColumn.SetButtonText(2, GetLanguageButtonLabel(Language::German, TextKey::Options::LanguageGerman));
	languageColumn.SetButtonText(3, GetLanguageButtonLabel(Language::Russian, TextKey::Options::LanguageRussian));
	languageColumn.SetButtonText(4, GetLanguageButtonLabel(Language::Ukrainian, TextKey::Options::LanguageUkrainian));
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

bool OptionsScreen::IsExitFinished() const
{
	return column.IsExitDone();
}

std::pair<std::string_view, sf::Color> OptionsScreen::GetCurrentLightbar() const
{
	if (openIndex == static_cast<std::size_t>(Row::Gameplay)) { return { "options_gameplay", GameplayColor }; }
	if (openIndex == static_cast<std::size_t>(Row::HUD)) { return { "options_hud", HUDColor }; }
	if (openIndex == static_cast<std::size_t>(Row::Graphics)) { return { "options_graphics", GraphicsColor }; }
	if (openIndex == static_cast<std::size_t>(Row::Audio)) { return { "options_audio", AudioColor }; }

	if (page != Page::Categories)
	{
		return subRow == Row::Language
			? std::pair<std::string_view, sf::Color>{ "options_language", LanguageRowColor }
			: std::pair<std::string_view, sf::Color>{ "options_controls", ControlsColor };
	}

	switch (column.GetSelectedIndex())
	{
	case Row::Gameplay: return { "options_gameplay", GameplayColor };
	case Row::HUD:      return { "options_hud", HUDColor };
	case Row::Graphics: return { "options_graphics", GraphicsColor };
	case Row::Audio:    return { "options_audio", AudioColor };
	case Row::Controls: return { "options_controls", ControlsColor };
	case Row::Language: return { "options_language", LanguageRowColor };
	default:            return { "menu_options", accent };
	}
}

std::optional<sf::Color> OptionsScreen::GetLightbarColor() const
{
	const auto [key, fallback] = GetCurrentLightbar();
	const HapticSettings::Color resolved =
		context.hapticSettings.LightbarFor(key, { fallback.r, fallback.g, fallback.b });
	return sf::Color(resolved.r, resolved.g, resolved.b);
}

void OptionsScreen::Leave()
{
	if (isLeaving)
	{
		return;
	}

	isLeaving = true;
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
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, OpenPitch);
}

void OptionsScreen::CloseCategory()
{
	if (!openIndex.has_value())
	{
		return;
	}

	panels[*openIndex]->Close();
	openIndex.reset();
	column.SetCompact(false, 0);
	context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, ClosePitch);
}

void OptionsScreen::OpenSub(std::size_t categoryRow)
{
	if (page != Page::Categories)
	{
		return;
	}

	subRow = categoryRow;
	page = Page::ToSub;
	pageFraction = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, OpenPitch);
}

void OptionsScreen::CloseSub()
{
	if (page != Page::Sub || openControlsItem.has_value())
	{
		return;
	}

	page = Page::ToCategories;
	pageFraction = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, ClosePitch);
}

void OptionsScreen::OpenControlsItem(std::size_t item)
{
	if (page != Page::Sub || subRow != Row::Controls || openControlsItem.has_value()
		|| item >= controlsPanels.size() || !controlsPanels[item])
	{
		return;
	}

	openControlsItem = item;
	controlsColumn.SetCompact(true, item);
	controlsPanels[item]->Open();
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, OpenPitch);
}

void OptionsScreen::CloseControlsItem()
{
	if (!openControlsItem.has_value())
	{
		return;
	}

	controlsPanels[*openControlsItem]->Close();
	openControlsItem.reset();
	controlsColumn.SetCompact(false, 0);
	context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, ClosePitch);
}

void OptionsScreen::ApplyColumnShifts()
{
	float slide = 0.f;   // 0 = category column fully in, 1 = the sub-list fully in
	switch (page)
	{
	case Page::Categories:   slide = 0.f; break;
	case Page::ToSub:        slide = SmoothStep(pageFraction); break;
	case Page::Sub:          slide = 1.f; break;
	case Page::ToCategories: slide = 1.f - SmoothStep(pageFraction); break;
	}

	column.SetRenderShift({ ColumnExitShiftX * slide, 0.f });

	const bool onCategories = page == Page::Categories;
	const auto configure = [&](UI::MenuButtonColumn& sub, std::size_t categoryRow)
	{
		const sf::Vector2f flyout = FlyoutShift(categoryRow, sub.GetButtonCount());
		const bool isActive = !onCategories && subRow == categoryRow;
		const bool isFlyout = onCategories && column.GetSelectedIndex() == categoryRow;

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
	if (isLeaving)
	{
		return;
	}

	if (page == Page::ToSub || page == Page::ToCategories)
	{
		return;   // no input mid-slide
	}

	if (page == Page::Sub)
	{
		if (openControlsItem.has_value())
		{
			OptionsCategoryPanel& panel = *controlsPanels[*openControlsItem];
			if (panel.HandleEvent(event))
			{
				return;
			}
			if (MenuInput::ResolveAction(event, context.gamepad) == MenuInput::Action::Back && !panel.IsStayOpenRequested())
			{
				CloseControlsItem();
			}
			return;
		}

		UI::MenuButtonColumn& sub = GetActiveSubColumn();
		switch (MenuInput::ResolveAction(event, context.gamepad))
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

	if (openIndex.has_value())
	{
		OptionsCategoryPanel& panel = *panels[*openIndex];
		if (panel.HandleEvent(event))
		{
			return;
		}

		if (MenuInput::ResolveAction(event, context.gamepad) == MenuInput::Action::Back && !panel.IsStayOpenRequested())
		{
			CloseCategory();
		}
		return;
	}

	switch (MenuInput::ResolveAction(event, context.gamepad))
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
	if (seenLocalizationRevision != context.localization.GetRevision())
	{
		seenLocalizationRevision = context.localization.GetRevision();
		RefreshText();
	}

	column.Update(deltaTime);
	controlsColumn.Update(deltaTime);
	languageColumn.Update(deltaTime);

	if (page == Page::ToSub || page == Page::ToCategories)
	{
		pageFraction += deltaTime / SlideDuration;
		if (pageFraction >= 1.f)
		{
			pageFraction = 0.f;
			page = (page == Page::ToSub) ? Page::Sub : Page::Categories;
		}
	}

	ApplyColumnShifts();

	const bool categoriesActive = page == Page::Categories;

	if (categoriesActive && !openIndex.has_value() && column.GetSelectedIndex() != previewIndex)
	{
		previewIndex = column.GetSelectedIndex();
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
		if (categoriesActive && openIndex.has_value() && *openIndex == i)
		{
			visibility = OptionsCategoryPanel::Visibility::Open;
		}
		else if (categoriesActive && !openIndex.has_value() && previewIndex == i)
		{
			visibility = OptionsCategoryPanel::Visibility::Preview;
		}

		panels[i]->SetVisibility(visibility, previewFade);
		panels[i]->Update(deltaTime);
	}

	if (openIndex.has_value() && panels[*openIndex] && panels[*openIndex]->IsCloseRequested())
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
		if (openControlsItem.has_value() && *openControlsItem == i)
		{
			visibility = OptionsCategoryPanel::Visibility::Open;
		}
		else if (controlsLive && !openControlsItem.has_value() && controlsColumn.GetSelectedIndex() == i)
		{
			visibility = OptionsCategoryPanel::Visibility::Preview;
		}

		controlsPanels[i]->SetVisibility(visibility, previewFade);
		controlsPanels[i]->Update(deltaTime);
	}

	if (openControlsItem.has_value() && controlsPanels[*openControlsItem]->IsCloseRequested())
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
		const bool asFlyout = page == Page::Categories && column.GetSelectedIndex() == categoryRow;
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
