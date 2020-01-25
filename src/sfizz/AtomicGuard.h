// SPDX-License-Identifier: BSD-2-Clause

// Copyright (c) 2019-2020, Paul Ferrand
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

/**
 * @brief This file contains a pair of RAII helpers that handle some form
 * of lock-free mutex-type protection adapter to audio applications where you have 1 priority thread
 * that should never block and would rather return silence than wait, and another low-priority
 * thread that handles long computations.
 *
 * @code{.cpp}
 *
 * // Somewhere in a class...
 * std::atomic<bool> canEnterCallback;
 * std::atomic<bool> inCallback;
 *
 * void functionThatSuspendsCallback()
 * {
 *     AtomicDisabler callbackDisabler { canEnterCallback };
 *
 *     while (inCallback) {
 *         std::this_thread::sleep_for(1ms);
 *     }
 *
 * 	   // Do your thing.
 * }
 *
 * void callback(int samplesPerBlock) noexcept
 * {
 *     AtomicGuard callbackGuard { inCallback };
 *     if (!canEnterCallback)
 *         return;
 *
 * 	   // Do your thing.
 * }
 * @endcode
 * There are probably many ways to improve these and probably even debug them.
 * The spinlocking itself could be integrated in the constructor, although the
 * check for return in the callback could not.
 */
#include <atomic>

namespace sfz
{
/**
 * @brief Simple class to set an atomic to true and automatically set it back to false on
 * destruction.
 *
 * You call it like this assuming you need indicate that you are in e.g. a callback
 * @code{.cpp}
 * void functionToProtect()
 * {
 * 		AtomicGuard { guard };
 *
 * 		// Do stuff, the atomic will be set back to false as soon as you're back
 * }
 * @endcode
 * Note that this is not thread-safe at all, in the sense that it is only meant to be
 * used with 2 threads along with the AtomicDisabler. One thread uses AtomicGuards, the other
 * AtomicDisablers, and no other contending thread can share this pair of atomics.
 */
class AtomicGuard
{
public:
	AtomicGuard() = delete;
	AtomicGuard(std::atomic<bool>& guard)
	: guard(guard)
	{
		guard = true;
	}
	~AtomicGuard()
	{
		guard = false;
	}
private:
	std::atomic<bool>& guard;
};

/**
 * @brief Simple class to set an atomic to false and automatically set it back to true on
 * destruction.
 *
 * You call it like this assuming you need to disable e.g. a callback
 * @code{.cpp}
 * void functionThatDisableAnotherFunction()
 * {
 * 		AtomicDisabler { disabler };
 *
 * 		// Do stuff, the atomic will be set back to true as soon as you're back
 * }
 * @endcode
 * Note that this is not thread-safe at all, in the sense that it is only meant to be
 * used with 2 threads along with the AtomicGuard. One thread uses AtomicGuards, the other
 * AtomicDisabler, and no other contending thread can share this pair of atomics.
 */
class AtomicDisabler
{
public:
	AtomicDisabler() = delete;
	AtomicDisabler(std::atomic<bool>& allowed)
	: allowed(allowed)
	{
		allowed = false;
	}
	~AtomicDisabler()
	{
		allowed = true;
	}
private:
	std::atomic<bool>& allowed;
};
}
