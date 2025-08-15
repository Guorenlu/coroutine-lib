#include "fiber.h"

static bool debug = false;

namespace sylar
{

	// 当前线程上的协程控制信息

	// 正在运行的协程
	static thread_local Fiber *t_fiber = nullptr;
	// 主协程
	static thread_local std::shared_ptr<Fiber> t_thread_fiber = nullptr;
	// 调度协程
	static thread_local Fiber *t_scheduler_fiber = nullptr;

	// 协程计数器
	static std::atomic<uint64_t> s_fiber_id{0};
	// 协程总数
	static std::atomic<uint64_t> s_fiber_count{0};

	// 设置当前运行的协程
	void Fiber::SetThis(Fiber *f)
	{
		t_fiber = f;
	}

	// 获取“当前线程的当前协程”（shared_ptr）
	// 说明：
	// - 若 t_fiber 已存在：直接返回当前协程 t_fiber->shared_from_this()（可能是主协程，也可能是子协程）。
	// - 若 t_fiber 不存在：说明尚未初始化当前线程的主协程；此时构造主协程（t_thread_fiber），
	//   并将其设为默认调度协程（t_scheduler_fiber），随后返回主协程。
	std::shared_ptr<Fiber> Fiber::GetThis()
	{
		if (t_fiber)
		{
			return t_fiber->shared_from_this();
		}

		// 创建主协程
		std::shared_ptr<Fiber> main_fiber(new Fiber());
		t_thread_fiber = main_fiber;
		t_scheduler_fiber = main_fiber.get(); // 默认为主协程作为调度协程，除非主动更改

		assert(t_fiber == main_fiber.get());
		return t_fiber->shared_from_this();
	}

	// 设置调度协程
	void Fiber::SetSchedulerFiber(Fiber *f)
	{
		t_scheduler_fiber = f;
	}

	// 获取当前运行的协程 ID
	uint64_t Fiber::GetFiberId()
	{
		if (t_fiber)
		{
			return t_fiber->getId();
		}
		return (uint64_t)-1;
	}

	// 构造函数：创建主协程
	Fiber::Fiber()
	{
		SetThis(this);
		m_state = RUNNING;

		// 初始化主协程的上下文
		if (getcontext(&m_ctx))
		{
			std::cerr << "Fiber() failed\n";
			pthread_exit(NULL);
		}

		m_id = s_fiber_id++;
		s_fiber_count++;
		if (debug)
			std::cout << "Fiber(): main id = " << m_id << std::endl;
	}

	// 构造函数：创建子协程
	Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler) : m_cb(cb), m_runInScheduler(run_in_scheduler)
	{
		m_state = READY;

		// 分配协程栈空间
		m_stacksize = stacksize ? stacksize : 128000; // 默认栈大小为128KB
		m_stack = malloc(m_stacksize);

		// 初始化子协程的上下文
		if (getcontext(&m_ctx))
		{
			std::cerr << "Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler) failed\n";
			pthread_exit(NULL);
		}

		m_ctx.uc_link = nullptr;
		m_ctx.uc_stack.ss_sp = m_stack;
		m_ctx.uc_stack.ss_size = m_stacksize;
		makecontext(&m_ctx, &Fiber::MainFunc, 0); // 设置协程入口函数

		m_id = s_fiber_id++;
		s_fiber_count++;
		if (debug)
			std::cout << "Fiber(): child id = " << m_id << std::endl;
	}

	// 析构函数：释放协程资源
	Fiber::~Fiber()
	{
		s_fiber_count--;
		if (m_stack)
		{
			free(m_stack); // 释放栈空间
		}
		if (debug)
			std::cout << "~Fiber(): id = " << m_id << std::endl;
	}

	// 重置协程：复用已结束的协程
	void Fiber::reset(std::function<void()> cb)
	{
		assert(m_stack != nullptr && m_state == TERM);

		m_state = READY;
		m_cb = cb;

		// 重新初始化协程上下文
		if (getcontext(&m_ctx))
		{
			std::cerr << "reset() failed\n";
			pthread_exit(NULL);
		}

		m_ctx.uc_link = nullptr;
		m_ctx.uc_stack.ss_sp = m_stack;
		m_ctx.uc_stack.ss_size = m_stacksize;
		makecontext(&m_ctx, &Fiber::MainFunc, 0);
	}

	// 恢复协程：切换到当前协程运行
	void Fiber::resume()
	{
		assert(m_state == READY);

		m_state = RUNNING;

		// 根据是否运行在调度协程中选择上下文切换目标
		if (m_runInScheduler)
		{
			SetThis(this);
			if (swapcontext(&(t_scheduler_fiber->m_ctx), &m_ctx))
			{
				std::cerr << "resume() to t_scheduler_fiber failed\n";
				pthread_exit(NULL);
			}
		}
		else
		{
			SetThis(this);
			if (swapcontext(&(t_thread_fiber->m_ctx), &m_ctx))
			{
				std::cerr << "resume() to t_thread_fiber failed\n";
				pthread_exit(NULL);
			}
		}
	}

	// 让出协程：切换到调度协程或主协程
	void Fiber::yield()
	{
		assert(m_state == RUNNING || m_state == TERM);

		if (m_state != TERM)
		{
			m_state = READY;
		}

		// 根据是否运行在调度协程中选择上下文切换目标
		if (m_runInScheduler)
		{
			SetThis(t_scheduler_fiber);
			if (swapcontext(&m_ctx, &(t_scheduler_fiber->m_ctx)))
			{
				std::cerr << "yield() to t_scheduler_fiber failed\n";
				pthread_exit(NULL);
			}
		}
		else
		{
			SetThis(t_thread_fiber.get());
			if (swapcontext(&m_ctx, &(t_thread_fiber->m_ctx)))
			{
				std::cerr << "yield() to t_thread_fiber failed\n";
				pthread_exit(NULL);
			}
		}
	}

	// 协程入口函数：执行协程的回调函数
	void Fiber::MainFunc()
	{
		std::shared_ptr<Fiber> curr = GetThis();
		assert(curr != nullptr);

		curr->m_cb(); // 执行回调函数
		curr->m_cb = nullptr;
		curr->m_state = TERM;

		// 运行完毕 -> 让出执行权
		auto raw_ptr = curr.get();
		curr.reset();
		raw_ptr->yield();
	}

}