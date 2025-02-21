
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