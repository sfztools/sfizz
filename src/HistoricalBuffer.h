#include <vector>
#include "SIMDHelpers.h"
#include "absl/types/span.h"

namespace sfz
{
template<class ValueType>
class HistoricalBuffer {
public:
	HistoricalBuffer() = delete;
	HistoricalBuffer(size_t size)
	: size(size)
	{
		resize(size);
	}

	void resize(int size)
	{
		buffer.resize(size);
		fill<ValueType>(absl::MakeSpan(buffer), 0.0);
		index = 0;
	}
	
	void push(ValueType value)
	{
		if (size > 0) {
			buffer[index] = value;
			if (++index == size)
				index = 0;
		}
	}

	ValueType getAverage() const
	{
		return mean<ValueType>(buffer);
	}
private:
	std::vector<ValueType> buffer;
	size_t size { 0 };
	size_t index { 0 };
};
}