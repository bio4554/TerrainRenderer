#include "pch.h"
#include "DMUtilities.h"

namespace dm::core::utility
{
	std::wstring ToWideString(const std::string& input)
	{
		if (input.empty())
			return {};

		// Determine the required buffer size in wide characters.
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), NULL, 0);

		// Create a buffer for the wide string.
		std::wstring wstr(size_needed, 0);

		// Do the actual conversion.
		MultiByteToWideChar(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), &wstr[0], size_needed);

		return wstr;
	}

}