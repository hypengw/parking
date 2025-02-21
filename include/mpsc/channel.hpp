#pragma once

#include <atomic>
#include <optional>
#include <chrono>
#include <tuple>
#include <condition_variable>
#include <mutex>
#include <chrono>

#include <tbb/concurrent_queue.h>

namespace mpsc
{
namespace details::channel
{
template<typename C>
struct Counter {
    std::atomic_size_t senders;
    std::atomic_size_t receivers;
    std::atomic_bool   destroy;
    std::unique_ptr<C> chan;
};

template<typename C>
struct Sender {
    using chan_t = C;

    Counter<C>* pCounter;

    auto counter() const -> Counter<C>& { return *pCounter; }

    auto acquire() const -> Sender<C> {
        auto count = this->counter().senders.fetch_add(1, std::memory_order_relaxed);
        return Sender<C> { this->pCounter };
    }

    auto release(void (*disconnect)(C& q)) -> void {
        if (this->counter().senders.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            disconnect(*(this->counter().chan));

            if (this->counter().destroy.exchange(true, std::memory_order_acq_rel)) {
                delete this->pCounter;
            }
        }
    }
};

template<typename C>
struct Receiver {
    using chan_t = C;

    Counter<C>* pCounter;

    auto counter() const -> Counter<C>& { return *pCounter; }

    auto acquire() const -> Receiver<C> {
        auto count = this->counter().receivers.fetch_add(1, std::memory_order_relaxed);
        return Receiver<C> { this->pCounter };
    }

    auto release(void (*disconnect)(C& q)) -> void {
        if (this->counter().receivers.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            disconnect(*(this->counter().chan));

            if (this->counter().destroy.exchange(true, std::memory_order_acq_rel)) {
                delete this->pCounter;
            }
        }
    }
};

template<typename C>
auto create(std::unique_ptr<C>&& chan) -> std::tuple<Sender<C>, Receiver<C>> {
    auto pCounter = new Counter<C> { 1, 1, false, std::move(chan) };
    pCounter->chan->set_counter(pCounter);
    return { Sender<C> { pCounter }, Receiver<C> { pCounter } };
}

template<typename T>
class TbbChannel {
    struct TW {
        std::optional<T> t;
    };

public:
    void set_counter(Counter<TbbChannel<T>>* c) { this->counter = c; }

    template<typename Tt>
    auto send(Tt&& t) -> bool {
        try {
            this->queue.push(TW { std::forward<Tt>(t) });
            if (this->counter->receivers == 0) {
                TW tw {};
                this->queue.try_pop(tw);
                return false;
            }
        } catch (const tbb::user_abort& e) {
            return false;
        }
        recv_cv.notify_one();
        return true;
    }

    template<typename Tt>
    auto try_send(Tt&& t) -> bool {
        auto ret = this->queue.try_push(TW { std::forward<Tt>(t) });
        if (ret) {
            if (this->counter->receivers == 0) {
                this->queue.try_pop();
                recv_cv.notify_one();
                return false;
            }
            recv_cv.notify_one();
        }
        return ret;
    }

    auto recv() -> std::optional<T> {
        try {
            TW tw;
            this->queue.pop(tw);
            if (! tw.t) {
                this->queue.push(tw);
            }
            return tw.t;
        } catch (const tbb::user_abort& e) {
            return std::nullopt;
        }
    }

    template<class TRep, class TPeriod>
    auto recv(const std::chrono::duration<TRep, TPeriod>& dur) -> std::optional<T> {
        std::unique_lock lock { this->recv_mutex };
        auto             end = std::chrono::steady_clock::now() + dur;
        T                t;
        while (! this->queue.try_pop(t)) {
            auto cur = std::chrono::steady_clock::now();
            if (cur < end) {
                this->recv_cv.wait_for(lock, end - cur);
            } else
                return std::nullopt;
        }
        return t;
    }

    auto try_recv() -> std::optional<T> {
        TW tw;
        if (this->queue.try_pop(tw)) {
            if (! tw.t) {
                this->queue.push(tw);
            }
            return tw.t;
        }
        return std::nullopt;
    }

    auto disconnect() -> void {
        if (this->counter->senders == 0) {
            this->queue.push(TW {});
        } else if (this->counter->receivers == 0) {
            TW tw;
            this->queue.try_pop(tw);
        }
        recv_cv.notify_all();
        // this->queue.abort();
    }

private:
    std::condition_variable recv_cv;
    std::mutex              recv_mutex;

    tbb::concurrent_bounded_queue<TW> queue;
    Counter<TbbChannel<T>>*           counter;
};
} // namespace details::channel

namespace channel
{
template<typename T>
class Sender;

template<typename T>
class Receiver;

template<typename T>
auto channel() -> std::tuple<Sender<T>, Receiver<T>>;

template<typename T>
class Sender {
    friend auto channel<T>() -> std::tuple<Sender<T>, Receiver<T>>;

public:
    Sender(const Sender& o): inner(o.inner.acquire()) {}
    ~Sender() {
        inner.release([](auto& c) {
            c.disconnect();
        });
    }

    Sender& operator=(const Sender& o) {
        inner = o.inner.acquire();
        return *this;
    }

    Sender(Sender&&)           = delete;
    Sender operator=(Sender&&) = delete;

    auto send(T) const -> bool;
    auto try_send(T) const -> bool;

private:
    using Inner = details::channel::Sender<details::channel::TbbChannel<T>>;
    Sender(Inner inner): inner(inner) {}
    auto chan() const -> typename Inner::chan_t& { return *(inner.counter().chan); }

    Inner inner;
};

template<typename T>
class Receiver {
    friend auto channel<T>() -> std::tuple<Sender<T>, Receiver<T>>;

public:
    Receiver(const Receiver& o): inner(o.inner.acquire()) {}
    ~Receiver() {
        inner.release([](auto& c) {
            c.disconnect();
        });
    }

    Receiver& operator=(const Receiver& o) {
        inner = o.inner.acquire();
        return *this;
    }

    Receiver(Receiver&&)           = delete;
    Receiver operator=(Receiver&&) = delete;

    auto try_recv() -> std::optional<T>;
    auto recv() -> std::optional<T>;
    template<typename TR, typename TP>
    auto recv_timeout(const std::chrono::duration<TR, TP>&) -> std::optional<T>;
    // iter
    // try_iter
private:
    using Inner = details::channel::Receiver<details::channel::TbbChannel<T>>;
    Receiver(Inner inner): inner(inner) {}
    auto chan() const -> typename Inner::chan_t& { return *(inner.counter().chan); }

    Inner inner;
};

template<typename T>
auto channel() -> std::tuple<Sender<T>, Receiver<T>> {
    using C     = details::channel::TbbChannel<T>;
    auto [s, r] = details::channel::create<C>(std::make_unique<C>());
    return { Sender<T>(s), Receiver<T>(r) };
}

template<typename T>
auto Sender<T>::send(T t) const -> bool {
    return this->chan().send(t);
}
template<typename T>
auto Sender<T>::try_send(T t) const -> bool {
    return this->chan().try_send(t);
}

template<typename T>
auto Receiver<T>::try_recv() -> std::optional<T> {
    return this->chan().try_recv();
}
template<typename T>
auto Receiver<T>::recv() -> std::optional<T> {
    return this->chan().recv();
}

template<typename T>
template<typename TR, typename TP>
auto Receiver<T>::recv_timeout(const std::chrono::duration<TR, TP>& dur) -> std::optional<T> {
    return this->chan().recv(dur);
}
} // namespace channel
} // namespace mpsc
