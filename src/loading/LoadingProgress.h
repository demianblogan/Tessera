#pragma once

#include <atomic>

namespace Loading
{
	// The asset groups loaded on the background thread, in the order they run.
	// Count is a sentinel: it is also the value GetStage() reports once every
	// real stage has finished.
	// Textures and shaders are loaded synchronously before this screen (GPU
	// uploads on a background thread deadlock some drivers), so the background
	// job only handles these.
	enum class Stage
	{
		Audio,
		Music,
		Interface,

		Count
	};

	inline constexpr int StageCount = static_cast<int>(Stage::Count);

	// The one channel the background loader uses to talk to the loading screen:
	// which stage it is on, and whether it has finished. Both sides touch only
	// the atomic, so no locking is needed.
	class Progress
	{
	public:
		void SetStage(Stage stage) noexcept
		{
			currentStage.store(stage, std::memory_order_release);
		}

		void MarkDone() noexcept
		{
			finished.store(true, std::memory_order_release);
		}

		[[nodiscard]] Stage GetStage() const noexcept
		{
			return currentStage.load(std::memory_order_acquire);
		}

		[[nodiscard]] bool IsDone() const noexcept
		{
			return finished.load(std::memory_order_acquire);
		}

		// 0..1 -- how many stages are complete. Coarse (it steps once per
		// stage); the loading screen smooths it out for the animation.
		[[nodiscard]] float Fraction() const noexcept
		{
			if (IsDone())
			{
				return 1.f;
			}

			return static_cast<float>(static_cast<int>(GetStage())) / static_cast<float>(StageCount);
		}

	private:
		std::atomic<Stage> currentStage{ Stage::Audio };
		std::atomic<bool> finished{ false };
	};
}
