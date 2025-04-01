#include "pch.h"
#include "DMRealFileSystem.h"

#include <filesystem>
#include <fstream>

namespace dm::core
{
	void RealFileSystem::Mount(std::string folderPath)
	{
		std::filesystem::path mountPath(folderPath);
		if (!std::filesystem::exists(mountPath) || !std::filesystem::is_directory(mountPath))
		{
			throw std::runtime_error("Failed to mount directory, mount path is not a valid directory: " + folderPath);
		}

		_mountedFolder = folderPath;
	}

	bool RealFileSystem::FileExists(std::string path)
	{
		std::filesystem::path fullPath = std::filesystem::path(_mountedFolder) / path;
		return std::filesystem::exists(fullPath);
	}

	size_t RealFileSystem::FileSize(std::string path)
	{
		std::filesystem::path fullPath = std::filesystem::path(_mountedFolder) / path;
		if (!std::filesystem::exists(fullPath))
		{
			throw std::runtime_error("File does not exist: " + fullPath.string());
		}

		return std::filesystem::file_size(fullPath);
	}

	void RealFileSystem::ReadFile(std::string path, char* pData)
	{
		std::filesystem::path fullPath = std::filesystem::path(_mountedFolder) / path;
		std::ifstream file(fullPath, std::ios::binary);
		if (!file)
		{
			throw std::runtime_error("Failed to open file for reading: " + fullPath.string());
		}

		file.seekg(0, std::ios::end);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		if (!file.read(pData, size))
		{
			throw std::runtime_error("Failed to read file: " + fullPath.string());
		}
	}

	void RealFileSystem::WriteFile(std::string path, char* pData, size_t size, bool createNew)
	{
		std::filesystem::path fullPath = std::filesystem::path(_mountedFolder) / path;
		if (createNew && std::filesystem::exists(fullPath)) {
			throw filesystem::exceptions::AlreadyExists(fullPath.string());
		}
		
		std::filesystem::path parentPath = fullPath.parent_path();
		if (!std::filesystem::exists(parentPath)) {
			std::filesystem::create_directories(parentPath);
		}
		std::ofstream file(fullPath, std::ios::binary | std::ios::trunc);
		if (!file) {
			throw std::runtime_error("Failed to open file for writing: " + fullPath.string());
		}
		file.write(reinterpret_cast<char*>(pData), size);
		if (!file) {
			throw std::runtime_error("Failed to write file: " + fullPath.string());
		}
	}

	std::string RealFileSystem::ReadFileText(std::string path)
	{
		std::filesystem::path fullPath = std::filesystem::path(_mountedFolder) / path;
		std::ifstream file(fullPath);
		if (!file)
		{
			throw std::runtime_error("Failed to read file: " + fullPath.string());
		}

		std::ostringstream ss;
		ss << file.rdbuf();

		return ss.str();
	}

}
