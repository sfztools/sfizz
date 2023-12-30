// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Messaging.h"
#include "absl/types/optional.h"
#include "absl/strings/string_view.h"
#include "absl/strings/str_format.h"

namespace sfz {

class Logger {
public:
    enum class Level: int { NONE = 0, ERROR, WARNING, INFO, TRACE };
    void setReceiver(sfizz_receive_t* receiver, void* data) {
        client_ = Client(data);
        client_->setReceiveCallback(receiver);
    }
    void setLevel(Level level) { level_ = level; }
    Level level() const { return level_; }
    template<char... Sig>
    void error(int delay, absl::string_view subpath, OscDecayedType<Sig>... values)
    {
        absl::StrAppendFormat
        log(Level::ERROR, delay, subpath, std::forward<OscDecayedType<Sig>...>(values));
    }
    template<char... Sig>
    void log(Level level, int delay, absl::string_view path, OscDecayedType<Sig>... values)
    {
        if (!client_)
            return;

        if (level_ < level)
            return;

        client_<Sig...>.receive(delay, subpath, std::forward<OscDecayedType<Sig>...>(values));
    }
private:
    absl::optional<Client> client_ {};
    std::string path_buffer_;
    Level level_ { Level::ERROR };
};
}
