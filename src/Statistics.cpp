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

#include "Statistics.hpp"

#include <string>
#include <iostream>
#include <sstream>

#include <boost/date_time/posix_time/posix_time.hpp>

namespace enyx {
namespace net_tester {

namespace pt = boost::posix_time;

namespace {

std::string
compute_bandwidth(Size bytes_count,
                 const pt::time_duration & duration)
{
    std::ostringstream out;

    if (duration.is_special() || duration.total_milliseconds() == 0)
        return "undefined";

    bytes_count /= duration.total_milliseconds();
    bytes_count *= 1000;

    out << bytes_count << "/s";

    return out.str();
}

} // anonymous namespace

std::ostream &
operator<<(std::ostream & out, const Statistics & statistics)
{
    return out << "started: " << statistics.start_date << "\n"
               << "received_bytes_count: "
               << statistics.received_bytes_count << "\n"
               << "receive_bandwidth: "
               << compute_bandwidth(statistics.received_bytes_count,
                                   statistics.receive_duration) << "\n"
               << "sent_bytes_count: "
               << statistics.sent_bytes_count << "\n"
               << "send_bandwidth: "
               << compute_bandwidth(statistics.sent_bytes_count,
                                   statistics.send_duration)
               << std::endl;
}

} // namespace net_tester
} // namespace enyx
