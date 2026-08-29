#include "GameOverState.h"

#include <string>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/StateMachine.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../statistics/HighScoreManager.h"
#include "GameplayState.h"
#include "MainMenuState.h"

namespace
{
	constexpr float TopSpacing = 220.f;
	constexpr float MenuGap = 30.f;
	constexpr float ButtonWidth = 500.f;
	constexpr float ButtonHeight = 120.f;
	constexpr unsigned int TitleSize = 220;
	constexpr unsigned int ScoreSize = 70;
	constexpr unsigned int ButtonTextSize = 80;
}

GameOverState::GameOverState(Context& context, int finalScore)
	: State(context.stateMachine)
	, context(context)
	, neonGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, rootLayout(UI::Layout::Orientation::Vertical)
	, finalScore(finalScore)
	, isHighScore(context.highScores.IsHighScore(finalScore))
{
	rootLayout.SetHorizontalAlignment(UI::Layout::Alignment::Center);
	rootLayout.SetVerticalAlignment(UI::Layout::Alignment::Start);
	rootLayout.SetGap(60.f);

	rootLayout.Add(std::make_unique<UI::Spacer>(sf::Vector2f{ 0.f, TopSpacing }));

	{
		auto title = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::GameOver::Title), TitleSize);
		title->SetFillColor(sf::Color::White);
		rootLayout.Add(std::move(title));
	}

	{
		auto label = std::make_unique<UI::Label>(
			context.fonts.Get(Assets::FontID::Main),
			context.localization.FormatText(TextKey::GameOver::Score, "{score}", std::to_string(finalScore)),
			ScoreSize
		);
		label->SetFillColor(sf::Color::White);
		rootLayout.Add(std::move(label));
	}

	auto menuLayoutElement = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);
	menuLayoutElement->SetGap(MenuGap);
	menuLayoutElement->SetHorizontalAlignment(UI::Layout::Alignment::Center);
	menuLayout = menuLayoutElement.get();

	menuList.onSelectionChanged = [this]
		{
			this->context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
		};

	menuList.onActivate = [this](std::size_t index)
		{
			this->context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
			PerformAction(actions[index]);
		};

	if (isHighScore)
	{
		{
			auto label = std::make_unique<UI::Label>(
				context.fonts.Get(Assets::FontID::Main),
				context.localization.GetText(TextKey::GameOver::NewRecord),
				70
			);
			label->SetFillColor(sf::Color(255, 215, 0));
			rootLayout.Add(std::move(label));
		}

		{
			auto layout = std::make_unique<UI::Layout>(UI::Layout::Orientation::Horizontal);
			layout->SetGap(20.f);
			layout->SetHorizontalAlignment(UI::Layout::Alignment::Center);
			layout->SetVerticalAlignment(UI::Layout::Alignment::End);

			{
				auto label = std::make_unique<UI::Label>(
					context.fonts.Get(Assets::FontID::Main),
					context.localization.GetText(TextKey::GameOver::EnterName),
					70
				);
				label->SetFillColor(sf::Color::White);
				layout->Add(std::move(label));
			}

			{
				auto label = std::make_unique<UI::Label>(
					context.fonts.Get(Assets::FontID::Main),
					"_",
					70
				);
				label->SetFillColor(sf::Color::White);
				playerNameLabel = label.get();
				layout->Add(std::move(label));
			}

			rootLayout.Add(std::move(layout));
		}

		CreateMenuButton(context.localization.GetText(TextKey::GameOver::Save), MenuAction::SaveRecord);
	}
	else
	{
		CreateMenuButton(context.localization.GetText(TextKey::GameOver::Restart), MenuAction::RestartGame);
		CreateMenuButton(context.localization.GetText(TextKey::GameOver::MainMenu), MenuAction::MainMenu);
	}

	rootLayout.Add(std::move(menuLayoutElement));

	RefreshSaveButton();
	UpdateLayout();

	context.music.Get(Assets::MusicID::Gameplay).stop();

	sf::Music& music = context.music.Get(Assets::MusicID::GameOver);
	if (music.getStatus() != sf::Music::Status::Playing)
	{
		music.play();
	}
}

