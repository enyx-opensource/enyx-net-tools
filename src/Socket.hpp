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
#include <boost/system/error_code.hpp>
#include <boost/regex.hpp>

#include "Configuration.hpp"

namespace enyx {
namespace net_tester {

class Socket
{
public:
    explicit
    Socket(boost::asio::io_service & io_service);

protected:
    template<typename Protocol>
    std::pair<typename Protocol::endpoint, typename Protocol::endpoint>
    resolve(const std::string & endpoint);

    template<typename SocketType>
    static void
    setup_windows(const Configuration & configuration,
                  SocketType & socket);

protected:
    boost::asio::io_service & io_service_;
};

template<typename Protocol>
std::pair<typename Protocol::endpoint, typename Protocol::endpoint>
Socket::resolve(const std::string & s)
{
    boost::smatch m;
    {
        boost::regex r("(?:(?:([^:]+):)?([^:]+):)?([^:]+):([^:]+)");
        if (! boost::regex_match(s, m, r))
        {
            std::ostringstream error;
            error << "invalid endpoint'" << s << "'";
            throw std::runtime_error(error.str());
        }
    }

    typename Protocol::resolver::query local{"0"};
    if (m[1].matched)
        local = typename Protocol::resolver::query{m.str(1), m.str(2)};
    else if (m[2].matched)
        local = typename Protocol::resolver::query{m.str(2)};

    typename Protocol::resolver::query remote{m.str(3), m.str(4)};

    typename Protocol::resolver r{io_service_};
    return std::make_pair(*r.resolve(local), *r.resolve(remote));
}

template<typename SocketType>
void
Socket::setup_windows(const Configuration & configuration,
                      SocketType & socket)
{
    if (configuration.windows)
    {
        typename SocketType::receive_buffer_size r(configuration.windows);
        socket.set_option(r);

        typename SocketType::send_buffer_size s(configuration.windows);
        socket.set_option(s);
    }
}

} // namespace net_tester
} // namespace enyx
