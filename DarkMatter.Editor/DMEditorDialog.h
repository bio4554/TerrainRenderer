#pragma once
#include <atomic>

namespace dm::editor
{
	class Editor;

	class Dialog
	{
	public:
		Dialog(Editor* editor)
		{
			_id = _idCounter++;
			_editor = editor;
		}

		virtual ~Dialog() = default;
		bool IsOpen() const { return _currentState == DialogState::Open; }
		virtual void Render() = 0;
		void Close() { _currentState = DialogState::Closed; }

	protected:
		Editor* _editor = nullptr;
		uint64_t GetId() const { return _id; }
	private:
		enum class DialogState
		{
			Open,
			Closed
		};

		DialogState _currentState = DialogState::Open;
		static std::atomic<uint64_t> _idCounter;
		uint64_t _id;
	};
}
