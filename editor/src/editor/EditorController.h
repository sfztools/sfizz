// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <absl/strings/string_view.h>
#include <absl/types/variant.h>
#include <cstdint>
enum class EditId : int;
typedef absl::variant<float, std::string> EditValue;

class EditorController {
public:
    virtual ~EditorController() {}

    // called by Editor
    virtual void uiSendValue(EditId id, const EditValue& v) = 0;
    virtual void uiBeginSend(EditId id) = 0;
    virtual void uiEndSend(EditId id) = 0;
    virtual void uiSendMIDI(const uint8_t* msg, uint32_t len) = 0;
    class Receiver;
    void decorate(Receiver* r) { r_ = r; }

    class Receiver {
    public:
        virtual ~Receiver() {}
        virtual void uiReceiveValue(EditId id, const EditValue& v) = 0;
    };

    // called by DSP
    void uiReceiveValue(EditId id, const EditValue& v);

private:
    Receiver* r_ = nullptr;
};

inline void EditorController::uiReceiveValue(EditId id, const EditValue& v)
{
    if (r_)
        r_->uiReceiveValue(id, v);
}
