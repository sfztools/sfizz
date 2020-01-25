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

#include "Buffer.h"
#include "SIMDHelpers.h"
#include "absl/types/span.h"

namespace sfz
{
/**
 * @brief A naive circular buffer which is supposed to hold power values
 * and return the average of its content.
 *
 * @tparam ValueType
 */
template<class ValueType>
class HistoricalBuffer {
public:
	HistoricalBuffer() = delete;
	HistoricalBuffer(size_t size)
	: size(size)
	{
		resize(size);
	}

    /**
     * @brief Resize the underlying buffer. Newly added "slots" are
     * initialized to 0.0
     *
     * @param size
     */
	void resize(size_t size)
	{
		buffer.resize(size);
		fill<ValueType>(absl::MakeSpan(buffer), 0.0);
		index = 0;
	}

    /**
     * @brief Add a value to the buffer
     *
     * @param value
     */
	void push(ValueType value)
	{
		if (size > 0) {
			buffer[index] = value;
			if (++index == size)
				index = 0;
		}
	}

    /**
     * @brief Return the average of all the values in the buffer
     *
     * @return ValueType
     */
	ValueType getAverage() const
	{
		return mean<ValueType>(buffer);
	}
private:
	Buffer<ValueType> buffer;
	size_t size { 0 };
	size_t index { 0 };
};
}