void GameOverState::HandleEvent(const sf::Event& event)
{
	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Up:      menuList.SelectPrevious(); return;
	case MenuInput::Action::Down:    menuList.SelectNext();     return;
	case MenuInput::Action::Confirm: menuList.Activate();       return;
	case MenuInput::Action::Back:
		// Leave without saving.
		RequestClear();
		RequestPush(std::make_unique<MainMenuState>(context));
		return;
	default:
		break;
	}

	if (const auto* textEntered = event.getIf<sf::Event::TextEntered>())
	{
		if (isHighScore)
		{
			HandleTextInput(textEntered->unicode);
		}
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

void GameOverState::Update(float deltaTime)
{
	neonGlow.Update(deltaTime);
}

void GameOverState::Render(sf::RenderTarget& target)
{
	sf::RectangleShape overlay;
	overlay.setPosition({ 0.f, 0.f });
	overlay.setSize(target.getView().getSize());
	overlay.setFillColor(sf::Color(0, 0, 0, 220));

	target.draw(overlay);

	rootLayout.Render(target, &neonGlow);
}

void GameOverState::CreateMenuButton(const sf::String& text, MenuAction action)
{
	sf::Sprite buttonSprite(context.textures.Get(Assets::TextureID::ButtonBackground));

	auto button = std::make_unique<UI::Button>(buttonSprite);
	button->SetPreferredSize({ ButtonWidth, ButtonHeight });
	button->SetWidthPixels(ButtonWidth);
	button->SetHeightPixels(ButtonHeight);
	button->SetLabel(std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), text, ButtonTextSize));
	button->SetNormalStyle({ .backgroundColor = sf::Color(140, 140, 140), .textColor = sf::Color::White });
	button->SetSelectedStyle({ .backgroundColor = sf::Color(200, 200, 200), .textColor = sf::Color::Yellow });
	button->SetDisabledStyle({ .backgroundColor = sf::Color(80, 80, 80), .textColor = sf::Color(110, 110, 110) });

	if (action == MenuAction::SaveRecord)
	{
		saveButton = button.get();
		button->SetEnabled(false);
	}

	menuList.AddButton(*button);
	menuLayout->Add(std::move(button));
	actions.push_back(action);
}

void GameOverState::PerformAction(MenuAction action)
{
	switch (action)
	{
	case MenuAction::RestartGame:
		RequestClear();
		RequestPush(std::make_unique<GameplayState>(context));
		break;

	case MenuAction::MainMenu:
		RequestClear();
		RequestPush(std::make_unique<MainMenuState>(context));
		break;

	case MenuAction::SaveRecord:
	{
		// Activate() already checks the button is enabled, which tracks
		// IsPlayerNameValid(); this is a belt-and-braces guard.
		if (!IsPlayerNameValid())
		{
			break;
		}

		context.highScores.AddRecord({ TrimPlayerName(playerName), finalScore });
		context.highScores.Save();

		RequestClear();
		RequestPush(std::make_unique<MainMenuState>(context));
		break;
	}
	}
}

void GameOverState::HandleTextInput(char32_t character)
{
	if (character == U'\b')
	{
		if (!playerName.isEmpty())
		{
			playerName.erase(playerName.getSize() - 1, 1);
		}
	}
	else if (character >= 32 && character != 127)
	{
		if (playerName.getSize() >= MaxNameLength)
		{
			return;
		}

		if (character == U' ' && playerName.isEmpty())
		{
			return;
		}

		playerName += character;
	}

	playerNameLabel->SetString(playerName + "_");
	RefreshSaveButton();
}

void GameOverState::RefreshSaveButton()
{
	if (saveButton == nullptr)
	{
		return;
	}

	saveButton->SetEnabled(IsPlayerNameValid());

	if (saveButton->IsEnabled())
	{
		menuList.Select(0, false);
	}
}

sf::String GameOverState::TrimPlayerName(const sf::String& string) const
{
	std::size_t start = 0;
	std::size_t end = string.getSize();

	while (start < end && string[start] == U' ')
	{
		start++;
	}

	while (end > start && string[end - 1] == U' ')
	{
		end--;
	}

	return string.substring(start, end - start);
}

bool GameOverState::IsPlayerNameValid() const
{
	for (char32_t character : playerName)
	{
		if (character != U' ')
		{
			return true;
		}
	}

	return false;
}

void GameOverState::UpdateLayout()
{
	const sf::Vector2f viewSize = context.window.getView().getSize();
	rootLayout.Arrange({ 0.f, 0.f }, viewSize);
}
