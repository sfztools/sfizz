// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "absl/strings/string_view.h"
#include <cstdint>

enum class EditId {
    SfzFile,
    Volume,
    Polyphony,
    Oversampling,
    PreloadSize,
    ScalaFile,
    ScalaRootKey,
    TuningFrequency,
};

class EditorController {
public:
    virtual ~EditorController() {}

    // called by Editor
    virtual void uiSendNumber(EditId id, float v) = 0;
    virtual void uiSendString(EditId id, absl::string_view v) = 0;
    virtual void uiBeginSend(EditId id) = 0;
    virtual void uiEndSend(EditId id) = 0;
    virtual void uiSendMIDI(const uint8_t* msg, uint32_t len) = 0;
    class Receiver;
    void decorate(Receiver* r) { r_ = r; }

    class Receiver {
    public:
        virtual ~Receiver() {}
        virtual void uiReceiveNumber(EditId id, float v) = 0;
        virtual void uiReceiveString(EditId id, absl::string_view v) = 0;
    };

    // called by DSP
    void uiReceiveNumber(EditId id, float v);
    void uiReceiveString(EditId id, absl::string_view v);

private:
    Receiver* r_ = nullptr;
};

inline void EditorController::uiReceiveNumber(EditId id, float v)
{
    if (r_)
        r_->uiReceiveNumber(id, v);
}

inline void EditorController::uiReceiveString(EditId id, absl::string_view v)
{
    if (r_)
        r_->uiReceiveString(id, v);
}
