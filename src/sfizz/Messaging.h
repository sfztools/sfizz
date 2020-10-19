// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "sfizz_message.h"

namespace sfz {

class Client {
public:
    explicit Client(void* data) : data_(data) {}
    void* getClientData() const { return data_; }
    void setReceiveCallback(sfizz_receive_t* receive) { receive_ = receive; }
    bool canReceive() const { return receive_ != nullptr; }
    void receive(int delay, const char* path, const char* sig, const sfizz_arg_t* args);

private:
    void* data_ = nullptr;
    sfizz_receive_t* receive_ = nullptr;
};

inline void Client::receive(int delay, const char* path, const char* sig, const sfizz_arg_t* args)
{
    if (receive_)
        receive_(data_, delay, path, sig, args);
}

} // namespace sfz
