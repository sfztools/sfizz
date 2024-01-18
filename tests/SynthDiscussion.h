// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz
#pragma once
#include <cstddef>
#include <string>
#include <absl/strings/str_cat.h>
#include <absl/types/variant.h>
#include "absl/strings/string_view.h"
#include "sfizz/Synth.h"
#include "sfizz_message.h"
#include "catch2/catch.hpp"
#include "utility/Size.h"

using namespace sfz;
constexpr int maxArgs = 8;

enum class OSCValueLess { True, False, None };
using OSCVariant = absl::variant<int32_t, int64_t, float, double, std::string, OSCValueLess>;

// template <class ... Args>
// struct vecToTuple
// {
//     template <unsigned I>
//     static void rewrap(std::tuple<Args...> &tup, const std::vector<OSCVariant>& variants)
//     {
//         std::get<I-1>(tup) = std::get<decltype(std::get<I-1>(tup))>(variants[I-1]);
//         rewrap<I-1>(tup, variants);
//     }
//     template <>
//     static void rewrap<0>(std::tuple<Args...> &, const std::vector<OSCVariant>&) { }
// };

// template<int MaxArgs = 8>
struct SynthDiscussion
{
    SynthDiscussion()
    {
        client.setReceiveCallback(&SynthDiscussion::receiver);
    }

    static void receiver(void* data, int, const char* path, const char* sig, const sfizz_arg_t* args)
    {
        SynthDiscussion* self = (SynthDiscussion*)data;
        self->recv_path = path;
        self->recv_sig = sig;
        self->recv_args.clear();
        for (int i = 0, n = self->recv_sig.size(); i < n; ++i) {
            switch (self->recv_sig[i]) {
            case 'i': self->recv_args.emplace_back(args[i].i); break;
            case 'f': self->recv_args.emplace_back(args[i].f); break;
            case 'd': self->recv_args.emplace_back(args[i].d); break;
            case 'h': self->recv_args.emplace_back(args[i].h); break;
            case 's': self->recv_args.emplace_back(args[i].s); break;
            case 'T': self->recv_args.emplace_back(OSCValueLess::True); break;
            case 'F': self->recv_args.emplace_back(OSCValueLess::False); break;
            case 'N': self->recv_args.emplace_back(OSCValueLess::None); break;
            default:
                ASSERTFALSE;
            }
        }
    }

    void load(absl::string_view sfz)
    {
        synth.loadSfzString(fs::current_path() / "tests/TestFiles/discussion.sfz", sfz);
    }

    void send(absl::string_view path, int32_t value)
    {
        sent_args[0].i = value;
        synth.dispatchMessage(client, 0, path.data(), "i", sent_args);
    }

    void send(absl::string_view path, int64_t value)
    {
        sent_args[0].h = value;
        synth.dispatchMessage(client, 0, path.data(), "h", sent_args);
    }

    void send(absl::string_view path, float value)
    {
        sent_args[0].f = value;
        synth.dispatchMessage(client, 0, path.data(), "f", sent_args);
    }

    void send(absl::string_view path, const std::string& value)
    {
        sent_args[0].s = value.c_str();
        synth.dispatchMessage(client, 0, path.data(), "s", sent_args);
    }

    void send(absl::string_view path, std::nullptr_t)
    {
        synth.dispatchMessage(client, 0, path.data(), "N", nullptr);
    }

    void send(absl::string_view path, bool value)
    {
        synth.dispatchMessage(client, 0, path.data(), value ? "T" : "F", nullptr);
    }

    template<class T> T read(absl::string_view path)
    {
        synth.dispatchMessage(client, 0, path.data(), "", nullptr);
        return absl::get<T>(recv_args[0]);
    }

    bool replied(absl::string_view path)
    {
        recv_path = "";
        synth.dispatchMessage(client, 0, path.data(), "", nullptr);
        return !recv_path.empty();
    }

    template<class T> std::vector<T> readAll(absl::string_view path)
    {
        synth.dispatchMessage(client, 0, path.data(), "", nullptr);
        std::vector<T> returned;
        for (const auto& arg: recv_args)
            returned.push_back(absl::get<T>(arg));

        return returned;
    }

    void sendAll(absl::string_view path, const std::vector<float>& value)
    {
        std::string signature(value.size(), 'f');
        for (int i = 0; i < ssize(value) && i < maxArgs; ++i)
            sent_args[i].f = value[i];
        synth.dispatchMessage(client, 0, path.data(), signature.c_str(), sent_args);
    }

    void sendAll(absl::string_view path, const std::vector<int32_t>& value)
    {
        std::string signature(value.size(), 'i');
        for (int i = 0; i < ssize(value) && i < maxArgs; ++i)
            sent_args[i].i = value[i];
        synth.dispatchMessage(client, 0, path.data(), signature.c_str(), sent_args);
    }

    void sendAll(absl::string_view path, const std::vector<int64_t>& value)
    {
        std::string signature(value.size(), 'h');
        for (int i = 0; i < ssize(value) && i < maxArgs; ++i)
            sent_args[i].h = value[i];
        synth.dispatchMessage(client, 0, path.data(), signature.c_str(), sent_args);
    }

    template<class T>
    T sendAndRead(absl::string_view path, T value)
    {
        send(path, value);
        return read<T>(path);
    }

    template<class T>
    std::vector<T> sendAndReadAll(absl::string_view path, const std::vector<T>& value)
    {
        sendAll(path, value);
        return readAll<T>(path);
    }

    std::string formatLast() const
    {
        std::string newMessage = absl::StrCat(recv_path, ",", recv_sig, " : { ");
        for (unsigned i = 0, n = recv_sig.size(); i < n; ++i) {
            absl::visit([&](auto&& arg){ absl::StrAppend(&newMessage, arg); }, recv_args[i]);
            if (i == (n - 1))
                absl::StrAppend(&newMessage, " }");
            else
                absl::StrAppend(&newMessage, ", ");
        }

        return newMessage;
    }

    Synth synth;
    sfizz_arg_t sent_args[maxArgs];
    std::vector<OSCVariant> recv_args;
    std::string recv_path;
    std::string recv_sig;
    AudioBuffer<float> buffer { 2, 256 };
    Client client { this };
};
