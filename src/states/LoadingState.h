#pragma once

#include <functional>
#include <thread>
#include <vector>

#include <SFML/Graphics/Text.hpp>

#include "../core/State.h"
#include "../loading/AssetLoadJob.h"
#include "../loading/LoadingProgress.h"

struct Context;

namespace sf
{
	class Event;
}

// The first screen the game shows: a Tetris-block progress bar that fills as a
// background thread loads every heavyweight asset. When the thread finishes it
// hands off to the company splash. Skipping is deliberately not allowed -- the
// assets genuinely have to be ready before anything else can run.
class LoadingState final : public State
{
public:
	LoadingState(Context& context, std::function<void()> onLoaded);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

private:
	static constexpr int CellCount = 48;

	void RefreshStageLabel();

	// The block spritesheet is loaded by the first stage, so its texture is
	// only safe to sample once that stage is behind us.
	[[nodiscard]] bool TexturesReady() const;

	Context& context;

	Loading::Progress progress;
	Loading::AssetLoadJob job;
	std::function<void()> onLoaded;
	std::jthread worker;

	sf::Text stageLabel;
	Loading::Stage labelledStage = Loading::Stage::Count;

	float elapsed = 0.f;
	float displayedFraction = 0.f;

	// elapsed-time stamp at which cell i started animating in; negative until
	// the bar reaches it.
	std::vector<float> cellAppearTime = std::vector<float>(CellCount, -1.f);

	bool handedOff = false;
};
