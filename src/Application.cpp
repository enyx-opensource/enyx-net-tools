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

#include "Application.hpp"

#include <thread>
#include <memory>
#include <iostream>

#include <boost/asio/io_service.hpp>

#include "TcpSession.hpp"
#include "UdpSession.hpp"

namespace enyx {
namespace net_tester {

namespace {

using SessionPtr = std::unique_ptr<Session>;

SessionPtr
create_session(boost::asio::io_service & io_service,
               const SessionConfiguration & configuration)
{
    SessionPtr session;
    if (configuration.protocol == SessionConfiguration::TCP)
        session.reset(new TcpSession{io_service, configuration});
    else
        session.reset(new UdpSession{io_service, configuration});

    return session;
}

}

namespace Application {

void
run(const ApplicationConfiguration & configuration)
{
    std::cout << "Starting.." << std::endl;

    boost::asio::io_service io_service(configuration.threads_count);

    std::vector<SessionPtr> sessions;
    for (auto const& c: configuration.session_configurations)
        sessions.push_back(create_session(io_service, c));

    std::cout << "Started." << std::endl;

    std::vector<std::thread> threads;
    for (auto i = 0ULL; i != configuration.threads_count; ++i)
        threads.emplace_back([&io_service] { io_service.run(); });

    for (auto & thread : threads)
        thread.join();

    boost::system::error_code first_failure;
    for (auto & session : sessions) {
        boost::system::error_code failure = session->finalize();
        if (failure && ! first_failure)
            first_failure = failure;
    }

    if (first_failure)
        throw boost::system::system_error(first_failure);

}

} // namespace Application

} // namespace net_tester
} // namespace enyx
