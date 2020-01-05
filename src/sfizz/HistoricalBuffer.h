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
