//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef GOPP_GO_SCHEDULER_HPP
#define GOPP_GO_SCHEDULER_HPP

#include <thread>
#include <vector>
#include <boost/context/fiber.hpp>
#include <condition_variable>
#include <queue>
#include <atomic>

namespace go
{

struct scheduler;

inline scheduler & default_scheduler();

template<typename Func>
void go(Func && func, scheduler & schel = default_scheduler());

struct scheduler
{
    static boost::context::fiber & current()
    {
        static thread_local boost::context::fiber current_;
        return current_;
    }
    static auto target() -> boost::context::fiber * &
    {
        static thread_local boost::context::fiber * target = nullptr;
        return target;
    }


    scheduler(std::size_t num_threads = std::thread::hardware_concurrency() - 1)
    {
        for (int i = 0u; i != num_threads; i++)
        {
            threads_.emplace_back([this]{work_();});
        }
    }

    ~scheduler()
    {
        started_ = true;
        cond_.notify_all();
        for (auto & thr : threads_)
            thr.join();
    }

    template<typename Func>
    void spawn(Func && func)
    {
        boost::context::fiber fb([this, func = std::forward<Func>(func)](boost::context::fiber && fb) mutable
                {
                    current() = std::move(fb);
                    started_ = true;
                    struct exiter
                    {
                        std::atomic<std::size_t> & working_;
                        std::condition_variable & cond_;
                        ~exiter()
                        {
                            working_ --;
                            cond_.notify_all();
                        }
                    };
                    working_++;
                    exiter _{working_, cond_};

                    try
                    {
                        func();
                    }
                    catch (std::exception & e)
                    {
                        fprintf(stderr, "Exception while executing go-routine: %s\n", e.what());
                    }
                    return std::move(current());
                });

        enqueue(std::move(fb));
    }

    void enqueue(boost::context::fiber && fb)
    {
        std::lock_guard<std::mutex> lk{mtx_};
        scheduled_.push(std::move(fb));
        cond_.notify_one();
    }

  private:
    void work_()
    {
        while (!started_ || working_ > 0u)
        {
            boost::context::fiber fb;
            {
                // poll one
                std::unique_lock<std::mutex> lock(mtx_);
                cond_.wait(lock, [this] { return !scheduled_.empty() || ((working_ == 0u) && started_); });
                if (scheduled_.empty())
                    return;
                fb = std::move(scheduled_.front());
                scheduled_.pop();
            }
            // this can be used to steal
            target() = &fb;
            *target() = std::move(fb).resume();
        }
    }


    std::mutex mtx_;
    std::condition_variable cond_;
    std::atomic<std::size_t> working_{0u};
    std::atomic<bool> started_{false};
    std::vector<std::thread> threads_;
    std::queue<boost::context::fiber> scheduled_;
};

inline scheduler & default_scheduler()
{
    static scheduler sch{};
    return sch;
}


template<typename Func>
void go(Func && func, scheduler & schel)
{
    schel.spawn(std::forward<Func>(func));
}


}

#endif //GOPP_GO_SCHEDULER_HPP
