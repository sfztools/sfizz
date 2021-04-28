// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstController.h"
#include "editor/EditorController.h"
#include "public.sdk/source/vst/vstguieditor.h"
#include "public.sdk/source/common/threadchecker.h"
#include <absl/types/span.h>
#include <mutex>
#include <set>
class Editor;
#if !defined(__APPLE__) && !defined(_WIN32)
namespace VSTGUI { class RunLoop; }
#endif

using namespace Steinberg;
using namespace VSTGUI;

class SfizzVstEditor : public Vst::VSTGUIEditor,
                       public EditorController {
public:
    using Self = SfizzVstEditor;

    SfizzVstEditor(SfizzVstController* controller, absl::Span<FObject*> updates);
    ~SfizzVstEditor();

    bool PLUGIN_API open(void* parent, const VSTGUI::PlatformType& platformType) override;
    void PLUGIN_API close() override;

    SfizzVstController* getController() const
    {
        return static_cast<SfizzVstController*>(Vst::VSTGUIEditor::getController());
    }

    void updateEditorIsOpenParameter();

    // VSTGUIEditor
    CMessageResult notify(CBaseObject* sender, const char* message) override;
    // FObject
    void PLUGIN_API update(FUnknown* changedUnknown, int32 message) override;

    //
private:
    bool processUpdate(FUnknown* changedUnknown, int32 message);
    void processParameterUpdates();
    void updateParameter(Vst::Parameter* parameterToUpdate);

protected:
    // EditorController
    void uiSendValue(EditId id, const EditValue& v) override;
    void uiBeginSend(EditId id) override;
    void uiEndSend(EditId id) override;
    void uiSendMIDI(const uint8_t* data, uint32_t len) override;
    void uiSendMessage(const char* path, const char* sig, const sfizz_arg_t* args) override;

private:
    void loadSfzFile(const std::string& filePath);
    void loadScalaFile(const std::string& filePath);

    Vst::ParamID parameterOfEditId(EditId id);

    std::unique_ptr<Editor> editor_;

#if !defined(__APPLE__) && !defined(_WIN32)
    SharedPointer<RunLoop> _runLoop;
#endif

    // messaging
    std::unique_ptr<uint8[]> oscTemp_;

    // subscribed updates
    std::vector<IPtr<FObject>> updates_;

    // thread safety
    std::unique_ptr<Vst::ThreadChecker> threadChecker_;

    // parameters to process, whose values have received changes
    // Note(jpc) it's because hosts send us parameter updates in the wrong thread..
    std::set<Vst::ParamID> parametersToUpdate_;
    std::mutex parametersToUpdateMutex_;
};
