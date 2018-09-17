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

#include "TcpSocket.hpp"

#include <iostream>

#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace enyx {
namespace tcp_tester {

namespace ao = boost::asio;
namespace pt = boost::posix_time;

TcpSocket::TcpSocket(boost::asio::io_service & io_service,
               const Configuration & configuration)
    : Socket(io_service),
      socket_(io_service_)
{
    switch (configuration.mode)
    {
        default:
        case Configuration::CLIENT:
            connect(configuration);
            break;
        case Configuration::SERVER:
            listen(configuration, pt::pos_infin);
            break;
    }
}

void
TcpSocket::connect(const Configuration & configuration)
{
    const auto e(resolve<protocol_type>(configuration.endpoint));

    socket_.open(e.protocol());
    setup_windows(configuration, socket_);

    socket_.connect(e);

    std::cout << "Connected to '" << e << "' from '"
              << socket_.local_endpoint() << "'" << std::endl;
}

void
TcpSocket::listen(const Configuration & configuration,
               const boost::posix_time::time_duration & timeout)
{
    const ao::ip::tcp::endpoint e(resolve<protocol_type>(configuration.endpoint));

    // Schedule an asynchronous accept.
    ao::ip::tcp::acceptor a(socket_.get_io_service(), e.protocol());
    ao::socket_base::reuse_address reuse_address(true);
    a.set_option(reuse_address);
    setup_windows(configuration, a);
    a.bind(e);
    a.listen();

    std::cout << "Waiting " << timeout
              << " for client to connect" << std::endl;

    boost::system::error_code failure;
    // Asynchronously Wait for a client to connect.
    a.async_accept(socket_, [&failure](const boost::system::error_code & f) {
        if (f != boost::asio::error::operation_aborted)
            failure = f;
    });

    // Asynchronously wait for a timer to expire.
    ao::deadline_timer t(socket_.get_io_service(), timeout);
    t.async_wait([&failure](const boost::system::error_code & f) {
        failure = boost::asio::error::timed_out;
    });

    // Wait for any to complete.
    // If the timer completed, the failure variable
    // will be set to timed_out.
    socket_.get_io_service().run_one();

    if (failure)
        throw boost::system::system_error(failure);

    std::cout << "Connected to '" << socket_.remote_endpoint() << "' from '"
              << socket_.local_endpoint() << "'" << std::endl;
}

void
TcpSocket::shutdown_send()
{
    boost::system::error_code failure;
    socket_.shutdown(socket_type::shutdown_send, failure);
}

void
TcpSocket::close()
{
    boost::system::error_code failure;
    socket_.close(failure);
}

} // namespace tcp_tester
} // namespace enyx
