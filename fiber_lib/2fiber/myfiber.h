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
  // Fiber类
  class Fiber : public std::enable_shared_from_this<Fiber>
  {
    // 运行状态
  public:
    enum State
    {
      READY,
      RUNNING,
      TERM
    };

  private:
    // 主协程构造函数
    Fiber();

  public:
    // 子协程构造函数
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool run_in_scheduler = true);

    // 析构函数
    ~Fiber();

    // 重置协程
    void reset(std::function<void()> cb);

    // 恢复协程
    void resume();

    // 让出协程
    void yield();

    // 获取协程id
    uint64_t getId() const { return m_id; }

    // 获取协程状态
    State getState() const { return m_state; }

  public:
    // 设置当前运行的协程
    static void SetThis(Fiber *f);

    // 获取当前运行的进程
    static std::shared_ptr<Fiber> GetThis();

    // 设置调度进程
    static void SetSchedulerFiber(Fiber *f);

    // 获取当前运行的协程id
    static uint64_t GetFiberId();

    // 协程函数：协程的入口函数
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
  };
}
#endif