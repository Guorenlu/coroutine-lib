#include "myfiber.h"

static bool debug = false;

namespace sylar
{
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

  // 获取当前线程的主协程
  std::shared_ptr<Fiber> Fiber::GetThis()
  {
    // 主协程存在，直接返回
    if (t_fiber)
    {
      return t_fiber->shared_from_this();
    }

    // 创建主协程
    std::shared_ptr<Fiber> main_fiber(new Fiber());
    t_thread_fiber = main_fiber;
    t_scheduler_fiber = main_fiber.get();
    assert(t_fiber == main_fiber.get());
    return t_fiber->shared_from_this();
  }

  // 设置调度协程
  void Fiber::SetSchedulerFiber(Fiber *f)
  {
    t_scheduler_fiber = f;
  }

  // 获取当前运行协程的ID
  uint64_t Fiber::GetFiberId()
  {
    if (t_fiber)
    {
      return t_fiber->getId();
    }
    return (uint64_t)-1;
  }
}