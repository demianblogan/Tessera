#include "LoadingState.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <utility>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
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

	constexpr sf::Vector2f BarSize{ 1200.f, 64.f };
	constexpr float BarOutline = 3.f;
	constexpr float BarInnerPadding = 10.f;
	constexpr float CellGap = 3.f;
	constexpr float BarCentreY = 640.f;

	constexpr unsigned int LabelSize = 40;
	constexpr float LabelGap = 34.f;

	// How fast the drawn fill chases the loader's coarse per-stage fraction,
	// and how a freshly filled cell slides + fades into its slot.
	constexpr float FractionSmoothing = 4.f;
	constexpr float CellAppearDuration = 0.4f;
	constexpr float CellSlideCells = 2.5f;

	// A short beat after the last block settles before leaving the screen.
	constexpr float HandoffLinger = 0.35f;

	constexpr sf::Color Background{ 12, 14, 22 };
	constexpr sf::Color BarFrame{ 90, 120, 160 };
	constexpr sf::Color BarTrack{ 8, 10, 16, 220 };

	// Classic tetromino colours, brightened a little for the neon look.
	constexpr std::array<sf::Color, 7> BlockPalette{
		sf::Color{ 0, 240, 240 },   // I
		sf::Color{ 245, 220, 40 },  // O
		sf::Color{ 180, 60, 240 },  // T
		sf::Color{ 60, 230, 90 },   // S
		sf::Color{ 240, 60, 70 },   // Z
		sf::Color{ 70, 110, 240 },  // J
		sf::Color{ 245, 160, 40 },  // L
	};

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
	, stageLabel(context.fonts.Get(Assets::FontID::Pixel), "", LabelSize)
{
	stageLabel.setFillColor(sf::Color::White);

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

	const sf::Vector2f barTopLeft{
		(VirtualSize.x - BarSize.x) * 0.5f,
		BarCentreY - BarSize.y * 0.5f };

	sf::RectangleShape frame(BarSize);
	frame.setPosition(barTopLeft);
	frame.setFillColor(BarTrack);
	frame.setOutlineThickness(BarOutline);
	frame.setOutlineColor(BarFrame);
	target.draw(frame);

	const float innerLeft = barTopLeft.x + BarInnerPadding;
	const float innerTop = barTopLeft.y + BarInnerPadding;
	const float innerWidth = BarSize.x - 2.f * BarInnerPadding;
	const float slotWidth = innerWidth / static_cast<float>(CellCount);
	const float blockWidth = std::max(1.f, slotWidth - CellGap);
	const float blockHeight = BarSize.y - 2.f * BarInnerPadding;
	const float slideDistance = slotWidth * CellSlideCells;

	sf::RectangleShape block({ blockWidth, blockHeight });

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

		sf::Color colour = BlockPalette[static_cast<std::size_t>(i) % BlockPalette.size()];
		colour.a = static_cast<std::uint8_t>(eased * 255.f);

		block.setPosition({ x, innerTop });
		block.setFillColor(colour);
		target.draw(block);
	}

	stageLabel.setPosition({ VirtualSize.x * 0.5f, barTopLeft.y - LabelGap });
	target.draw(stageLabel);
}
