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

#include <cstddef>

#include "Application.hpp"
#include "Configuration.hpp"
#include "UdpSocket.hpp"

namespace enyx {
namespace tcp_tester {

class UdpApplication : public Application
{
public:
    UdpApplication(const Configuration & configuration);

private:
    enum { MAX_DATAGRAM_SIZE = 1024 };

private:
    virtual void
    async_receive(std::size_t slice_remaining_size = 0ULL) override;

    virtual void
    finish_receive() override;

    virtual void
    async_send(std::size_t slice_remaining_size = 0ULL) override;

    virtual void
    finish_send() override;

    virtual void
    finish() override;

private:
    UdpSocket socket_;
};

} // namespace tcp_tester
} // namespace enyx

