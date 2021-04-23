// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <pluginterfaces/base/funknown.h>

/*
  Note(jpc) Generated at random with uuidgen.
  Can't find docs on it... maybe it's to register somewhere?
 */
#define SfizzVstProcessor_cid \
    Steinberg::FUID(0xe8fab718, 0x15ed46e3, 0x8b598310, 0x1e12993f)
#define SfizzVstController_cid \
    Steinberg::FUID(0x7129736c, 0xbc784134, 0xbb899d56, 0x2ebafe4f)

template <class T>
Steinberg::FUnknown* createInstance(void*);
