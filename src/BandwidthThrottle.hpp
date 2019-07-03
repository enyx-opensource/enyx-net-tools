#pragma once

#include <boost/bind.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

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
    delay(Functor f)
    {
        namespace pt = boost::posix_time;

        const pt::ptime now = pt::microsec_clock::universal_time();
        if (next_slice_start_ <= now)
        {
            next_slice_start_ = now + slice_duration_;
            f(slice_bytes_count_);
        }
        else
        {
            timer_.expires_at(next_slice_start_);

            // This prevent boost::bind function composition.
            // i.e. we don't want f to be called before
            // on_ready is executed but after.
            typedef Holder<Functor> holder_type;
            holder_type holder = { f };

            timer_.async_wait(boost::bind(&BandwidthThrottle::on_ready<holder_type>,
                                          this,
                                          boost::asio::placeholders::error,
                                          holder));
        }
    }

    template<typename Functor>
    void
    on_ready(const boost::system::error_code & failure,
             Functor f)
    {
        namespace pt = boost::posix_time;

        if (failure)
            return;

        next_slice_start_ += slice_duration_;
        f.f(slice_bytes_count_);
    }

private:
    template<typename Functor>
    struct Holder
    {
        Functor f;
    };

private:
    static boost::posix_time::time_duration
    to_slice_duration(std::size_t sampling_frequency);


    static std::size_t
    to_slice_bytes_count(std::size_t bandwidth,
                         std::size_t sampling_frequency);

private:
    boost::asio::deadline_timer timer_;
    std::size_t slice_bytes_count_;
    boost::posix_time::time_duration slice_duration_;
    boost::posix_time::ptime next_slice_start_;
};

} // namespace net_tester
} // namespace enyx
