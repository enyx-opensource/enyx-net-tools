#include "BandwidthThrottle.hpp"

namespace enyx {
namespace net_tester {

namespace pt = boost::posix_time;

BandwidthThrottle::BandwidthThrottle(boost::asio::io_service & io_service,
                                     std::size_t bandwidth,
                                     std::size_t sampling_frequency)
        : timer_(io_service),
          slice_bytes_count_(to_slice_bytes_count(bandwidth,
                                                  sampling_frequency)),
          slice_duration_(to_slice_duration(sampling_frequency)),
          next_slice_start_(pt::microsec_clock::universal_time())
{ }

pt::time_duration
BandwidthThrottle::to_slice_duration(std::size_t sampling_frequency)
{
    return boost::posix_time::seconds(1) / sampling_frequency;
}

std::size_t
BandwidthThrottle::to_slice_bytes_count(std::size_t bandwidth,
                                        std::size_t sampling_frequency)
{
    if (! bandwidth)
        return -1;

    return bandwidth / sampling_frequency;
}

} // namespace net_tester
} // namespace enyx
