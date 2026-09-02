#pragma once

#include <cstddef>
#include <memory>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include "../ui/MenuAurora.h"
#include "../ui/MenuBackdrop.h"
#include "../ui/MenuSparks.h"
#include "ScreenHost.h"

struct Context;
class MenuScreen;

// The ScreenHost for the whole main-menu system. Its background is the shared
// animated ambient -- aurora, drifting tetrominoes, rising sparks -- plus the
// version stamp; its home screen is the MainMenuScreen ring.
class MenuShell final : public ScreenHost
{
public:
	explicit MenuShell(Context& context);

	void OnNavigate(float direction) override;

protected:
	void UpdateBackground(float deltaTime) override;
	void RenderBackground(sf::RenderTarget& target) override;
	void RenderOverlay(sf::RenderTarget& target) override;
	[[nodiscard]] std::unique_ptr<MenuScreen> BuildHomeScreen(std::size_t returnEntryIndex) override;

private:
	sf::Sprite backgroundSprite;
	UI::MenuAurora aurora;
	UI::MenuBackdrop backdrop;
	UI::MenuSparks sparks;
	sf::Text versionText;
};
