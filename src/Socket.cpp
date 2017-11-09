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

#include "Socket.hpp"

#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

namespace enyx {
namespace tcp_tester {

namespace ao = boost::asio;
namespace pt = boost::posix_time;

Socket::Socket(boost::asio::io_service & io_service,
               const Configuration & configuration)
    : io_service_(io_service), socket_(io_service_)
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

ao::ip::tcp::endpoint
Socket::resolve(const std::string & s)
{
    boost::smatch m;
    {
        boost::regex r("([^:]+):([^:]+)");
        if (! boost::regex_match(s, m, r))
        {
            std::ostringstream error;
            error << "invalid endpoint'" << s << "'";
            throw std::runtime_error(error.str());
        }
    }

    ao::ip::tcp::resolver::query q(m.str(1), m.str(2));

    ao::ip::tcp::resolver r(io_service_);
    return *r.resolve(q);
}

void
Socket::connect(const Configuration & configuration)
{
    const ao::ip::tcp::endpoint e(resolve(configuration.endpoint));

    socket_.open(e.protocol());
    setup_windows(configuration, socket_);

    socket_.connect(e);

    std::cout << "Connected to '" << e << "'" << std::endl;
}

void
Socket::listen(const Configuration & configuration,
               const boost::posix_time::time_duration & timeout)
{
    const ao::ip::tcp::endpoint e(resolve(configuration.endpoint));

    // Schedule an asynchronous accept.
    ao::ip::tcp::acceptor a(io_service_, e.protocol());
    ao::socket_base::reuse_address reuse_address(true);
    a.set_option(reuse_address);
    setup_windows(configuration, a);
    a.bind(e);
    a.listen();

    std::cout << "Waiting " << timeout
              << " for client to connect" << std::endl;

    boost::system::error_code failure;
    on_accept on_accept = { failure };
    a.async_accept(socket_, on_accept);

    // And an asynchronous wait.
    ao::deadline_timer t(io_service_, timeout);
    on_timeout on_timeout = { failure };
    t.async_wait(on_timeout);

    // Wait for any to complete.
    // If the timer completed, the failure variable
    // will be set to timed_out.
    io_service_.run_one();

    if (failure)
        throw boost::system::system_error(failure);
}

template<typename SocketType>
void
Socket::setup_windows(const Configuration & configuration,
                      SocketType & socket)
{
    if (configuration.windows)
    {
        ao::socket_base::receive_buffer_size r(configuration.windows);
        socket.set_option(r);

        ao::socket_base::send_buffer_size s(configuration.windows);
        socket.set_option(s);
    }
}


void
Socket::shutdown_send()
{
    boost::system::error_code failure;
    socket_.shutdown(ao::ip::tcp::socket::shutdown_send, failure);
}

void
Socket::close()
{
    boost::system::error_code failure;
    socket_.close(failure);
}

} // namespace tcp_tester
} // namespace enyx
