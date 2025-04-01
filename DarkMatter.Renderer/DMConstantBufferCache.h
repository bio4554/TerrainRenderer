#pragma once

#include "DM3DContext.h"
#include "DMLogger.h"
#include "DMTaskSystem.h"

namespace dm::renderer
{
	template <typename T>
	class ConstantBufferCache
	{
	public:
		ConstantBufferCache(dm3d::Context* context)
		{
			_context = context;
		}

		std::shared_ptr<dm3d::Buffer> Allocate()
		{
			++_usedThisFrame;
			std::shared_ptr<dm3d::Buffer> popped;
			if (_freeList.try_dequeue(popped))
			{
				_usedList.enqueue(popped);
				return popped;
			}

			auto buffer = _context->create_buffer(sizeof(T), true);
			_context->register_constant_view(buffer);

			_usedList.enqueue(buffer);

			return buffer;
		}

		void Reset()
		{
			while (_usedList.size_approx() != 0)
			{
				std::shared_ptr<dm3d::Buffer> popped;
				if (_usedList.try_dequeue(popped))
				{
					_freeList.enqueue(popped);
				}
			}

			_usedLastFrame = _usedThisFrame.load();
			_usedThisFrame = 0;
			if (_freeList.size_approx() - _usedLastFrame > 50)
			{
				// don't block the main thread to clean up the queue
				core::task::GTaskSystem->async_([this]()
				{
					_log.information("Cleaning up cache...");
					// dump 50 out of this queue if we aren't using them.
					for (int i = 0; i < 50; i++)
					{
						std::shared_ptr<dm3d::Buffer> popped;
						_freeList.try_dequeue(popped);
					}
				}, std::make_shared<core::task::SyncCounter>());
			}
		}

		uint32_t GetCurrentFrameUsage() const { return _usedThisFrame.load(); }
		uint32_t GetLastFrameUsage() const { return _usedLastFrame; }

	private:
		LoggerContext _log = LoggerContext("ConstantBufferCache");
		dm3d::Context* _context;

		moodycamel::ConcurrentQueue<std::shared_ptr<dm3d::Buffer>> _freeList;
		moodycamel::ConcurrentQueue<std::shared_ptr<dm3d::Buffer>> _usedList;
		std::atomic<uint32_t> _usedThisFrame = 0;
		uint32_t _usedLastFrame = 0;
	};
}
