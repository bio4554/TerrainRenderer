#include "pch.h"
#include "DMAssetManager.h"

#include "DM3DContext.h"
#include "DMTaskSystem.h"

namespace dm::core
{
	void AssetManager::SetRegistry(AssetRegistry* pRegistry)
	{
		_log.information("Setting asset registry");
		_currentRegistry = pRegistry;
	}

	void AssetManager::SetFileSystem(FileSystem* pFileSystem)
	{
		_log.information("Setting file system");
		_fileSystem = pFileSystem;
	}

	void AssetManager::SetGpuContext(dm3d::Context* pContext)
	{
		_log.information("Setting GPU context");
		_gpuContext = pContext;
	}

	std::shared_ptr<dm3d::Image> AssetManager::TryGetImage(uint32_t imageId, bool queueLoad)
	{
		{
			std::shared_lock l(_rwLockTexture);
			if (_textureStore.contains(imageId) && _textureStore[imageId] != nullptr)
			{
				return _textureStore[imageId];
			}
		}

		if (!queueLoad || IsTextureLoading(imageId))
			return nullptr;

		task::GTaskSystem->async_([this, imageId]()
			{
				// final sanity check before doing the upload. if a bunch of jobs were queued for the same texture upload, this will catch almost all of the duplicates.
				if ((_textureStore.contains(imageId) && _textureStore[imageId] != nullptr) || !ExchangeTextureLoading(imageId, true))
					return;

				const auto asset = _currentRegistry->GetTexture(imageId);
				const auto assetPath = asset.GetPath();

				if (!_fileSystem->FileExists(assetPath))
				{
					_log.error("File doesn't exist! " + assetPath);
					return;
				}

				const auto fileSize = _fileSystem->FileSize(assetPath);
				std::vector<char> buffer(fileSize);
				_fileSystem->ReadFile(assetPath, buffer.data());
				// ReSharper disable once CppTooWideScope // loading the image is thread safe, don't force a lock if we don't need to
				const auto image = LoadImageToGPU(buffer.data(), buffer.size(), asset.GetFormat(), asset.GetExtent());
				{
					std::unique_lock lock(_rwLockTexture);
					_textureStore[imageId] = image;
				}

				ExchangeTextureLoading(imageId, false);
			}, std::make_shared<task::SyncCounter>());

		return nullptr;
	}

	void AssetManager::TryUnloadMesh(uint32_t assetId)
	{
		task::GTaskSystem->async_([this, assetId]()
			{
				std::unique_lock l(_rwLockMesh);
				if (_meshStore.contains(assetId))
				{
					_meshStore[assetId] = nullptr;
				}
			}, std::make_shared<task::SyncCounter>());
	}

	void AssetManager::TryUnloadImage(uint32_t imageId)
	{
		task::GTaskSystem->async_([this, imageId]()
			{
				std::unique_lock lock(_rwLockTexture);
				if (_textureStore.contains(imageId))
				{
					_textureStore[imageId] = nullptr;
				}
			}, std::make_shared<task::SyncCounter>());
	}

	std::shared_ptr<dm3d::Image> AssetManager::LoadImageToGPU(char* pData, size_t size, dm3d::ImageFormat format, dm3d::Extent3D extent) const
	{
		auto image = _gpuContext->load_dds(pData, size);
		return image;
	}

	MeshRenderable* AssetManager::TryGetMesh(uint32_t assetId, bool queueLoad)
	{
		{
			std::shared_lock l(_rwLockMesh);
			if (_meshStore.contains(assetId) && _meshStore[assetId] != nullptr)
			{
				return _meshStore[assetId].get();
			}
		}

		if (!queueLoad || IsMeshLoading(assetId))
			return nullptr;

		task::GTaskSystem->async_([this, assetId]()
			{
				if (!ExchangeMeshLoading(assetId, true))
					return;

				auto asset = _currentRegistry->GetMesh(assetId);
				auto assetPath = asset.GetPath();

				if (!_fileSystem->FileExists(assetPath))
				{
					_log.error("File doesn't exist! " + assetPath);
					return;
				}

				auto fileSize = _fileSystem->FileSize(assetPath);
				std::vector<char> buffer(fileSize);
				// ReSharper disable once CppTooWideScope // loading the mesh is thread safe, don't force a lock if we don't need to
				auto mesh = ParseAndLoadMeshToGPU(buffer.data(), buffer.size());
				{
					std::unique_lock lock(_rwLockMesh);
					_meshStore[assetId] = mesh;
				}

				ExchangeMeshLoading(assetId, false);
			}, std::make_shared<task::SyncCounter>());

		return nullptr;
	}

	bool AssetManager::IsTextureLoading(uint32_t id)
	{
		std::unique_lock l(_loadLockTexture);
		return _textureLoads.contains(id) && _textureLoads[id];
	}

	bool AssetManager::IsMeshLoading(uint32_t id)
	{
		std::unique_lock l(_loadLockMesh);
		return _meshLoads.contains(id) && _meshLoads[id];
	}

	bool AssetManager::ExchangeMeshLoading(uint32_t id, bool loading)
	{
		std::unique_lock l(_loadLockMesh);

		if (_meshLoads.contains(id) && _meshLoads[id] == loading)
		{
			return false;
		}

		_meshLoads[id] = loading;
		return true;
	}

	std::shared_ptr<MeshRenderable> AssetManager::ParseAndLoadMeshToGPU(char* pData, size_t size)
	{
		_log.warning("CANT LOAD MESH");
		return nullptr;
	}

	bool AssetManager::ExchangeTextureLoading(uint32_t id, bool loading)
	{
		std::unique_lock l(_loadLockTexture);

		if (_textureLoads.contains(id) && _textureLoads[id] == loading)
		{
			return false;
		}

		_textureLoads[id] = loading;
		return true;
	}

}
