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

#include <memory>
#include <array>

namespace enyx {
namespace net_tester {

struct HandlerMemory
{
public:
    HandlerMemory() noexcept
        : in_use_()
    { }

    void *
    allocate(std::size_t size) noexcept
    {
        assert(! in_use_ && "handler memory is free to use");
        assert(size <= sizeof(storage_) && "handler memory is large enough");
        return &storage_;
    }

    void
    deallocate(void * pointer) noexcept
    {
        assert(pointer == &storage_ && "handler comes from this memory");
        in_use_ = false;
    }

private:
    typename std::aligned_storage<1024>::type storage_;
    bool in_use_;
};

template<typename Handler>
class HandlerAllocator
{
    template<typename> friend class HandlerAllocator;

public:
    using value_type = Handler;

public:
    explicit
    HandlerAllocator(HandlerMemory & memory) noexcept
        : memory_(memory)
    { }

    template <typename U>
    HandlerAllocator(const HandlerAllocator<U>& other) noexcept
        : memory_(other.memory_)
    { }

    bool
    operator==(const HandlerAllocator& other) const noexcept
    {
        return &memory_ == &other.memory_;
    }

    bool
    operator!=(const HandlerAllocator& other) const noexcept
    {
        return &memory_ != &other.memory_;
    }

    Handler *
    allocate(std::size_t n) const noexcept
    {
        return static_cast<Handler *>(memory_.allocate(sizeof(Handler) * n));
    }

    void
    deallocate(Handler * p, std::size_t /*n*/) const noexcept
    {
        return memory_.deallocate(p);
    }

private:
    HandlerMemory & memory_;
};

template <typename Handler>
class CustomAllocHandler
{
public:
    using allocator_type = HandlerAllocator<Handler>;

public:
    CustomAllocHandler(HandlerMemory & m, Handler h) noexcept
        : memory_(m),
          handler_(h)
    { }

    allocator_type
    get_allocator() const noexcept
    {
        return allocator_type{memory_};
    }

    template<typename ...Args>
    void
    operator()(Args&&... args)
    {
        handler_(std::forward<Args>(args)...);
    }

private:
    HandlerMemory & memory_;
    Handler handler_;
};

// Helper function to wrap a handler object to add custom allocation.
template<typename Handler>
inline CustomAllocHandler<Handler>
make_handler(HandlerMemory & m, Handler h) noexcept
{
    return CustomAllocHandler<Handler>(m, std::move(h));
}

} // namespace net_tester
} // namespace enyx

