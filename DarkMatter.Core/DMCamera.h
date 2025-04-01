#pragma once
#include <memory>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "DMGameObject.h"

#if defined(near)
#undef near
#endif

#if defined(far)
#undef far
#endif

namespace dm::core
{
	class Camera : public GameObject
	{
	public:
		Camera();
		Camera(const Camera& other);

		std::shared_ptr<GameObject> DeepClone() override;
		virtual void Tick() override;

		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetRotationMatrix() const;
		glm::mat4 GetProjectionMatrix() const;

		glm::vec3 position;
		float pitch;
		float yaw;
		float fov;
		float near;
		float far;
		float aspectRatio;
	};
}
