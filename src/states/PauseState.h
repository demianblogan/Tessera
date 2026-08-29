#pragma once

#include "MenuScreenState.h"

class PauseState final : public MenuScreenState
{
public:
	explicit PauseState(Context& context);

	void Render(sf::RenderTarget& target) override;

	[[nodiscard]] Backdrop GetBackdrop() const override;

private:
	void OnBack() override;
};
