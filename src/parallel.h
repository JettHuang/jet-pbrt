// \brief
//		parallel
//

#pragma once

#include "pbrt.h"

#include <thread>
#include <mutex>
#include <condition_variable>


namespace pbrt
{

// task
class FTask
{
public:
	virtual void Execute() = 0;
};


// parallel system
class FParallelSystem
{
public:
	FParallelSystem();

	void AddTask(FTask* inTask);
	FTask* WaitForTask();

	void Start(int numthreads);
	void Terminate();
	void WaitForFinish();
	void WaitForEmpty();

protected:
	std::vector<std::thread>  _threads;
	std::mutex	_mutex;
	std::condition_variable _cv;

	volatile bool _bTerminateFlag;
	std::vector<FTask*> _tasks;
};

} // namespace pbrt

