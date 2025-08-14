#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include <iostream>
#include <memory>
#include <atomic>
#include <functional>
#include <cassert>
#include <ucontext.h>
#include <unistd.h>
#include <mutex>

namespace sylar
{

	// Fiber 类：协程的实现
	// 提供协程的创建、切换和销毁功能
	class Fiber : public std::enable_shared_from_this<Fiber>
	{
	public:
		// 协程状态枚举
		enum State
		{
			READY,	 // 准备运行
			RUNNING, // 正在运行
			TERM		 // 已结束
		};

	private:
		// 私有构造函数：仅用于创建主协程
		// 主协程是线程的入口协程，负责初始化协程环境。
		Fiber();

	public:
		// 构造函数：创建子协程
		// 参数：
		// - cb：协程的回调函数
		// - stacksize：协程栈大小（默认为128KB）
		// - run_in_scheduler：是否运行在调度协程中
		Fiber(std::function<void()> cb, size_t stacksize = 0, bool run_in_scheduler = true);

		// 析构函数：释放协程资源
		~Fiber();

		// 重置协程：复用已结束的协程
		// 参数：
		// - cb：新的回调函数
		void reset(std::function<void()> cb);

		// 恢复协程：切换到当前协程运行
		void resume();

		// 让出协程：切换到调度协程或主协程
		void yield();

		// 获取协程 ID
		uint64_t getId() const { return m_id; }

		// 获取协程状态
		State getState() const { return m_state; }

	public:
		// 设置当前运行的协程
		// 参数：
		// - f：当前运行的协程对象
		static void SetThis(Fiber *f);

		// 获取当前运行的协程
		// 返回：当前运行的协程对象
		static std::shared_ptr<Fiber> GetThis();

		// 设置调度协程（默认为主协程）
		// 参数：
		// - f：调度协程对象
		static void SetSchedulerFiber(Fiber *f);

		// 获取当前运行的协程 ID
		// 返回：当前协程的 ID
		static uint64_t GetFiberId();

		// 协程函数：协程的入口函数
		// 负责执行协程的回调函数，并在结束后让出执行权。
		static void MainFunc();

	private:
		// 协程 ID
		uint64_t m_id = 0;

		// 栈大小
		uint32_t m_stacksize = 0;

		// 协程状态
		State m_state = READY;

		// 协程上下文
		ucontext_t m_ctx;

		// 协程栈指针
		void *m_stack = nullptr;

		// 协程函数
		std::function<void()> m_cb;

		// 是否运行在调度协程中
		bool m_runInScheduler;

	public:
		// 协程的互斥锁
		std::mutex m_mutex;
	};

}

#endif
