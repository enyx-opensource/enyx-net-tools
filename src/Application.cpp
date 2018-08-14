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
#include "Socket.hpp"
#include "Statistics.hpp"

namespace enyx {
namespace tcp_tester {

namespace ao = boost::asio;
namespace pt = boost::posix_time;

Application::Application(const Configuration & configuration)
    : configuration_(configuration),
      io_service_(), work_(io_service_),
      socket_(io_service_, configuration),
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

    async_receive();
    async_send();
}

void
Application::run()
{
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
Application::async_receive(std::size_t slice_remaining_size)
{
    // If we've sent all data allowed within the current slice.
    if (slice_remaining_size == 0)
        // The throttle will call this method again
        // when the next slice will start with a slice_remaining_size
        // set as required by bandwidth.
        receive_throttle_.delay(boost::bind(&Application::async_receive,
                                            this, _1));
    else
        socket_.async_receive(boost::asio::buffer(receive_buffer_,
                                                  slice_remaining_size),
                              boost::bind(&Application::on_receive,
                                          this,
                                          ao::placeholders::bytes_transferred,
                                          ao::placeholders::error,
                                          slice_remaining_size));
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
            async_receive_eof();
    }
}

void
Application::async_receive_eof()
{
    socket_.async_receive(boost::asio::buffer(receive_buffer_, 1),
                          boost::bind(&Application::on_eof,
                                      this,
                                      ao::placeholders::bytes_transferred,
                                      ao::placeholders::error));

}

void
Application::on_eof(std::size_t bytes_transferred,
                    const boost::system::error_code & failure)
{
    if (failure == ao::error::eof)
        on_receive_complete();
    else if (failure)
        abort(failure);
    else
        abort(error::unexpected_data);
}

void
Application::on_receive_complete()
{
    std::cout << "Finished receiving" << std::endl;

    if (statistics_.sent_bytes_count == configuration_.size)
        finish();
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
Application::async_send(std::size_t slice_remaining_size)
{
    if (slice_remaining_size == 0)
        send_throttle_.delay(boost::bind(&Application::async_send,
                                         this, _1));
    else
    {
        std::size_t remaining_size = configuration_.size -
                                     statistics_.sent_bytes_count;

        slice_remaining_size = std::min(slice_remaining_size, remaining_size);
        std::size_t offset = statistics_.sent_bytes_count % send_buffer_.size();
        std::size_t size = std::min(slice_remaining_size,
                                    send_buffer_.size() - offset);

        socket_.async_send(boost::asio::buffer(&send_buffer_[offset], size),
                           boost::bind(&Application::on_send,
                                       this,
                                       ao::placeholders::bytes_transferred,
                                       ao::placeholders::error,
                                       slice_remaining_size));
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

            socket_.shutdown_send();
            on_send_complete();
        }
    }
}

void
Application::on_send_complete()
{
    std::cout << "Finished sending" << std::endl;

    if (statistics_.received_bytes_count == configuration_.size)
        finish();
}

void
Application::abort(const boost::system::error_code & failure)
{
    failure_ = failure;
    socket_.close();
    io_service_.stop();
}

void
Application::finish()
{
    socket_.close();
    io_service_.stop();
}

} // namespace tcp_tester
} // namespace enyx
