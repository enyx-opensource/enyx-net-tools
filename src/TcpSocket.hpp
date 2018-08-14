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

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include "Configuration.hpp"
#include "Socket.hpp"

namespace enyx {
namespace tcp_tester {

class TcpSocket : public Socket
{
public:
    using socket_type = boost::asio::ip::tcp::socket;

public:
    explicit
    TcpSocket(boost::asio::io_service & io_service,
              const Configuration & configuration);

    template<typename MutableBufferSequence, typename ReadHandler>
    void
    async_receive(const MutableBufferSequence & buffers, ReadHandler handler)
    {
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
    void
    connect(const Configuration & configuration);

    void
    listen(const Configuration & configuration,
           const boost::posix_time::time_duration & timeout);

private:
    socket_type socket_;
};

} // namespace tcp_tester
} // namespace enyx

