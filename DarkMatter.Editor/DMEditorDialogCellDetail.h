#pragma once
#include "DMCell.h"
#include "DMEditorDialog.h"

namespace dm::editor
{
	class DialogCellDetail : public Dialog
	{
	public:
		DialogCellDetail(Editor* pEditor, model::Cell* pCell) : Dialog(pEditor)
		{
			_cell = pCell;
		}

		void Render() override;

	private:
		model::Cell* _cell;
	};
}
