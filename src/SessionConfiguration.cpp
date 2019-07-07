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

#include "SessionConfiguration.hpp"

#include <stdexcept>
#include <iostream>

#include <boost/regex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace enyx {
namespace net_tester {

std::istream &
operator>>(std::istream & in, SessionConfiguration::Verify & verify)
{
    std::istream::sentry sentry(in);

    if (sentry)
    {
        std::string s;
        in >> s;

        if (s == "none")
            verify = SessionConfiguration::NONE;
        else if (s == "first")
            verify = SessionConfiguration::FIRST;
        else if (s == "all")
            verify = SessionConfiguration::ALL;
        else
            throw std::runtime_error("Unexpected verification mode");
    }

    return in;
}

std::ostream &
operator<<(std::ostream & out, const SessionConfiguration::Verify & verify)
{
    std::ostream::sentry sentry(out);

    if (! sentry)
        return out;

    switch (verify)
    {
    default:
    case SessionConfiguration::NONE:
        return out << "none";
    case SessionConfiguration::FIRST:
        return out << "first";
    case SessionConfiguration::ALL:
        return out << "all";
    }
}

std::ostream &
operator<<(std::ostream & out, const SessionConfiguration::Mode & mode)
{
    std::ostream::sentry sentry(out);

    if (! sentry)
        return out;

    switch (mode)
    {
    default:
    case SessionConfiguration::CLIENT:
        return out << "client";
    case SessionConfiguration::SERVER:
        return out << "server";
    }
}

std::istream &
operator>>(std::istream & in, SessionConfiguration::Direction & direction)
{
    std::istream::sentry sentry(in);

    if (sentry)
    {
        std::string s;
        in >> s;

        if (s == "tx")
            direction = SessionConfiguration::TX;
        else if (s == "rx")
            direction = SessionConfiguration::RX;
        else if (s == "both")
            direction = SessionConfiguration::BOTH;
        else
            throw std::runtime_error("Unexpected mode option value");
    }

    return in;
}

std::ostream &
operator<<(std::ostream & out, const SessionConfiguration::Direction & direction)
{
    std::ostream::sentry sentry(out);

    if (! sentry)
        return out;

    switch (direction)
    {
    default:
    case SessionConfiguration::RX:
        return out << "rx";
    case SessionConfiguration::TX:
        return out << "tx";
    case SessionConfiguration::BOTH:
        return out << "both";
    }
}

std::ostream &
operator<<(std::ostream & out, const SessionConfiguration & configuration)
{
    std::ostream::sentry sentry(out);

    if (sentry)
    {
        out << "mode: " << configuration.mode << "\n";
        out << "direction: " << configuration.direction << "\n";
        out << "endpoint: " << configuration.endpoint << "\n";
        out << "send_bandwidth: "
            << configuration.send_bandwidth << "/s\n";
        out << "receive_bandwidth: "
            << configuration.receive_bandwidth << "/s\n";
        out << "bandwidth_sampling_frequency: "
            << configuration.bandwidth_sampling_frequency << "Hz\n";
        out << "verify: " << configuration.verify << "\n";
        if (configuration.windows != 0)
            out << "windows: " << configuration.windows << "\n";
        else
            out << "windows: default system value\n";
        out << "size: " << configuration.size << "\n";
        if (configuration.duration_margin.is_special())
            out << "duration_margin: default\n";
        else
            out << "duration_margin: "
                << configuration.duration_margin << "\n";
        out << "shutdown_policy: "
            << configuration.shutdown_policy << "\n";
        out << std::flush;
    }

    return out;
}

std::istream &
operator>>(std::istream & in, SessionConfiguration::ShutdownPolicy & policy)
{
    std::istream::sentry sentry(in);

    if (sentry)
    {
        std::string s;
        in >> s;

        if (s == "wait_for_peer")
            policy = SessionConfiguration::WAIT_FOR_PEER;
        else if (s == "send_complete")
            policy = SessionConfiguration::SEND_COMPLETE;
        else if (s == "receive_complete")
            policy = SessionConfiguration::RECEIVE_COMPLETE;
        else
            throw std::runtime_error("Unexpected shutdown policy");
    }

    return in;
}

std::ostream &
operator<<(std::ostream & out, const SessionConfiguration::ShutdownPolicy & policy)
{
    std::ostream::sentry sentry(out);

    if (! sentry)
        return out;

    switch (policy)
    {
    default:
    case SessionConfiguration::WAIT_FOR_PEER:
        return out << "wait_for_peer";
    case SessionConfiguration::SEND_COMPLETE:
        return out << "send_complete";
    case SessionConfiguration::RECEIVE_COMPLETE:
        return out << "receive_complete";
    }
}

std::istream &
operator>>(std::istream & in, SessionConfiguration::Protocol & protocol)
{
    std::istream::sentry sentry(in);

    if (sentry)
    {
        std::string s;
        in >> s;

        if (s == "tcp")
            protocol = SessionConfiguration::TCP;
        else if (s == "udp")
            protocol = SessionConfiguration::UDP;
        else
            throw std::runtime_error("Unexpected protocol");
    }

    return in;
}

std::ostream &
operator<<(std::ostream & out, const SessionConfiguration::Protocol & protocol)
{
    std::ostream::sentry sentry(out);

    if (! sentry)
        return out;

    switch (protocol)
    {
    default:
    case SessionConfiguration::TCP:
        return out << "tcp";
    case SessionConfiguration::UDP:
        return out << "udp";
    }
}

} // namespace net_tester
} // namespace enyx

