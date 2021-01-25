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

#include "Session.hpp"

#include <iostream>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/error.hpp>

#include "Error.hpp"

namespace enyx {
namespace net_tester {

namespace ao = boost::asio;
namespace pt = boost::posix_time;

Session::Session(boost::asio::io_service & io_service,
                 const SessionConfiguration & configuration)
    : io_service_(io_service),
      configuration_(configuration),
#ifdef SIGHUP
      signals_(io_service, SIGHUP, SIGINT, SIGTERM),
#else
      signals_(io_service, SIGINT, SIGTERM),
#endif
      timeout_timer_(io_service),
      statistics_(),
      failure_(),
      send_buffer_(BUFFER_SIZE),
      send_throttle_(io_service,
                     configuration.send_bandwidth,
                     configuration.bandwidth_sampling_frequency),
      receive_buffer_(BUFFER_SIZE),
      receive_throttle_(io_service,
                        configuration.receive_bandwidth,
                        configuration.bandwidth_sampling_frequency),
      is_receive_complete_(),
      is_send_complete_()
{
    auto handler = [this] (const boost::system::error_code& error,
                        int signal_number) {
        if (error)
            // Signal unregistered
            return;

        switch (signal_number) {
        case SIGINT:
            abort(error::user_interrupt);
            break;
        case SIGTERM:
            abort(error::program_termination);
            break;
        default:
            abort(error::unknown_signal);
            break;
        };
    };

    signals_.async_wait(handler);

    for (std::size_t i = 0, e = send_buffer_.size(); i != e; ++i)
        send_buffer_[i] = uint8_t(i);
}

void
Session::on_init()
{
    statistics_.start_date = pt::microsec_clock::universal_time();

    if (configuration_.direction == SessionConfiguration::TX)
        finish_receive();
    else
    {
        receive_throttle_.reset();
        async_receive();
    }

    if (configuration_.direction == SessionConfiguration::RX)
        finish_send();
    else
    {
        send_throttle_.reset();
        async_send();
    }

    timeout_timer_.expires_from_now(estimate_test_duration(configuration_));
    timeout_timer_.async_wait([this](boost::system::error_code const& failure) {
        if (failure)
            return;

        abort(error::test_timeout);
    });
}

void
Session::finalize()
{
    std::cout << statistics_ << std::endl;

    if (failure_)
        throw boost::system::system_error(failure_);
}

pt::time_duration
Session::estimate_test_duration(const SessionConfiguration & configuration)
{
    uint64_t bandwidth = std::min(configuration.receive_bandwidth,
                                  configuration.send_bandwidth);

    pt::time_duration duration = pt::seconds(configuration.size / bandwidth + 1);

    auto duration_margin = configuration.duration_margin;
    if (duration_margin.is_special())
        duration_margin = duration / 10;

    return duration + duration_margin;
}

void
Session::on_receive(const boost::system::error_code & failure,
                    std::size_t bytes_transferred,
                    std::size_t slice_remaining_size)
{
    if (failure == ao::error::operation_aborted)
        return;

    if (failure == ao::error::eof)
        abort(error::unexpected_eof);
    else if (failure)
        abort(failure);
    else
    {
        verify(bytes_transferred);
        statistics_.received_bytes_count += bytes_transferred;

        std::size_t size = slice_remaining_size - bytes_transferred;
        if (statistics_.received_bytes_count < configuration_.size)
            async_receive(size);
        else
            finish_receive();
    }
}

void
Session::finish_receive()
{
    statistics_.receive_duration = pt::microsec_clock::universal_time() -
                                   statistics_.start_date;
}

void
Session::on_receive_complete()
{
    is_receive_complete_ = true;

    if (is_send_complete_)
        on_finish();
}

void
Session::verify(std::size_t bytes_transferred)
{
    uint8_t expected_byte = uint8_t(statistics_.received_bytes_count);

    switch (configuration_.verify)
    {
    default:
    case SessionConfiguration::NONE:
        break;
    case SessionConfiguration::FIRST:
        verify(0, expected_byte);
        break;
    case SessionConfiguration::ALL:
        for (std::size_t i = 0; i != bytes_transferred; ++i)
            verify(i, expected_byte++);
        break;
    }
}

void
Session::verify(std::size_t offset, uint8_t expected_byte)
{
    uint8_t actual = receive_buffer_[offset];
    if (actual != expected_byte)
    {
        std::cerr << "Checksum error on byte "
                  << statistics_.received_bytes_count + offset
                  << ": expected " << int(expected_byte)
                  << " got " << int(actual) << "." << std::endl;

        abort(error::checksum_failed);
    }
}

void
Session::on_send(const boost::system::error_code & failure,
                 std::size_t bytes_transferred,
                 std::size_t slice_remaining_size)
{
    if (failure == ao::error::operation_aborted)
        return;

    if (failure)
        abort(failure);
    else
    {
        statistics_.sent_bytes_count += bytes_transferred;
        std::size_t size = slice_remaining_size - bytes_transferred;
        if (statistics_.sent_bytes_count < configuration_.size)
            async_send(size);
        else
            finish_send();
    }
}

void
Session::finish_send()
{
    statistics_.send_duration = pt::microsec_clock::universal_time() -
                                statistics_.start_date;
}

void
Session::on_send_complete()
{
    is_send_complete_ = true;

    if (is_receive_complete_)
        on_finish();
}

void
Session::abort(const boost::system::error_code & failure)
{
    // Stop the whole application
    io_service_.stop();
    failure_ = failure;
}

void
Session::on_finish()
{
    signals_.cancel();
    timeout_timer_.cancel();
    finish();
}

} // namespace net_tester
} // namespace enyx
