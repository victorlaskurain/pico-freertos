#ifndef VLA_QUEUE_HPP
#define VLA_QUEUE_HPP

#include <memory>
#include <queue.h>
#include <type_traits>

namespace vla {

template <typename ItemType> class Queue;

template <typename ItemType> class QueueSender {
    Queue<ItemType> *impl;

  public:
    QueueSender(Queue<ItemType> *q) : impl(q) {}

    bool send(const ItemType &v, TickType_t wait = portMAX_DELAY) {
        return impl->send(v, wait);
    }

    bool sendFront(const ItemType &v, TickType_t wait = portMAX_DELAY) {
        return impl->sendFront(v, wait);
    }
};

template <typename ItemType> class QueueReceiver {
    Queue<ItemType> *impl;

  public:
    QueueReceiver(Queue<ItemType> *q) : impl(q) {}

    bool receive(ItemType &v, TickType_t wait = portMAX_DELAY) {
        return impl->receive(v, wait);
    }

    ItemType receive(TickType_t wait = portMAX_DELAY) {
        return impl->receive(wait);
    }
};

template <typename ItemType> class Queue {
    static_assert(std::is_trivially_copyable<ItemType>::value);
    struct QueueHandleDeleter {
        using pointer = QueueHandle_t;
        void operator()(QueueHandle_t q) { vQueueDelete(q); }
    };
    std::unique_ptr<QueueHandle_t, QueueHandleDeleter> queue;

  public:
    Queue(UBaseType_t length) : queue(xQueueCreate(length, sizeof(ItemType))) {}

    using Sender = QueueSender<ItemType>;
    Sender sender() { return Sender(this); }

    using Receiver = QueueReceiver<ItemType>;
    Receiver receiver() { return Receiver(this); }

    bool send(const ItemType &v, TickType_t wait = portMAX_DELAY) {
        return pdTRUE == xQueueSendToBack(queue.get(), &v, wait);
    }

    bool sendFront(const ItemType &v, TickType_t wait = portMAX_DELAY) {
        return pdTRUE == xQueueSendToFront(queue.get(), &v, wait);
    }

    bool sendFromIsr(const ItemType &v, BaseType_t *taskWoken = nullptr) {
        return pdTRUE == xQueueSendToBackFromIsr(queue.get(), &v, taskWoken);
    }

    bool sendFrontFromIsr(const ItemType &v, BaseType_t *taskWoken = nullptr) {
        return pdTRUE == xQueueSendToFrontFromISR(queue.get(), &v, taskWoken);
    }

    bool receive(ItemType &v, TickType_t wait = portMAX_DELAY) {
        return pdTRUE == xQueueReceive(queue.get(), &v, wait);
    }

    bool receiveFromIsr(ItemType &v, TickType_t wait = portMAX_DELAY) {
        return pdTRUE == xQueueReceiveFromISR(queue.get(), &v, wait);
    }

    ItemType receive(TickType_t wait = portMAX_DELAY) {
        ItemType v;
        xQueueReceive(queue.get(), &v, wait);
        return v;
    }

    operator bool() const { return queue; }
};

/**
 * Utility class to enable passing unique_ptr instances through a queue.

 * Creating an instance of this class will consume the unique_ptr
 * passed as paramenter. The box can be considered a simple non owning
 * smart pointer. It can be freely copied and turned into back into a a
 * unique_ptr at any time.
 *
 * It is the responsability of the user calling the unbox function
 * exactly once for the set of all the copies derived from a single
 * initial unique_ptr.
 */
template <typename UniquePtr> class Box {
    typename UniquePtr::element_type *data;
    typename UniquePtr::deleter_type deleter;

  public:
    Box(UniquePtr ptr) : data(ptr.get()), deleter(ptr.get_deleter()) {
        ptr.release();
    }
    Box() = default;
    UniquePtr unbox() { return UniquePtr(data, deleter); }
};

/**
 * This class can be used to easily define messages with require a
 * reply in a given queue.
 *
 * For example, let's say that we have a service that reports the
 * current temperature as a float when requested. This service
 * could declare that it receives the get_temperature message and
 * replies to it with the temperature message this way:
 *
 * struct temperature_msg_t {
 *     float temp;
 * };
 * struct get_temperature_msg_t : public WithReply<temperature_msg_t>
 * {
 *     template<typename ReplyQueue>
 *     get_temperature_msg_t(ReplyQueue &q):
 *         WithReply<adc_set_value_msg_t>(q){}
 * }
 */
template <typename MsgReply> class WithReply {
    typedef bool (*reply_callback_t)(void *queue, const MsgReply &reply,
                                     TickType_t);
    reply_callback_t cb = nullptr;
    void *queue = nullptr;

  public:
    WithReply() = default;
    template <typename ReplyQueue> WithReply(ReplyQueue &q) {
        queue = &q;
        cb = [](void *q, const MsgReply &reply, TickType_t wait) {
            ReplyQueue *queue = static_cast<ReplyQueue *>(q);
            return queue->sender().send(reply, wait);
        };
    }
    template <typename ConvertibleToMsgReply>
    bool reply(const ConvertibleToMsgReply &reply,
               TickType_t wait = portMAX_DELAY) {
        return cb && queue && cb(queue, reply, wait);
    }
};

} // namespace vla
#endif
