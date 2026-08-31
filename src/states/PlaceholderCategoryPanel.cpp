#include "PlaceholderCategoryPanel.h"

#include <algorithm>
#include <cstdint>

#include <SFML/Graphics/RenderTarget.hpp>

#include "../core/Context.h"
#include "../localization/LocalizationManager.h"
#include "../localization/TextKeys.h"
#include "../resources/Assets.h"
#include "../ui/TextLayout.h"

namespace
{
	constexpr sf::FloatRect PanelBounds{ { 660.f, 250.f }, { 1160.f, 620.f } };
	constexpr unsigned int PanelSourceBorder = 28u;
	constexpr sf::Vector2f PanelTargetBorder{ 44.f, 44.f };

	constexpr unsigned int TitleSize = 64;
	constexpr unsigned int BodySize = 34;

	constexpr float CentreX = PanelBounds.position.x + PanelBounds.size.x * 0.5f;
	constexpr float ContentFadeDuration = 0.2f;
}

PlaceholderCategoryPanel::PlaceholderCategoryPanel(Context& context, const sf::String& title, sf::Color accent)
	: accent(accent)
	, frame(context.textures.Get(Assets::TextureID::UiFrame), PanelBounds, PanelSourceBorder, PanelTargetBorder)
	, titleText(context.fonts.Get(Assets::FontID::Menu), title, TitleSize)
	, bodyText(context.fonts.Get(Assets::FontID::Main), context.localization.GetText(TextKey::Options::ComingSoon), BodySize)
{
	UI::TextLayout::CentreOrigin(titleText);
	titleText.setPosition({ CentreX, PanelBounds.position.y + 120.f });

	UI::TextLayout::CentreOrigin(bodyText);
	bodyText.setPosition({ CentreX, PanelBounds.position.y + PanelBounds.size.y * 0.5f });
}

void PlaceholderCategoryPanel::Open()
{
	contentFade = 0.f;
}

void PlaceholderCategoryPanel::Update(float deltaTime)
{
	contentFade = std::min(1.f, contentFade + deltaTime / ContentFadeDuration);
}

void PlaceholderCategoryPanel::Draw(sf::RenderTarget& target, float alphaFraction)
{
	alphaFraction = std::clamp(alphaFraction, 0.f, 1.f);
	const auto alpha = static_cast<std::uint8_t>(alphaFraction * 255.f);

	frame.SetColor(sf::Color(255, 255, 255, alpha));
	frame.Draw(target);

	titleText.setFillColor(sf::Color(accent.r, accent.g, accent.b, alpha));
	target.draw(titleText);

	bodyText.setFillColor(sf::Color(210, 216, 224, alpha));
	target.draw(bodyText);
}

void PlaceholderCategoryPanel::RenderPreview(sf::RenderTarget& target, float alpha)
{
	Draw(target, alpha * 0.55f);
}

void PlaceholderCategoryPanel::RenderContent(sf::RenderTarget& target)
{
	Draw(target, contentFade);
}

bool PlaceholderCategoryPanel::HandleEvent(const sf::Event& /*event*/)
{
	return false;
}
