#include "PauseState.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/StateMachine.h"
#include "../input/MenuInput.h"
#include "../resources/Assets.h"
#include "MainMenuState.h"
#include "GameplayState.h"

namespace
{
	constexpr float MenuGap = 30.f;

	constexpr float ButtonWidth = 500.f;
	constexpr float ButtonHeight = 120.f;

	constexpr unsigned int TitleSize = 220;
	constexpr unsigned int ButtonTextSize = 80;
}

PauseState::PauseState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, rootLayout(UI::Layout::Orientation::Vertical)
{
	rootLayout.SetHorizontalAlignment(UI::Layout::Alignment::Center);
	rootLayout.SetVerticalAlignment(UI::Layout::Alignment::Start);
	rootLayout.SetGap(80.f);

	rootLayout.Add(std::make_unique<UI::Spacer>(sf::Vector2f{ 0.f, 120.f }));

	{
		auto title = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), "Pause", TitleSize);
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

	menuList.onSelectionChanged = [this]
		{
			this->context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		};

	menuList.onActivate = [this](std::size_t index)
		{
			this->context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
			PerformAction(actions[index]);
		};

	CreateMenuButton("Resume Game", MenuAction::ResumeGame);
	CreateMenuButton("Restart Game", MenuAction::RestartGame);
	CreateMenuButton("Main Menu", MenuAction::MainMenu);

	UpdateLayout();
}

void PauseState::HandleEvent(const sf::Event& event)
{
	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Up:      menuList.SelectPrevious(); return;
	case MenuInput::Action::Down:    menuList.SelectNext();     return;
	case MenuInput::Action::Confirm: menuList.Activate();       return;
	case MenuInput::Action::Back:    RequestPop();              return;
	default:                                                    break;
	}

	if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
	{
		menuList.SelectAt(context.window.mapPixelToCoords(moved->position));
	}
	else if (const auto* clicked = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (clicked->button == sf::Mouse::Button::Left)
		{
			menuList.PointerPressed(context.window.mapPixelToCoords(clicked->position));
		}
	}
}

void PauseState::Update(float deltaTime)
{
	// No code
}

void PauseState::Render(sf::RenderTarget& target)
{
	sf::RectangleShape overlay;
	overlay.setPosition({ 0.f, 0.f });
	overlay.setSize(target.getView().getSize());
	overlay.setFillColor(sf::Color(0, 0, 0, 180));

	target.draw(overlay);

	sf::Shader& glowShader = context.shaders.Get(Assets::ShaderID::Glow);
	rootLayout.Render(target, &glowShader, context.totalTime);
}

bool PauseState::IsTransparent() const
{
	return true;
}

void PauseState::CreateMenuButton(const sf::String& text, MenuAction action)
{
	sf::Sprite buttonSprite(context.textures.Get(Assets::TextureID::ButtonBackground));

	auto button = std::make_unique<UI::Button>(buttonSprite);
	button->SetLabel(std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), text, ButtonTextSize));
	button->SetWidthPixels(ButtonWidth);
	button->SetHeightPixels(ButtonHeight);
	button->SetNormalStyle({ .backgroundColor = sf::Color(140, 140, 140), .textColor = sf::Color::White });
	button->SetSelectedStyle({ .backgroundColor = sf::Color(200, 200, 200), .textColor = sf::Color::Yellow });

	menuList.AddButton(*button);
	menuLayout->Add(std::move(button));
	actions.push_back(action);
}

void PauseState::PerformAction(MenuAction action)
{
	switch (action)
	{
	case MenuAction::ResumeGame:
		RequestPop();
		break;

	case MenuAction::RestartGame:
		RequestClear();
		RequestPush(std::make_unique<GameplayState>(context));
		break;

	case MenuAction::MainMenu:
		RequestClear();
		RequestPush(std::make_unique<MainMenuState>(context));
		break;
	}
}

void PauseState::UpdateLayout()
{
	const sf::Vector2f viewSize = context.window.getView().getSize();
	rootLayout.Arrange({ 0.f, 0.f }, viewSize);
}
