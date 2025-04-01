#pragma once

#include "DMFileSystem.h"

namespace dm::core
{
	class RealFileSystem : public FileSystem
	{
	public:
		void Mount(std::string folderPath);

		bool FileExists(std::string path) override;
		size_t FileSize(std::string path) override;
		void ReadFile(std::string path, char* pData) override;
		void WriteFile(std::string path, char* pData, size_t size, bool createNew = true) override;
		std::string ReadFileText(std::string path) override;
	private:
		std::string _mountedFolder;
	};
}