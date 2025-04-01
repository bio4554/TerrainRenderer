#include "pch.h"
#include <format>
#include "DMTextureTools.h"

#include "DMAssetRegistry.h"

namespace dm::asset
{
	LoggerContext TextureTools::_log = LoggerContext("TextureTools");
	bool TextureTools::_init = false;

	void TextureTools::Init()
	{
		if (_init)
		{
			_log.error("Already initialized!");
			return;
		}

		_init = true;
	}

	std::optional<core::TextureAsset> TextureTools::LoadDDS(std::string path, core::FileSystem* fileSystem)
	{
		auto fileSize = fileSystem->FileSize(path);
		std::vector<uint8_t> buffer(fileSize);
		fileSystem->ReadFile(path, reinterpret_cast<char*>(buffer.data()));

		DirectX::TexMetadata metadata;
		DirectX::ScratchImage image;
		HRESULT result = DirectX::LoadFromDDSMemory(buffer.data(), buffer.size(), DirectX::DDS_FLAGS_NONE, &metadata, image);
		if (FAILED(result))
		{
			_log.error("HRESULT fail");
			return std::optional<core::TextureAsset>();
		}

		core::TextureAsset asset(0, core::AssetType::Texture, path, fileSize, TextureTools::ConvertToDM3D(metadata.format), {.width = static_cast
			                         <uint32_t>(metadata.width), .height = static_cast<uint32_t>(metadata.height), .depth = static_cast<uint32_t>(metadata.depth)}, metadata.mipLevels);

		return asset;
	}

	std::optional<core::TextureAsset> TextureTools::ConvertToDDS(uint8_t* pData, size_t size, std::string name, core::FileSystem* fileSystem, bool generateMips, size_t mipLevels)
	{
		if (fileSystem == nullptr)
		{
			_log.error("fileSystem was nullptr");
			return std::optional<core::TextureAsset>();
		}

		{
			DirectX::TexMetadata texMetadata;
			DirectX::ScratchImage* scratchImage = new DirectX::ScratchImage;
			DirectX::ScratchImage outImage;
			HRESULT result;
			result = DirectX::GetMetadataFromWICMemory(pData, size, DirectX::WIC_FLAGS::WIC_FLAGS_NONE, texMetadata);
			result = DirectX::LoadFromWICMemory(pData, size, DirectX::WIC_FLAGS_NONE, &texMetadata, *scratchImage);

			if (texMetadata.format != DXGI_FORMAT_R8G8B8A8_UNORM)
			{
				DirectX::ScratchImage* newScratch = new DirectX::ScratchImage;
				DirectX::Convert(scratchImage->GetImages(), scratchImage->GetImageCount(), texMetadata, DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, *newScratch);
				texMetadata = newScratch->GetMetadata();
				delete scratchImage;
				scratchImage = newScratch;
			}

			result = DirectX::GenerateMipMaps(scratchImage->GetImages(), scratchImage->GetImageCount(), texMetadata, DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT, mipLevels, outImage);
			DirectX::Blob blob;
			texMetadata = outImage.GetMetadata();
			result = DirectX::SaveToDDSMemory(outImage.GetImages(), outImage.GetImageCount(), texMetadata, DirectX::DDS_FLAGS::DDS_FLAGS_NONE, blob);
			auto assetPath = "textures/" + name + ".dds";
			fileSystem->WriteFile(assetPath, reinterpret_cast<char*>(blob.GetBufferPointer()), blob.GetBufferSize());
			texMetadata = outImage.GetMetadata();
			core::TextureAsset texture(0, core::AssetType::Texture, assetPath, fileSystem->FileSize(assetPath), ConvertToDM3D(texMetadata.format), { .width = static_cast
										   <uint32_t>(texMetadata.width), .height = static_cast<uint32_t>(texMetadata.height), .depth = static_cast<uint32_t>(texMetadata.depth) }, texMetadata.mipLevels);

			delete scratchImage;

			return texture;
		}
	}

	void TextureTools::ConvertToDDS(core::FileSystem* fileSystem, std::string path, bool generateMips, size_t mipLevels)
	{
		if (fileSystem == nullptr)
		{
			_log.error("fileSystem was nullptr");
			return;
		}

		if (!fileSystem->FileExists(path))
		{
			_log.error(std::format("File does not exist: {}", path));
			return;
		}

		{
			auto fileSize = fileSystem->FileSize(path);
			auto buffer = std::vector<char>(fileSize);
			fileSystem->ReadFile(path, buffer.data());
			DirectX::TexMetadata texMetadata;
			DirectX::ScratchImage scratchImage;
			DirectX::ScratchImage outImage;
			HRESULT result;
			result = DirectX::GetMetadataFromWICMemory(reinterpret_cast<uint8_t*>(buffer.data()), fileSize, DirectX::WIC_FLAGS::WIC_FLAGS_NONE, texMetadata);
			result = DirectX::LoadFromWICMemory(reinterpret_cast<uint8_t*>(buffer.data()), fileSize, DirectX::WIC_FLAGS_NONE, &texMetadata, scratchImage);
			result = DirectX::GenerateMipMaps(scratchImage.GetImages(), scratchImage.GetImageCount(), texMetadata, DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT, 4, outImage);
			DirectX::Blob blob;
			result = DirectX::SaveToDDSMemory(outImage.GetImages(), outImage.GetImageCount(), texMetadata, DirectX::DDS_FLAGS::DDS_FLAGS_NONE, blob);
			fileSystem->WriteFile(path + ".dds", reinterpret_cast<char*>(blob.GetBufferPointer()), blob.GetBufferSize());
		}
	}

	dm3d::ImageFormat TextureTools::ConvertToDM3D(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return dm3d::ImageFormat::R8G8B8A8_UNORM;
		default:
			throw std::runtime_error(std::format("Invalid DXGI_FORMAT: {}", static_cast<int>(format)));
		}
	}

}
