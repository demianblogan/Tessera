#include "Tetromino.h"

#include "PieceData.h"

Tetromino::Tetromino(Type type, const sf::Vector2i& startPosition)
	: type(type), position(startPosition)
{
	// No code
}

void Tetromino::Move(int offsetX, int offsetY)
{
	position.x += offsetX;
	position.y += offsetY;
}

void Tetromino::RotateClockwise()
{
	rotationStateIndex = (rotationStateIndex + 1) % TetrominoShapes::ROTATION_COUNT;
}

void Tetromino::RotateCounterClockwise()
{
	rotationStateIndex = (rotationStateIndex - 1 + TetrominoShapes::ROTATION_COUNT) % TetrominoShapes::ROTATION_COUNT;
}

Tetromino::Type Tetromino::GetType() const
{
	return type;
}

int Tetromino::GetRotationIndex() const
{
	return rotationStateIndex;
}

const sf::Vector2i& Tetromino::GetPosition() const
{
	return position;
}

std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT> Tetromino::GetBlockPositions() const
{
	return GetBlockPositions(rotationStateIndex);
}

std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT> Tetromino::GetBlockPositions(
	int rotationIndex, sf::Vector2i extraOffset) const
{
	const PieceData::BlockOffsets& offsets = PieceData::Blocks(type, rotationIndex);

	std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT> blockPositions;

	for (int i = 0; i < TetrominoShapes::BLOCK_COUNT; ++i)
	{
		blockPositions[i] = position + extraOffset + offsets[i];
	}

	return blockPositions;
}
