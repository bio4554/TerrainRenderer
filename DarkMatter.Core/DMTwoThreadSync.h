#pragma once

#include <atomic>

namespace dm::core
{
	class TwoThreadSync
	{
	public:
		void Lock()
		{
			_lockRequested = true;
			while (!_locked){}
		}

		void Unlock()
		{
			_lockRequested = false;
			while (_locked){}
		}

		void AcknowledgeLock()
		{
			if (_lockRequested)
			{
				_locked = true;
				while (_lockRequested){}
				_locked = false;
			}
		}
	private:
		std::atomic<bool> _lockRequested = false;
		std::atomic<bool> _locked = false;
	};
}