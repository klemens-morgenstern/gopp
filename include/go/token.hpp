//
// Copyright (c) 2022 Klemens Morgenstern (klemens.morgenstern@gmx.net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef GOPP_GO_TOKEN_HPP
#define GOPP_GO_TOKEN_HPP

#include <boost/asio/async_result.hpp>
#include <boost/optional.hpp>

namespace go
{
struct token_t
{

};

constexpr token_t token;

}

template <typename T>
struct boost::asio::async_result<go::token_t, void(T)>
{
    using return_type = T;

    template <typename Initiation, typename... InitArgs>
    static return_type initiate(Initiation initiation,
                                go::token_t t, InitArgs... args)
    {
        boost::optional<T> res;
        boost::context::fiber fb;
        std::move(initiation)(
                [&](T res_)
                {
                    res = std::move(res_);
                   go::scheduler::current() = std::move(fb).resume();
                }, std::forward<InitArgs>(args)...);

        go::scheduler::target() = &fb;
        go::scheduler::current() = std::move(go::scheduler::current()).resume();
        return res.value();
    }
};


template <typename ... Ts>
struct boost::asio::async_result<go::token_t, void(Ts...)>
{
    using return_type = std::tuple<Ts...>;

    template <typename Initiation, typename... InitArgs>
    static return_type initiate(Initiation initiation,
                                go::token_t t, InitArgs... args)
    {
        boost::optional<std::tuple<Ts...>> res;
        boost::context::fiber fb;
        std::move(initiation)(
                [&](Ts... res_)
                {
                    res.emplace(std::move(res_)...);
                    go::scheduler::current() = std::move(fb).resume();
                }, std::forward<InitArgs>(args)...);

        go::scheduler::target() = &fb;
        go::scheduler::current() = std::move(go::scheduler::current()).resume();
        return res.value();
    }
};


#endif //GOPP_GO_TOKEN_HPP
