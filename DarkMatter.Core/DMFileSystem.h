#pragma once

#include <string>

namespace dm::core
{
	namespace filesystem::exceptions
	{
		class AlreadyExists : public std::exception
		{
		public:
			AlreadyExists(std::string n)
			{
				name = n;
			}

			std::string name;
		};
	}

	class FileSystem
	{
	public:
		virtual ~FileSystem() = default;
		virtual bool FileExists(std::string path) = 0;
		virtual size_t FileSize(std::string path) = 0;
		virtual void ReadFile(std::string path, char* pData) = 0;
		virtual void WriteFile(std::string path, char* pData, size_t size, bool createNew = true) = 0;
		virtual std::string ReadFileText(std::string path) = 0;
	};
}