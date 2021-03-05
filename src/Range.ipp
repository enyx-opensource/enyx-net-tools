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

#include <iostream>
#include <string>
#include <sstream>

namespace enyx {
namespace net_tester {

template<typename Type>
inline std::istream &
operator>>(std::istream & in, Range<Type> & range)
{
    std::string s;
    in >> s;

    auto const offset = s.find('-');

    Type low, high;
    std::istringstream{s.substr(0, offset)} >> low;

    if (offset == std::string::npos)
        high = low;
    else
        std::istringstream{s.substr(offset + 1)} >> high;

    range = Range<Type>{low, high};

    return in;
}

template<typename Type>
inline std::ostream &
operator<<(std::ostream & out, const Range<Type> & range)
{
    out << range.low();

    if (range.low() != range.high())
        out << "-" << range.high();

    return out;
}

template<typename Type>
inline std::vector<Type>
as_sequence(const Range<Type> & range)
{
    std::vector<Type> sequence;
    for (auto i = range.low(), e = range.high(); i != e; ++i)
        sequence.push_back(i);

    sequence.push_back(range.high());
    return sequence;
}

} // namespace net_tester
} // namespace enyx

