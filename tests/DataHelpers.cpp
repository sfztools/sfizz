// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "DataHelpers.h"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstring>
#include <fstream>

void load_txt(DataPoints& dp, std::istream& in)
{
    struct RawValue {
        bool rowJump;
        float value;
    };

    std::vector<RawValue> raw;
    raw.reserve(1024);

    // read raw value data
    {
        std::string line;
        line.reserve(256);

        while (std::getline(in, line)) {
            size_t commentPos = line.find('#');
            if (commentPos == line.npos)
                line = line.substr(0, commentPos);

            std::istringstream lineIn(line);

            RawValue rv;
            rv.rowJump = true;
            while (lineIn >> rv.value) {
                raw.push_back(rv);
                rv.rowJump = false;
            }
        }
    }

    if (raw.empty()) {
        dp.rows = 0;
        dp.cols = 0;
        dp.data.reset();
        return;
    }

    // count rows and columns
    size_t numRows = 0;
    size_t numCols = 0;
    {
        size_t c = 0;
        for (const RawValue& rv : raw) {
            if (!rv.rowJump)
                ++c;
            else {
                numRows += c != 0;
                c = 1;
            }
            numCols = std::max(numCols, c);
        }
        numRows += c != 0;
    }

    // fill the data
    float* data = new float[numRows * numCols];
    dp.rows = numRows;
    dp.cols = numCols;
    dp.data.reset(data);
    for (size_t i = 0, j = 0; i < numRows * numCols; ) {
        size_t c = 1;
        data[i++] = raw[j++].value;
        for (; j < raw.size() && !raw[j].rowJump; ++c)
            data[i++] = raw[j++].value;
        for ( ; c < numCols; ++c)
            data[i++] = 0.0f;
    }
}

bool load_txt_file(DataPoints& dp, const fs::path& path)
{
    fs::ifstream in(path);
    load_txt(dp, in);
    return !in.bad();
}
