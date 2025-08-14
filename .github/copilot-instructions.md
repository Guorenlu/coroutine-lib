# Copilot Instructions for coroutine-lib

## Overview & Architecture

This project is a modular C++ coroutine library, inspired by the sylar server framework, focused on user-level coroutine scheduling, timer management, and high-performance network programming. The codebase is organized into several key modules, each in its own subdirectory under `fiber_lib/`:

- **fiber**: Implements the core coroutine (协程) abstraction (`fiber_lib/2fiber`). All user tasks run inside `Fiber` objects. See `fiber.h`, `fiber.cpp`, and `test.cpp`.
- **thread**: Provides thread abstraction and synchronization primitives (`fiber_lib/1thread`). Used to enable multi-core concurrency alongside coroutines.
- **scheduler**: Implements cooperative scheduling of fibers, allowing automatic suspension and resumption (`fiber_lib/3scheduler`).
- **ioscheduler**: Extends the scheduler to support IO event-driven scheduling using `epoll` (`fiber_lib/5iomanager`).
- **timer**: Provides timer management using a min-heap for efficient timeout handling (`fiber_lib/4timer`).
- **hook**: Uses function hooking to transparently convert blocking system calls (e.g., `sleep`, `read`, `write`) into coroutine-friendly, non-blocking operations (`fiber_lib/6hook`).
- **epoll/libevent**: Standalone high-performance HTTP servers using either raw `epoll` or `libevent` for benchmarking and comparison.

## Developer Workflows

- **Build**: Each module can be built independently. For example, to build the hook-enabled coroutine server:
  ```sh
  cd fiber_lib/6hook
  g++ *.cpp -std=c++17 -o main -ldl -lpthread
  ./main
  ```
  For `epoll`/`libevent` servers, use the provided `CMakeLists.txt` in their respective directories.
- **Test**: Each module has its own `main.cpp` or `test.cpp` for targeted testing. Run these directly after building.
- **Benchmark**: Use ApacheBench (`ab`) to stress-test the HTTP servers as described in the root `README.md`.

## Project-Specific Patterns & Conventions

- **Fiber Lifecycle**: Always initialize the main fiber in a thread with `Fiber::GetThis()`. Use `std::shared_ptr<Fiber>` for all fiber objects to ensure proper resource management.
- **Scheduler Usage**: Do not manually switch fibers outside the scheduler context. Use the provided scheduler classes to manage execution order.
- **IO Integration**: When using `ioscheduler`, always register file descriptors with the event manager and use coroutine-friendly hooks for IO.
- **Timer Integration**: Use the timer module for all time-based events; do not use raw `sleep` in coroutine code unless hooked.
- **Hooking**: All blocking system calls in coroutine code should be replaced or hooked to ensure non-blocking, cooperative behavior.
- **Module Independence**: Each module is self-contained for learning and benchmarking. Integration is demonstrated in the `6hook` directory.

## Integration Points & External Dependencies

- **libevent**: Required for the `fiber_lib/libevent` module. Ensure it is installed and discoverable by CMake.
- **epoll**: Used natively in `fiber_lib/epoll` and `fiber_lib/5iomanager`.
- **POSIX Threads**: Used throughout for thread management and synchronization.
- **Dynamic Linking**: The hook module requires `-ldl` for dynamic function interception.

## Key Files & Directories

- `fiber_lib/2fiber/fiber.h`, `fiber.cpp`, `test.cpp`: Core coroutine implementation and usage.
- `fiber_lib/3scheduler/scheduler.h`, `scheduler.cpp`: Cooperative scheduler.
- `fiber_lib/5iomanager/ioscheduler.h`, `main.cpp`: IO event-driven scheduling.
- `fiber_lib/6hook/hook.h`, `main.cpp`: System call hooking and integration demo.
- `README.md`, `简介.md`: High-level project and module documentation.

## Example Patterns

- **Creating and running a fiber**:
  ```cpp
  std::shared_ptr<Fiber> fiber = std::make_shared<Fiber>([](){ /* task */ });
  fiber->resume();
  ```
- **Scheduling fibers**:
  ```cpp
  Scheduler sc;
  sc.schedule(fiber);
  sc.run();
  ```
- **Hooked sleep in coroutine**:
  ```cpp
  sleep(1); // Will be non-blocking if hook is enabled
  ```

---

If any section is unclear or missing important project-specific details, please provide feedback so this guide can be improved.
