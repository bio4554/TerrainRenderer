#pragma once
#include "concurrentqueue.h"

namespace dm3d
{
	class FencePool
	{
	public:
		FencePool()
		{
			auto num = std::thread::hardware_concurrency();
			for (uint32_t i = 0; i < num; i++)
			{
				auto event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
				if (!_fenceQueueFree.enqueue(event))
				{
					throw std::runtime_error("failed to enqueue fence event in pool");
				}
			}
		}

		HANDLE get_event()
		{
			HANDLE h;
			if (_fenceQueueFree.try_dequeue(h))
			{
				return h;
			}

			h = CreateEventA(nullptr, FALSE, FALSE, nullptr);

			return h;
		}

		void free(HANDLE handle)
		{
			if (!_fenceQueueFree.enqueue(handle))
			{
				throw std::runtime_error("failed to enqueue fence event in pool");
			}
		}
	private:
		moodycamel::ConcurrentQueue<HANDLE> _fenceQueueFree;
	};
}
