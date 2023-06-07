// \brief
//		parallel.cc
//

#include "parallel.h"


namespace pbrt
{
	static void thread_callback(FParallelSystem *inSystem)
	{
		while (1)
		{
			FTask* task = inSystem->WaitForTask();
			if (!task) {
				break;
			}

			task->Execute();
		} // end while
	}


	FParallelSystem::FParallelSystem()
		: _bTerminateFlag(false)
	{

	}

	void FParallelSystem::AddTask(FTask* inTask)
	{
		std::unique_lock<std::mutex> lock(_mutex);

		_tasks.push_back(inTask);
	
		_cv.notify_all();
	}

	FTask* FParallelSystem::WaitForTask()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		
		while(_tasks.size() <= 0 && !_bTerminateFlag)
		{
			_cv.wait(lock);
		}

		FTask* outTask = nullptr;
		if (_tasks.size() > 0)
		{
			outTask = _tasks[0];
			_tasks.erase(_tasks.begin());
		}

		_cv.notify_all();
		return outTask;
	}

	void FParallelSystem::Start(int numthreads)
	{
		_bTerminateFlag = false;
		for (int i=0; i<numthreads; ++i)
		{
			_threads.push_back(std::thread(&thread_callback, this));
		} // end for 
	}

	void FParallelSystem::Terminate()
	{
		_bTerminateFlag = true;
		_cv.notify_all();

		for (int i = 0; i < _threads.size(); ++i)
		{
			_threads[i].join();
		} // end for 
	}

	void FParallelSystem::WaitForEmpty()
	{
		std::unique_lock<std::mutex> lock(_mutex);

		while (_tasks.size() > 0) {
			_cv.wait(lock);
		}
	}

	void FParallelSystem::WaitForFinish()
	{
		WaitForEmpty();
		Terminate();
	}

} // namespace pbrt
