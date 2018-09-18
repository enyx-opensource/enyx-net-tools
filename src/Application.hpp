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
#include <cstddef>
#include <vector>

#include <boost/system/error_code.hpp>

#include "Configuration.hpp"
#include "BandwidthThrottle.hpp"
#include "Statistics.hpp"

namespace enyx {
namespace tcp_tester {

struct Application
{
public:
    explicit
    Application(const Configuration & configuration);

    void
    run();

private:
    using buffer_type = std::vector<std::uint8_t>;

    static constexpr std::size_t BUFFER_SIZE = 1024 * 1024;

protected:
    boost::posix_time::time_duration
    estimate_test_duration();

    void
    on_timeout(const boost::system::error_code & failure);

    virtual void
    async_receive(std::size_t slice_remaining_size = 0ULL) = 0;

    void
    on_receive(std::size_t bytes_transferred,
               const boost::system::error_code & failure,
               std::size_t slice_remaining_size);

    virtual void
    finish_receive();

    void
    on_receive_complete();

    virtual void
    async_send(std::size_t slice_remaining_size = 0ULL) = 0;

    void
    on_send(std::size_t bytes_transferred,
            const boost::system::error_code & failure,
            std::size_t slice_remaining_size);

    virtual void
    finish_send();

    void
    on_send_complete();

    void
    verify(std::size_t bytes_transferred);

    void
    verify(std::size_t offset, uint8_t expected_byte);

    void
    abort(const boost::system::error_code & failure);

    void
    on_finish();

    virtual void
    finish() = 0;

protected:
    Configuration configuration_;
    boost::asio::io_service io_service_;
    boost::asio::io_service::work work_;
    Statistics statistics_;
    boost::system::error_code failure_;
    buffer_type send_buffer_;
    BandwidthThrottle send_throttle_;
    buffer_type receive_buffer_;
    BandwidthThrottle receive_throttle_;
};

} // namespace tcp_tester
} // namespace enyx

