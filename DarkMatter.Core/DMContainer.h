#pragma once
#include <memory>
#include <vector>

#include "DMGameObject.h"

namespace dm::core
{
	class Container
	{
	public:
		std::vector<std::shared_ptr<GameObject>> _objects;
	};
}
