#include "GameOverState.h"

#include <memory>
#include <string>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>

#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../statistics/HighScoreManager.h"
#include "../ui/Button.h"
#include "../ui/Label.h"
#include "../ui/Spacer.h"
#include "GameplayState.h"
#include "MenuShell.h"

namespace
{
	constexpr float TopSpacing = 220.f;
	constexpr float MenuGap = 30.f;
	constexpr unsigned int TitleSize = 220;
	constexpr unsigned int ScoreSize = 70;
}

GameOverState::GameOverState(Context& context, int finalScore)
	: MenuScreenState(context)
	, finalScore(finalScore)
	, isHighScore(context.highScores.IsHighScore(finalScore))
{
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
				auto label = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main), "_", 70);
				label->SetFillColor(sf::Color::White);
				playerNameLabel = label.get();
				layout->Add(std::move(label));
			}

			rootLayout.Add(std::move(layout));
		}

		UI::Button& save = AddMenuItem(context.localization.GetText(TextKey::GameOver::Save), [this] { SaveRecordAndLeave(); });
		save.SetDisabledStyle({ .backgroundColor = sf::Color(80, 80, 80), .textColor = sf::Color(110, 110, 110) });
		save.SetEnabled(false);
		saveButton = &save;
	}
	else
	{
		AddMenuItem(context.localization.GetText(TextKey::GameOver::Restart), [this]
			{
				RequestClear();
				RequestPush(std::make_unique<GameplayState>(this->context));
			});
		AddMenuItem(context.localization.GetText(TextKey::GameOver::MainMenu), [this]
			{
				RequestClear();
				RequestPush(std::make_unique<MenuShell>(this->context));
			});
	}

	rootLayout.Add(std::move(menuLayoutElement));

	RefreshSaveButton();
	RefreshLayout();

	context.music.Get(Assets::MusicID::Gameplay).stop();

	sf::Music& music = context.music.Get(Assets::MusicID::GameOver);
	if (music.getStatus() != sf::Music::Status::Playing)
	{
		music.play();
	}
}

void GameOverState::OnBack()
{
	// Leave without saving.
	RequestClear();
	RequestPush(std::make_unique<MenuShell>(this->context));
}

bool GameOverState::HandleExtraEvent(const sf::Event& event)
{
	if (const auto* textEntered = event.getIf<sf::Event::TextEntered>())
	{
		if (isHighScore)
		{
			HandleTextInput(textEntered->unicode);
		}
		return true;
	}

	return false;
}

void GameOverState::SaveRecordAndLeave()
{
	// AddMenuItem's activation only runs for an enabled button, which tracks
	// IsPlayerNameValid(); this is a belt-and-braces guard.
	if (!IsPlayerNameValid())
	{
		return;
	}

	context.highScores.AddRecord({ TrimPlayerName(playerName), finalScore });
	context.highScores.Save();

	RequestClear();
	RequestPush(std::make_unique<MenuShell>(this->context));
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

void GameOverState::Render(sf::RenderTarget& target)
{
	sf::RectangleShape overlay(target.getView().getSize());
	overlay.setFillColor(sf::Color(0, 0, 0, 220));
	target.draw(overlay);

	RenderMenu(target);
}
