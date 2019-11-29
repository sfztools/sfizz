// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once
#include <atomic>
#include "Debug.h"

#if __cplusplus >= 201703L
/**
 * @brief Tries to catch memory leaks by counting constructions
 * and deletions of objects. This will trap at the end of the program
 * execution if some elements were not properly deleted for one reason
 * or another. Use by adding the LEAK_DETECTOR macro at the end of a class
 * definition with the proper class name, e.g.
 *
 * @code{.cpp}
 * class Buffer
 * {
 *      // Some code for buffer
 *      LEAK_DETECTOR(Buffer);
 * }
 * @endcode
 *
 * @tparam Owner
 */
template <class Owner>
class LeakDetector {
public:
    LeakDetector()
    {
        objectCounter.count++;
    }
    LeakDetector(const LeakDetector&)
    {
        objectCounter.count++;
    }
    ~LeakDetector()
    {
        objectCounter.count--;
        if (objectCounter.count.load() < 0) {
            DBG("Deleted a dangling pointer for class " << Owner::getClassName());
            // Deleted a dangling pointer!
            ASSERTFALSE;
        }
    }

private:
    struct ObjectCounter {
        ObjectCounter() = default;
        ~ObjectCounter()
        {
            if (auto residualCount = count.load() > 0) {
                DBG("Leaked " << residualCount << " instance(s) of class " << Owner::getClassName());
                // Leaked ojects
                ASSERTFALSE;
            }
        };
        std::atomic<int> count { 0 };
    };
	inline static ObjectCounter objectCounter;
};

#ifndef NDEBUG
#define LEAK_DETECTOR(Class)                             \
    friend class LeakDetector<Class>;                    \
    static const char* getClassName() { return #Class; } \
    LeakDetector<Class> leakDetector;
#else
#define LEAK_DETECTOR(Class)
#endif

#else
#define LEAK_DETECTOR(Class)
#endif
