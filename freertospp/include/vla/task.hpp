#ifndef VLA_TASK_HPP
#define VLA_TASK_HPP

#include <memory>
#include <task.h>

namespace vla {

class Task {
    struct ClosureHandle {
        void *handle;
        ClosureHandle(void *h = nullptr) : handle(h) {
        }
        bool operator==(ClosureHandle other) const {
            return handle == other.handle;
        }
        bool operator!=(ClosureHandle other) const {
            return handle != other.handle;
        }
    };
    struct ClosureDeleter {
        using pointer = ClosureHandle;
        using Deleter = void (*)(void *);
        Deleter deleter;
        ClosureDeleter(Deleter d = nullptr) : deleter(d) {
        }
        void operator()(ClosureHandle c) {
            deleter(c.handle);
        }
    };
    struct TaskHandleDeleter {
        using pointer = TaskHandle_t;
        void operator()(TaskHandle_t h) {
            vTaskDelete(h);
        }
    };
    using ClosureUniquePtr = std::unique_ptr<ClosureHandle, ClosureDeleter>;
    using TaskUniquePtr    = std::unique_ptr<TaskHandle_t, TaskHandleDeleter>;
    ClosureUniquePtr task_closure =
        ClosureUniquePtr(ClosureHandle(), ClosureDeleter());
    TaskUniquePtr freertos_task = TaskUniquePtr();

  public:
    Task(TaskFunction_t f, const char *name, configSTACK_DEPTH_TYPE stack = 128,
         UBaseType_t priority = tskIDLE_PRIORITY) {
        TaskHandle_t task;
        auto err = xTaskCreate(f, name, stack, nullptr, priority, &task);
        if (err == pdPASS) {
            freertos_task = TaskUniquePtr(task);
        }
    }

    template <typename Callable>
    Task(const Callable &f, const char *name,
         configSTACK_DEPTH_TYPE stack = 128,
         UBaseType_t priority         = tskIDLE_PRIORITY) {
        TaskHandle_t task;
        auto tc       = std::make_unique<Callable>(f);
        auto taskCode = [](void *f) { (*static_cast<Callable *>(f))(); };
        auto err =
            xTaskCreate(taskCode, name, stack, tc.get(), priority, &task);
        if (err != pdPASS) {
            return;
        }
        task_closure =
            ClosureUniquePtr(tc.release(), ClosureDeleter([](void *ptr) {
                                 delete static_cast<const Callable *>(ptr);
                             }));
        freertos_task = TaskUniquePtr(task);
    }

    Task(const Task &) = delete;
    Task(Task &&other) = default;
    Task &operator=(const Task &) = delete;
    Task &operator=(Task &&other) = default;

    operator bool() const {
        return !!task_closure;
    }
};

} // namespace vla
#endif
