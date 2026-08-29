#include "MenuScreenState.h"

#include <utility>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../input/MenuInput.h"
#include "../resources/Assets.h"
#include "../ui/Button.h"
#include "../ui/Label.h"

MenuScreenState::MenuScreenState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, neonGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
{
	rootLayout.SetHorizontalAlignment(UI::Layout::Alignment::Center);
	rootLayout.SetVerticalAlignment(UI::Layout::Alignment::Start);

	menuList.onSelectionChanged = [this]
		{
			this->context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		};

	menuList.onActivate = [this](std::size_t index)
		{
			this->context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
			activations[index]();
		};
}

UI::Button& MenuScreenState::AddMenuItem(const sf::String& text, std::function<void()> onActivate)
{
	sf::Sprite sprite(context.textures.Get(Assets::TextureID::ButtonBackground));

	auto button = std::make_unique<UI::Button>(sprite);
	button->SetLabel(std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), text, menuButtonTextSize));
	button->SetPreferredSize(menuButtonSize);
	button->SetWidthPixels(menuButtonSize.x);
	button->SetHeightPixels(menuButtonSize.y);
	button->SetNormalStyle({ .backgroundColor = sf::Color(140, 140, 140), .textColor = sf::Color::White });
	button->SetSelectedStyle({ .backgroundColor = sf::Color(200, 200, 200), .textColor = sf::Color::Yellow });

	UI::Button& reference = *button;

	menuList.AddButton(reference);
	menuLayout->Add(std::move(button));
	activations.push_back(std::move(onActivate));

	return reference;
}

void MenuScreenState::RefreshLayout()
{
	rootLayout.Arrange({ 0.f, 0.f }, context.window.getView().getSize());
}

void MenuScreenState::RenderMenu(sf::RenderTarget& target)
{
	rootLayout.Render(target, &neonGlow);
}

bool MenuScreenState::HandleExtraEvent(const sf::Event& /* event */)
{
	return false;
}

void MenuScreenState::HandleEvent(const sf::Event& event)
{
	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Up:      menuList.SelectPrevious(); return;
	case MenuInput::Action::Down:    menuList.SelectNext();     return;
	case MenuInput::Action::Confirm: menuList.Activate();       return;
	case MenuInput::Action::Back:    OnBack();                  return;
	default:                                                    break;
	}

	if (HandleExtraEvent(event))
	{
		return;
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

void MenuScreenState::Update(float deltaTime)
{
	neonGlow.Update(deltaTime);
}
