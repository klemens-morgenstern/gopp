//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <go.hpp>

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/detached.hpp>


#include <cstdio>
#include "doctest.h"


TEST_SUITE_BEGIN("go");

TEST_CASE("basics")
{
    go::scheduler sch;

    boost::asio::io_context ctx;
    boost::asio::steady_timer tim{ctx, std::chrono::milliseconds(10)};
    tim.async_wait(boost::asio::detached);

    go::channel<int> chan{3};

    go::go([&]{
        int i = 0;
        while (i != 100)
        {
            i = chan.receive();
            printf("Received %d\n", i);
        }
    }, sch);

    go::go([&]{
        // PoC of asio integration
        tim.async_wait(go::token);

        for (int i = 11; i <= 100; i++)
        {
            printf("Sending %d\n", i);
            chan.send(i);
            printf(" - Sent %d\n", i);

        }
    }, sch);



    ctx.run();
}

TEST_SUITE_END();