#pragma once

#include <array>

#include <SFML/System/Vector2.hpp>

#include "TetrominoShapes.h"

class Tetromino
{
public:
	enum class Type
	{
		I,
		O,
		T,
		S,
		Z,
		J,
		L
	};

private:
	Type type;
	sf::Vector2i position;

	// Current rotation state index.
	//
	// Possible values:
	// 0 = Spawn
	// 1 = Right
	// 2 = Reverse
	// 3 = Left
	int rotationStateIndex = 0;

	[[nodiscard]] const TetrominoShapes::RotationSet& GetRotationSet() const;
	[[nodiscard]] const TetrominoShapes::ShapeMatrix& GetCurrentShape() const;

public:
	Tetromino(Type type, const sf::Vector2i& startPosition);

	void Move(int offsetX, int offsetY);

	void RotateClockwise();
	void RotateCounterClockwise();

	[[nodiscard]] Type GetType() const;
	[[nodiscard]] int GetRotationIndex() const;
	[[nodiscard]] const sf::Vector2i& GetPosition() const;
	[[nodiscard]] std::array<sf::Vector2i, TetrominoShapes::BLOCK_COUNT> GetBlockPositions() const;
};