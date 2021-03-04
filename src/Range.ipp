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

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <boost/io/ios_state.hpp>
#include <boost/regex.hpp>

namespace enyx {
namespace net_tester {

namespace {

template<typename ValueType>
Range<ValueType>
parse_range(const std::string & s)
{
    boost::regex regex(R"(([^-]+)(?:-(.+))?)");
    boost::smatch m;

    if (! boost::regex_match(s, m, regex))
    {
        std::ostringstream error;
        error << "range '" << s
              << "' doesn't match ([^-]+)(?:-(.+))";
        throw std::runtime_error(error.str());
    }

    ValueType low;
    std::istringstream{std::string{m[1].first, m[1].second}} >> low;
    if (! m[2].matched)
        return Range<ValueType>{low};

    ValueType high;
    std::istringstream{std::string{m[2].first, m[2].second}} >> high;

    return Range<ValueType>{low, high};
}

} // anonymous namespace

template<typename ValueType>
std::istream &
operator>>(std::istream & in, Range<ValueType> & range)
{
    std::string s;
    in >> s;

    range = parse_range<ValueType>(s);

    return in;
}


template<typename ValueType>
std::ostream &
operator<<(std::ostream & out, const Range<ValueType> & range)
{
    out << range.low();

    if (range.low() != range.high())
        out << "-" << range.high();

    return out;
}

} // namespace net_tester
} // namespace enyx

