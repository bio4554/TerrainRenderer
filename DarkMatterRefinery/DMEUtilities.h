#pragma once

#include <string>
#include <afxwin.h>

namespace dme
{
	inline std::string ToString(const CString& cString)
	{
		const CT2A asciiString(cString);

		std::string r(asciiString);
		return r;
	}

	inline CString ToCString(const std::string& str)
	{
		return CString(str.c_str());
	}
}