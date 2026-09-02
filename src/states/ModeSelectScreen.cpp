#include "ModeSelectScreen.h"

#include <algorithm>
#include <memory>

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
#include "GameplayState.h"
#include "MenuShell.h"

namespace
{
	constexpr unsigned int ButtonTextSize = 38;
	constexpr sf::Vector2f ColumnTopLeft{ 130.f, 300.f };   // matches OptionsScreen
	constexpr float RowGap = 120.f;

	constexpr float SubRowGap = 96.f;   // tighter spacing for a sub-list column
	constexpr float FlyoutX = 470.f;    // the sub-list's x while it is a hover flyout
	constexpr float FlyoutDim = 0.42f;

	constexpr float SlideDuration = 0.34f;
	constexpr float ColumnExitShiftX = -1500.f;

	// Row order in the main column.
	enum Row : std::size_t { Campaign = 0, OtherModes = 1, Back = 2 };

	constexpr sf::Color CampaignColour{ 240, 180, 90 };     // amber
	constexpr sf::Color OtherModesColour{ 80, 200, 140 };   // emerald

	[[nodiscard]] float SmoothStep(float t) noexcept
	{
		t = std::clamp(t, 0.f, 1.f);
		return t * t * (3.f - 2.f * t);
	}

	[[nodiscard]] sf::Vector2f Lerp(sf::Vector2f a, sf::Vector2f b, float t) noexcept
	{
		return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
	}

	// The render shift that places a sub-list as a hover flyout: pushed right of
	// the main column, top-aligned with it so the list always clears the header
	// and reads as a separate panel (the main column's short "Back" keeps it
	// from colliding horizontally). Sliding in, it eases from here to the main
	// column's own slot.
	[[nodiscard]] sf::Vector2f FlyoutShift() noexcept
	{
		return { FlyoutX - ColumnTopLeft.x, 0.f };
	}
}

