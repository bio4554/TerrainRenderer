#include "pch.h"
#include "DMEngine.h"

namespace dm
{
	void Engine::LoadFromFolder(const std::string& path)
	{
		 if (_fileSystem->FileExists(path + "/assets.json"))
		 {
			 LoadAssetsRegistryFile(path + "/assets.json");
		 }
	}

	void Engine::LoadAssetsRegistryFile(const std::string& path) const
	{
		_world->assetRegistry.DeserializeRegistry(_fileSystem->ReadFileText(path));
	}


	void Engine::SaveToFolder(const std::string& path) const
	{
		{
			auto assetJson = _world->assetRegistry.SerializeRegistry();
			_fileSystem->WriteFile(path + "/assets.json", assetJson.data(), assetJson.size(), false);
		}
	}

}