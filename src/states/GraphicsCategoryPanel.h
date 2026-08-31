#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "../rendering/NeonGlow.h"
#include "../settings/GameSettings.h"
#include "../ui/ConfirmDialog.h"
#include "../ui/MenuLabel.h"
#include "../ui/NineSliceFrame.h"
#include "../ui/OptionRow.h"
#include "OptionsCategoryPanel.h"

struct Context;

namespace sf
{
	class Event;
	class RenderTarget;
}

// The Graphics category content: a framed list of setting rows (resolution and
// window mode carousels; vertical sync / show FPS / CRT filter toggles) and an
// Apply / Reset / Back button row. Edits collect in a working copy; Apply
// writes and saves them (and queues a window recreation if the display mode
// changed); Reset returns to the factory defaults; Back with unsaved changes
// asks to save first.
class GraphicsCategoryPanel final : public OptionsCategoryPanel
{
public:
	GraphicsCategoryPanel(Context& context, sf::Color accent);

	void Open() override;
	void Close() override;

	void SetVisibility(Visibility visibility, float previewFade) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;
	bool HandleEvent(const sf::Event& event) override;

	[[nodiscard]] bool WantsToStayOpen() const override;
	[[nodiscard]] bool WantsToClose() const override { return closeRequested; }

private:
	enum class Focus { Rows, Buttons };
	enum ButtonId : std::size_t { Apply = 0, Reset = 1, Back = 2, ButtonCount = 3 };

	void BuildRows();
	void LayOutButtons();
	void SyncRows();

	void MoveVertical(int direction);
	void MoveHorizontal(int direction);
	void ConfirmFocused();
	void BackPressed();
	void DoApply();
	void DoReset();

	[[nodiscard]] std::size_t ResolutionIndexFor(sf::Vector2u resolution) const;
	[[nodiscard]] GameSettings Defaults() const;
	[[nodiscard]] bool GraphicsMatch(const GameSettings& a, const GameSettings& b) const;
	[[nodiscard]] bool IsDirty() const;
	[[nodiscard]] bool IsAtDefaults() const;
	[[nodiscard]] bool ButtonEnabled(std::size_t index) const;
	[[nodiscard]] std::size_t FirstEnabledButton() const;
	[[nodiscard]] std::size_t LastEnabledRow() const;

	Context& context;
	sf::Color accent;

	UI::NineSliceFrame frame;

	std::vector<std::unique_ptr<UI::OptionRow>> rows;
	UI::CarouselRow* resolutionRowPtr = nullptr;
	UI::CarouselRow* windowModeRowPtr = nullptr;
	UI::ToggleRow* vsyncRowPtr = nullptr;
	UI::ToggleRow* showFpsRowPtr = nullptr;
	UI::ToggleRow* crtRowPtr = nullptr;

	std::array<UI::MenuLabel, ButtonCount> buttons;
	std::array<sf::Vector2f, ButtonCount> buttonPositions{};
	std::array<std::optional<UI::NineSliceFrame>, ButtonCount> buttonFrames;
	mutable NeonGlow buttonGlow;

	UI::ConfirmDialog dialog;

	std::vector<sf::Vector2u> resolutions;
	GameSettings working;
	GameSettings applied;

	Focus focus = Focus::Rows;
	std::size_t selectedRow = 0;
	std::size_t selectedButton = ButtonId::Back;

	float alpha = 0.f;
	float targetAlpha = 0.f;
	bool active = false;
	bool closeRequested = false;
};
