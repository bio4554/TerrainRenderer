#pragma once

namespace dm::core
{
	class GameObject
	{
	public:
		virtual ~GameObject() = default;
		virtual std::shared_ptr<GameObject> DeepClone() = 0;
		virtual void Tick() = 0;
	};
}