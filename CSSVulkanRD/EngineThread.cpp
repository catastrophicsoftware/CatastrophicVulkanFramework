#include "EngineThread.h"



EngineThread::EngineThread()
{
}


EngineThread::~EngineThread()
{
}

void EngineThread::Start(EngineThreadWorkCallback WorkFunction, void* pWorkData)
{
	EngineThreadData* ThreadData = new EngineThreadData();

	ThreadData->WorkFunctionData = pWorkData;
	ThreadData->WorkCallback = WorkFunction;

	hThread = CreateThread(nullptr,
		0,
		EngineThread_Proc,
		ThreadData,
		0,
		0);
}

void EngineThread::WaitForThreadFinish()
{
	WaitForSingleObject(hThread, INFINITE);
}

DWORD WINAPI EngineThread_Proc(void * pThreadData)
{
	EngineThreadData* Data = static_cast<EngineThreadData*>(pThreadData);

	if (Data)
	{
		if (Data->WorkCallback != nullptr)
		{
			Data->WorkCallback(Data->WorkFunctionData);
		}
	}

	return 0;
}