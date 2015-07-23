#include "ThreadPool.h"
#include "LogWriter.h"
#include <mutex>
#include <future>
#include <deque>
#include "ITask.h"

struct ThreadPool::Impl
{
	struct sRunFunc
	{
		FunctionHandler func;
		CallbackHandler callback;
		unsigned int flags;
	};
public:
	void QueueFunc(sRunFunc const& func)
	{
		std::lock_guard<std::mutex> lk(m_funcMutex);
		if (func.flags & FLAG_HIGH_PRIORITY)
		{
			m_funcs.push_front(func);
		}
		else
		{
			m_funcs.push_back(func);
		}
		m_conditional.notify_one();
	}

	void RunFunc(FunctionHandler const& func, CallbackHandler const& callback, unsigned int flags)
	{
		QueueFunc(sRunFunc{ func, callback, flags });
	}

	void QueueCallback(CallbackHandler const& callback, unsigned int flags)
	{
		std::lock_guard<std::mutex> lk(m_callbackMutex);
		if (flags & FLAG_HIGH_PRIORITY)
		{
			m_callbacks.push_front(callback);
		}
		else
		{
			m_callbacks.push_back(callback);
		}
	}

	void AddTask(std::shared_ptr<ITask> task)
	{
		std::lock_guard<std::mutex> lk(m_tasksMutex);
		m_tasks.push_back(task);
		task->Queue();
		m_conditional.notify_one();
	}

	void RemoveTask(ITask * task)
	{
		std::lock_guard<std::mutex> lk(m_tasksMutex);
		auto it = std::find_if(m_tasks.begin(), m_tasks.end(), [task](std::shared_ptr<ITask> const& taskptr) {return taskptr.get() == task;});
		if (it != m_tasks.end())
		{
			m_tasks.erase(it);
		}
		auto it2 = std::find_if(m_storedTasks.begin(), m_storedTasks.end(), [task](std::shared_ptr<ITask> const& taskptr) {return taskptr.get() == task;});
		if (it2 != m_storedTasks.end())
		{
			m_storedTasks.erase(it2);
		}
	}

	void Update()
	{
		if ((!m_funcs.empty() || !m_tasks.empty()) && m_threads.size() < m_maxThreads)
		{
			m_threads.push_back(std::async(std::launch::async, std::bind(&Impl::WorkerThread, this)));
		}
		while (!m_callbacks.empty())
		{
			if (m_callbacks.front()) m_callbacks.front()();
			bool last = m_callbacks.size() == 1;
			if (last) m_callbackMutex.lock();
			m_callbacks.pop_front();
			if (last) m_callbackMutex.unlock();
		}
	}

	size_t GetTasksAndFuncsCount()
	{
		m_funcMutex.lock();
		size_t result = m_funcs.size();
		m_funcMutex.unlock();
		m_tasksMutex.lock();
		result += m_tasks.size();
		m_tasksMutex.unlock();
		return result;
	}

	void CancelAll()
	{
		m_cancelled = true;
		m_funcMutex.lock();
		m_funcs.clear();
		m_funcMutex.unlock();
		m_tasksMutex.lock();
		m_tasks.clear();
		m_tasksMutex.unlock();
		m_conditional.notify_all();
		m_callbackMutex.lock();
		m_callbacks.clear();
		m_callbackMutex.unlock();
		m_threads.clear();
		m_cancelled = false;
	}

	void WorkerThread()
	{
		for (;;)
		{
			bool hasTasks = false;
			{
				std::unique_lock<std::mutex> lk(m_funcMutex);
				if (!m_funcs.empty())
				{
					hasTasks = true;
					sRunFunc func = std::move(m_funcs.front());
					m_funcs.pop_front();
					lk.unlock();
					func.func();
					if (func.callback)
					{
						QueueCallback(func.callback, func.flags);
					}
				}
			}
			{
				std::unique_lock<std::mutex> lk(m_tasksMutex);
				if (!m_tasks.empty())
				{
					hasTasks = true;
					std::shared_ptr<ITask> task = m_tasks.front();
					m_tasks.pop_front();
					m_storedTasks.push_back(task);
					lk.unlock();
					task->Execute();
				}
			}
			if (!hasTasks)
			{
				std::unique_lock<std::mutex> lk(m_conditionalMutex);
				m_conditional.wait(lk);
				if (m_cancelled)
				{
					return;
				}
			}
		}
		
	}

	std::deque<FunctionHandler> m_callbacks;
	std::deque<sRunFunc> m_funcs;
	std::deque<std::shared_ptr<ITask>> m_tasks;
	std::vector<std::shared_ptr<ITask>> m_storedTasks;
	unsigned int m_maxThreads = std::thread::hardware_concurrency();
	bool m_cancelled = false;
	std::vector<std::future<void>> m_threads;
	std::condition_variable m_conditional;
	std::mutex m_conditionalMutex;
	std::mutex m_callbackMutex;
	std::mutex m_funcMutex;
	std::mutex m_tasksMutex;
};

std::unique_ptr<ThreadPool::Impl> ThreadPool::m_pImpl(std::unique_ptr<ThreadPool::Impl>(new ThreadPool::Impl()));

void ThreadPool::RunFunc(FunctionHandler const& func, CallbackHandler const& callback, unsigned int flags)
{
	m_pImpl->RunFunc(func, callback, flags);
}

void ThreadPool::QueueCallback(CallbackHandler const& func, unsigned int flags)
{
	m_pImpl->QueueCallback(func, flags);
}

void ThreadPool::Update()
{
	m_pImpl->Update();
}

size_t ThreadPool::GetTasksAndFuncsCount()
{
	return m_pImpl->GetTasksAndFuncsCount();
}

void ThreadPool::CancelAll()
{
	m_pImpl->CancelAll();
}

void ThreadPool::AddTask(std::shared_ptr<ITask> task)
{
	m_pImpl->AddTask(task);
}

void ThreadPool::RemoveTask(ITask * task)
{
	m_pImpl->RemoveTask(task);
}
