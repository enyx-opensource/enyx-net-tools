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

#include <cstdint>
#include <string>
#include <iosfwd>
#include <vector>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "Size.hpp"
#include "Range.hpp"

namespace enyx {
namespace net_tester {

struct SessionConfiguration
{
    enum Mode { CLIENT, SERVER };
    enum Verify { NONE, FIRST, ALL };
    enum Direction { RX, TX, BOTH };
    enum ShutdownPolicy { WAIT_FOR_PEER, SEND_COMPLETE, RECEIVE_COMPLETE };
    enum Protocol { UDP, TCP };

    Mode mode;
    Verify verify;
    Direction direction;
    std::string endpoint;
    Size send_bandwidth;
    Size receive_bandwidth;
    std::uint64_t bandwidth_sampling_frequency;
    Size windows;
    Size size;
    Range<Size> packet_size;
    boost::posix_time::time_duration duration_margin;
    ShutdownPolicy shutdown_policy;
    Protocol protocol;
    std::size_t threads_count;
};

std::istream &
operator>>(std::istream & in, SessionConfiguration::Verify & verify);

std::ostream &
operator<<(std::ostream & out, const SessionConfiguration::Verify & verify);

std::ostream &
operator<<(std::ostream & out, const SessionConfiguration::Mode & mode);

std::istream &
operator>>(std::istream & in, SessionConfiguration::Direction & direction);

std::ostream &
operator<<(std::ostream & out, const SessionConfiguration::Direction & direction);

std::ostream &
operator<<(std::ostream & out, const SessionConfiguration & configuration);

std::istream &
operator>>(std::istream & in, SessionConfiguration::ShutdownPolicy & policy);

std::ostream &
operator<<(std::ostream & out, const SessionConfiguration::ShutdownPolicy & policy);

std::istream &
operator>>(std::istream & in, SessionConfiguration::Protocol & protocol);

std::ostream &
operator<<(std::ostream & out, const SessionConfiguration::Protocol & protocol);

using SessionConfigurations = std::vector<SessionConfiguration>;

} // namespace net_tester
} // namespace enyx
