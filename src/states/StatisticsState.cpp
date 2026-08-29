#include "StatisticsState.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "../core/StateMachine.h"
#include "../input/MenuInput.h"
#include "../resources/Assets.h"
#include "MainMenuState.h"

namespace
{
	constexpr float TopSpacing = 80.f;
	constexpr float RootGap = 20.f;
	constexpr float ScoresGap = 30.f;
	constexpr float FooterGap = 20.f;
	constexpr float FooterSpacing = 80.f;
	constexpr unsigned int TitleSize = 120;
	constexpr unsigned int ScoreSize = 70;
	constexpr unsigned int FooterSize = 50;
}

StatisticsState::StatisticsState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, rootLayout(UI::Layout::Orientation::Vertical)
	, backgroundSprite(context.textures.Get(Assets::TextureID::MenuBackground))
{
	backgroundSprite.setColor(sf::Color(255, 255, 255, 180));

	rootLayout.SetHorizontalAlignment(UI::Layout::Alignment::Center);
	rootLayout.SetVerticalAlignment(UI::Layout::Alignment::Start);
	rootLayout.SetGap(RootGap);

	rootLayout.Add(std::make_unique<UI::Spacer>(sf::Vector2f{ 0.f, TopSpacing }));

	// =====================================================
	// Title
	// =====================================================
	{
		auto title = std::make_unique<UI::Label>(
			context.fonts.Get(Assets::FontID::Main),
			"TOP-" + std::to_string(HighScoreManager::MAX_RECORDS) + " BEST PLAYERS",
			TitleSize
		);

		title->SetFillColor(sf::Color::White);

		rootLayout.Add(std::move(title));
	}

	// =====================================================
	// Spacer between title and scores
	// =====================================================

	rootLayout.Add(std::make_unique<UI::Spacer>(sf::Vector2f{ 0.f, 130.f }));

	// =====================================================
	// Scores layout
	// =====================================================
	{
		auto scoresLayout = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);

		scoresLayout->SetGap(ScoresGap);
		scoresLayout->SetHorizontalAlignment(UI::Layout::Alignment::Center);

		for (std::size_t i = 0; i < HighScoreManager::MAX_RECORDS; i++)
		{
			auto label = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main),
				"...",
				ScoreSize
			);

			label->SetFillColor(sf::Color::White);
			scoreLabels[i] = label.get();

			scoresLayout->Add(std::move(label));
		}

		rootLayout.Add(std::move(scoresLayout));
	}

	UpdateScoreLabels();

	// =====================================================
	// Spacer between scores and footer
	// =====================================================

	rootLayout.Add(std::make_unique<UI::Spacer>(sf::Vector2f{ 0.f, FooterSpacing }));

	// =====================================================
	// Footer layout
	// =====================================================
	{
		auto footerLayout = std::make_unique<UI::Layout>(UI::Layout::Orientation::Vertical);

		footerLayout->SetGap(FooterGap);
		footerLayout->SetHorizontalAlignment(UI::Layout::Alignment::Center);

		// =================================================
		// Escape
		// =================================================
		{
			auto label = std::make_unique<UI::Label>(context.fonts.Get(Assets::FontID::Main),
				"ESC - RETURN TO MAIN MENU",
				FooterSize
			);

			label->SetFillColor(sf::Color(180, 180, 180));

			footerLayout->Add(std::move(label));
		}

		// =================================================
		// Delete
		// =================================================
		{
			auto label = std::make_unique<UI::Label>(
				context.fonts.Get(Assets::FontID::Main),
				"DEL - DELETE ALL RECORDS",
				FooterSize
			);

			label->SetFillColor(sf::Color(180, 180, 180));

			footerLayout->Add(std::move(label));
		}

		rootLayout.Add(std::move(footerLayout));
	}

	UpdateLayout();
}

void StatisticsState::UpdateScoreLabels()
{
	const std::vector<HighScoreEntry>& records = context.highScores.GetRecords();

	for (std::size_t i = 0; i < HighScoreManager::MAX_RECORDS; i++)
	{
		if (i < records.size())
		{
			scoreLabels[i]->SetString(std::to_string(i + 1) + ". " + records[i].playerName + " = " + std::to_string(records[i].score));
		}
		else
		{
			scoreLabels[i]->SetString(std::to_string(i + 1) + ". ...");
		}
	}

	UpdateLayout();
}

void StatisticsState::UpdateLayout()
{
	const sf::Vector2f viewSize = context.window.getView().getSize();
	rootLayout.Arrange({ 0.f, 0.f }, viewSize);
}

void StatisticsState::HandleEvent(const sf::Event& event)
{
	if (MenuInput::Resolve(event, context.gamepad) == MenuInput::Action::Back)
	{
		RequestChange(std::make_unique<MainMenuState>(context));
		return;
	}

	if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
	{
		if (keyPressed->scancode == sf::Keyboard::Scancode::Delete)
		{
			context.highScores.Clear();
			context.highScores.Save();
			UpdateScoreLabels();
		}
	}
}

void StatisticsState::Update(float /* deltaTime */)
{
	// Static screen; nothing advances per frame.
}

void StatisticsState::Render(sf::RenderTarget& target)
{
	target.draw(backgroundSprite);
	rootLayout.Render(target);
}