ModeSelectScreen::ModeSelectScreen(MenuShell& shell, sf::Color accent)
	: MenuScreen(shell)
	, accent(accent)
	, column(context.fonts.Get(Assets::FontID::MenuList), ButtonTextSize,
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, campaignColumn(context.fonts.Get(Assets::FontID::MenuList), ButtonTextSize,
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, otherModesColumn(context.fonts.Get(Assets::FontID::MenuList), ButtonTextSize,
		context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{
	const LocalizationManager& text = context.localization;

	column.AddButton(text.GetText(TextKey::ModeSelect::Campaign), [this] { OpenSub(Row::Campaign); }, true, CampaignColour);
	column.AddButton(text.GetText(TextKey::ModeSelect::OtherModes), [this] { OpenSub(Row::OtherModes); }, true, OtherModesColour);
	column.AddButton(text.GetText(TextKey::ModeSelect::Back), [this] { Leave(); }, true);   // white

	column.SetLayout(ColumnTopLeft, RowGap);
	column.SetSelectionChangedCallback([this](std::size_t)
		{
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		});
	column.SetSwooshCallback([this](std::size_t index)
		{
			context.audioPlayer.Play(Assets::SoundID::MenuItemAppeared, 1.02f + 0.05f * static_cast<float>(index));
		});

	const auto subSelectionSound = [this](std::size_t)
		{
			context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		};

	// Campaign: every entry is a stub until v1.6.0.
	campaignColumn.AddButton(text.GetText(TextKey::ModeSelect::StartCampaign), nullptr, false);
	campaignColumn.AddButton(text.GetText(TextKey::ModeSelect::ContinueCampaign), nullptr, false);
	campaignColumn.AddButton(text.GetText(TextKey::ModeSelect::SelectLevel), nullptr, false);
	campaignColumn.AddButton(text.GetText(TextKey::ModeSelect::SubBack), [this] { CloseSub(); }, true);   // white
	campaignColumn.SetLayout(ColumnTopLeft, SubRowGap);
	campaignColumn.SetSelectionChangedCallback(subSelectionSound);
	campaignColumn.AppearInstantly();

	// Other Modes: only Marathon is live; the rest land in v1.5.0 / v1.7.0.
	otherModesColumn.AddButton(text.GetText(TextKey::ModeSelect::Marathon), [this] { StartMarathon(); }, true, OtherModesColour);
	otherModesColumn.AddButton(text.GetText(TextKey::ModeSelect::Sprint), nullptr, false);
	otherModesColumn.AddButton(text.GetText(TextKey::ModeSelect::Ultra), nullptr, false);
	otherModesColumn.AddButton(text.GetText(TextKey::ModeSelect::Zen), nullptr, false);
	otherModesColumn.AddButton(text.GetText(TextKey::ModeSelect::Versus), nullptr, false);
	otherModesColumn.AddButton(text.GetText(TextKey::ModeSelect::SubBack), [this] { CloseSub(); }, true);   // white
	otherModesColumn.SetLayout(ColumnTopLeft, SubRowGap);
	otherModesColumn.SetSelectionChangedCallback(subSelectionSound);
	otherModesColumn.AppearInstantly();

	ApplyColumnShifts();
}

UI::MenuButtonColumn& ModeSelectScreen::SubColumnFor(std::size_t mainRow)
{
	return mainRow == Row::Campaign ? campaignColumn : otherModesColumn;
}

void ModeSelectScreen::PlayIntro()
{
	column.Begin();
}

void ModeSelectScreen::StartExit()
{
	column.PlayExit();
}

bool ModeSelectScreen::ExitFinished() const
{
	return column.IsExitDone();
}

std::pair<std::string_view, sf::Color> ModeSelectScreen::CurrentLightbar() const
{
	if (page != Page::Main)
	{
		return subRow == Row::Campaign
			? std::pair<std::string_view, sf::Color>{ "mode_select_campaign", CampaignColour }
			: std::pair<std::string_view, sf::Color>{ "mode_select_other", OtherModesColour };
	}

	switch (column.SelectedIndex())
	{
	case Row::Campaign:   return { "mode_select_campaign", CampaignColour };
	case Row::OtherModes: return { "mode_select_other", OtherModesColour };
	default:              return { "mode_select", accent };
	}
}

std::optional<sf::Color> ModeSelectScreen::LightbarColour() const
{
	const auto [key, fallback] = CurrentLightbar();
	const HapticSettings::Colour resolved =
		context.hapticSettings.LightbarFor(key, { fallback.r, fallback.g, fallback.b });
	return sf::Color(resolved.r, resolved.g, resolved.b);
}

void ModeSelectScreen::Leave()
{
	if (leaving)
	{
		return;
	}

	leaving = true;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	shell.BeginBack();
}

void ModeSelectScreen::StartMarathon()
{
	if (leaving)
	{
		return;
	}

	leaving = true;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 1.05f);
	shell.ExitTo(std::make_unique<GameplayState>(context));
}

void ModeSelectScreen::OpenSub(std::size_t mainRow)
{
	if (page != Page::Main)
	{
		return;
	}

	subRow = mainRow;
	page = Page::ToSub;
	pageT = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 1.05f);
}

void ModeSelectScreen::CloseSub()
{
	if (page != Page::Sub)
	{
		return;
	}

	page = Page::ToMain;
	pageT = 0.f;
	context.audioPlayer.Play(Assets::SoundID::MenuItemSelected, 0.78f);
}

void ModeSelectScreen::ApplyColumnShifts()
{
	float slide = 0.f;   // 0 = main column fully in, 1 = the sub-list fully in
	switch (page)
	{
	case Page::Main:   slide = 0.f; break;
	case Page::ToSub:  slide = SmoothStep(pageT); break;
	case Page::Sub:    slide = 1.f; break;
	case Page::ToMain: slide = 1.f - SmoothStep(pageT); break;
	}

	column.SetRenderShift({ ColumnExitShiftX * slide, 0.f });

	const bool onMain = page == Page::Main;
	const auto configure = [&](UI::MenuButtonColumn& sub, std::size_t mainRow)
	{
		const sf::Vector2f flyout = FlyoutShift();
		const bool isActive = !onMain && subRow == mainRow;
		const bool isFlyout = onMain && column.SelectedIndex() == mainRow;

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

		sub.SetSelectionHighlight(isActive && (page == Page::Sub || page == Page::ToSub));
	};
	configure(campaignColumn, Row::Campaign);
	configure(otherModesColumn, Row::OtherModes);
}

void ModeSelectScreen::HandleEvent(const sf::Event& event)
{
	if (leaving || page == Page::ToSub || page == Page::ToMain)
	{
		return;   // no input mid-slide
	}

	if (page == Page::Sub)
	{
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

void ModeSelectScreen::Update(float deltaTime)
{
	column.Update(deltaTime);
	campaignColumn.Update(deltaTime);
	otherModesColumn.Update(deltaTime);

	if (page == Page::ToSub || page == Page::ToMain)
	{
		pageT += deltaTime / SlideDuration;
		if (pageT >= 1.f)
		{
			pageT = 0.f;
			page = (page == Page::ToSub) ? Page::Sub : Page::Main;
		}
	}

	ApplyColumnShifts();
}

void ModeSelectScreen::Render(sf::RenderTarget& target)
{
	column.Render(target);

	const auto renderSub = [&](UI::MenuButtonColumn& sub, std::size_t mainRow)
	{
		const bool asPage = page != Page::Main && subRow == mainRow;
		const bool asFlyout = page == Page::Main && column.SelectedIndex() == mainRow;
		if (asPage || asFlyout)
		{
			sub.Render(target);
		}
	};
	renderSub(campaignColumn, Row::Campaign);
	renderSub(otherModesColumn, Row::OtherModes);
}
