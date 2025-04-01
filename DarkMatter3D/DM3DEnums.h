#pragma once

namespace dm3d
{
	enum ImageFormat
	{
		R8G8B8A8_UNORM,
		R32G32B32A32_FLOAT,
		R16G16B16A16_FLOAT,
		D32_FLOAT,
		R32_TYPELESS,
		R32_UINT,
		R32_FLOAT
	};

	enum ResourceFlags
	{
		None,
		RenderTarget,
		DepthStencil,
		UnorderedAccess
	};

	enum class ResourceState
	{
		Unknown,
		ShaderRead,
		DepthWrite,
		RenderTarget,
		CopySrc,
		CopyDst,
		Present
	};

	enum class ShaderStage
	{
		Vertex,
		Geometry,
		Pixel,
		Mesh,
		Amp
	};

	enum CullMode
	{
		CullBack,
		CullFront,
		CullNone
	};

	enum FillMode
	{
		Solid,
		Wireframe
	};

	enum TopologyType
	{
		Triangle,
		Line,
		Patch
	};

	enum InputTopologyType
	{
		TriangleList,
		LineList,
		PatchList3,
		PatchList4
	};
}