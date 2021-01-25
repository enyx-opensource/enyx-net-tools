#pragma once

#include <boost/system/error_code.hpp>

namespace enyx {
namespace net_tester {

namespace error {

enum Error
{
    success = 0,
    generic_fault = 1,
    unexpected_eof = 2,
    checksum_failed = 3,
    test_timeout = 4,
    unexpected_data = 5,
    user_interrupt = 6,
    program_termination = 7,
    unknown_signal = 8,
};

boost::system::error_code
make_error_code(Error e);

} // namespace error

const boost::system::error_category &
tcp_tester_category();

} // namespace net_tester
} // namespace enyx

namespace boost {
namespace system {

template<>
struct is_error_code_enum<enyx::net_tester::error::Error>
{ static const bool value =  true; };

} // namespace system
} // namespace boost

