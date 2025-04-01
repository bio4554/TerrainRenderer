#pragma once
#include <atomic>
#include <cstdint>
#include <memory>

namespace dm::core::task
{
	class SyncCounter
	{
	public:
		SyncCounter() = default;

		SyncCounter(const SyncCounter& other) = delete;

		void Increment()
		{
			++_internalCounter;
		}

		void Decrement()
		{
			--_internalCounter;
		}

		[[nodiscard]] bool IsZero() const
		{
			return _internalCounter.load() == 0;
		}

	private:
		std::atomic<uint32_t> _internalCounter;
	};
}
