// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Messaging.h"
#include "absl/types/optional.h"
#include "absl/strings/string_view.h"
#include "absl/strings/str_format.h"
#include "utility/Debug.h"
#include <map>

namespace sfz {

enum class LogMessage {
    Debug = 0,
    Info,
    Reloading,
    ChangingDefaultPath,
    FileNotFound,
    VoiceStarted,
    Sentinel,
};

class Logger {
public:
    Logger()
    {
        subscribed_.resize(static_cast<int>(LogMessage::Sentinel), nullptr);
        for (int i = 0; i < static_cast<int>(LogMessage::Sentinel); ++i) {
            LogMessage msg = static_cast<LogMessage>(i);
            auto path = _message_to_path(msg);
            path_to_msg_.emplace(path, msg);
            msg_to_path_.emplace_back(path);
        }
        buffer_.reserve(1024);
    }

    template<char... Sig>
    void send(int delay, LogMessage msg, OscDecayedType<Sig>&&... values)
    {
        int index = static_cast<int>(msg);
        Client* client = subscribed_[index];
        if (client == nullptr)
            return;

        const char* path = msg_to_path_[index].c_str();
        client->receive<Sig...>(delay, path, std::forward<OscDecayedType<Sig>...>(values...));
    }

    template<class...Args>
    void send(int delay, LogMessage msg, const absl::FormatSpec<Args...> &format, const Args&...args)
    {
        int index = static_cast<int>(msg);
        Client* client = subscribed_[index];
        if (client == nullptr)
            return;
        
        buffer_.clear();
        absl::StrAppendFormat(&buffer_, format, args...);
        const char* path = msg_to_path_[index].c_str();
        client->receive<'s'>(delay, path, buffer_.c_str());
    }

    template<class...Args>
    void debug(int delay, const absl::FormatSpec<Args...> &format, const Args&...args)
    {
        send(delay, LogMessage::Debug, format, args...);
    }

    template<class...Args>
    void info(int delay, const absl::FormatSpec<Args...> &format, const Args&...args)
    {
        send(delay, LogMessage::Info, format, args...);
    }

    bool subscribed(absl::string_view path) const
    {
        if (auto index = _path_to_message(path))
            return subscribed_[static_cast<int>(*index)] != nullptr;

        return false;
    }

    void subscribe(absl::string_view path, Client& client)
    {
        if (auto index = _path_to_message(path))
            subscribed_[static_cast<int>(*index)] = &client;
    }

    void unsubscribe(absl::string_view path)
    {
        if (auto index = _path_to_message(path))
            subscribed_[static_cast<int>(*index)] = nullptr;
    }

private:
    absl::optional<LogMessage> _path_to_message(absl::string_view path) const
    {
        auto it = path_to_msg_.find(path);
        if (it == path_to_msg_.end())
            return {};

        return it->second;
    }

    absl::string_view _message_to_path(LogMessage message)
    {
        switch(message) {
        case LogMessage::Debug: return "/log/debug";
        case LogMessage::Info: return "/log/info";
        case LogMessage::Reloading: return "/log/reloading";
        case LogMessage::ChangingDefaultPath: return "/log/default_path_changed";
        case LogMessage::FileNotFound: return "/log/file_not_found";
        case LogMessage::VoiceStarted: return "/log/voice_started";
        case LogMessage::Sentinel: ASSERTFALSE; return "";
        }
    }
    std::string buffer_;
    std::map<absl::string_view, LogMessage> path_to_msg_;
    std::vector<std::string> msg_to_path_;
    std::vector<Client*> subscribed_;
};
}
