#pragma once
#include <optional>
#include <string>

#include "DMAssetRegistry.h"
#include "DMFileSystem.h"
#include "DMLogger.h"

namespace dm::asset
{
	class TextureTools
	{
	public:
		static void Init();
		static std::optional<core::TextureAsset> LoadDDS(std::string path, core::FileSystem* fileSystem);
		static std::optional<core::TextureAsset> ConvertToDDS(uint8_t* pData, size_t size, std::string name, core::FileSystem* fileSystem, bool generateMips = true, size_t mipLevels = 4);
		static void ConvertToDDS(core::FileSystem* fileSystem, std::string path, bool generateMips = true, size_t mipLevels = 4);
	private:
		static dm3d::ImageFormat ConvertToDM3D(DXGI_FORMAT format);
		static bool _init;
		static LoggerContext _log;
	};
}
