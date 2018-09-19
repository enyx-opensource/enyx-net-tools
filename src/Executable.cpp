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
#include <sstream>

#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Configuration.hpp"
#include "TcpApplication.hpp"
#include "UdpApplication.hpp"

namespace enyx {
namespace tcp_tester {

namespace po = boost::program_options;
namespace pt = boost::posix_time;

namespace {

constexpr Size DEFAULT_BANDWIDTH = Size(128 * 1000 * 1000, Size::SI);

Configuration
parse_command_line(int argc, char** argv)
{
    Configuration c = {};

    po::options_description required("Required arguments");
    required.add_options()
        ("connect,c",
            po::value<std::string>(&c.endpoint),
            "Connect to following address\n")
        ("listen,l",
            po::value<std::string>(&c.endpoint),
            "Listen on following address\n")
        ("size,s",
            po::value<Size>(&c.size),
            "Amount of data to send (e.g. 8KiB, 16MiB, 1Gibit, 1GiB)\n");

    po::options_description optional("Optional arguments");
    optional.add_options()
        ("protocol,p",
            po::value<Configuration::Protocol>(&c.protocol)
                ->default_value(Configuration::TCP),
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
                ->default_value(10),
            "Bandwidth calculation frequency Hz\n")
        ("verify,v",
            po::value<Configuration::Verify>(&c.verify)
                ->default_value(Configuration::NONE),
            "Verify received bytes. Accepted values:\n"
            "  - none\n  - first\n  - all\n")
        ("mode,m",
            po::value<Configuration::Direction>(&c.direction)
                ->default_value(Configuration::BOTH),
            "Transfer mode. Accepted values:\n"
            "  - rx\n  - tx\n  - both\n")
        ("windows,w",
            po::value<Size>(&c.windows),
            "Tcp socket buffer size (e.g. 8KiB, 16MiB)\n")
        ("duration-margin,d",
            po::value<pt::time_duration>(&c.duration_margin)
                ->default_value(pt::not_a_date_time, "infinity"),
            "Extra time from theoretical test time allowed to"
            " complete without timeout error\n")
        ("help,h",
            "Print the command lines arguments\n");

    po::options_description tcp_optional("Tcp related optional arguments");
    tcp_optional.add_options()
        ("shutdown-policy,S",
            po::value<Configuration::ShutdownPolicy>(&c.shutdown_policy)
                ->default_value(Configuration::SEND_COMPLETE),
            "Connection shutdown policy. Accepted values:\n"
            "  - send_complete\n  - receive_complete\n  - wait_for_peer\n");

    po::options_description all("Allowed options");
    all.add(required).add(optional).add(tcp_optional);

    po::variables_map args;
    po::store(po::parse_command_line(argc, argv, all), args);
    po::notify(args);

    if (args.count("help"))
    {
        std::cout << all << std::endl;
        throw std::runtime_error("help is requested");
    }

    if (args.count("connect") && args.count("listen"))
        throw std::runtime_error("--connect and --listen are mutually exclusive");

    if (args["bandwidth-sampling-frequency"].as<uint64_t>() == 0)
        throw std::runtime_error("invalid --bandwidth-sampling-frequency");

    if (! args.count("size") || args["size"].as<Size>() == 0)
        throw std::runtime_error("--size is required");

    if (c.direction == Configuration::TX &&
            c.shutdown_policy == Configuration::RECEIVE_COMPLETE)
        throw std::runtime_error("TX mode isn't compatible with shutdown "
                "policy receive_complete");

    if (c.direction == Configuration::RX &&
            c.shutdown_policy == Configuration::SEND_COMPLETE)
        throw std::runtime_error("RX mode isn't compatible with shutdown "
                "policy send_complete");

    if (args.count("connect"))
        c.mode = Configuration::CLIENT;
    else if (args.count("listen"))
        c.mode = Configuration::SERVER;
    else
        throw std::runtime_error("--connect or --listen are required");

    return c;
}

} // namespace

namespace Executable {

void
run(int argc, char** argv)
{
    std::cout << "Starting.." << std::endl;

    auto const configuration = parse_command_line(argc, argv);
    if (configuration.protocol == Configuration::TCP)
        TcpApplication(configuration).run();
    else
        UdpApplication(configuration).run();
}

}

} // namespace tcp_tester
} // namespace enyx
