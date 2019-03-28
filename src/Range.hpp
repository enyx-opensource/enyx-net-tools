/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 EnyxSA
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

#pragma once

#include <stdint.h>
#include <iosfwd>

#include "Size.hpp"

namespace enyx {
namespace tcp_tester {

class Range
{
public:
    using ValueType = Size;

public:
    Range(void) = default;

    Range(const ValueType & value)
        : low_(value), high_(value)
    { }

    Range(const ValueType & low, const ValueType & high)
        : low_(low), high_(high)
    { }

    ValueType
    low() const
    { return low_; }

    ValueType
    high() const
    { return high_; }

private:
    ValueType low_;
    ValueType high_;
};

std::istream &
operator>>(std::istream & in, Range & range);

std::ostream &
operator<<(std::ostream & out, const Range & range);

} // namespace tcp_tester
} // namespace enyx
