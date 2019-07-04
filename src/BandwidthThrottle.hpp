#pragma once

#include <chrono>

#include <boost/bind.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/steady_timer.hpp>

#include "HandlerAllocator.hpp"

namespace enyx {
namespace net_tester {

class BandwidthThrottle
{
public:
    BandwidthThrottle(boost::asio::io_service & io_service,
                      std::size_t bandwidth,
                      std::size_t sampling_frequency);

    template<typename Functor>
    void
    delay(Functor && f)
    {
        timer_.expires_at(next_slice_start_);

        auto handler = [this, f](boost::system::error_code const& failure) {
            if (failure)
                return;

            next_slice_start_ += slice_duration_;
            f(slice_bytes_count_);
        };

        timer_.async_wait(make_handler(handler_memory_,
                                       std::move(handler)));
    }

private:
    static std::chrono::steady_clock::duration
    to_slice_duration(std::size_t sampling_frequency);


    static std::size_t
    to_slice_bytes_count(std::size_t bandwidth,
                         std::size_t sampling_frequency);

private:
    boost::asio::steady_timer timer_;
    HandlerMemory handler_memory_;
    std::size_t slice_bytes_count_;
    std::chrono::steady_clock::duration slice_duration_;
    std::chrono::steady_clock::time_point next_slice_start_;
};

} // namespace net_tester
} // namespace enyx
