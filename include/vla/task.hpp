#ifndef VLA_TASK_HPP
#define VLA_TASK_HPP

#include <memory>
#include <task.h>

namespace vla {

class Task {
    TaskHandle_t freertos_task;
    void *task_closure = nullptr;
    using Deleter = void (*)(const void *);
    Deleter task_closure_deleter = nullptr;

  public:
    Task(TaskFunction_t f, const char *name, configSTACK_DEPTH_TYPE stack = 128,
         UBaseType_t priority = tskIDLE_PRIORITY) {
        auto err =
            xTaskCreate(f, name, stack, nullptr, priority, &freertos_task);
        if (err == pdPASS) {
            task_closure_deleter = [](const void *) {};
        }
    }

    template <typename Callable>
    Task(const Callable &f, const char *name,
         configSTACK_DEPTH_TYPE stack = 128,
         UBaseType_t priority = tskIDLE_PRIORITY) {
        auto tc = std::make_unique<Callable>(f);
        auto taskCode = [](void *f) { (*static_cast<Callable *>(f))(); };
        auto err = xTaskCreate(taskCode, name, stack, tc.get(), priority,
                               &freertos_task);
        if (err != pdPASS) {
            return;
        }
        task_closure = tc.release();
        task_closure_deleter = [](const void *ptr) {
            delete static_cast<const Callable *>(ptr);
        };
    }

    /**
     * Once the destructor is called the task closure data will not be
     * available, so make sure the task is not going to try such an access
     * before freeing the instance.
     */
    ~Task() {
        if (task_closure) {
            vTaskDelete(freertos_task);
            task_closure_deleter(task_closure);
        }
    }

    Task(const Task &) = delete;

    Task(Task &&other) noexcept {
        freertos_task = std::exchange(other.freertos_task, TaskHandle_t());
        task_closure = std::exchange(other.task_closure, nullptr);
        task_closure_deleter =
            std::exchange(other.task_closure_deleter, nullptr);
    }

    Task &operator=(const Task &) = delete;

    Task &operator=(Task &&other) {
        std::swap(*this, other);
        return *this;
    }

    operator bool() const { return task_closure_deleter; }
};

} // namespace vla
#endif
