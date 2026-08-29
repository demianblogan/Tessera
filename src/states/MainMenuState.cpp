#include "MainMenuState.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/StateMachine.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "SettingsState.h"
#include "StatisticsState.h"
#include "GameplayState.h"

namespace
{
	constexpr float TopSpacing = 100.f;
	constexpr float TitleMenuSpacing = 90.f;
	constexpr float MenuGap = 30.f;
	constexpr float ButtonWidth = 500.f;
	constexpr float ButtonHeight = 120.f;
	constexpr unsigned int TitleSize = 300;
	constexpr unsigned int ButtonTextSize = 80;
}

MainMenuState::MainMenuState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, neonGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, rootLayout(UI::Layout::Orientation::Vertical)
	, backgroundSprite(context.textures.Get(Assets::TextureID::MenuBackground))
	, titleBackgroundSprite(context.textures.Get(Assets::TextureID::TitleBackground))
{
	backgroundSprite.setColor(sf::Color(255, 255, 255, 180));

	titleBackgroundSprite.setScale({ 0.6f, 0.45f });
	titleBackgroundSprite.setPosition({ 565.f, 30.f });

	rootLayout.SetHorizontalAlignment(UI::Layout::Alignment::Center);
	rootLayout.SetVerticalAlignment(UI::Layout::Alignment::Start);
	rootLayout.SetGap(0.f);

	rootLayout.Add(std::make_unique<UI::Spacer>(sf::Vector2f{ 0.f, TopSpacing }));

	{
		auto title = std::make_unique<UI::Label>(
			context.fonts.Get(Assets::FontID::Main),
			context.localization.GetText(TextKey::MainMenu::Title),
			TitleSize
		);

		title->SetFillColor(sf::Color::White);

		rootLayout.Add(std::move(title));
	}

	rootLayout.Add(std::make_unique<UI::Spacer>(sf::Vector2f{ 0.f, TitleMenuSpacing }));

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

	CreateMenuButton(context.localization.GetText(TextKey::MainMenu::StartGame), MenuAction::StartGame);
	CreateMenuButton(context.localization.GetText(TextKey::MainMenu::Options), MenuAction::Options);
	CreateMenuButton(context.localization.GetText(TextKey::MainMenu::Statistics), MenuAction::Statistics);
	CreateMenuButton(context.localization.GetText(TextKey::MainMenu::Exit), MenuAction::Exit);

	UpdateLayout();

	context.music.Get(Assets::MusicID::Gameplay).stop();
	context.music.Get(Assets::MusicID::GameOver).stop();

	sf::Music& music = context.music.Get(Assets::MusicID::MainMenu);
	music.setLooping(true);

	if (music.getStatus() != sf::Music::Status::Playing)
	{
		music.play();
	}
}

void MainMenuState::CreateMenuButton(const sf::String& text, MenuAction action)
{
	sf::Sprite buttonSprite(context.textures.Get(Assets::TextureID::ButtonBackground));

	auto button = std::make_unique<UI::Button>(buttonSprite);
	button->SetLabel(std::make_unique<UI::Label>(
		context.fonts.Get(Assets::FontID::Main),
		text,
		ButtonTextSize)
	);
	button->SetWidthPixels(ButtonWidth);
	button->SetHeightPixels(ButtonHeight);
	button->SetNormalStyle({ .backgroundColor = sf::Color(140, 140, 140), .textColor = sf::Color::White });
	button->SetSelectedStyle({ .backgroundColor = sf::Color(200, 200, 200), .textColor = sf::Color::Yellow });

	menuList.AddButton(*button);
	menuLayout->Add(std::move(button));
	actions.push_back(action);
}

void MainMenuState::HandleEvent(const sf::Event& event)
{
	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Up:      menuList.SelectPrevious(); return;
	case MenuInput::Action::Down:    menuList.SelectNext();     return;
	case MenuInput::Action::Confirm: menuList.Activate();       return;
	case MenuInput::Action::Back:    context.window.close();    return;
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

void MainMenuState::Update(float deltaTime)
{
	neonGlow.Update(deltaTime);
}

void MainMenuState::Render(sf::RenderTarget& target)
{
	target.draw(backgroundSprite);
	target.draw(titleBackgroundSprite);

	rootLayout.Render(target, &neonGlow);
}

void MainMenuState::UpdateLayout()
{
	const sf::Vector2f viewSize = context.window.getView().getSize();
	rootLayout.Arrange({ 0.f, 0.f }, viewSize);
}

void MainMenuState::PerformAction(MenuAction action)
{
	switch (action)
	{
	case MenuAction::StartGame:
		RequestChange(std::make_unique<GameplayState>(context));
		break;

	case MenuAction::Options:
		RequestChange(std::make_unique<SettingsState>(context));
		break;

	case MenuAction::Statistics:
		RequestChange(std::make_unique<StatisticsState>(context));
		break;

	case MenuAction::Exit:
		context.window.close();
		break;
	}
}
