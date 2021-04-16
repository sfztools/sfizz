// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Messaging.h"
#include "catch2/catch.hpp"
#include <absl/types/span.h>
#include <cstring>

TEST_CASE("[Messaging] OSC message creation")
{
    // http://opensoundcontrol.org/spec-1_0-examples

    {
        const char* path = "/oscillator/4/frequency";
        const char* sig = "f";
        sfizz_arg_t args[1];
        args[0].f = 440.0f;

        const uint8_t expected[] = {
            0x2f, /* / */  0x6f, /* o */  0x73, /* s */  0x63, /* c */
            0x69, /* i */  0x6c, /* l */  0x6c, /* l */  0x61, /* a */
            0x74, /* t */  0x6f, /* o */  0x72, /* r */  0x2f, /* / */
            0x34, /* 4 */  0x2f, /* / */  0x66, /* f */  0x72, /* r */
            0x65, /* e */  0x71, /* q */  0x75, /* u */  0x65, /* e */
            0x6e, /* n */  0x63, /* c */  0x79, /* y */  0x00,
            0x2c, /* , */  0x66, /* f */  0x00,          0x00,
            0x43,          0xdc,          0x00,          0x00,
        };

        uint32_t size = sfizz_prepare_message(nullptr, 0, path, sig, args);
        REQUIRE(size == sizeof(expected));

        uint8_t actual[sizeof(expected)];
        size = sfizz_prepare_message(actual, sizeof(actual), path, sig, args);
        REQUIRE(size == sizeof(expected));
        REQUIRE(absl::MakeSpan(actual) == absl::MakeSpan(expected));

        const char* path2;
        const char* sig2;
        const sfizz_arg_t* args2;
        uint8_t buffer[256];
        REQUIRE(sfizz_extract_message(actual, sizeof(actual), buffer, sizeof(buffer), &path2, &sig2, &args2) > 0);
        REQUIRE(!strcmp(path, path2));
        REQUIRE(!strcmp(sig, sig2));
        REQUIRE(args2[0].f == 440.0f);
    }

    {
        const char* path = "/foo";
        const char* sig = "iisff";
        sfizz_arg_t args[5];
        args[0].i = 1000;
        args[1].i = -1;
        args[2].s = "hello";
        args[3].f = 1.234f;
        args[4].f = 5.678f;

        const uint8_t expected[] = {
            0x2f, /* / */  0x66, /* f */  0x6f, /* o */  0x6f, /* o */
            0x00,          0x00,          0x00,          0x00,
            0x2c, /* , */  0x69, /* i */  0x69, /* i */  0x73, /* s */
            0x66, /* f */  0x66, /* f */  0x00,          0x00,
            0x00,          0x00,          0x03,          0xe8,
            0xff,          0xff,          0xff,          0xff,
            0x68,          0x65,          0x6c,          0x6c,
            0x6f,          0x00,          0x00,          0x00,
            0x3f,          0x9d,          0xf3,          0xb6,
            0x40,          0xb5,          0xb2,          0x2d,
        };

        uint32_t size = sfizz_prepare_message(nullptr, 0, path, sig, args);
        REQUIRE(size == sizeof(expected));

        uint8_t actual[sizeof(expected)];
        size = sfizz_prepare_message(actual, sizeof(actual), path, sig, args);
        REQUIRE(size == sizeof(expected));
        REQUIRE(absl::MakeSpan(actual) == absl::MakeSpan(expected));

        const char* path2;
        const char* sig2;
        const sfizz_arg_t* args2;
        uint8_t buffer[256];
        REQUIRE(sfizz_extract_message(actual, sizeof(actual), buffer, sizeof(buffer), &path2, &sig2, &args2) > 0);
        REQUIRE(!strcmp(path, path2));
        REQUIRE(!strcmp(sig, sig2));
        REQUIRE(args2[0].i == 1000);
        REQUIRE(args2[1].i == -1);
        REQUIRE(!strcmp(args2[2].s, "hello"));
        REQUIRE(args2[3].f == 1.234f);
        REQUIRE(args2[4].f == 5.678f);
    }
}

TEST_CASE("[Messaging] Type-safe client API")
{
    sfz::Client client(nullptr);

    static const int32_t i = 777;
    static const int64_t h = 0x100000000LL;
    static const float f = 3.14f;
    static const double d = 6.28;
    static const uint8_t m[4] = {0x90, 0x40, 0xFF};
    static const sfizz_blob_t b { reinterpret_cast<const uint8_t*>("MyBinaryString"), 14 };
    static const char s[] = "Hello, World!";

    client.setReceiveCallback(+[](void*, int, const char* path, const char* sig, const sfizz_arg_t* args) {
        REQUIRE(!strcmp(path, "/test"));
        REQUIRE(!strcmp(sig, "imhfdsbTFNI"));
        unsigned index = 0;
        REQUIRE(args[index++].i == i);
        REQUIRE(!memcmp(args[index++].m, m, 4));
        REQUIRE(args[index++].h == h);
        REQUIRE(args[index++].f == f);
        REQUIRE(args[index++].d == d);
        REQUIRE(!strcmp(args[index++].s, s));
        REQUIRE(args[index  ].b->data == b.data);
        REQUIRE(args[index++].b->size == b.size);
    });

    client.receive<'i', 'm', 'h', 'f', 'd', 's', 'b', 'T', 'F', 'N', 'I'>(
        0, "/test", i, m, h, f, d, s, &b, {}, {}, {}, {});
}
