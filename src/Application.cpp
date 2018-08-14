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

#include <iostream>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/error.hpp>

#include "Error.hpp"

namespace enyx {
namespace tcp_tester {

namespace ao = boost::asio;
namespace pt = boost::posix_time;

Application::Application(const Configuration & configuration)
    : configuration_(configuration),
      io_service_(), work_(io_service_),
      statistics_(),
      failure_(),
      send_buffer_(BUFFER_SIZE),
      send_throttle_(io_service_,
                     configuration.send_bandwidth,
                     configuration.bandwidth_sampling_frequency),
      receive_buffer_(BUFFER_SIZE),
      receive_throttle_(io_service_,
                        configuration.receive_bandwidth,
                        configuration.bandwidth_sampling_frequency)
{
    for (std::size_t i = 0, e = send_buffer_.size(); i != e; ++i)
        send_buffer_[i] = uint8_t(i);

    std::cout << configuration_ << std::endl;
}

void
Application::run()
{
    async_receive();
    async_send();

    ao::deadline_timer t(io_service_, estimate_test_duration());
    t.async_wait(boost::bind(&Application::on_timeout, this,
                             ao::placeholders::error));

    statistics_.start_date = pt::microsec_clock::universal_time();
    io_service_.run();
    statistics_.total_duration = pt::microsec_clock::universal_time() -
                                 statistics_.start_date;

    std::cout << statistics_ << std::endl;

    if (failure_)
        throw boost::system::system_error(failure_);
}

pt::time_duration
Application::estimate_test_duration()
{
    uint64_t bandwidth = std::min(configuration_.receive_bandwidth,
                                  configuration_.send_bandwidth);

    pt::time_duration duration = pt::seconds(configuration_.size / bandwidth + 1);

    if (configuration_.duration_margin.is_special())
        configuration_.duration_margin = duration / 10;

    std::cout << "Estimated duration " << duration
              << " (+" << configuration_.duration_margin
              << ")." << std::endl;

    return duration + configuration_.duration_margin;
}

void
Application::on_timeout(const boost::system::error_code & failure)
{
    if (failure)
        return;

    abort(error::test_timeout);
}

void
Application::on_receive(std::size_t bytes_transferred,
                        const boost::system::error_code & failure,
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
Application::on_receive_complete()
{
    std::cout << "Finished receiving" << std::endl;

    if (statistics_.sent_bytes_count == configuration_.size)
        on_finish();
}

void
Application::verify(std::size_t bytes_transferred)
{
    uint8_t expected_byte = uint8_t(statistics_.received_bytes_count);

    switch (configuration_.verify)
    {
    default:
    case Configuration::NONE:
        break;
    case Configuration::FIRST:
        verify(0, expected_byte);
        break;
    case Configuration::ALL:
        for (std::size_t i = 0; i != bytes_transferred; ++i)
            verify(i, expected_byte++);
        break;
    }
}

void
Application::verify(std::size_t offset, uint8_t expected_byte)
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
Application::on_send(std::size_t bytes_transferred,
                     const boost::system::error_code & failure,
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
        {
            statistics_.receive_duration = pt::microsec_clock::universal_time() -
                                           statistics_.start_date;

            finish_send();
        }
    }
}

void
Application::on_send_complete()
{
    std::cout << "Finished sending" << std::endl;

    if (statistics_.received_bytes_count == configuration_.size)
        on_finish();
}

void
Application::abort(const boost::system::error_code & failure)
{
    failure_ = failure;
    on_finish();
}

void
Application::on_finish()
{
    finish();
    io_service_.stop();
}

} // namespace tcp_tester
} // namespace enyx
