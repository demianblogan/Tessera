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

TEST_CASE("a new piece spawns inside the hidden buffer, above the visible field")
{
	const GameplaySession session;

	for (const sf::Vector2i& block : session.GetCurrentTetromino().GetBlockPositions())
	{
		CHECK(block.y >= 0);
		CHECK(block.y < Board::BufferHeight);
	}
}

TEST_CASE("the next queue always holds five pieces")
{
	GameplaySession session;
	CHECK(session.GetNextCount() == 5);

	// Locking pieces refills the queue from the bag each time.
	for (int i = 0; i < 30 && session.GetPhase() != GameplaySession::Phase::GameOver; i++)
	{
		session.HardDrop();
		session.Update(1.0f);
		(void)session.ConsumeEvents();

		CHECK(session.GetNextCount() == 5);
	}
}

TEST_CASE("the piece that spawns next matches the front of the queue")
{
	GameplaySession session;
	const Tetromino::Type expected = session.GetNextPiece(0).GetType();

	session.HardDrop();
	session.Update(1.0f);
	(void)session.ConsumeEvents();

	CHECK(session.GetCurrentTetromino().GetType() == expected);
}

TEST_CASE("the queue advances by exactly one slot per spawn")
{
	GameplaySession session;

	std::array<Tetromino::Type, 5> before{};
	for (int i = 0; i < 5; i++)
	{
		before[static_cast<std::size_t>(i)] = session.GetNextPiece(i).GetType();
	}

	session.HardDrop();
	session.Update(1.0f);
	(void)session.ConsumeEvents();

	// The old slots 1..4 are now the new slots 0..3.
	for (int i = 0; i < 4; i++)
	{
		CHECK(session.GetNextPiece(i).GetType() == before[static_cast<std::size_t>(i) + 1]);
	}
}

TEST_CASE("the spawn count is a pure change signal for the renderer")
{
	GameplaySession session;
	CHECK(session.GetSpawnCount() == 0);

	session.HardDrop();
	session.Update(1.0f);
	(void)session.ConsumeEvents();

	CHECK(session.GetSpawnCount() == 1);
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

namespace
{
	// Soft-drop the active piece until it rests on the floor (soft drop no longer
	// locks, so this just parks it).
	void DropToFloor(GameplaySession& session)
	{
		for (int i = 0; i < Board::HEIGHT + 4; i++)
		{
			session.SoftDropStep();
		}
	}
}

TEST_CASE("a piece resting on the floor waits out the lock delay before locking")
{
	GameplaySession session;
	DropToFloor(session);

	REQUIRE(session.GetPhase() == GameplaySession::Phase::Falling);
	CHECK_FALSE(session.ConsumeEvents().landed);

	// Just short of the delay: still in play.
	session.Update(0.4f);
	CHECK(session.GetPhase() == GameplaySession::Phase::Falling);

	// Past it: locked, and a fresh piece has spawned back up in the buffer.
	session.Update(0.2f);
	CHECK(session.ConsumeEvents().landed);
	CHECK(session.GetCurrentTetromino().GetPosition().y < Board::BufferHeight);
}

TEST_CASE("moving a resting piece resets the lock delay")
{
	GameplaySession session;
	DropToFloor(session);
	(void)session.ConsumeEvents();

	for (int i = 0; i < 10; i++)
	{
		session.Update(0.4f);
		session.MoveHorizontal(i % 2 == 0 ? 1 : -1);
		CHECK(session.GetPhase() == GameplaySession::Phase::Falling);
	}
}

TEST_CASE("the lock-delay reset is capped so a piece cannot be stalled forever")
{
	GameplaySession session;
	DropToFloor(session);
	(void)session.ConsumeEvents();

	bool locked = false;
	for (int i = 0; i < 200 && !locked; i++)
	{
		session.Update(0.45f);
		session.MoveHorizontal(i % 2 == 0 ? 1 : -1);
		locked = session.ConsumeEvents().landed;
	}

	CHECK(locked);
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
	GameplaySession::GameOverReason reason = GameplaySession::GameOverReason::None;

	for (int piece = 0; piece < 400 && session.GetPhase() != GameplaySession::Phase::GameOver; piece++)
	{
		session.HardDrop();
		session.Update(1.0f); // flushes any row-clear delay and one gravity tick
		const GameplaySession::Events events = session.ConsumeEvents();

		if (events.gameOver)
		{
			sawGameOverEvent = true;
			reason = events.gameOverReason;
		}

		// Scoring invariants hold at every step.
		CHECK(session.GetScore() % 10 == 0);
		CHECK(session.GetLevel() == session.GetScore() / 50 + 1);
	}

	CHECK(session.GetPhase() == GameplaySession::Phase::GameOver);
	CHECK(sawGameOverEvent);

	// The end is classified, and the event agrees with the query.
	CHECK(reason != GameplaySession::GameOverReason::None);
	CHECK(reason == session.GetGameOverReason());
}

TEST_CASE("hard-dropping into one narrow column ends the game without clearing a line")
{
	GameplaySession session;

	// Never move the piece: every shape spawns around the middle columns, so the
	// stack only ever fills columns 3-6 and no row can complete.
	for (int piece = 0; piece < 400 && session.GetPhase() != GameplaySession::Phase::GameOver; piece++)
	{
		session.HardDrop();
		session.Update(1.0f);
		(void)session.ConsumeEvents();
	}

	REQUIRE(session.GetPhase() == GameplaySession::Phase::GameOver);
	CHECK(session.GetLinesCleared() == 0);

	// The stack overflowed the top: either a piece locked entirely in the buffer
	// (lock-out) or the next one had no room to spawn (block-out).
	const GameplaySession::GameOverReason reason = session.GetGameOverReason();
	CHECK((reason == GameplaySession::GameOverReason::LockOut
		|| reason == GameplaySession::GameOverReason::BlockOut));
}

TEST_CASE("four rotations return the active piece to its spawn orientation")
{
	GameplaySession session;

	for (int i = 0; i < 4; i++)
	{
		session.Rotate(true);
	}

	CHECK(session.GetCurrentTetromino().GetRotationIndex() == 0);
}

TEST_CASE("a rotation blocked in place kicks the piece off the wall")
{
	GameplaySession session;

	// The I piece has the most distinctive kicks; it arrives within one 7-bag.
	int guard = 0;
	while (session.GetCurrentTetromino().GetType() != Tetromino::Type::I && guard++ < 10)
	{
		session.HardDrop();
		session.Update(1.0f);
		(void)session.ConsumeEvents();
	}
	REQUIRE(session.GetCurrentTetromino().GetType() == Tetromino::Type::I);

	// Stand it up and push it against the right wall.
	session.Rotate(true);
	while (session.MoveHorizontal(1)) {}

	const int xBefore = session.GetCurrentTetromino().GetPosition().x;

	// Rotating back to flat would poke through the wall in place, so it must kick.
	const bool rotated = session.Rotate(true);

	CHECK(rotated);
	CHECK(session.GetCurrentTetromino().GetPosition().x < xBefore);
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
