// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstController.h"
#include "public.sdk/source/vst/vstguieditor.h"

using namespace Steinberg;
using namespace VSTGUI;

class SfizzVstEditor : public Vst::VSTGUIEditor, public IControlListener, public SfizzVstController::StateListener {
public:
    explicit SfizzVstEditor(void *controller);
    ~SfizzVstEditor();

    bool PLUGIN_API open(void* parent, const VSTGUI::PlatformType& platformType = VSTGUI::kDefaultNative) override;
    void PLUGIN_API close() override;

    // IControlListener
    void valueChanged(CControl* ctl) override;

    // SfizzVstController::StateListener
    void onStateChanged() override;

private:
    void chooseSfzFile();
    void loadSfzFile(const std::string& filePath);

    void createFrameContents();
    void updateStateDisplay();

    enum {
        kTagLoadSfzFile,
    };

    CTextLabel* _fileLabel = nullptr;
};
