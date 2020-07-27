#include "EngineThreadPool.h"

ThreadPool::ThreadPool()
{
	Run = true;
}

ThreadPool::~ThreadPool()
{
}

void ThreadPool::Initialize(int NumThreads)
{
	WorkAvailableHandles = new HANDLE[NumThreads];
	WorkCompleteHandles = new HANDLE[NumThreads];
	QueueLocks = new CRITICAL_SECTION[NumThreads];
	Data = new ThreadPoolData[NumThreads];

	for (int i = 0; i < NumThreads; ++i)
	{
		std::queue<ThreadPoolJob>* NewQueue = new std::queue<ThreadPoolJob>();

		WorkAvailableHandles[i] = CreateEvent(nullptr, true, false, L"WorkAvailableEvent");
		WorkCompleteHandles[i] = CreateEvent(nullptr, true, false, L"WorkCompleteEvent");

		InitializeCriticalSection(&QueueLocks[i]);

		Data[i].Run = &this->Run;
		Data[i].WorkAvailable = &WorkAvailableHandles[i];
		Data[i].WorkComplete = &WorkCompleteHandles[i];
		Data[i].WorkStack = NewQueue;
		Data[i].QueueLock = &QueueLocks[i];

		std::thread* WorkerThread = new std::thread(ThreadPoolProc, &Data[i]);

		Workers.push_back(WorkerThread);
		WorkQueues.push_back(NewQueue);
	}
}

void ThreadPool::Submit(ThreadPoolJob Job, uint32_t DestinationStack)
{
	LOCK(QueueLocks[DestinationStack]);

	WorkQueues[DestinationStack]->push(Job);

	UNLOCK(QueueLocks[DestinationStack]);
}

void ThreadPool::Submit(std::function<void(void*)> Work,void* WorkParam, uint32_t DestinationQueue)
{
	LOCK(QueueLocks[DestinationQueue]);

	WorkQueues[DestinationQueue]->push(ThreadPoolJob(Work, WorkParam));

	UNLOCK(QueueLocks[DestinationQueue]);
}

HANDLE* ThreadPool::SubmitAndExecute(std::vector<ThreadPoolJob> Work, uint32_t DestinationQueue)
{
	LOCK(QueueLocks[DestinationQueue]);
	for (int i = 0; i < Work.size(); ++i)
	{
		WorkQueues[DestinationQueue]->push(Work[i]);

		return ExecuteWorkQueue(DestinationQueue);
	}
	UNLOCK(QueueLocks[DestinationQueue]);
}


HANDLE* ThreadPool::ExecuteWorkQueue(uint32_t WorkQueue)
{
	SetEvent(WorkAvailableHandles[WorkQueue]);

	return &WorkCompleteHandles[WorkQueue];
}

void ThreadPool::SubmitUnpooled(std::function<void(void*)> Work, void * pWorkArg)
{
	std::thread thread(Work, pWorkArg);
}


void ThreadPoolProc(ThreadPoolData* Data)
{
	if (Data)
	{
		while (*Data->Run)
		{
			WaitForSingleObject(*Data->WorkAvailable, INFINITE); //wait for signal that work is ready
			LOCK(*Data->QueueLock);

			while (Data->WorkStack->size() > 0)
			{
				ThreadPoolJob Job = Data->WorkStack->front();
				Data->WorkStack->pop();

				Job.WorkFunction(Job.WorkParameter);
			}

			//all work complete

			UNLOCK(*Data->QueueLock);
			SetEvent(*Data->WorkComplete);
			ResetEvent(*Data->WorkAvailable);
		}
	}
}