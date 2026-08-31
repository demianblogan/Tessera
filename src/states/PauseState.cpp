#include "PauseState.h"

#include <memory>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../ui/Label.h"
#include "../ui/Spacer.h"
#include "MenuShell.h"
#include "GameplayState.h"

namespace
{
	constexpr float MenuGap = 30.f;
	constexpr unsigned int TitleSize = 220;
}

PauseState::PauseState(Context& context)
	: MenuScreenState(context)
{
	rootLayout.SetGap(80.f);

	rootLayout.Add(std::make_unique<UI::Spacer>(sf::Vector2f{ 0.f, 120.f }));

	{
		auto title = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::Pause::Title), TitleSize);
		title->SetFillColor(sf::Color::White);
		rootLayout.Add(std::move(title));
	}

	{
		auto layout = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);
		layout->SetGap(MenuGap);
		layout->SetHorizontalAlignment(UI::Layout::Alignment::Center);
		menuLayout = layout.get();
		rootLayout.Add(std::move(layout));
	}

	AddMenuItem(context.localization.GetText(TextKey::Pause::Resume), [this] { RequestPop(); });

	AddMenuItem(context.localization.GetText(TextKey::Pause::Restart), [this]
		{
			RequestClear();
			RequestPush(std::make_unique<GameplayState>(this->context));
		});

	AddMenuItem(context.localization.GetText(TextKey::Pause::MainMenu), [this]
		{
			RequestClear();
			RequestPush(std::make_unique<MenuShell>(this->context));
		});

	RefreshLayout();
}

void PauseState::OnBack()
{
	RequestPop();
}

void PauseState::Render(sf::RenderTarget& target)
{
	sf::RectangleShape overlay(target.getView().getSize());
	overlay.setFillColor(sf::Color(0, 0, 0, 180));
	target.draw(overlay);

	RenderMenu(target);
}

State::Backdrop PauseState::GetBackdrop() const
{
	return Backdrop::BlurredPrevious;
}
