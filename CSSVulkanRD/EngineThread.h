#pragma once
#include <Windows.h>
#include <functional>

typedef void(*EngineThreadWorkCallback)(void* pArg);

struct EngineThreadData
{
	void* WorkFunctionData;
	EngineThreadWorkCallback WorkCallback;
};

DWORD WINAPI EngineThread_Proc(void* pThreadData);

class EngineThread
{
public:
	EngineThread();
	~EngineThread();

	void Start(EngineThreadWorkCallback WorkFunction, void* pWorkData);

	void WaitForThreadFinish();
private:
	HANDLE hThread;
};