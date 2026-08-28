#include "PauseState.h"

#include <optional>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/StateMachine.h"
#include "../resources/Assets.h"
#include "MainMenuState.h"
#include "GameState.h"

namespace
{
	constexpr float MenuGap = 30.f;

	constexpr float ButtonWidth = 500.f;
	constexpr float ButtonHeight = 120.f;

	constexpr unsigned int TitleSize = 220;
	constexpr unsigned int ButtonTextSize = 80;
}

PauseState::PauseState(Context& context)
	: context(context)
	, rootLayout(UI::Layout::Orientation::Vertical)
{
	rootLayout.SetHorizontalAlignment(UI::Layout::Alignment::Center);
	rootLayout.SetVerticalAlignment(UI::Layout::Alignment::Start);
	rootLayout.SetGap(80.f);

	// =====================================================
	// Top spacer
	// =====================================================

	rootLayout.Add(std::make_unique<UI::Spacer>(sf::Vector2f{ 0.f, 120.f }));

	// =====================================================
	// Title
	// =====================================================
	{
		auto title = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), "Pause", TitleSize);
		title->SetFillColor(sf::Color::White);
		rootLayout.Add(std::move(title));
	}

	// =====================================================
	// Menu layout
	// =====================================================
	{
		auto layout = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);
		layout->SetGap(MenuGap);
		layout->SetHorizontalAlignment(UI::Layout::Alignment::Center);
		menuLayout = layout.get();
		rootLayout.Add(std::move(layout));
	}

	// =====================================================
	// Buttons
	// =====================================================

	CreateMenuButton("Resume Game", MenuAction::ResumeGame);
	CreateMenuButton("Restart Game", MenuAction::RestartGame);
	CreateMenuButton("Main Menu", MenuAction::MainMenu);

	UpdateSelection();
	UpdateLayout();
}

void PauseState::ProcessEvents(sf::RenderWindow& window)
{
	while (const std::optional event = window.pollEvent())
	{
		if (event->is<sf::Event::Closed>())
		{
			window.close();
		}
		else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
		{
			switch (keyPressed->scancode)
			{
			case sf::Keyboard::Scancode::Escape:
				context.stateMachine.PopState();
				break;

			case sf::Keyboard::Scancode::Up:
				SelectPreviousMenuItem();
				context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
				break;

			case sf::Keyboard::Scancode::Down:
				SelectNextMenuItem();
				context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
				break;

			case sf::Keyboard::Scancode::Enter:
				context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
				ActivateSelectedButton();
				break;
			}
		}
	}
}

void PauseState::Update(float deltaTime)
{
	// No code
}

void PauseState::Render(sf::RenderTarget& target)
{
	// =====================================================
	// Dark overlay
	// =====================================================

	sf::RectangleShape overlay;
	overlay.setPosition({ 0.f, 0.f });
	overlay.setSize(target.getView().getSize());
	overlay.setFillColor(sf::Color(0, 0, 0, 180));

	target.draw(overlay);

	// =====================================================
	// UI
	// =====================================================

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
	button->SetSelectedStyle({ .backgroundColor = sf::Color(200, 200, 200),	.textColor = sf::Color::Yellow });

	UI::Button* buttonPointer = button.get();
	menuLayout->Add(std::move(button));

	buttons.push_back({ .button = buttonPointer,.action = action });
}

void PauseState::SelectPreviousMenuItem()
{
	selectedIndex--;

	if (selectedIndex < 0)
	{
		selectedIndex = static_cast<int>(buttons.size()) - 1;
	}

	UpdateSelection();
}

void PauseState::SelectNextMenuItem()
{
	selectedIndex++;

	if (selectedIndex >= static_cast<int>(buttons.size()))
	{
		selectedIndex = 0;
	}

	UpdateSelection();
}

void PauseState::UpdateSelection()
{
	for (std::size_t i = 0; i < buttons.size(); i++)
	{
		buttons[i].button->SetSelected(i == static_cast<std::size_t>(selectedIndex));
	}
}

void PauseState::ActivateSelectedButton()
{
	switch (buttons[selectedIndex].action)
	{
	case MenuAction::ResumeGame:
		context.stateMachine.PopState();
		break;

	case MenuAction::RestartGame:
		context.stateMachine.ClearStates();
		context.stateMachine.PushState(std::make_unique<GameState>(context));
		break;

	case MenuAction::MainMenu:
		context.stateMachine.ClearStates();
		context.stateMachine.PushState(std::make_unique<MainMenuState>(context));
		break;
	}
}

void PauseState::UpdateLayout()
{
	const sf::Vector2f viewSize = context.window.getView().getSize();
	rootLayout.Arrange({ 0.f, 0.f }, viewSize);
}