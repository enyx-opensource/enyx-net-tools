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

#include "UdpSession.hpp"

#include <iostream>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/error.hpp>

#include "Error.hpp"
#include "Socket.hpp"
#include "Statistics.hpp"

namespace enyx {
namespace net_tester {

UdpSession::UdpSession(boost::asio::io_service & io_service,
                       const SessionConfiguration & configuration)
    : Session(io_service, configuration),
      socket_(io_service, configuration),
      send_handler_memory_(),
      receive_handler_memory_(),
      random_generator_(std::random_device{}()),
      distribution_{configuration_.packet_size.low(),
                    configuration_.packet_size.high()}
{
}

void
UdpSession::async_receive(std::size_t slice_remaining_size)
{
    auto self(shared_from_this());
    // If we've sent all data allowed within the current slice.
    if (slice_remaining_size == 0)
        // The throttle will call this method again
        // when the next slice will start with a slice_remaining_size
        // set as required by bandwidth.
        receive_throttle_.delay([this, self](std::size_t s){ async_receive(s); });
    else
    {
        auto handler = [this, self, slice_remaining_size]
                (boost::system::error_code const& failure,
                 std::size_t size) {
            on_receive(failure, size, slice_remaining_size);
        };

        auto custom_handler = make_handler(receive_handler_memory_,
                                           std::move(handler));

        auto buffer = boost::asio::buffer(receive_buffer_,
                                          slice_remaining_size);

        socket_.async_receive(std::move(buffer), std::move(custom_handler));
    }
}

void
UdpSession::finish_receive()
{
    Session::finish_receive();
    on_receive_complete();
}

void
UdpSession::async_send(std::size_t slice_remaining_size)
{
    auto self(shared_from_this());
    if (slice_remaining_size == 0)
        send_throttle_.delay([this, self](std::size_t s){ async_send(s); });
    else
    {
        std::size_t const remaining_size = configuration_.size -
                                           statistics_.sent_bytes_count;

        slice_remaining_size = std::min(slice_remaining_size, remaining_size);
        std::size_t const offset = std::uint8_t(statistics_.sent_bytes_count);
        std::size_t const datagram_size = std::min(slice_remaining_size,
                                                   get_max_datagram_size());
        assert(datagram_size <= BUFFER_SIZE - offset);

        auto handler = [this, self, slice_remaining_size]
                (boost::system::error_code const& failure,
                 std::size_t size) {
            on_send(failure, size, slice_remaining_size);
        };

        auto custom_handler =  make_handler(send_handler_memory_,
                                            std::move(handler));

        auto buffer = boost::asio::buffer(&send_buffer_[offset],
                                          datagram_size);

        socket_.async_send(std::move(buffer), std::move(custom_handler));
    }
}

void
UdpSession::finish_send()
{
    Session::finish_send();
    on_send_complete();
}

void
UdpSession::finish()
{
    socket_.close();
}

std::size_t
UdpSession::get_max_datagram_size()
{
    if (distribution_.a() == distribution_.b())
        return distribution_.a();

    return distribution_(random_generator_);
}

} // namespace net_tester
} // namespace enyx
