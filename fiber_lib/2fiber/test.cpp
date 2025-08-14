#include "fiber.h"
#include <vector>

using namespace sylar;

// 调度器类：负责管理协程任务并执行调度
class Scheduler
{
public:
	// 添加协程调度任务
	// 参数：
	// - task：需要调度的协程任务
	void schedule(std::shared_ptr<Fiber> task)
	{
		m_tasks.push_back(task); // 将任务添加到任务队列
	}

	// 执行调度任务
	// 遍历任务队列，依次恢复协程运行
	void run()
	{
		std::cout << " number " << m_tasks.size() << std::endl;

		std::shared_ptr<Fiber> task;
		auto it = m_tasks.begin();
		while (it != m_tasks.end())
		{
			// 获取任务
			task = *it;

			// 恢复协程运行
			// 由主协程切换到子协程，子协程函数运行完毕后自动切换回主协程
			task->resume();

			// 移动到下一个任务
			it++;
		}

		// 清空任务队列
		m_tasks.clear();
	}

private:
	// 任务队列：存储需要调度的协程任务
	std::vector<std::shared_ptr<Fiber>> m_tasks;
};

// 测试函数：协程任务的具体逻辑
// 参数：
// - i：任务编号
void test_fiber(int i)
{
	std::cout << "hello world " << i << std::endl;
}

int main()
{
	// 初始化当前线程的主协程
	// 主协程是线程的入口协程，负责初始化协程环境
	Fiber::GetThis();
	// 新增：打印当前线程的“当前协程”（主协程）ID，帮助理解 GetThis 作用
	std::cout << "current fiber id = " << Fiber::GetFiberId() << " (main fiber)" << std::endl;

	// 创建调度器
	Scheduler sc;

	// 添加调度任务（任务和子协程绑定）
	for (auto i = 0; i < 20; i++)
	{
		// 创建子协程
		// 使用共享指针自动管理资源 -> 过期后自动释放子协程创建的资源
		// bind函数 -> 绑定函数和参数用来返回一个函数对象
		std::shared_ptr<Fiber> fiber = std::make_shared<Fiber>(std::bind(test_fiber, i), 0, false);

		// 将协程任务添加到调度器
		sc.schedule(fiber);
	}

	// 执行调度任务
	// 调度器会依次恢复每个协程任务运行
	sc.run();

	return 0;
}