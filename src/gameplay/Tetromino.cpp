#include "Tetromino.h"

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

std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT>Tetromino::GetBlockPositions() const
{
	std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT> blockPositions;

	const TetrominoShapes::ShapeMatrix& shape = GetCurrentShape();
	int blockIndex = 0;

	for (int y = 0; y < TetrominoShapes::MATRIX_SIZE; y++)
	{
		for (int x = 0; x < TetrominoShapes::MATRIX_SIZE; x++)
		{
			const char cell = shape[y][x];

			if (cell == '.')
			{
				continue;
			}

			blockPositions[blockIndex] = { position.x + x, position.y + y };
			blockIndex++;
		}
	}

	return blockPositions;
}

const TetrominoShapes::RotationSet& Tetromino::GetRotationSet() const
{
	switch (type)
	{
	case Type::I:
		return TetrominoShapes::I;

	case Type::O:
		return TetrominoShapes::O;

	case Type::T:
		return TetrominoShapes::T;

	case Type::S:
		return TetrominoShapes::S;

	case Type::Z:
		return TetrominoShapes::Z;

	case Type::J:
		return TetrominoShapes::J;

	case Type::L:
		return TetrominoShapes::L;
	}

	std::unreachable();
}

const TetrominoShapes::ShapeMatrix& Tetromino::GetCurrentShape() const
{
	return GetRotationSet()[rotationStateIndex];
}