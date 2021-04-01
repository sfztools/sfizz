// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "InstrumentDescription.h"
#include "MessageUtils.h"
#include "sfizz.hpp"
#include <absl/strings/str_cat.h>
#include <algorithm>
#include <memory>
#include <iostream>
#include <cstring>
#include <cstdint>

template <class... Args>
static void bufferedStrCat(std::string* buffer, const Args&... args)
{
    buffer->clear();
    absl::StrAppend(buffer, args...);
}

std::string getDescriptionBlob(sfizz_synth_t* handle)
{
    std::string blob;
    blob.reserve(128 * 1024);

    std::vector<char> msgbuf;
    msgbuf.resize(1024);

    std::string pathbuf;
    pathbuf.reserve(256);

    struct ClientData {
        sfz::Sfizz* synth = nullptr;
        sfz::Client* client = nullptr;
        std::string* blob = nullptr;
        std::vector<char>* msgbuf = nullptr;
        std::string* pathbuf = nullptr;
    };

    sfz::Sfizz synth(handle);
    ClientData cdata;
    sfz::ClientPtr client = synth.createClient(&cdata);
    cdata.synth = &synth;
    cdata.client = client.get();
    cdata.blob = &blob;
    cdata.msgbuf = &msgbuf;
    cdata.pathbuf = &pathbuf;

    synth.setReceiveCallback(*client, [](void* data, int, const char* path, const char* sig, const sfizz_arg_t* args) {
        ClientData& cdata = *reinterpret_cast<ClientData*>(data);
        unsigned indices[8];

        ///
        uint32_t msglen = sfizz_prepare_message(cdata.msgbuf->data(), cdata.msgbuf->size(), path, sig, args);
        if (msglen > cdata.msgbuf->size()) {
            cdata.msgbuf->resize(msglen);
            sfizz_prepare_message(cdata.msgbuf->data(), cdata.msgbuf->size(), path, sig, args);
        }
        cdata.blob->append(cdata.msgbuf->data(), msglen);

        ///
        if (Messages::matchOSC("/key/slots", path, indices) && !strcmp(sig, "b")) {
            ConstBitSpan bits(args[0].b->data, 8 * args[0].b->size);
            for (unsigned key = 0; key < 128 && key < bits.bit_size(); ++key) {
                if (bits.test(key)) {
                    bufferedStrCat(cdata.pathbuf, "/key", key, "/label");
                    cdata.synth->sendMessage(*cdata.client, 0, cdata.pathbuf->c_str(), "", nullptr);
                }
            }
        }
        else if (Messages::matchOSC("/sw/last/slots", path, indices) && !strcmp(sig, "b")) {
            ConstBitSpan bits(args[0].b->data, 8 * args[0].b->size);
            for (unsigned key = 0; key < 128 && key < bits.bit_size(); ++key) {
                if (bits.test(key)) {
                    bufferedStrCat(cdata.pathbuf, "/sw/last/", key, "/label");
                    cdata.synth->sendMessage(*cdata.client, 0, cdata.pathbuf->c_str(), "", nullptr);
                }
            }
        }
        else if (Messages::matchOSC("/cc/slots", path, indices) && !strcmp(sig, "b")) {
            ConstBitSpan bits(args[0].b->data, 8 * args[0].b->size);
            for (unsigned cc = 0; cc < sfz::config::numCCs && cc < bits.bit_size(); ++cc) {
                if (bits.test(cc)) {
                    bufferedStrCat(cdata.pathbuf, "/cc", cc, "/label");
                    cdata.synth->sendMessage(*cdata.client, 0, cdata.pathbuf->c_str(), "", nullptr);
                    bufferedStrCat(cdata.pathbuf, "/cc", cc, "/default");
                    cdata.synth->sendMessage(*cdata.client, 0, cdata.pathbuf->c_str(), "", nullptr);
                }
            }
        }
    });

    synth.sendMessage(*client, 0, "/key/slots", "", nullptr);
    synth.sendMessage(*client, 0, "/sw/last/slots", "", nullptr);
    synth.sendMessage(*client, 0, "/cc/slots", "", nullptr);

    blob.shrink_to_fit();
    return blob;
}

InstrumentDescription parseDescriptionBlob(absl::string_view blob)
{
    InstrumentDescription desc;

    const uint8_t* src = reinterpret_cast<const uint8_t*>(blob.data());
    uint32_t srcSize = blob.size();
    char buffer[1024];

    const char* path;
    const char* sig;
    const sfizz_arg_t* args;

    int32_t byteCount;
    while ((byteCount = sfizz_extract_message(src, srcSize, buffer, sizeof(buffer), &path, &sig, &args)) > 0) {
        unsigned indices[8];

        ///
        auto copyArgToBitSpan = [](const sfizz_arg_t& arg, BitSpan bits)
        {
            size_t size = std::min<size_t>(bits.byte_size(), arg.b->size);
            memcpy(bits.data(), arg.b->data, size);
        };

        //
        if (Messages::matchOSC("/key/slots", path, indices) && !strcmp(sig, "b"))
            copyArgToBitSpan(args[0], desc.keyUsed.span());
        else if (Messages::matchOSC("/sw/last/slots", path, indices) && !strcmp(sig, "b"))
            copyArgToBitSpan(args[0], desc.keyswitchUsed.span());
        else if (Messages::matchOSC("/cc/slots", path, indices) && !strcmp(sig, "b"))
            copyArgToBitSpan(args[0], desc.ccUsed.span());
        else if (Messages::matchOSC("/key&/label", path, indices) && !strcmp(sig, "s"))
            desc.keyLabel[indices[0]] = args[0].s;
        else if (Messages::matchOSC("/sw/last/&/label", path, indices) && !strcmp(sig, "s"))
            desc.keyswitchLabel[indices[0]] = args[0].s;
        else if (Messages::matchOSC("/cc&/label", path, indices) && !strcmp(sig, "s"))
            desc.ccLabel[indices[0]] = args[0].s;
        else if (Messages::matchOSC("/cc&/default", path, indices) && !strcmp(sig, "f"))
            desc.ccDefault[indices[0]] = args[0].f;

        src += byteCount;
        srcSize -= byteCount;
    }

    return desc;
}

std::ostream& operator<<(std::ostream& os, const InstrumentDescription& desc)
{
    os << "instrument:\n";

    os << "  keys:\n";
    for (unsigned i = 0; i < 128; ++i) {
        if (desc.keyUsed.test(i)) {
            os << "  - number: " << i << "\n";
            if (!desc.keyLabel[i].empty())
                os << "    label: " << desc.keyLabel[i].c_str() << "\n";
        }
    }

    os << "  keyswitches:\n";
    for (unsigned i = 0; i < 128; ++i) {
        if (desc.keyswitchUsed.test(i)) {
            os << "  - number: " << i << "\n";
            if (!desc.keyswitchLabel[i].empty())
                os << "    label: " << desc.keyswitchLabel[i].c_str() << "\n";
        }
    }

    os << "  cc:\n";
    for (unsigned i = 0; i < sfz::config::numCCs; ++i) {
        if (desc.ccUsed.test(i)) {
            os << "  - number: " << i << "\n";
            os << "    default: " << desc.ccDefault[i] << "\n";
            if (!desc.ccLabel[i].empty())
                os << "    label: " << desc.ccLabel[i].c_str() << "\n";
        }
    }

    return os;
}
