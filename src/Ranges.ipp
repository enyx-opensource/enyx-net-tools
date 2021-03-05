/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 EnyxSA
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
#include <algorithm>

namespace enyx {
namespace net_tester {

template<typename Type>
inline std::istream &
operator>>(std::istream & in, Ranges<Type> & ranges)
{
    ranges.clear();
    for (std::string token; std::getline(in, token, ','); )
    {
        Range<Type> range;
        std::istringstream{token} >> range;
        ranges.push_back(range);
    }

    return in;
}

template<typename Type>
inline std::ostream &
operator<<(std::ostream & out, const Ranges<Type> & ranges)
{
    bool first = true;
    for (auto const& range : ranges)
    {
        if (first)
        {
            out << ", ";
            first = false;
        }

        out << range;
    }

    return out;
}

template<typename Type>
inline std::vector<Type>
as_sequence(const Ranges<Type> & ranges)
{
    std::vector<Type> sequence;
    for (auto const& range : ranges)
    {
        auto const partial_sequence = as_sequence(range);
        sequence.insert(sequence.end(),
                        partial_sequence.begin(),
                        partial_sequence.end());
    }

    // Remove duplicate
    sequence.erase(std::unique(sequence.begin(), sequence.end()),
                   sequence.end());

    return sequence;
}

} // namespace net_tester
} // namespace enyx

