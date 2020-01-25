// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

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
