#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Keyboard.hpp>

#include "../settings/GameSettings.h"
#include "SettingsCategoryPanel.h"

struct Context;

namespace sf
{
	class Event;
	class RenderTarget;
}

namespace UI
{
	class KeyBindRow;
}

// Controls > Keyboard: a list of the six rebindable gameplay actions, each a
// KeyBindRow. Activating a row starts a key capture (the keycap blinks); the
// next key press is validated -- Escape cancels, a reserved / menu key is
// rejected with an amber flash, a key already used by another row flashes both
// rows red, otherwise it is accepted with a green flash. Menu navigation keys
// (arrows / Enter / Escape) and pause (Escape) are fixed and not listed.
class KeyboardCategoryPanel final : public SettingsCategoryPanel
{
public:
	KeyboardCategoryPanel(Context& context, sf::Color accent);

	bool HandleEvent(const sf::Event& event) override;
	void Close() override;
	[[nodiscard]] bool WantsToStayOpen() const override;

protected:
	void BuildRows() override;
	[[nodiscard]] bool SettingsEqual(const GameSettings& a, const GameSettings& b) const override;
	[[nodiscard]] GameSettings DefaultSettings() const override;
	void ApplyWorking() override;
	void ResetWorking() override;

	void AdjustRow(std::size_t index, int direction) override;
	void ActivateRow(std::size_t index) override;
	void RowClicked(std::size_t index) override;

private:
	static constexpr std::size_t ActionCount = 6;
	using Field = sf::Keyboard::Scancode ControlSettings::*;

	void SyncRows();
	void BeginCapture(std::size_t index, bool fromKeyboard);
	void EndCapture();
	void TryAssign(std::size_t index, sf::Keyboard::Scancode key);

	[[nodiscard]] static bool IsReserved(sf::Keyboard::Scancode key);
	[[nodiscard]] std::optional<std::size_t> ClashingRow(std::size_t index, sf::Keyboard::Scancode key) const;

	std::array<Field, ActionCount> fields{};
	std::array<UI::KeyBindRow*, ActionCount> rowPtrs{};

	std::optional<std::size_t> capturingRow;
	std::optional<sf::Keyboard::Scancode> captureIgnore;
};
