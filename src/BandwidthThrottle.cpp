#include "BandwidthThrottle.hpp"

#include <cassert>

namespace enyx {
namespace net_tester {

BandwidthThrottle::BandwidthThrottle(boost::asio::io_service & io_service,
                                     std::size_t bandwidth,
                                     std::size_t sampling_frequency)
        : timer_(io_service),
          handler_memory_(),
          slice_bytes_count_(to_slice_bytes_count(bandwidth,
                                                  sampling_frequency)),
          slice_duration_(to_slice_duration(sampling_frequency)),
          next_slice_start_(std::chrono::steady_clock::now())
{ }

void
BandwidthThrottle::reset()
{
    next_slice_start_ = std::chrono::steady_clock::now();
}

std::chrono::steady_clock::duration
BandwidthThrottle::to_slice_duration(std::size_t sampling_frequency)
{
    assert(sampling_frequency && "is not 0");
    return std::chrono::nanoseconds(1000 * 1000 * 1000) / sampling_frequency;
}

std::size_t
BandwidthThrottle::to_slice_bytes_count(std::size_t bandwidth,
                                        std::size_t sampling_frequency)
{
    assert(sampling_frequency && "is not 0");
    return bandwidth / sampling_frequency;
}

} // namespace net_tester
} // namespace enyx
