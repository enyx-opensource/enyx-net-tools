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

#pragma once

#include <stdint.h>
#include <iosfwd>

namespace enyx {
namespace net_tester {

class Size
{
public:
    enum UnitSystem { SI, IEC };

public:
    constexpr
    Size(uint64_t value = 0ULL,
         UnitSystem unit_system = IEC)
        : value_(value),
          unit_system_(unit_system)
    { }

    operator const uint64_t &() const
    { return value_; }

    operator uint64_t &()
    { return value_; }

    UnitSystem
    get_unit_system() const
    { return unit_system_; }

private:
    uint64_t value_;
    UnitSystem unit_system_;
};

std::istream &
operator>>(std::istream & in, Size & size);

std::ostream &
operator<<(std::ostream & out, const Size & size);

} // namespace net_tester
} // namespace enyx
