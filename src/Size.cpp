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

#include "Size.hpp"

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <boost/io/ios_state.hpp>
#include <boost/regex.hpp>

namespace enyx {
namespace net_tester {

namespace {

Size
parse_size(const std::string & s)
{
    boost::regex regex_iec(R"((\d+)\s*([KMGTPE]i)?(B|bit))");
    boost::regex regex_si(R"((\d+)\s*([kKMGTPE])?(B|b|bit))");
    boost::smatch m;

    Size::UnitSystem unit_system;
    if (boost::regex_match(s, m, regex_iec))
        unit_system = Size::IEC;
    else if (boost::regex_match(s, m, regex_si))
        unit_system = Size::SI;
    else
    {
        std::ostringstream error;
        error << "size '" << s
              << "' doesn't match either \\d+\\s*([KMGTPE]i)?(B|bit)"
                 " nor \\d+\\s*([kKMGTPE])?(B|b|bit)";
        throw std::runtime_error(error.str());
    }

    const uint64_t factor = unit_system == Size::SI ? 1000 : 1024;

    uint64_t size = std::atoll(m.str(1).c_str());

    if (m[2].matched)
        switch (m.str(2)[0])
        {
            case 'E':
                size *= factor;
            case 'P':
                size *= factor;
            case 'T':
                size *= factor;
            case 'G':
                size *= factor;
            case 'M':
                size *= factor;
            case 'K':
            case 'k':
                size *= factor;
        }

    if (m.str(3) == "bit" || m.str(3) == "b")
        size /= 8;

    return Size{size, unit_system};
}

const char * units_iec[] = { "bit", "Kibit", "Mibit",
                             "Gibit", "Tibit", "Pibit",
                             "Eibit" };

const char * units_si[] = { "bit", "kbit", "Mbit",
                            "Gbit", "Tbit", "Pbit",
                            "Ebit" };

#define UNITS_COUNT ((sizeof(units_iec)) / sizeof(units_iec[0]))


} // anonymous namespace

std::istream &
operator>>(std::istream & in, Size & size)
{
    std::istream::sentry sentry(in);

    if (sentry)
    {
        std::string s;
        in >> s;

        size = parse_size(s);
    }

    return in;
}


std::ostream &
operator<<(std::ostream & out, const Size & size)
{
    std::ostream::sentry sentry(out);
    if (sentry)
    {
        const Size::UnitSystem unit_system = size.get_unit_system();
        const uint64_t factor = unit_system == Size::SI ? 1000 : 1024;
        const char ** units = unit_system == Size::SI ? units_si : units_iec;

        long double value = size * 8;
        uint64_t i = 0;
        for (i = 0; i != UNITS_COUNT - 1 && value / factor >= 1.; ++i)
            value /= factor;

        boost::io::ios_flags_saver s(out);
        out << std::fixed << std::setprecision(1) << value << units[i]
            << "(" << size * 8 << units[0] << ")";
    }

    return out;
}

} // namespace net_tester
} // namespace enyx

