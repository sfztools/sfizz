// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "EditValue.h"
#include <sfizz_message.h>
#include <absl/strings/string_view.h>
#include <string>
#include <cstdint>
enum class EditId : int;

class EditorController {
public:
    virtual ~EditorController() {}

    // called by Editor
    virtual void uiSendValue(EditId id, const EditValue& v) = 0;
    virtual void uiBeginSend(EditId id) = 0;
    virtual void uiEndSend(EditId id) = 0;
    virtual void uiSendMIDI(const uint8_t* msg, uint32_t len) = 0;
    virtual void uiSendMessage(const char* path, const char* sig, const sfizz_arg_t* args) = 0;
    class Receiver;
    void decorate(Receiver* r) { r_ = r; }

    class Receiver {
    public:
        virtual ~Receiver() {}
        virtual void uiReceiveValue(EditId id, const EditValue& v) = 0;
        virtual void uiReceiveMessage(const char* path, const char* sig, const sfizz_arg_t* args) = 0;
    };

    // called by DSP
    void uiReceiveValue(EditId id, const EditValue& v);
    void uiReceiveMessage(const char* path, const char* sig, const sfizz_arg_t* args);

private:
    Receiver* r_ = nullptr;
};

inline void EditorController::uiReceiveValue(EditId id, const EditValue& v)
{
    if (r_)
        r_->uiReceiveValue(id, v);
}

inline void EditorController::uiReceiveMessage(const char* path, const char* sig, const sfizz_arg_t* args)
{
    if (r_)
        r_->uiReceiveMessage(path, sig, args);
}
