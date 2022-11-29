#include "thread_pool.hpp"

namespace gottvergessen
{
	thread_pool::thread_pool() : m_accept_jobs(true)
	{
		m_managing_thread = std::thread(&thread_pool::create, this);

		g_thread_pool = this;
	}

	thread_pool::~thread_pool()
	{
		g_thread_pool = nullptr;
	}

	void thread_pool::create()
	{
		const std::uint32_t thread_count = std::thread::hardware_concurrency();

		LOG(G3LOG_DEBUG) << "Allocating " << thread_count << " threads in thread pool.";
		m_thread_pool.reserve(thread_count);

		for (std::uint32_t i = 0; i < thread_count; i++)
			m_thread_pool.emplace_back(std::thread(&thread_pool::run, this));
	}

	void thread_pool::destroy()
	{
		m_managing_thread.join();
		{
			std::unique_lock lock(m_lock);
			m_accept_jobs = false;
		}
		m_data_condition.notify_all();

		for (auto& thread : m_thread_pool)
			thread.join();

		m_thread_pool.clear();
	}

	void thread_pool::add_job(std::function<void()> func)
	{
		if (func)
		{
			{
				std::unique_lock lock(m_lock);
				this->m_job_stack.push(std::move(func));
			}
			m_data_condition.notify_all();
		}
	}

	void thread_pool::run()
	{
		for (;;)
		{
			std::unique_lock lock(m_lock);

			m_data_condition.wait(lock, [this]()
				{
					return !m_job_stack.empty() || !m_accept_jobs;
				});

			if (!m_accept_jobs) break;
			if (m_job_stack.empty()) continue;

			std::function<void()> job = std::move(m_job_stack.top());
			m_job_stack.pop();
			lock.unlock();

			try
			{
				std::invoke(std::move(job));
			}
			catch (const std::exception& e)
			{
				LOG(WARNING) << "Exception thrown while executing job in thread:" << std::endl << e.what();
			}
		}

		LOG(G3LOG_DEBUG) << "Thread " << std::this_thread::get_id() << " exiting...";
	}
}