#include "NineSliceFrame.h"

#include <algorithm>
#include <array>
#include <cstddef>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace
{
	constexpr std::size_t GridSize = 3;
	constexpr std::size_t SliceCount = GridSize * GridSize;

	// GridSize cells need GridSize + 1 boundary coordinates (3 gaps, 4 posts).
	constexpr std::size_t GridLineCount = GridSize + 1;
}

namespace UI
{
	NineSliceFrame::NineSliceFrame(const sf::Texture& texture, sf::FloatRect destinationBounds,
		unsigned int sourceBorderPixels, sf::Vector2f targetBorderSize)
	{
		const sf::Vector2u textureSize = texture.getSize();
		const int textureWidth = static_cast<int>(textureSize.x);
		const int textureHeight = static_cast<int>(textureSize.y);

		// Two opposing borders must not overlap on a small texture.
		const int sourceBorder = static_cast<int>(
			std::min({ sourceBorderPixels, textureSize.x / 2u, textureSize.y / 2u }));

		// Likewise the target borders must fit inside the destination rectangle.
		const sf::Vector2f targetBorder
		{
			std::min(targetBorderSize.x, destinationBounds.size.x / 2.f),
			std::min(targetBorderSize.y, destinationBounds.size.y / 2.f)
		};

		const std::array<int, GridLineCount> sourceColumns =
		{
			0, sourceBorder, textureWidth - sourceBorder, textureWidth
		};
		const std::array<int, GridLineCount> sourceRows =
		{
			0, sourceBorder, textureHeight - sourceBorder, textureHeight
		};

		const std::array<float, GridLineCount> targetColumns =
		{
			destinationBounds.position.x,
			destinationBounds.position.x + targetBorder.x,
			destinationBounds.position.x + destinationBounds.size.x - targetBorder.x,
			destinationBounds.position.x + destinationBounds.size.x
		};
		const std::array<float, GridLineCount> targetRows =
		{
			destinationBounds.position.y,
			destinationBounds.position.y + targetBorder.y,
			destinationBounds.position.y + destinationBounds.size.y - targetBorder.y,
			destinationBounds.position.y + destinationBounds.size.y
		};

		slices.reserve(SliceCount);

		for (std::size_t row = 0; row < GridSize; row++)
		{
			for (std::size_t column = 0; column < GridSize; column++)
			{
				const int sliceSourceWidth = sourceColumns[column + 1] - sourceColumns[column];
				const int sliceSourceHeight = sourceRows[row + 1] - sourceRows[row];

				if (sliceSourceWidth <= 0 || sliceSourceHeight <= 0)
				{
					continue;
				}

				slices.emplace_back(texture, sf::IntRect(
					{ sourceColumns[column], sourceRows[row] },
					{ sliceSourceWidth, sliceSourceHeight }));

				sf::Sprite& slice = slices.back();
				slice.setPosition({ targetColumns[column], targetRows[row] });
				slice.setScale(
					{
						(targetColumns[column + 1] - targetColumns[column]) / static_cast<float>(sliceSourceWidth),
						(targetRows[row + 1] - targetRows[row]) / static_cast<float>(sliceSourceHeight)
					});
			}
		}
	}

	NineSliceFrame NineSliceFrame::ForWidget(const sf::Texture& texture, sf::FloatRect destinationBounds)
	{
		const sf::Vector2u textureSize = texture.getSize();

		const unsigned int sourceBorder =
			static_cast<unsigned int>(std::min(textureSize.x, textureSize.y) * 0.20f);

		const float shorterSide = std::min(destinationBounds.size.x, destinationBounds.size.y);
		const float targetBorder = std::clamp(shorterSide * 0.30f, 12.f, 46.f);

		return NineSliceFrame(texture, destinationBounds, sourceBorder, { targetBorder, targetBorder });
	}

	void NineSliceFrame::SetColor(sf::Color color)
	{
		for (sf::Sprite& slice : slices)
		{
			slice.setColor(color);
		}
	}

	void NineSliceFrame::Draw(sf::RenderTarget& target) const
	{
		for (const sf::Sprite& slice : slices)
		{
			target.draw(slice);
		}
	}
}
