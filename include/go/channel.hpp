//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef GOPP_GO_CHANNEL_HPP
#define GOPP_GO_CHANNEL_HPP

#include <boost/circular_buffer.hpp>
#include <boost/intrusive/slist.hpp>


namespace go
{

template<typename T, typename Alloc = std::allocator<T>>
struct channel
{
    channel(std::size_t size, Alloc alloc = {})
        : buffer(size, std::move(alloc))
    {}


    boost::circular_buffer<T, Alloc> buffer;
    std::mutex mtx;

    struct waitor :
            boost::intrusive::slist_base_hook</*
                    boost::intrusive::link_mode<
                            boost::intrusive::auto_unlink> */>
    {
        boost::context::fiber fb;
        boost::optional<T> value;
    };

    boost::intrusive::slist<waitor, boost::intrusive::cache_last<true>> send_waitors, receive_waitors;

    void send(T value)
    {
        waitor w;
        {
            std::lock_guard<std::mutex> lock_{mtx};

            if (receive_waitors.size() > 0)
            {
                auto & fr = receive_waitors.front();
                receive_waitors.pop_front();
                fr.value = std::move(value);
                default_scheduler().enqueue(std::move(fr.fb));

                return;
            }

            if (!buffer.full())
            {
                buffer.push_back(std::move(value));
                return ;
            }

            go::scheduler::target() = &w.fb;
            send_waitors.push_back(w);
        }
        go::scheduler::current() = std::move(go::scheduler::current()).resume();
    }

    T receive()
    {
        waitor w;
        {
            boost::optional<T> res;
            std::lock_guard<std::mutex> lock_{mtx};

            if (!buffer.empty())
            {
                auto f = std::move(buffer.front());
                buffer.pop_front();
                return f;
            }

            if (send_waitors.size() > 0)
            {
                auto & fr = send_waitors.front();
                send_waitors.pop_front();
                default_scheduler().enqueue(std::move(fr.fb));
                return std::move(fr.value.value());
            }


            go::scheduler::target() = &w.fb;
            receive_waitors.push_back(w);
        }
        go::scheduler::current() = std::move(go::scheduler::current()).resume();

        return std::move(w.value.value());
    }

    // TODO add closing


};

}

#endif //GOPP_GO_CHANNEL_HPP
