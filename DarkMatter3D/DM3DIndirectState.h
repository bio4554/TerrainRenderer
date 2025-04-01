#pragma once
#include <memory>

#include "DM3DShader.h"

namespace dm3d
{
	enum IndirectStateType : uint32_t
	{
		IDNone = 0,
		IDVertexShader = 1 << 0,
		IDPixelShader = 1 << 1,
		IDAmpShader = 1 << 2,
		IDMeshShader = 1 << 3,
		IDGeoShader = 1 << 4,
		IDDepthClip = 1 << 5,
		IDDepthTestEnable = 1 << 6,
		IDCullMode = 1 << 7,
		IDDrawExtent = 1 << 8,
		IDFillMode = 1 << 9,
		IDInputPrimitive = 1 << 10,
		IDNumRenderTargets = 1 << 11,
		IDPrimitive = 1 << 12,
		IDStencilTestEnable = 1 << 13,
		IDDepthBind = 1 << 14,
		IDIndexBind = 1 << 15,
		IDRenderTargetBind = 1 << 16,
		IDResourceTable = 1 << 17
	};

	namespace indirect
	{
		struct VertexShader
		{
		private:
			IndirectStateType _type = IDVertexShader;
		public:
			explicit VertexShader(const std::shared_ptr<Shader>& shader)
			{
				pShader = shader.get();
			}

			Shader* pShader;
		};

		struct PixelShader
		{
		private:
			IndirectStateType _type = IDPixelShader;
		public:
			explicit PixelShader(const std::shared_ptr<Shader>& shader)
			{
				pShader = shader.get();
			}

			Shader* pShader;
		};

		struct AmpShader
		{
		private:
			IndirectStateType _type = IDAmpShader;
		public:
			explicit AmpShader(const std::shared_ptr<Shader>& shader)
			{
				pShader = shader.get();
			}

			Shader* pShader;
		};

		struct MeshShader
		{
		private:
			IndirectStateType _type = IDMeshShader;
		public:
			explicit MeshShader(const std::shared_ptr<Shader>& shader)
			{
				pShader = shader.get();
			}

			Shader* pShader;
		};

		struct GeoShader
		{
		private:
			IndirectStateType _type = IDGeoShader;
		public:
			explicit GeoShader(const std::shared_ptr<Shader>& shader)
			{
				pShader = shader.get();
			}

			Shader* pShader;
		};

		struct DepthClip
		{
		private:
			IndirectStateType _type = IDDepthClip;
		public:
			explicit DepthClip(const bool enabled)
			{
				this->enabled = enabled;
			}

			bool enabled;
		};

		struct DepthTest
		{
		private:
			IndirectStateType _type = IDDepthTestEnable;
		public:
			explicit DepthTest(const bool enabled)
			{
				this->enabled = enabled;
			}

			bool enabled;
		};
	}
}
