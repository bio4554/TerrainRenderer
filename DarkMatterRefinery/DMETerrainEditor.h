#pragma once

namespace dme
{
	class TerrainEditor
	{
	public:
		enum class EditState
		{
			None,
			Texture
		};

	public:
		void SetState(EditState state);
		void Tick();

	private:
		

		enum class EditMouseState
		{
			Idle
		};

		EditState _editState = EditState::None;
		EditMouseState _editMouseState = EditMouseState::Idle;
	};
}