/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 EnyxSA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <memory>
#include <iostream>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include "SessionConfiguration.hpp"
#include "Socket.hpp"

#ifdef __linux__
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <netinet/tcp.h>
#endif

namespace enyx {
namespace net_tester {

class TcpSocket : public Socket
{
public:
    using socket_type = boost::asio::ip::tcp::socket;
    using acceptor_type = boost::asio::ip::tcp::acceptor;
    using protocol_type = socket_type::protocol_type;

public:
    template<typename OnConnectHandler>
    explicit
    TcpSocket(boost::asio::io_service & io_service,
              const SessionConfiguration & configuration,
              OnConnectHandler on_connect)
        : Socket(io_service),
          socket_(io_service_)
    {
        switch (configuration.mode)
        {
            default:
            case SessionConfiguration::CLIENT:
                connect(configuration, on_connect);
                break;
            case SessionConfiguration::SERVER:
                listen(configuration, io_service, on_connect);
                break;
        }
    }

    template<typename MutableBufferSequence, typename ReadHandler>
    void
    async_receive(const MutableBufferSequence & buffers, ReadHandler handler)
    {
#ifdef __linux__
        int const flag = 1;
        int failure = ::setsockopt(socket_.native_handle(),
                                   IPPROTO_TCP, TCP_QUICKACK,
                                   &flag, sizeof(flag));
        if (failure)
            throw std::system_error{errno, std::generic_category(),
                                    "Failed to set TCP_QUICKACK"};
#endif
        socket_.async_receive(buffers, handler);
    }

    template<typename ConstBufferSequence, typename WriteHandler>
    void
    async_send(const ConstBufferSequence & buffers, WriteHandler handler)
    {
        socket_.async_send(buffers, handler);
    }

    void
    shutdown_send();

    void
    close();

private:
    template<typename OnConnectHandler>
    void
    connect(const SessionConfiguration & configuration,
            OnConnectHandler on_connect)
    {
        const auto e = resolve<protocol_type>(configuration.endpoint);

        socket_.open(e.first.protocol());
        socket_type::reuse_address reuse_address(true);
        socket_.set_option(reuse_address);
        socket_.bind(e.first);
        setup_windows(configuration, socket_);

        auto handler = [this, on_connect]
                (const boost::system::error_code & failure) {
            on_connect();
        };

        socket_.async_connect(e.second, std::move(handler));
    }

    template<typename OnConnectHandler>
    void
    listen(const SessionConfiguration & configuration,
           boost::asio::io_service & io_service,
           OnConnectHandler on_connect)
    {
        const auto e = resolve<protocol_type>(configuration.endpoint);

        // Schedule an asynchronous accept.
        auto a = std::make_shared<acceptor_type>(io_service,
                                                 e.second.protocol());
        socket_type::reuse_address reuse_address(true);
        a->set_option(reuse_address);
        setup_windows(configuration, *a);
        a->bind(e.second);
        a->listen();

        // Asynchronously Wait for a client to connect.
        auto handler = [this, a, on_connect]
                (const boost::system::error_code & f) {
            on_connect();
        };

        a->async_accept(socket_, std::move(handler));
    }

private:
    socket_type socket_;
};

} // namespace net_tester
} // namespace enyx

