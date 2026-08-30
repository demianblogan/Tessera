#include "LoadingState.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <utility>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Window/Event.hpp>

#include "../core/Context.h"
#include "../core/StateMachine.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "CompanySplashState.h"

namespace
{
	constexpr sf::Vector2f VirtualSize{ 1920.f, 1080.f };

	constexpr float BarWidth = 1200.f;
	constexpr float BarOutline = 3.f;
	constexpr float BarInnerPadding = 10.f;
	constexpr float CellGap = 4.f;
	constexpr float BarCentreY = VirtualSize.y * 0.5f;

	// The block spritesheet is a horizontal strip of 16px cells; the first
	// five are the most distinct colours.
	constexpr int BlockSpriteSize = 16;
	constexpr int BlockVariants = 5;

	constexpr unsigned int LabelSize = 52;
	constexpr float LabelGap = 40.f;

	// How fast the drawn fill chases the loader's coarse per-stage fraction,
	// and how a freshly filled cell slides + fades into its slot.
	constexpr float FractionSmoothing = 4.f;
	constexpr float CellAppearDuration = 0.4f;
	constexpr float CellSlideCells = 1.8f;

	// A short beat after the last block settles before leaving the screen.
	constexpr float HandoffLinger = 0.35f;

	constexpr sf::Color Background{ 12, 14, 22 };
	constexpr sf::Color BarFrame{ 90, 120, 160 };
	constexpr sf::Color BarTrack{ 8, 10, 16, 220 };

	[[nodiscard]] std::string_view StageKey(Loading::Stage stage) noexcept
	{
		switch (stage)
		{
		case Loading::Stage::Textures:  return TextKey::Loading::Textures;
		case Loading::Stage::Audio:     return TextKey::Loading::Audio;
		case Loading::Stage::Music:     return TextKey::Loading::Music;
		case Loading::Stage::Shaders:   return TextKey::Loading::Shaders;
		case Loading::Stage::Interface:
		case Loading::Stage::Count:     return TextKey::Loading::Interface;
		}

		return TextKey::Loading::Interface;
	}

	[[nodiscard]] float EaseOutCubic(float t) noexcept
	{
		const float inverted = 1.f - t;
		return 1.f - inverted * inverted * inverted;
	}
}

LoadingState::LoadingState(Context& context, std::function<void()> onLoaded)
	: State(context.stateMachine)
	, context(context)
	, job(context.textures, context.soundBuffers, context.music, context.shaders, context.fonts)
	, onLoaded(std::move(onLoaded))
	, stageLabel(context.fonts.Get(Assets::FontID::Loading), "", LabelSize)
{
	stageLabel.setFillColor(sf::Color::White);

	// The shell music runs from here through the splash and into the menu.
	// (Loaded synchronously by Application so it is ready this early.)
	sf::Music& music = context.music.Get(Assets::MusicID::MainMenu);
	music.setLooping(true);
	if (music.getStatus() != sf::Music::Status::Playing)
	{
		music.play();
	}

	worker = std::jthread(
		[this](std::stop_token stopToken)
		{
			job.Run(std::move(stopToken), progress);
		});

	RefreshStageLabel();
}

void LoadingState::HandleEvent(const sf::Event& /*event*/)
{
	// No skipping: the assets have to finish loading regardless.
}

void LoadingState::RefreshStageLabel()
{
	const Loading::Stage stage = progress.GetStage();
	stageLabel.setString(context.localization.GetText(StageKey(stage)));

	const sf::FloatRect bounds = stageLabel.getLocalBounds();
	stageLabel.setOrigin({
		bounds.position.x + bounds.size.x * 0.5f,
		bounds.position.y + bounds.size.y });

	labelledStage = stage;
}

bool LoadingState::TexturesReady() const
{
	return progress.IsDone() || progress.GetStage() != Loading::Stage::Textures;
}

void LoadingState::Update(float deltaTime)
{
	elapsed += deltaTime;

	if (progress.GetStage() != labelledStage)
	{
		RefreshStageLabel();
	}

	const float target = progress.Fraction();
	displayedFraction += (target - displayedFraction) * std::min(1.f, FractionSmoothing * deltaTime);
	if (progress.IsDone() && displayedFraction > 0.999f)
	{
		displayedFraction = 1.f;
	}

	const int filledCells = std::clamp(
		static_cast<int>(std::lround(displayedFraction * CellCount)), 0, CellCount);

	for (int i = 0; i < filledCells; ++i)
	{
		if (cellAppearTime[static_cast<std::size_t>(i)] < 0.f)
		{
			cellAppearTime[static_cast<std::size_t>(i)] = elapsed;
		}
	}

	if (handedOff || !progress.IsDone() || filledCells < CellCount)
	{
		return;
	}

	const float lastSettled = cellAppearTime[static_cast<std::size_t>(CellCount - 1)] + CellAppearDuration + HandoffLinger;
	if (elapsed >= lastSettled)
	{
		handedOff = true;
		if (onLoaded)
		{
			onLoaded();
		}
		RequestChange(std::make_unique<CompanySplashState>(context));
	}
}

void LoadingState::Render(sf::RenderTarget& target)
{
	const sf::Vector2f viewSize = target.getView().getSize();

	sf::RectangleShape backdrop(viewSize);
	backdrop.setFillColor(Background);
	target.draw(backdrop);

	// Square blocks: pick the block size from the width budget, then let the
	// bar height follow from it.
	const float slotWidth = (BarWidth - 2.f * BarInnerPadding) / static_cast<float>(CellCount);
	const float blockSize = std::max(1.f, slotWidth - CellGap);
	const float barHeight = blockSize + 2.f * BarInnerPadding;
	const sf::Vector2f barSize{ BarWidth, barHeight };

	const sf::Vector2f barTopLeft{
		(VirtualSize.x - barSize.x) * 0.5f,
		BarCentreY - barSize.y * 0.5f };

	sf::RectangleShape frame(barSize);
	frame.setPosition(barTopLeft);
	frame.setFillColor(BarTrack);
	frame.setOutlineThickness(BarOutline);
	frame.setOutlineColor(BarFrame);
	target.draw(frame);

	if (TexturesReady())
	{
		const float innerLeft = barTopLeft.x + BarInnerPadding;
		const float innerTop = barTopLeft.y + BarInnerPadding;
		const float slideDistance = slotWidth * CellSlideCells;

		const sf::Texture& sheet = context.textures.Get(Assets::TextureID::BlockSpritesheetWithOutline);
		sf::Sprite block(sheet);
		block.setScale(sf::Vector2f{ blockSize, blockSize } / static_cast<float>(BlockSpriteSize));

		for (int i = 0; i < CellCount; ++i)
		{
			const float appearTime = cellAppearTime[static_cast<std::size_t>(i)];
			if (appearTime < 0.f)
			{
				continue;
			}

			const float t = std::clamp((elapsed - appearTime) / CellAppearDuration, 0.f, 1.f);
			const float eased = EaseOutCubic(t);

			const float slotX = innerLeft + static_cast<float>(i) * slotWidth;
			const float x = slotX + (1.f - eased) * slideDistance;

			block.setTextureRect(sf::IntRect{
				{ (i % BlockVariants) * BlockSpriteSize, 0 },
				{ BlockSpriteSize, BlockSpriteSize } });
			block.setPosition({ x, innerTop });
			block.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(eased * 255.f)));
			target.draw(block);
		}
	}

	stageLabel.setPosition({ VirtualSize.x * 0.5f, barTopLeft.y - LabelGap });
	target.draw(stageLabel);
}
