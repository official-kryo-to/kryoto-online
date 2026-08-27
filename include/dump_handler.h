// Minidump writer, Debug builds only.
//
// It catches crashes inside this DLL, which is the case worth having a
// dump for - a crash in the game's own code is the game's business.
// MiniDumpWriteDump is resolved from DbgHelp.dll at runtime rather than
// linked, so the release build carries no trace of it.
//
// This never compiled before 1.8.1: the Debug configuration defined
// NDEBUG, so the guard below was false in every build that has ever
// shipped.

#pragma once

#ifdef _DEBUG

#include <Windows.h>
#include <string>
#include <DbgHelp.h>

typedef BOOL(WINAPI* Fn_MiniDumpWriteDump)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile,
	MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

class CDumpHandler
{
private:
	HMODULE m_hDbgHelp;
	Fn_MiniDumpWriteDump m_pfnWriteDump;
	std::wstring m_Comment;
	SRWLOCK m_Lock;
	bool m_bReady;

public:
	CDumpHandler();
	~CDumpHandler();

	bool IsReady();
	void SetComment(const wchar_t* comment);
	size_t GetCommentByteSize();
	const wchar_t* GetComment();
	void ClearComment();
	void WriteDump(DWORD exceptionCode, _EXCEPTION_POINTERS* pExceptionInfo);
};

#endif
