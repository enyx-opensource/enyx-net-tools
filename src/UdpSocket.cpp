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

#include "UdpSocket.hpp"

#include <iostream>

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace enyx {
namespace net_tester {

namespace ao = boost::asio;
namespace pt = boost::posix_time;

UdpSocket::UdpSocket(boost::asio::io_service & io_service,
                     const Configuration & configuration)
    : Socket(io_service),
      socket_(io_service_),
      discarded_endpoint_(),
      peer_endpoint_()
{
    switch (configuration.mode)
    {
        default:
        case Configuration::SERVER:
            throw std::runtime_error{"Udp supports client mode only"};
        case Configuration::CLIENT:
            connect(configuration);
            break;
    }
}

void
UdpSocket::connect(const Configuration & configuration)
{
    const auto e = resolve<protocol_type>(configuration.endpoint);

    socket_.open(e.second.protocol());

    setup_windows(configuration, socket_);

    ao::socket_base::reuse_address reuse_address(true);
    socket_.set_option(reuse_address);

    socket_.bind(e.first);

    // Set the default destination address of this datagram socket.
    peer_endpoint_ = e.second;

    std::cout << "Local '" << socket_.local_endpoint() << "' to remote '"
              << peer_endpoint_ << "'" << std::endl;
}

void
UdpSocket::close()
{
    boost::system::error_code failure;
    socket_.close(failure);
}

} // namespace net_tester
} // namespace enyx
