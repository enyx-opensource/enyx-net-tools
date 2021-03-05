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
#include <vector>
#include <memory>
#include <iostream>

#include <boost/asio/io_service.hpp>

#include "TcpSession.hpp"
#include "UdpSession.hpp"
#include "Signal.hpp"

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

class Thread final
{
public:
    Thread(boost::asio::io_service & io_service)
        : io_service_(io_service)
        , thread_([this] { run(); })
    { }

    Thread(boost::asio::io_service & io_service,
           CpuCoreId core_id)
        : io_service_(io_service)
        , thread_([this, core_id] { run_pinned(core_id); })
    { }

    Thread(const Thread &) = delete;

    Thread(Thread &&) = default;

    void
    join()
    { thread_.join(); }

private:
    void
    run()
    {
        // Loop until there is pending work and exit is not requested
        while (! is_exit_requested() && ! io_service_.stopped())
            io_service_.poll_one();
    }

    void
    run_pinned(CpuCoreId core_id)
    {
        pin_current_thread_to_cpu_core(core_id);
        run();
    }

private:
    boost::asio::io_service & io_service_;
    std::thread thread_;
};

using Threads = std::vector<Thread>;

using IoService = std::unique_ptr<boost::asio::io_service>;

using IoServices = std::vector<IoService>;

std::size_t
get_concurrency(CpuCoreIds const& core_ids)
{
    return core_ids.empty() ? 1 : core_ids.size();
}

} // anonymous namespace

namespace Application {

void
run(const ApplicationConfiguration & configuration)
{
    install_signal_handlers();

    std::cout << "Starting.." << std::endl;

    auto const core_ids = to_cpu_core_list(configuration.cpus);

    IoServices io_services;
    for (std::size_t i = 0U, e = get_concurrency(core_ids); i != e; ++i)
    {
        // Create each io_service with a concurrency hint of 1
        // indicating that it won't be accessed by more than 1 thread
        IoService new_io_service{new boost::asio::io_service{1}};
        io_services.push_back(std::move(new_io_service));
    }

    // Create all the sessions
    std::vector<SessionPtr> sessions;
    std::size_t i = 0;
    for (auto const& conf: configuration.session_configurations)
    {
        // Partition the sessions on the io_services using round robin
        auto & io_service = *io_services[i ++ % io_services.size()];

        sessions.push_back(create_session(io_service, conf));
    }

    // Create all the thread running the io_service reactor
    Threads threads;
    for (std::size_t i = 0U, e = io_services.size(); i != e; ++i)
    {
        if (i >= core_ids.size())
            threads.emplace_back(*io_services[i]);
        else
            threads.emplace_back(*io_services[i], core_ids[i]);
    }

    std::cout << "Started." << std::endl;

    for (auto & thread : threads)
        thread.join();

    for (auto & session : sessions)
        session->finalize();
}

} // namespace Application

} // namespace net_tester
} // namespace enyx
