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

#include <cstddef>

#include <boost/system/error_code.hpp>

#include "Session.hpp"
#include "TcpSocket.hpp"
#include "HandlerAllocator.hpp"

namespace enyx {
namespace net_tester {

class TcpSession : public Session, public std::enable_shared_from_this<TcpSession>
{
public:
    TcpSession(boost::asio::io_service & io_service,
               const SessionConfiguration & configuration);

    virtual void
    initialize() override
    {
        Session::initialize();
        auto self(shared_from_this());
        socket_.open(configuration_, [this, self] { start_transfer(); });
        io_service_.post([this, self] { start_timer(); } );
    }

protected:

    virtual std::shared_ptr<Session>
    shared_from_child() override
    {
        return shared_from_this();
    }

    virtual void
    async_receive(std::size_t slice_remaining_size = 0ULL) override;

    virtual void
    finish_receive() override;

    void
    on_eof(const boost::system::error_code & failure,
           std::size_t bytes_transferred);

    virtual void
    async_send(std::size_t slice_remaining_size = 0ULL) override;

    virtual void
    finish_send() override;

    virtual void
    finish() override;

private:
    TcpSocket socket_;
    HandlerMemory send_handler_memory_;
    HandlerMemory receive_handler_memory_;
};

} // namespace net_tester
} // namespace enyx
