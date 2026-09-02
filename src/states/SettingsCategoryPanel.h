#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
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

// Shared frame for the Options category panels (Graphics, Audio, ...): a
// 9-slice frame, a list of OptionRows, an Apply / Reset / Back button row with
// dirty / defaults tracking, and the unsaved-changes dialog. Navigation, the
// dialog flow and the audio cues live here; a subclass only builds its rows and
// says how its slice of the settings compares, applies and resets.
class SettingsCategoryPanel : public OptionsCategoryPanel
{
public:
	void Open() override;
	void Close() override;

	void SetVisibility(Visibility visibility, float previewFade) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;
	bool HandleEvent(const sf::Event& event) override;

	[[nodiscard]] bool WantsToStayOpen() const override;
	[[nodiscard]] bool WantsToClose() const override { return closeRequested; }

protected:
	enum class Focus { Rows, Buttons };
	enum ButtonId : std::size_t { Apply = 0, Reset = 1, Back = 2, ButtonCount = 3 };

	SettingsCategoryPanel(Context& context, sf::Color accent, sf::FloatRect panelBounds);

	// --- subclass contract ---
	virtual void BuildRows() = 0;
	[[nodiscard]] virtual bool SettingsEqual(const GameSettings& a, const GameSettings& b) const = 0;
	[[nodiscard]] virtual GameSettings DefaultSettings() const = 0;
	// Copy `working` into the live settings, apply side effects, save.
	virtual void ApplyWorking() = 0;
	// Put `working` back to DefaultSettings() and re-sync the rows.
	virtual void ResetWorking() = 0;

	// Optional per-row behaviour; defaults call the row directly.
	virtual void AdjustRow(std::size_t index, int direction);
	virtual void ActivateRow(std::size_t index);
	virtual void RowClicked(std::size_t index);
	virtual void RenderExtra(sf::RenderTarget& /*target*/, float /*alpha*/) {}

	// Helpers for subclasses.
	void LayOutRows(float rowsTop, float rowMargin, float rowHeight, float rowGap);
	[[nodiscard]] bool IsDirty() const;
	[[nodiscard]] bool IsAtDefaults() const;
	[[nodiscard]] sf::FloatRect Panel() const { return panelBounds; }

	Context& context;
	sf::Color accent;

	std::vector<std::unique_ptr<UI::OptionRow>> rows;
	GameSettings working;
	GameSettings applied;
	std::size_t selectedRow = 0;

private:
	void LayOutButtons();
	void MoveVertical(int direction);
	void MoveButtons(int direction);
	void ConfirmFocused();
	void BackPressed();
	void DoApply();
	void DoReset();

	[[nodiscard]] bool ButtonEnabled(std::size_t index) const;
	[[nodiscard]] std::size_t FirstEnabledButton() const;
	[[nodiscard]] std::size_t LastEnabledRow() const;

	sf::FloatRect panelBounds;
	UI::NineSliceFrame frame;
	std::array<UI::MenuLabel, ButtonCount> buttons;
	std::array<sf::Vector2f, ButtonCount> buttonPositions{};
	std::array<sf::FloatRect, ButtonCount> buttonBoxes{};
	UI::ConfirmDialog dialog;

	Focus focus = Focus::Rows;
	std::size_t selectedButton = ButtonId::Back;

	float alpha = 0.f;
	float targetAlpha = 0.f;
	bool active = false;
	bool closeRequested = false;
};
