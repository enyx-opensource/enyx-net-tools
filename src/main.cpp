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
#include <stdexcept>
#include <cstdlib>

#include <boost/system/system_error.hpp>

#include "Executable.hpp"

int
main(int argc, char **argv)
{
    try
    {
        enyx::net_tester::Executable::run(argc, argv);
        return EXIT_SUCCESS;
    }
    catch (const boost::system::system_error & e)
    {
        std::cerr << e.what() << "." << std::endl;
        return e.code().value();
    }
    catch (const std::exception & e)
    {
        std::cerr << "Failed because " << e.what() << "." << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "Failed because an unknow error occured." << std::endl;
        return EXIT_FAILURE;
    }
}

