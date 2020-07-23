#pragma once
#include <Windows.h>
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

struct ThreadPoolJob;

#define LOCK(x)   {EnterCriticalSection(&x);}
#define UNLOCK(x) {LeaveCriticalSection(&x);}

struct ThreadPoolData
{
	bool* Run;

	std::queue <ThreadPoolJob>* WorkStack;

	HANDLE* WorkAvailable;
	HANDLE* WorkComplete;
	
	CRITICAL_SECTION* QueueLock;
};

struct ThreadPoolJob
{
	std::function<void(void*)> WorkFunction;
	void* WorkParameter;

	ThreadPoolJob() {}

	ThreadPoolJob(std::function<void(void*)> WorkCallback, void* WorkParam)
	{
		WorkFunction = WorkCallback;
		WorkParameter = WorkParam;
	}

	~ThreadPoolJob() {}
};

void ThreadPoolProc(ThreadPoolData* Data);

class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();

	void Initialize(int NumThreads);

	void Submit(ThreadPoolJob Job, uint32_t DestinationStack);
	void Submit(std::function<void(void*)> Work,void* WorkParam, uint32_t DestinationQueue);

	HANDLE* SubmitAndExecute(std::vector<ThreadPoolJob> Work, uint32_t DestinationQueue);

	HANDLE* ExecuteWorkQueue(uint32_t WorkQueue);

	void SubmitUnpooled(std::function<void(void*)> Work, void* pWorkArg);
private:
	std::vector<std::thread*> Workers;

	std::vector<std::queue<ThreadPoolJob>*> WorkQueues;

	ThreadPoolData* Data;
	HANDLE* WorkAvailableHandles;
	HANDLE* WorkCompleteHandles;
	
	CRITICAL_SECTION* QueueLocks;

	bool Run;
};