#include "MainMenuState.h"

#include <memory>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../ui/Label.h"
#include "../ui/Spacer.h"
#include "SettingsState.h"
#include "StatisticsState.h"
#include "GameplayState.h"

namespace
{
	constexpr float TopSpacing = 100.f;
	constexpr float TitleMenuSpacing = 90.f;
	constexpr float MenuGap = 30.f;
	constexpr unsigned int TitleSize = 300;
}

MainMenuState::MainMenuState(Context& context)
	: MenuScreenState(context)
	, backgroundSprite(context.textures.Get(Assets::TextureID::MenuBackground))
	, titleBackgroundSprite(context.textures.Get(Assets::TextureID::TitleBackground))
{
	backgroundSprite.setColor(sf::Color(255, 255, 255, 180));

	titleBackgroundSprite.setScale({ 0.6f, 0.45f });
	titleBackgroundSprite.setPosition({ 565.f, 30.f });

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

	AddMenuItem(context.localization.GetText(TextKey::MainMenu::StartGame),
		[this] { RequestChange(std::make_unique<GameplayState>(this->context)); });
	AddMenuItem(context.localization.GetText(TextKey::MainMenu::Options),
		[this] { RequestChange(std::make_unique<SettingsState>(this->context)); });
	AddMenuItem(context.localization.GetText(TextKey::MainMenu::Statistics),
		[this] { RequestChange(std::make_unique<StatisticsState>(this->context)); });
	AddMenuItem(context.localization.GetText(TextKey::MainMenu::Exit),
		[this] { this->context.window.close(); });

	RefreshLayout();

	context.music.Get(Assets::MusicID::Gameplay).stop();
	context.music.Get(Assets::MusicID::GameOver).stop();

	sf::Music& music = context.music.Get(Assets::MusicID::MainMenu);
	music.setLooping(true);

	if (music.getStatus() != sf::Music::Status::Playing)
	{
		music.play();
	}
}

void MainMenuState::OnBack()
{
	context.window.close();
}

void MainMenuState::Render(sf::RenderTarget& target)
{
	target.draw(backgroundSprite);
	target.draw(titleBackgroundSprite);

	RenderMenu(target);
}
