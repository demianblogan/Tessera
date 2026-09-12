#include "doctest/doctest.h"

#include <algorithm>

#include "gameplay/Board.h"
#include "gameplay/EscalationDirector.h"
#include "gameplay/GameplaySession.h"

namespace
{
	// The topmost occupied row in column x, or Board::HEIGHT if the column is
	// empty (so "lower topRow" always means "more full").
	int TopRow(const Board& board, int x)
	{
		for (int y = 0; y < Board::HEIGHT; y++)
		{
			if (board.GetGrid()[y][x].occupied)
			{
				return y;
			}
		}
		return Board::HEIGHT;
	}

	// Moves the active piece to whichever rotation and horizontal position
	// creates the fewest new holes when dropped (ties broken by landing as low
	// as possible), then hard-drops it there. There is no seedable RNG in this
	// project, so a test that needs real line clears drives play through a
	// small hole-avoiding bot rather than hoping convenient shapes turn up --
	// a heuristic that only tracked height (and not per-column overhangs)
	// quickly buries gaps under S/Z/L/J that nothing can ever fill again,
	// which stalls every row well short of complete.
	void DropAvoidingHoles(GameplaySession& session)
	{
		const Tetromino piece = session.GetCurrentTetromino();
		const int startRotation = piece.GetRotationIndex();

		int bestRotation = startRotation;
		int bestX0 = 0;
		int bestHoles = -1;
		int bestLandingOffset = -1;

		for (int rotation = 0; rotation < 4; rotation++)
		{
			const auto blocks = piece.GetBlockPositions(rotation);

			int pieceMinX = Board::WIDTH;
			int pieceMaxX = -1;
			for (const sf::Vector2i& block : blocks)
			{
				pieceMinX = std::min(pieceMinX, block.x);
				pieceMaxX = std::max(pieceMaxX, block.x);
			}
			const int width = pieceMaxX - pieceMinX + 1;

			// The piece's own lowest block in each local column it occupies (in
			// current, unmoved board Y), or -1 if it has no block in that column.
			std::array<int, 4> bottomOfColumn = { -1, -1, -1, -1 };
			for (const sf::Vector2i& block : blocks)
			{
				const int localColumn = block.x - pieceMinX;
				bottomOfColumn[static_cast<std::size_t>(localColumn)] =
					std::max(bottomOfColumn[static_cast<std::size_t>(localColumn)], block.y);
			}

			for (int x0 = 0; x0 <= Board::WIDTH - width; x0++)
			{
				int landingOffset = Board::HEIGHT;
				for (int c = 0; c < width; c++)
				{
					if (bottomOfColumn[static_cast<std::size_t>(c)] < 0)
					{
						continue;
					}
					const int topRow = TopRow(session.GetBoard(), x0 + c);
					landingOffset = std::min(landingOffset, (topRow - 1) - bottomOfColumn[static_cast<std::size_t>(c)]);
				}

				int holes = 0;
				for (int c = 0; c < width; c++)
				{
					if (bottomOfColumn[static_cast<std::size_t>(c)] < 0)
					{
						continue;
					}
					const int topRow = TopRow(session.GetBoard(), x0 + c);
					const int resultingBottom = landingOffset + bottomOfColumn[static_cast<std::size_t>(c)];
					holes += std::max(0, (topRow - 1) - resultingBottom);
				}

				if (bestHoles < 0 || holes < bestHoles ||
					(holes == bestHoles && landingOffset > bestLandingOffset))
				{
					bestHoles = holes;
					bestLandingOffset = landingOffset;
					bestRotation = rotation;
					bestX0 = x0;
				}
			}
		}

		// Rotate clockwise to the chosen orientation. A kick can fail at the
		// board edge; if so, just place whatever orientation was reached.
		for (int steps = ((bestRotation - startRotation) % 4 + 4) % 4; steps > 0; steps--)
		{
			if (!session.Rotate(true))
			{
				break;
			}
		}

		int minX = Board::WIDTH;
		for (const sf::Vector2i& block : session.GetCurrentTetromino().GetBlockPositions())
		{
			minX = std::min(minX, block.x);
		}

		const int delta = bestX0 - minX;
		const int direction = delta > 0 ? 1 : -1;

		for (int i = 0; i < delta * direction; i++)
		{
			session.MoveHorizontal(direction);
		}

		session.HardDrop();
	}

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

TEST_CASE("Config controls the next queue length")
{
	const GameplaySession session({ 3, true });
	CHECK(session.GetNextCount() == 3);
}

TEST_CASE("Config clamps an out-of-range next queue length")
{
	const GameplaySession tooSmall({ 0, true });
	CHECK(tooSmall.GetNextCount() == 1);

	const GameplaySession tooBig({ 99, true });
	CHECK(tooBig.GetNextCount() == 5);
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

TEST_CASE("holding for the first time stashes the piece and draws the next queued one")
{
	GameplaySession session;

	CHECK_FALSE(session.HasHeldPiece());
	CHECK(session.CanHold());

	const Tetromino::Type active = session.GetCurrentTetromino().GetType();
	const Tetromino::Type upcoming = session.GetNextPiece(0).GetType();

	CHECK(session.Hold());

	CHECK(session.HasHeldPiece());
	CHECK(session.GetHeldPiece().GetType() == active);
	CHECK(session.GetCurrentTetromino().GetType() == upcoming);
	CHECK_FALSE(session.CanHold());
}

TEST_CASE("hold cannot be used twice on the same piece")
{
	GameplaySession session;

	CHECK(session.Hold());
	CHECK_FALSE(session.Hold());
}

TEST_CASE("holding again swaps with what is already held, without touching the queue")
{
	GameplaySession session;

	const Tetromino::Type firstHeld = session.GetCurrentTetromino().GetType();
	REQUIRE(session.Hold());

	// Lock the piece hold just gave us, so hold is available again -- this also
	// advances the queue by one (a normal spawn), which the swap below must not
	// repeat.
	session.HardDrop();
	session.Update(1.0f);
	(void)session.ConsumeEvents();
	REQUIRE(session.CanHold());

	const Tetromino::Type thirdPiece = session.GetCurrentTetromino().GetType();

	std::array<Tetromino::Type, 5> queueBefore{};
	for (int i = 0; i < session.GetNextCount(); i++)
	{
		queueBefore[static_cast<std::size_t>(i)] = session.GetNextPiece(i).GetType();
	}

	CHECK(session.Hold());

	CHECK(session.GetCurrentTetromino().GetType() == firstHeld);
	CHECK(session.GetHeldPiece().GetType() == thirdPiece);

	for (int i = 0; i < session.GetNextCount(); i++)
	{
		CHECK(session.GetNextPiece(i).GetType() == queueBefore[static_cast<std::size_t>(i)]);
	}
}

TEST_CASE("hold becomes available again once the piece in play locks")
{
	GameplaySession session;

	REQUIRE(session.Hold());
	CHECK_FALSE(session.CanHold());

	session.HardDrop();
	session.Update(1.0f);
	(void)session.ConsumeEvents();

	CHECK(session.CanHold());
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

	// The guideline gravity curve is 1 second per row at level 1.
	session.Update(1.0f);

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

TEST_CASE("a soft-drop step awards one point")
{
	GameplaySession session;

	CHECK(session.GetScore() == 0);
	session.SoftDropStep();
	CHECK(session.GetScore() == 1);
}

TEST_CASE("a hard drop awards two points per cell dropped")
{
	GameplaySession session;

	int startBottom = 0;
	for (const sf::Vector2i& block : session.GetCurrentTetromino().GetBlockPositions())
	{
		startBottom = std::max(startBottom, block.y);
	}

	session.HardDrop();
	(void)session.ConsumeEvents();

	const int cellsDropped = (Board::HEIGHT - 1) - startBottom;
	CHECK(session.GetScore() == cellsDropped * 2);
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
	int previousScore = 0;

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

		// Scoring invariants hold at every step: it never goes down, and the
		// level always matches the guideline "every 10 lines" rule.
		CHECK(session.GetScore() >= previousScore);
		CHECK(session.GetLevel() == session.GetLinesCleared() / 10 + 1);
		previousScore = session.GetScore();
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

TEST_CASE("combo tracks consecutive clears exactly, and back-to-back / perfect-clear stay honest")
{
	GameplaySession session;

	// Keep the stack flat (rather than piling into one column) so rows actually
	// complete. Piece shapes are still random -- the exact score each clear is
	// worth isn't asserted, but the *relationship* the combo counter must hold
	// to consecutive clears is checked exactly every time.
	int expectedCombo = -1;
	bool sawAnyClear = false;

	for (int piece = 0; piece < 1000 && session.GetPhase() != GameplaySession::Phase::GameOver; piece++)
	{
		DropAvoidingHoles(session);
		session.Update(1.0f);
		const GameplaySession::Events events = session.ConsumeEvents();

		if (events.rowsCleared)
		{
			++expectedCombo;
			sawAnyClear = true;

			CHECK(events.comboCount == expectedCombo);

			// Only a Tetris counts as "difficult" for back-to-back so far.
			if (events.backToBack)
			{
				CHECK(events.clearedRowCount == 4);
			}

			if (events.perfectClear)
			{
				CHECK(session.GetBoard().IsEmpty());
			}
		}
		else
		{
			expectedCombo = -1;
		}
	}

	CHECK(sawAnyClear);
}

TEST_CASE("escalation tiers, a garbage row and a golden piece all arrive on schedule")
{
	GameplaySession session;

	// Jump straight to the Chaos tier in one big Update(): it only ever resolves
	// one lock per call (gravity crashes the active piece down, then the
	// lock-delay check at the end locks it), and an empty board can't turn that
	// single piece into a bad stack -- unlike playing for real up to minute 5,
	// which needs more looking-ahead than this hole-avoiding bot has to reliably
	// survive that long.
	session.Update(EscalationDirector::ChaosTierStart + 1.f);
	(void)session.ConsumeEvents();
	REQUIRE(session.GetEscalationTier() == EscalationDirector::Tier::Chaos);
	REQUIRE(session.GetPhase() != GameplaySession::Phase::GameOver);

	// From here, play for real (hole-avoiding, so rows actually complete) to
	// exercise the Garbage and Chaos mechanics with a fresh, single-piece board.
	bool sawGarbageCell = false;
	bool sawGoldenPiece = false;

	for (int piece = 0; piece < 800 && session.GetPhase() != GameplaySession::Phase::GameOver
		&& !(sawGarbageCell && sawGoldenPiece); piece++)
	{
		DropAvoidingHoles(session);
		session.Update(1.0f);
		(void)session.ConsumeEvents();

		if (session.IsCurrentPieceGolden())
		{
			sawGoldenPiece = true;
		}

		if (!sawGarbageCell)
		{
			for (const Board::GridRow& row : session.GetBoard().GetGrid())
			{
				if (std::any_of(row.begin(), row.end(),
					[](const Cell& cell) { return cell.kind == Cell::Kind::Garbage; }))
				{
					sawGarbageCell = true;
					break;
				}
			}
		}
	}

	CHECK(sawGarbageCell);
	CHECK(sawGoldenPiece);
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
