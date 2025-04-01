#pragma once

namespace dm::core
{
	struct GlobalSettings
	{
		bool IgnoreQuitEvents = false;
		bool DebugDirectX = true;
	};

	extern GlobalSettings GSettings;
}