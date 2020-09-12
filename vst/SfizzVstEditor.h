// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstController.h"
#include "editor/EditorController.h"
#include "public.sdk/source/vst/vstguieditor.h"
class Editor;
#if !defined(__APPLE__) && !defined(_WIN32)
namespace VSTGUI { class RunLoop; }
#endif

using namespace Steinberg;
using namespace VSTGUI;

class SfizzVstEditor : public Vst::VSTGUIEditor,
                       public SfizzVstController::StateListener,
                       public SfizzVstController::ControllerChangeListener,
                       public EditorController {
public:
    explicit SfizzVstEditor(void *controller);
    ~SfizzVstEditor();

    bool PLUGIN_API open(void* parent, const VSTGUI::PlatformType& platformType = VSTGUI::kDefaultNative) override;
    void PLUGIN_API close() override;

    SfizzVstController* getController() const
    {
        return static_cast<SfizzVstController*>(Vst::VSTGUIEditor::getController());
    }

    // VSTGUIEditor
    CMessageResult notify(CBaseObject* sender, const char* message) override;

    // SfizzVstController::StateListener
    void onStateChanged() override;

    // SfizzVstController::ControllerChangeListener
    void onControllerChange(int ccNumber, float ccValue) override;

protected:
    // EditorController
    void uiSendValue(EditId id, const EditValue& v) override;
    void uiBeginSend(EditId id) override;
    void uiEndSend(EditId id) override;
    void uiSendMIDI(const uint8_t* msg, uint32_t len) override;

private:
    void loadSfzFile(const std::string& filePath);
    void loadScalaFile(const std::string& filePath);
    void requestControllerState();

    void updateStateDisplay();

    Vst::ParamID parameterOfEditId(EditId id);

    std::unique_ptr<Editor> editor_;

#if !defined(__APPLE__) && !defined(_WIN32)
    SharedPointer<RunLoop> _runLoop;
#endif
};
