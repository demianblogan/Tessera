#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

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

// The Audio category content: two volume sliders (sound and music, 0..100% in
// 10% steps) and the shared Apply / Reset / Back button row. Apply writes and
// saves; Reset returns to 100 / 100; Back with unsaved changes asks first.
class AudioCategoryPanel final : public OptionsCategoryPanel
{
public:
	AudioCategoryPanel(Context& context, sf::Color accent);

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

	[[nodiscard]] bool Equal(const GameSettings& a, const GameSettings& b) const;
	[[nodiscard]] bool IsDirty() const;
	[[nodiscard]] bool IsAtDefaults() const;
	[[nodiscard]] bool ButtonEnabled(std::size_t index) const;
	[[nodiscard]] std::size_t FirstEnabledButton() const;

	Context& context;
	sf::Color accent;

	UI::NineSliceFrame frame;

	std::vector<std::unique_ptr<UI::OptionRow>> rows;
	UI::SliderRow* soundRowPtr = nullptr;
	UI::SliderRow* musicRowPtr = nullptr;

	std::array<UI::MenuLabel, ButtonCount> buttons;
	std::array<sf::Vector2f, ButtonCount> buttonPositions{};
	std::array<sf::FloatRect, ButtonCount> buttonBoxes{};

	UI::ConfirmDialog dialog;

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
