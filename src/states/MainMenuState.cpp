#include "MainMenuState.h"

#include <memory>
#include <string>
#include <utility>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>

#include "../audio/AudioPlayer.h"
#include "../core/Context.h"
#include "../core/GameVersion.h"
#include "../input/MenuInput.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "GameplayState.h"
#include "SettingsState.h"
#include "StatisticsState.h"

namespace
{
	constexpr unsigned int TitleCharSize = 200;
	constexpr sf::Vector2f TitleCenter{ 960.f, 430.f };

	constexpr unsigned int EntryCharSize = 64;
	constexpr float EntryTop = 700.f;
	constexpr float EntryGap = 96.f;

	constexpr sf::Color EntryNormal{ 150, 155, 165 };
	constexpr sf::Color EntrySelected{ 255, 255, 255 };

	constexpr unsigned int VersionTextSize = 34;
	constexpr sf::Vector2f VersionMargin{ 28.f, 22.f };
}

MainMenuState::MainMenuState(Context& context)
	: State(context.stateMachine)
	, context(context)
	, title(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::MainMenu::Title), TitleCharSize)
	, titleGlow(context.shaders.Get(Assets::ShaderID::NeonDilate), context.shaders.Get(Assets::ShaderID::NeonBlur))
	, versionText(context.fonts.Get(Assets::FontID::Main), std::string(GameVersion::Text), VersionTextSize)
{
	title.SetCenter(TitleCenter);

	versionText.setFillColor(sf::Color(150, 160, 170));
	const sf::FloatRect versionBounds = versionText.getLocalBounds();
	versionText.setOrigin(
		{
			versionBounds.position.x + versionBounds.size.x,
			versionBounds.position.y + versionBounds.size.y
		});

	AddEntry(context.localization.GetText(TextKey::MainMenu::StartGame),
		[this] { RequestChange(std::make_unique<GameplayState>(this->context)); });
	AddEntry(context.localization.GetText(TextKey::MainMenu::Options),
		[this] { RequestChange(std::make_unique<SettingsState>(this->context)); });
	AddEntry(context.localization.GetText(TextKey::MainMenu::Records),
		[this] { RequestChange(std::make_unique<StatisticsState>(this->context)); });
	AddEntry(context.localization.GetText(TextKey::MainMenu::Quit),
		[this] { this->context.window.close(); });

	context.music.Get(Assets::MusicID::Gameplay).stop();
	context.music.Get(Assets::MusicID::GameOver).stop();

	sf::Music& menuMusic = context.music.Get(Assets::MusicID::MainMenu);
	menuMusic.setLooping(true);
	if (menuMusic.getStatus() != sf::Music::Status::Playing)
	{
		menuMusic.play();
	}
}

void MainMenuState::AddEntry(const sf::String& text, std::function<void()> activate)
{
	sf::Text label(context.fonts.Get(Assets::FontID::Main), text, EntryCharSize);
	const sf::FloatRect bounds = label.getLocalBounds();
	label.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });

	entries.push_back({ std::move(label), std::move(activate) });
}

bool MainMenuState::MenuReady() const
{
	return title.IsFinished();
}

void MainMenuState::Select(std::size_t index)
{
	if (index == selected)
	{
		return;
	}

	selected = index;
	context.audioPlayer.Restart(Assets::SoundID::MenuItemSelected);
}

void MainMenuState::Activate()
{
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed);
	entries[selected].activate();
}

void MainMenuState::HandleEvent(const sf::Event& event)
{
	if (!MenuReady())
	{
		return;
	}

	switch (MenuInput::Resolve(event, context.gamepad))
	{
	case MenuInput::Action::Up:
		Select((selected + entries.size() - 1) % entries.size());
		return;
	case MenuInput::Action::Down:
		Select((selected + 1) % entries.size());
		return;
	case MenuInput::Action::Confirm:
		Activate();
		return;
	case MenuInput::Action::Back:
		context.window.close();
		return;
	default:
		break;
	}
}

void MainMenuState::Update(float deltaTime)
{
	title.Update(deltaTime);
	titleGlow.Update(deltaTime);
}

void MainMenuState::Render(sf::RenderTarget& target)
{
	target.clear(sf::Color::Black);

	title.Render(target, &titleGlow);

	if (MenuReady())
	{
		for (std::size_t i = 0; i < entries.size(); ++i)
		{
			sf::Text& label = entries[i].label;
			label.setPosition({ TitleCenter.x, EntryTop + static_cast<float>(i) * EntryGap });
			label.setFillColor(i == selected ? EntrySelected : EntryNormal);
			target.draw(label);
		}
	}

	versionText.setPosition(target.getView().getSize() - VersionMargin);
	target.draw(versionText);
}
