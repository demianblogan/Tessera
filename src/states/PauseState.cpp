#include "PauseState.h"

#include <algorithm>
#include <cstdint>
#include <memory>

#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "../audio/AudioPlayer.h"
#include "../display/DisplayManager.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../ui/Label.h"
#include "../ui/Spacer.h"
#include "GameplayState.h"
#include "MenuShell.h"

namespace
{
	constexpr float MenuGap = 30.f;
	constexpr unsigned int TitleSize = 220;

	// "Solidified" look of the frozen frame.
	constexpr float MosaicCellPx = 22.f;
	constexpr float MosaicDarken = 0.58f;
	constexpr float MosaicFrontSoft = 46.f;

	// How long the frame takes to solidify (and, in reverse, to melt back).
	constexpr float SolidifyDuration = 0.32f;
}

PauseState::PauseState(Context& context, std::unique_ptr<sf::RenderTexture> frozenFrame)
	: MenuScreenState(context)
	, frozenFrame(std::move(frozenFrame))
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

	AddMenuItem(context.localization.GetText(TextKey::Pause::Resume), [this] { BeginResume(); });

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

PauseState::~PauseState() = default;

void PauseState::OnBack()
{
	BeginResume();
}

void PauseState::BeginResume()
{
	if (resuming)
	{
		return;
	}

	resuming = true;
	context.audioPlayer.Play(Assets::SoundID::MenuItemPressed, 0.9f);
}

void PauseState::HandleEvent(const sf::Event& event)
{
	// Ignore input until the frame has finished solidifying, and once Resume is
	// under way.
	if (resuming || reveal < 1.f)
	{
		return;
	}

	MenuScreenState::HandleEvent(event);
}

void PauseState::Update(float deltaTime)
{
	const float step = deltaTime / SolidifyDuration;

	if (resuming)
	{
		reveal -= step;
		if (reveal <= 0.f)
		{
			reveal = 0.f;
			RequestPop();
			return;
		}
	}
	else
	{
		reveal = std::min(1.f, reveal + step);
	}

	MenuScreenState::Update(deltaTime);
}

void PauseState::RenderFrozenBackdrop(sf::RenderTarget& target)
{
	if (!frozenFrame)
	{
		sf::RectangleShape overlay(target.getView().getSize());
		overlay.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(190.f * reveal)));
		target.draw(overlay);
		return;
	}

	sf::Shader& mosaic = context.shaders.Get(Assets::ShaderID::Mosaic);
	mosaic.setUniform("texture", sf::Shader::CurrentTexture);
	mosaic.setUniform("resolution", sf::Glsl::Vec2(Display::DisplayManager::VirtualSize));
	mosaic.setUniform("cellPx", MosaicCellPx);
	mosaic.setUniform("darken", MosaicDarken);
	mosaic.setUniform("reveal", reveal);
	mosaic.setUniform("frontSoft", MosaicFrontSoft);

	sf::Sprite frame(frozenFrame->getTexture());
	target.draw(frame, &mosaic);
}

void PauseState::Render(sf::RenderTarget& target)
{
	RenderFrozenBackdrop(target);

	// The menu belongs to the solidified state; hide it while the frame is
	// still setting or melting back.
	if (reveal >= 1.f && !resuming)
	{
		RenderMenu(target);
	}
}

State::Backdrop PauseState::GetBackdrop() const
{
	// We draw the (frozen) game frame ourselves; no need for the app to render
	// and blur the state below.
	return Backdrop::Opaque;
}
