// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstController.h"
#include "editor/EditorController.h"
#include "public.sdk/source/vst/vstguieditor.h"
#include <absl/types/span.h>
#include <mutex>
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

    SfizzVstEditor(
        SfizzVstController* controller,
        absl::Span<FObject*> continuousUpdates,
        absl::Span<FObject*> triggerUpdates);
    ~SfizzVstEditor();

    bool PLUGIN_API open(void* parent, const VSTGUI::PlatformType& platformType) override;
    void PLUGIN_API close() override;

    SfizzVstController* getController() const
    {
        return static_cast<SfizzVstController*>(Vst::VSTGUIEditor::getController());
    }

    // VSTGUIEditor
    CMessageResult notify(CBaseObject* sender, const char* message) override;
    // FObject
    void PLUGIN_API update(FUnknown* changedUnknown, int32 message) override;

    //
    void updateState(const SfizzVstState& state);
    void updatePlayState(const SfizzPlayState& playState);

private:
    void processOscQueue();
    void processNoteEventQueue();

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

    // editor state
    // note: might be updated from a non-UI thread
    typedef std::vector<uint8_t> OscByteVec;
    std::unique_ptr<OscByteVec> oscQueue_;
    std::mutex oscQueueMutex_;
    typedef std::vector<std::pair<uint32, float>> NoteEventsVec;
    std::unique_ptr<NoteEventsVec> noteEventQueue_;
    std::mutex noteEventQueueMutex_;

    // subscribed updates
    std::vector<IPtr<FObject>> continuousUpdates_;
    std::vector<IPtr<FObject>> triggerUpdates_;
};
