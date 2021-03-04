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

#include "Executable.hpp"

#include <stdexcept>
#include <iostream>
#include <iterator>
#include <sstream>
#include <fstream>
#include <thread>

#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "SessionConfiguration.hpp"
#include "ApplicationConfiguration.hpp"
#include "Application.hpp"

namespace enyx {
namespace net_tester {

namespace po = boost::program_options;
namespace pt = boost::posix_time;

namespace {

constexpr Size DEFAULT_BANDWIDTH = Size(128 * 1000 * 1000, Size::SI);

void
fill_configuration(SessionConfiguration & c,
                   const po::options_description & options,
                   std::vector<std::string> argv)
{
    auto file_parser = po::command_line_parser(argv)
            .options(options)
            .run();
    po::variables_map args;
    po::store(file_parser, args);
    po::notify(args);

    if (args.count("connect") && args.count("listen"))
        throw std::runtime_error{"--connect and --listen are mutually exclusive"};

    if (args["bandwidth-sampling-frequency"].as<std::uint64_t>() == 0)
        throw std::runtime_error{"invalid --bandwidth-sampling-frequency"};

    if (! args.count("size") || args["size"].as<Size>() == 0)
        throw std::runtime_error{"--size is required"};

    if (c.direction == SessionConfiguration::TX &&
            c.shutdown_policy == SessionConfiguration::RECEIVE_COMPLETE)
        throw std::runtime_error{"TX mode isn't compatible with shutdown "
                "policy receive_complete"};

    if (c.direction == SessionConfiguration::RX &&
            c.shutdown_policy == SessionConfiguration::SEND_COMPLETE)
        throw std::runtime_error{"RX mode isn't compatible with shutdown "
                "policy send_complete"};

    if (args.count("connect"))
        c.mode = SessionConfiguration::CLIENT;
    else if (args.count("listen"))
        c.mode = SessionConfiguration::SERVER;
    else
        throw std::runtime_error{"--connect or --listen are required"};
}

void
fill_configuration(SessionConfiguration & c,
                   const po::options_description & options,
                   std::string const& line)
{
    std::istringstream s{line};

    using tokenizer = std::istream_iterator<std::string>;
    std::vector<std::string> argv{"enyx-net-bench"};
    argv.insert(argv.end(), tokenizer{s}, tokenizer{});

    fill_configuration(c, options, argv);
}

SessionConfigurations
fill_configuration(SessionConfiguration & c,
                   po::options_description const& file_all,
                   std::istream & file)
{
    SessionConfigurations configurations;

    for (std::string line; std::getline(file, line); )
    {
        fill_configuration(c, file_all, line);
        configurations.emplace_back(std::move(c));
    }

    return configurations;
}


ApplicationConfiguration
parse(int argc, char ** argv)
{
    ApplicationConfiguration app_configuration{};
    SessionConfiguration c{};

    po::options_description file_required{"Required arguments"};
    file_required.add_options()
        ("connect,c",
            po::value<std::string>(&c.endpoint),
            "Connect to following address\n")
        ("listen,l",
            po::value<std::string>(&c.endpoint),
            "Listen on following address\n")
        ("size,s",
            po::value<Size>(&c.size),
            "Amount of data to send (e.g. 8KiB, 16MiB, 1Gibit, 1GiB)\n");

    po::options_description file_optional{"Optional arguments"};
    file_optional.add_options()
        ("protocol,p",
            po::value<SessionConfiguration::Protocol>(&c.protocol)
                ->default_value(SessionConfiguration::TCP),
            "Protocol used to transfer data. Accepted values:\n"
            "  - tcp\n  - udp\n")
        ("tx-bandwidth,t",
            po::value<Size>(&c.send_bandwidth)
                ->default_value(DEFAULT_BANDWIDTH),
            "Limit send bandwidth (e.g. 8kB, 16MB, 1Gbit, 1GB)\n")
        ("rx-bandwidth,r",
            po::value<Size>(&c.receive_bandwidth)
                ->default_value(DEFAULT_BANDWIDTH),
            "Limit receive bandwidth (e.g. 8kB, 16MB, 1Gbit, 1GB)\n")
        ("bandwidth-sampling-frequency,f",
            po::value<uint64_t>(&c.bandwidth_sampling_frequency)
                ->default_value(1000),
            "Bandwidth calculation frequency Hz\n")
        ("verify,v",
            po::value<SessionConfiguration::Verify>(&c.verify)
                ->default_value(SessionConfiguration::NONE),
            "Verify received bytes. Accepted values:\n"
            "  - none\n  - first\n  - all\n")
        ("mode,m",
            po::value<SessionConfiguration::Direction>(&c.direction)
                ->default_value(SessionConfiguration::BOTH),
            "Transfer mode. Accepted values:\n"
            "  - rx\n  - tx\n  - both\n")
        ("windows,w",
            po::value<Size>(&c.windows)
                ->default_value(0),
            "Tcp socket buffer size (e.g. 8KiB, 16MiB)\n")
        ("duration-margin,d",
            po::value<pt::time_duration>(&c.duration_margin)
                ->default_value(pt::not_a_date_time, "infinity"),
            "Extra time from theoretical test time allowed to"
            " complete without timeout error\n");

    po::options_description file_udp_optional{"Udp related optional arguments"};
    file_udp_optional.add_options()
        ("max-datagram-size,D",
            po::value<Range<Size>>(&c.packet_size)
                ->default_value(Range<Size>{Size{(1 << 16) - 64}}),
            "UDP and TCP packet maximum size. Accepted values:\n"
            "  - X The maximum size is equal to X\n"
            "  - X-Y The maximum size is randomly chosen for each packet "
            "between X & Y (inclusive)\n");

    po::options_description file_tcp_optional{"Tcp related optional arguments"};
    file_tcp_optional.add_options()
        ("shutdown-policy,S",
            po::value<SessionConfiguration::ShutdownPolicy>(&c.shutdown_policy)
                ->default_value(SessionConfiguration::SEND_COMPLETE),
            "Connection shutdown policy. Accepted values:\n"
            "  - send_complete\n  - receive_complete\n  - wait_for_peer\n");

    po::options_description file_all{"CONFIGURATION FILE OPTIONS"};
    file_all.add(file_required)
            .add(file_optional)
            .add(file_udp_optional)
            .add(file_tcp_optional);

    std::string file;
    po::options_description cmd_optional{"Optional arguments"};
    cmd_optional.add_options()
        ("configuration-file,c",
            po::value<std::string>(&file)
                ->default_value("-"),
            "A file with one session configuration per line\n")
        ("threads-count,x",
            po::value<std::size_t>(&app_configuration.threads_count)
                ->default_value(std::thread::hardware_concurrency()),
            "Threads used to process network events\n")
        ("help,h",
            "Print the command lines arguments\n");

    po::options_description cmd_all{"COMMAND LINE OPTIONS"};
    cmd_all.add(cmd_optional);

    po::variables_map args;
    auto cmd_parser = po::command_line_parser(argc, argv)
            .options(cmd_all)
            .run();
    po::store(cmd_parser, args);
    po::notify(args);

    if (args.count("help"))
    {
        po::options_description all;
        all.add(cmd_all).add(file_all);
        std::cout << all << std::endl;
        throw std::runtime_error{"help is requested"};
    }

    if (! args.count("configuration-file"))
        throw std::runtime_error{"--configuration-file argument is required"};

    if (args["threads-count"].as<std::size_t>() == 0)
        throw std::runtime_error{"--threads-count can't be equal to 0"};

    SessionConfigurations session_configurations;

    if (file == "-")
    {
        session_configurations = fill_configuration(c, file_all, std::cin);
    }
    else
    {
        std::ifstream response_file{file};
        session_configurations = fill_configuration(c, file_all, response_file);
    }

    app_configuration.session_configurations = session_configurations;

    return app_configuration;
}

} // namespace

namespace Executable {

void
run(int argc, char** argv)
{
    Application::run(parse(argc, argv));
}

}

} // namespace net_tester
} // namespace enyx
