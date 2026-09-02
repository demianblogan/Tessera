#pragma once

#include <cstddef>
#include <memory>
#include <optional>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include "../ui/ConfirmDialog.h"
#include "ScreenHost.h"

namespace sf
{
	class Event;
	class RenderTarget;
	class RenderTexture;
}

// The in-game pause host. Its background is the gameplay frame captured the
// moment the game was paused, drawn through mosaic.frag so it "solidifies" into
// large pixels top-down on entry and melts back bottom-up on Resume. Its home
// screen is the PauseMenuScreen column; Options opens over the frozen frame
// through the shared ScreenHost transition. Restart and Back to Main Menu each
// raise a yes / no confirmation.
class PauseState final : public ScreenHost
{
public:
	// The amber of the "Campaign" entry in the mode-select menu.
	static constexpr sf::Color Accent{ 240, 180, 90 };

	// The Options red, for the header and OptionsScreen while it is open here.
	static constexpr sf::Color OptionsAccent{ 240, 60, 70 };

	// `frozenFrame` may be null (capture failed) -- then a plain dim overlay is
	// used instead of the mosaic.
	PauseState(Context& context, std::unique_ptr<sf::RenderTexture> frozenFrame);
	~PauseState() override;

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render(sf::RenderTarget& target) override;

	// Called by the pause menu column.
	void RequestResume();
	void RequestRestart();
	void RequestQuitToMainMenu();
	// `from` / `fromHeight` are the "Options" entry's on-screen box, for the
	// header to rise out of it.
	void OpenOptions(sf::Vector2f from, float fromHeight);

	[[nodiscard]] Backdrop GetBackdrop() const override { return Backdrop::Opaque; }

protected:
	void UpdateBackground(float deltaTime) override;
	void RenderBackground(sf::RenderTarget& target) override;
	[[nodiscard]] std::unique_ptr<MenuScreen> BuildHomeScreen(std::size_t returnEntryIndex) override;

	[[nodiscard]] bool HomeDrivesHeader() const override { return true; }
	void OnHomeRebuilt() override;

private:
	enum class PendingAction { None, Restart, QuitToMainMenu };

	void PerformPendingAction();

	std::unique_ptr<sf::RenderTexture> frozenFrame;
	UI::ConfirmDialog confirmDialog;
	PendingAction pendingAction = PendingAction::None;

	// 0 = the frame is crisp, 1 = fully solidified.
	float reveal = 0.f;
	bool resuming = false;
	bool introRaised = false;   // has "PAUSE" + the column been brought in yet
};
