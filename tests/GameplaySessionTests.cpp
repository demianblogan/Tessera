#include "doctest/doctest.h"

#include "gameplay/Board.h"
#include "gameplay/GameplaySession.h"

namespace
{
	bool BoardHasAnyBlock(const Board& board)
	{
		for (int y = 0; y < Board::HEIGHT; y++)
		{
			for (int x = 0; x < Board::WIDTH; x++)
			{
				if (board.GetGrid()[y][x].occupied)
				{
					return true;
				}
			}
		}

		return false;
	}
}

TEST_CASE("a new session starts falling, at score 0 and level 1")
{
	const GameplaySession session;

	CHECK(session.GetPhase() == GameplaySession::Phase::Falling);
	CHECK(session.IsFalling());
	CHECK(session.GetScore() == 0);
	CHECK(session.GetLevel() == 1);
	CHECK(session.GetClearingRows().empty());
}

TEST_CASE("MoveHorizontal(0) is a no-op that reports no movement")
{
	GameplaySession session;
	CHECK_FALSE(session.MoveHorizontal(0));
}

TEST_CASE("horizontal movement stops at the wall")
{
	GameplaySession session;

	bool everBlocked = false;

	for (int i = 0; i < 15; i++)
	{
		if (!session.MoveHorizontal(-1))
		{
			everBlocked = true;
		}
	}

	CHECK(everBlocked);

	// Once blocked, it stays blocked while pushed the same way.
	CHECK_FALSE(session.MoveHorizontal(-1));
}

TEST_CASE("gravity drops the piece one row once the fall delay elapses")
{
	GameplaySession session;

	const int startY = session.GetCurrentTetromino().GetPosition().y;

	session.Update(0.5f);

	CHECK(session.GetCurrentTetromino().GetPosition().y == startY + 1);
}

TEST_CASE("a soft-drop step lowers the piece by one row")
{
	GameplaySession session;

	const int startY = session.GetCurrentTetromino().GetPosition().y;

	session.SoftDropStep();

	CHECK(session.GetCurrentTetromino().GetPosition().y == startY + 1);
}

TEST_CASE("a hard drop locks a piece and reports the landing")
{
	GameplaySession session;

	session.HardDrop();
	const GameplaySession::Events events = session.ConsumeEvents();

	CHECK(events.landed);
	CHECK(BoardHasAnyBlock(session.GetBoard()));

	// A fresh piece is in play again (unless the very first drop ended the game,
	// which it cannot on an empty board).
	CHECK(session.GetPhase() == GameplaySession::Phase::Falling);
}

TEST_CASE("stacking pieces eventually ends the game, and the score stays consistent")
{
	GameplaySession session;

	bool sawGameOverEvent = false;

	for (int piece = 0; piece < 400 && session.GetPhase() != GameplaySession::Phase::GameOver; piece++)
	{
		session.HardDrop();
		session.Update(1.0f); // flushes any row-clear delay and one gravity tick
		const GameplaySession::Events events = session.ConsumeEvents();

		if (events.gameOver)
		{
			sawGameOverEvent = true;
		}

		// Scoring invariants hold at every step.
		CHECK(session.GetScore() % 10 == 0);
		CHECK(session.GetLevel() == session.GetScore() / 50 + 1);
	}

	CHECK(session.GetPhase() == GameplaySession::Phase::GameOver);
	CHECK(sawGameOverEvent);
}

TEST_CASE("input is ignored once the game is over")
{
	GameplaySession session;

	for (int piece = 0; piece < 400 && session.GetPhase() != GameplaySession::Phase::GameOver; piece++)
	{
		session.HardDrop();
		session.Update(1.0f);
		(void)session.ConsumeEvents();
	}

	REQUIRE(session.GetPhase() == GameplaySession::Phase::GameOver);

	CHECK_FALSE(session.MoveHorizontal(-1));
	CHECK_FALSE(session.Rotate(true));
}
