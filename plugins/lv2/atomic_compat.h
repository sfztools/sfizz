/*
  SPDX-License-Identifier: ISC

  Sfizz LV2 plugin

  Copyright 2019-2020, Paul Ferrand <paul@ferrand.cc>

  This file was based on skeleton and example code from the LV2 plugin
  distribution available at http://lv2plug.in/

  The LV2 sample plugins have the following copyright and notice, which are
  extended to the current work:
  Copyright 2011-2016 David Robillard <d@drobilla.net>
  Copyright 2011 Gabriel M. Beddingfield <gabriel@teuton.org>
  Copyright 2011 James Morris <jwm.art.net@gmail.com>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  Compiling this plugin statically against libsndfile implies distributing it
  under the terms of the LGPL v3 license. See the LICENSE.md file for more
  information. If you did not receive a LICENSE.md file, inform the current
  maintainer.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#if defined(__cplusplus)

#include <atomic>

#elif defined(_MSC_VER)

#include <intrin.h>

typedef volatile int atomic_int;

inline int atomic_exchange(atomic_int *d, int x)
{
    return _InterlockedExchange((volatile long *)d, (long)x);
}

inline void atomic_store(atomic_int *d, int x)
{
    _InterlockedExchange((volatile long *)d, (long)x);
}

inline int atomic_load(const atomic_int *d)
{
    return _InterlockedOr((volatile long *)d, 0);
}

#else

#include <stdatomic.h>

#endif
