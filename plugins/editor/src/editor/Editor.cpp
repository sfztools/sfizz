// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Editor.h"
#include "EditorController.h"
#include "EditIds.h"
#include "GUIComponents.h"
#include "GUIHelpers.h"
#include "GUIPiano.h"
#include "DlgAbout.h"
#include "ImageHelpers.h"
#include "NativeHelpers.h"
#include "VSTGUIHelpers.h"
#include "BitArray.h"
#include "Theme.h"
#include "plugin/MessageUtils.h"
#include <absl/strings/string_view.h>
#include <absl/strings/match.h>
#include <absl/strings/ascii.h>
#include <absl/strings/numbers.h>
#include <ghc/fs_std.hpp>
#include <array>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <system_error>
#include <fstream>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>

#include "utility/vstgui_before.h"
#include "vstgui/vstgui.h"
#include "utility/vstgui_after.h"

using namespace VSTGUI;

const int Editor::viewWidth { 800 };
const int Editor::viewHeight { 475 };

struct Editor::Impl : EditorController::Receiver,
                      public IControlListener,
                      public Theme::ChangeListener
{
    EditorController* ctrl_ = nullptr;
    CFrame* frame_ = nullptr;
    SharedPointer<SFrameDisabler> frameDisabler_;
    SharedPointer<CViewContainer> mainView_;

    std::string currentSfzFile_;
    std::string currentScalaFile_;
    std::string currentThemeName_;
    std::string userFilesDir_;
    std::string fallbackFilesDir_;

    int currentKeyswitch_ = -1;
    std::unordered_map<unsigned, std::string> keyswitchNames_;

    SharedPointer<CVSTGUITimer> memQueryTimer_;

    enum {
        kPanelGeneral,
        kPanelControls,
        kPanelSettings,
        kNumPanels,
    };

    unsigned activePanel_ = 0;
    CViewContainer* subPanels_[kNumPanels] = {};
    STextButton* panelButtons_[kNumPanels] = {};

    enum {
        kTagLoadSfzFile,
        kTagEditSfzFile,
        kTagCreateNewSfzFile,
        kTagOpenSfzFolder,
        kTagPreviousSfzFile,
        kTagNextSfzFile,
        kTagFileOperations,
        kTagSetMainVolume,
        kTagSetNumVoices,
        kTagSetOversampling,
        kTagSetPreloadSize,
        kTagLoadScalaFile,
        kTagResetScalaFile,
        kTagSetScalaRootKey,
        kTagSetTuningFrequency,
        kTagSetStretchedTuning,
        kTagSetSampleQuality,
        kTagSetOscillatorQuality,
        kTagSetCCVolume,
        kTagSetCCPan,
        kTagChooseUserFilesDir,
        kTagAbout,
        kTagThemeMenu,
        kTagFirstChangePanel,
        kTagLastChangePanel = kTagFirstChangePanel + kNumPanels - 1,
    };

    STextButton* sfzFileLabel_ = nullptr;
    CTextLabel* scalaFileLabel_ = nullptr;
    STextButton* scalaFileButton_ = nullptr;
    STextButton* scalaResetButton_ = nullptr;
    CControl *volumeSlider_ = nullptr;
    CTextLabel* volumeLabel_ = nullptr;
    SValueMenu *numVoicesSlider_ = nullptr;
    CTextLabel* numVoicesLabel_ = nullptr;
    SValueMenu *oversamplingSlider_ = nullptr;
    CTextLabel* oversamplingLabel_ = nullptr;
    SValueMenu *preloadSizeSlider_ = nullptr;
    CTextLabel* preloadSizeLabel_ = nullptr;
    SValueMenu *scalaRootKeySlider_ = nullptr;
    SValueMenu *scalaRootOctaveSlider_ = nullptr;
    CTextLabel* scalaRootKeyLabel_ = nullptr;
    SValueMenu *tuningFrequencySlider_ = nullptr;
    CTextLabel* tuningFrequencyLabel_ = nullptr;
    CControl *stretchedTuningSlider_ = nullptr;
    CTextLabel* stretchedTuningLabel_ = nullptr;
    SValueMenu *sampleQualitySlider_ = nullptr;
    SValueMenu *oscillatorQualitySlider_ = nullptr;
    CTextLabel* keyswitchLabel_ = nullptr;
    CTextLabel* keyswitchInactiveLabel_ = nullptr;
    CTextLabel* keyswitchBadge_ = nullptr;
    COptionMenu* themeMenu_ = nullptr;
    std::unique_ptr<Theme> theme_;

    STitleContainer* userFilesGroup_ = nullptr;
    STextButton* userFilesDirButton_ = nullptr;

    CTextLabel* infoCurvesLabel_ = nullptr;
    CTextLabel* infoMastersLabel_ = nullptr;
    CTextLabel* infoGroupsLabel_ = nullptr;
    CTextLabel* infoRegionsLabel_ = nullptr;
    CTextLabel* infoSamplesLabel_ = nullptr;
    CTextLabel* infoVoicesLabel_ = nullptr;

    CViewContainer* imageContainer_ = nullptr;

    CTextLabel* memoryLabel_ = nullptr;

    SActionMenu* fileOperationsMenu_ = nullptr;

    SPiano* piano_ = nullptr;

    SControlsPanel* controlsPanel_ = nullptr;

    SKnobCCBox* volumeCCKnob_ = nullptr;
    SKnobCCBox* panCCKnob_ = nullptr;

    SAboutDialog* aboutDialog_ = nullptr;

    SharedPointer<CBitmap> backgroundBitmap_;
    SharedPointer<CBitmap> defaultBackgroundBitmap_;

    CControl* getSecondaryCCControl(unsigned cc)
    {
        switch (cc) {
        case 7: return volumeCCKnob_ ? volumeCCKnob_->getControl() : nullptr;
        case 10: return panCCKnob_ ? panCCKnob_->getControl() : nullptr;
        default: return nullptr;
        }
    }

    void uiReceiveValue(EditId id, const EditValue& v) override;
    void uiReceiveMessage(const char* path, const char* sig, const sfizz_arg_t* args) override;

    // queued OSC API; sends OSC with intermediate delay between messages
    // to prevent message bursts overloading the buffer
    void sendQueuedOSC(const char* path, const char* sig, const sfizz_arg_t* args);
    void clearQueuedOSC();
    void tickOSCQueue(CVSTGUITimer* timer);
    std::queue<std::string> oscSendQueue_;
    SharedPointer<CVSTGUITimer> oscSendQueueTimer_;

    void createFrameContents();

    template <class Control>
    void adjustMinMaxToEditRange(Control* c, EditId id)
    {
        if (!c)
            return;
        const EditRange er = EditRange::get(id);
        c->setMin(er.min);
        c->setMax(er.max);
        c->setDefaultValue(er.def);
    }

    void chooseSfzFile();
    void createNewSfzFile();
    void changeSfzFile(const std::string& filePath);
    void changeToNextSfzFile(long offset);
    void chooseScalaFile();
    void changeScalaFile(const std::string& filePath);
    void chooseUserFilesDir();
    std::string getFileChooserInitialDir(const std::string& previousFilePath) const;

    static bool scanDirectoryFiles(const fs::path& dirPath, std::function<bool(const fs::path&)> filter, std::vector<fs::path>& fileNames);

    static absl::string_view simplifiedFileName(absl::string_view path, absl::string_view removedSuffix, absl::string_view ifEmpty);

    void updateSfzFileLabel(const std::string& filePath);
    void updateScalaFileLabel(const std::string& filePath);
    void updateUserFilesDirLabel(const std::string& filePath);
    static void updateLabelWithFileName(CTextLabel* label, const std::string& filePath, absl::string_view removedSuffix);
    static void updateButtonWithFileName(STextButton* button, const std::string& filePath, absl::string_view removedSuffix);
    static void updateSButtonWithFileName(STextButton* button, const std::string& filePath, absl::string_view removedSuffix);
    void updateVolumeLabel(float volume);
    void updateNumVoicesLabel(int numVoices);
    void updateOversamplingLabel(int oversamplingLog2);
    void updatePreloadSizeLabel(int preloadSize);
    void updateScalaRootKeyLabel(int rootKey);
    void updateTuningFrequencyLabel(float tuningFrequency);
    void updateStretchedTuningLabel(float stretchedTuning);

    absl::string_view getCurrentKeyswitchName() const;
    void updateKeyswitchNameLabel();

    void updateKeyUsed(unsigned key, bool used);
    void updateKeyLabel(unsigned key, const char* label);
    void updateKeyswitchUsed(unsigned key, bool used);
    void updateCCUsed(unsigned cc, bool used);
    void updateCCValue(unsigned cc, float value);
    void updateCCDefaultValue(unsigned cc, float value);
    void updateCCLabel(unsigned cc, const char* label);
    void updateSWLastCurrent(int sw);
    void updateSWLastLabel(unsigned sw, const char* label);
    void updateBackgroundImage(const char* filepath);
    void updateMemoryUsed(uint64_t mem);

    // edition of CC by UI
    void performCCValueChange(unsigned cc, float value);
    void performCCBeginEdit(unsigned cc);
    void performCCEndEdit(unsigned cc);

    void setActivePanel(unsigned panelId);
    void setupCurrentPanel();
    void applyBackgroundForCurrentPanel();

    static void formatLabel(CTextLabel* label, const char* fmt, ...);
    static void vformatLabel(CTextLabel* label, const char* fmt, va_list ap);

    // IControlListener
    void valueChanged(CControl* ctl) override;
    void enterOrLeaveEdit(CControl* ctl, bool enter);
    void controlBeginEdit(CControl* ctl) override;
    void controlEndEdit(CControl* ctl) override;

    // Theme
    void onThemeChanged() override;
    std::vector<std::function<void()>> OnThemeChanged;

    // Misc
    static std::string getUnicodeNoteName(unsigned key)
    {
        const char* keyNames[12] = {
            u8"C", u8"C♯", u8"D", u8"D♯", u8"E",
            u8"F", u8"F♯", u8"G", u8"G♯", u8"A", u8"A♯", u8"B",
        };
        int octave = static_cast<int>(key / 12) - 1;
        const char* keyName = keyNames[key % 12];
        return std::string(keyName) + ' ' + std::to_string(octave);
    }
};

Editor::Editor(EditorController& ctrl)
    : impl_(new Impl)
{
    Impl& impl = *impl_;

    impl.ctrl_ = &ctrl;

    ctrl.decorate(&impl);

    impl.createFrameContents();
}

Editor::~Editor()
{
    Impl& impl = *impl_;

    close();

    EditorController& ctrl = *impl.ctrl_;
    ctrl.decorate(nullptr);
}

void Editor::open(CFrame& frame)
{
    Impl& impl = *impl_;

    fprintf(stderr, "[sfizz] The resource path of the bundle is %s\n",
            getResourceBasePath().u8string().c_str());

    impl.frame_ = &frame;
    frame.addView(impl.mainView_.get());

    impl.frameDisabler_ = makeOwned<SFrameDisabler>(&frame);

    impl.memQueryTimer_ = makeOwned<CVSTGUITimer>([this](CVSTGUITimer*) {
        impl_->sendQueuedOSC("/mem/buffers", "", nullptr);
    }, 1000, true);

    uint32_t oscSendInterval = 1; // milliseconds
    impl.oscSendQueueTimer_ = makeOwned<CVSTGUITimer>(
        [this](CVSTGUITimer* timer) { impl_->tickOSCQueue(timer); },
        oscSendInterval, false);
}

void Editor::close()
{
    Impl& impl = *impl_;

    impl.clearQueuedOSC();
    impl.oscSendQueueTimer_ = nullptr;

    impl.memQueryTimer_ = nullptr;

    impl.frameDisabler_ = nullptr;

    if (impl.frame_) {
        impl.frame_->removeView(impl.mainView_.get(), false);
        impl.frame_ = nullptr;
    }
}

void Editor::Impl::uiReceiveValue(EditId id, const EditValue& v)
{
    switch (id) {
    case EditId::SfzFile:
        {
            const std::string& value = v.to_string();
            currentSfzFile_ = value;
            updateSfzFileLabel(value);
        }
        break;
    case EditId::Volume:
        {
            const float value = v.to_float();
            if (volumeSlider_)
                volumeSlider_->setValue(value);
            updateVolumeLabel(value);
        }
        break;
    case EditId::Polyphony:
        {
            const int value = static_cast<int>(v.to_float());
            if (numVoicesSlider_)
                numVoicesSlider_->setValue(value);
            updateNumVoicesLabel(value);
        }
        break;
    case EditId::Oversampling:
        {
            const int value = static_cast<int>(v.to_float());

            int log2Value = 0;
            for (int f = value; f > 1; f /= 2)
                ++log2Value;

            if (oversamplingSlider_)
                oversamplingSlider_->setValue(log2Value);
            updateOversamplingLabel(log2Value);
        }
        break;
    case EditId::PreloadSize:
        {
            const int value = static_cast<int>(v.to_float());
            if (preloadSizeSlider_)
                preloadSizeSlider_->setValue(value);
            updatePreloadSizeLabel(value);
        }
        break;
    case EditId::ScalaFile:
        {
            const std::string& value = v.to_string();
            currentScalaFile_ = value;
            updateScalaFileLabel(value);
        }
        break;
    case EditId::ScalaRootKey:
        {
            const int value = std::max(0, static_cast<int>(v.to_float()));
            if (scalaRootKeySlider_)
                scalaRootKeySlider_->setValue(value % 12);
            if (scalaRootOctaveSlider_)
                scalaRootOctaveSlider_->setValue(value / 12);
            updateScalaRootKeyLabel(value);
        }
        break;
    case EditId::TuningFrequency:
        {
            const float value = v.to_float();
            if (tuningFrequencySlider_)
                tuningFrequencySlider_->setValue(value);
            updateTuningFrequencyLabel(value);
        }
        break;
    case EditId::StretchTuning:
        {
            const float value = v.to_float();
            if (stretchedTuningSlider_)
                stretchedTuningSlider_->setValue(value);
            updateStretchedTuningLabel(value);
        }
        break;
    case EditId::SampleQuality:
        {
            const int value = static_cast<int>(v.to_float());
            if (CControl* slider = sampleQualitySlider_) {
                slider->setValue(float(value));
                slider->invalid();
            }
        }
        break;
    case EditId::OscillatorQuality:
        {
            const int value = static_cast<int>(v.to_float());
            if (CControl* slider = oscillatorQualitySlider_) {
                slider->setValue(float(value));
                slider->invalid();
            }
        }
        break;
    case EditId::CanEditUserFilesDir:
        {
            if (STitleContainer* group = userFilesGroup_)
                group->setVisible(v.to_float());
            break;
        }
    case EditId::UserFilesDir:
        {
            userFilesDir_ = v.to_string();
            updateUserFilesDirLabel(userFilesDir_);
            break;
        }
    case EditId::FallbackFilesDir:
        {
            fallbackFilesDir_ = v.to_string();
            break;
        }
    case EditId::PluginFormat:
        aboutDialog_->setPluginFormat(v.to_string());
        break;
    case EditId::PluginHost:
        aboutDialog_->setPluginHost(v.to_string());
        break;
    case EditId::UINumCurves:
        {
            const int value = static_cast<int>(v.to_float());
            if (CTextLabel* label = infoCurvesLabel_)
                formatLabel(label, "%u", value);
        }
        break;
    case EditId::UINumMasters:
        {
            const int value = static_cast<int>(v.to_float());
            if (CTextLabel* label = infoMastersLabel_)
                formatLabel(label, "%u", value);
        }
        break;
    case EditId::UINumGroups:
        {
            const int value = static_cast<int>(v.to_float());
            if (CTextLabel* label = infoGroupsLabel_)
                formatLabel(label, "%u", value);
        }
        break;
    case EditId::UINumRegions:
        {
            const int value = static_cast<int>(v.to_float());
            if (CTextLabel* label = infoRegionsLabel_)
                formatLabel(label, "%u", value);
        }
        break;
    case EditId::UINumPreloadedSamples:
        {
            const int value = static_cast<int>(v.to_float());
            if (CTextLabel* label = infoSamplesLabel_)
                formatLabel(label, "%u", value);
        }
        break;
    case EditId::UINumActiveVoices:
        {
            const int value = static_cast<int>(v.to_float());
            if (CTextLabel* label = infoVoicesLabel_)
                formatLabel(label, "%u", value);
        }
        break;
    case EditId::UIActivePanel:
        {
            const int value = static_cast<int>(v.to_float());
            setActivePanel(value);
        }
        break;
    case EditId::BackgroundImage:
        {
            const std::string& value = v.to_string();
            updateBackgroundImage(value.c_str());
        }
        break;
    default:
        if (editIdIsKey(id)) {
            const int key = keyForEditId(id);
            const float value = v.to_float();
            if (SPiano* piano = piano_)
                piano->setKeyValue(key, value);
        }
        else if (editIdIsKeyUsed(id)) {
            updateKeyUsed(keyUsedForEditId(id), v.to_float() != 0);
        }
        else if (editIdIsKeyLabel(id)) {
            updateKeyLabel(keyLabelForEditId(id), v.to_string().c_str());
        }
        else if (editIdIsKeyswitchUsed(id)) {
            updateKeyswitchUsed(keyswitchUsedForEditId(id), v.to_float() != 0);
        }
        else if (editIdIsKeyswitchLabel(id)) {
            updateSWLastLabel(keyswitchLabelForEditId(id), v.to_string().c_str());
        }
        else if (editIdIsCC(id)) {
            updateCCValue(unsigned(ccForEditId(id)), v.to_float());
        }
        else if (editIdIsCCUsed(id)) {
            bool used = v.to_float() != 0;
            updateCCUsed(ccUsedForEditId(id), used);
        }
        else if (editIdIsCCDefault(id)) {
            updateCCDefaultValue(ccDefaultForEditId(id), v.to_float());
        }
        else if (editIdIsCCLabel(id)) {
            updateCCLabel(ccLabelForEditId(id), v.to_string().c_str());
        }
        break;
    }
}

void Editor::Impl::uiReceiveMessage(const char* path, const char* sig, const sfizz_arg_t* args)
{
    unsigned indices[8];

    if (Messages::matchOSC("/sw/last/current", path, indices) && !strcmp(sig, "i")) {
        updateSWLastCurrent(args[0].i);
    }
    else if (Messages::matchOSC("/sw/last/current", path, indices) && !strcmp(sig, "N")) {
        updateSWLastCurrent(-1);
    }
    else if (Messages::matchOSC("/mem/buffers", path, indices) && !strcmp(sig, "h")) {
        updateMemoryUsed(args[0].h);
    }
    else {
        //fprintf(stderr, "Receive unhandled OSC: %s\n", path);
    }
}

void Editor::Impl::sendQueuedOSC(const char* path, const char* sig, const sfizz_arg_t* args)
{
    if (!frame_)
        return;

    uint32_t oscSize = sfizz_prepare_message(nullptr, 0, path, sig, args);
    std::string oscData(oscSize, '\0');
    sfizz_prepare_message(&oscData[0], oscSize, path, sig, args);
    oscSendQueue_.push(std::move(oscData));
    oscSendQueueTimer_->start();
}

void Editor::Impl::clearQueuedOSC()
{
    while (!oscSendQueue_.empty())
        oscSendQueue_.pop();
}

void Editor::Impl::tickOSCQueue(CVSTGUITimer* timer)
{
    if (oscSendQueue_.empty()) {
        timer->stop();
        return;
    }
    const std::string& msg = oscSendQueue_.front();
    const char* path;
    const char* sig;
    const sfizz_arg_t* args;
    uint8_t buffer[1024];
    if (sfizz_extract_message(msg.data(), msg.size(), buffer, sizeof(buffer), &path, &sig, &args) > 0)
        ctrl_->uiSendMessage(path, sig, args);
    oscSendQueue_.pop();
}

void Editor::Impl::createFrameContents()
{
    CViewContainer* mainView;
    Theme* theme;

    SharedPointer<CBitmap> iconShaded = owned(new CBitmap("logo_text_shaded.png"));
    SharedPointer<CBitmap> background = owned(new CBitmap("background.png"));
    SharedPointer<CBitmap> knob48 = owned(new CBitmap("knob48.png"));
    SharedPointer<CBitmap> logoText = owned(new CBitmap("logo_text.png"));

    defaultBackgroundBitmap_ = background;
    backgroundBitmap_ = background;

    {
        theme = new Theme;
        theme_.reset(theme);

        theme->listener = this;
        OnThemeChanged.clear();
        OnThemeChanged.reserve(128);

        Palette& invertedPalette = theme->invertedPalette;
        Palette& defaultPalette = theme->normalPalette;
        Palette* palette = &defaultPalette;
        auto enterPalette = [&palette](Palette& p) { palette = &p; };

        auto createLogicalGroup = [](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            CViewContainer* container = new CViewContainer(bounds);
            container->setBackgroundColor(CColor(0x00, 0x00, 0x00, 0x00));
            return container;
        };
        auto createRoundedGroup = [this, &palette](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            auto* box =  new SBoxContainer(bounds);
            box->setCornerRadius(10.0);
            OnThemeChanged.push_back([box, palette]() {
                box->setBackgroundColor(palette->boxBackground);
            });
            return box;
        };
        auto createTitleGroup = [this, &palette](const CRect& bounds, int, const char* label, CHoriTxtAlign, int fontsize) {
            auto* box =  new STitleContainer(bounds, label);
            box->setCornerRadius(10.0);
            OnThemeChanged.push_back([box, palette]() {
                box->setBackgroundColor(palette->boxBackground);
                box->setTitleFontColor(palette->titleBoxText);
                box->setTitleBackgroundColor(palette->titleBoxBackground);
            });
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            box->setTitleFont(font);
            return box;
        };
        auto createAboutButton = [this, &iconShaded](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int) {
            return new CKickButton(bounds, this, tag, 0.0f, iconShaded);
        };
        auto createLabel = [this, &palette](const CRect& bounds, int, const char* label, CHoriTxtAlign align, int fontsize) {
            CTextLabel* lbl = new CTextLabel(bounds, label);
            lbl->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            OnThemeChanged.push_back([lbl, palette]() {
                lbl->setFontColor(palette->text);
            });
            lbl->setHoriAlign(align);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            lbl->setFont(font);
            return lbl;
        };
        auto createInactiveLabel = [this, &palette](const CRect& bounds, int, const char* label, CHoriTxtAlign align, int fontsize) {
            CTextLabel* lbl = new CTextLabel(bounds, label);
            lbl->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            OnThemeChanged.push_back([lbl, palette]() {
                lbl->setFontColor(palette->inactiveText);
            });
            lbl->setHoriAlign(align);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            lbl->setFont(font);
            return lbl;
        };
        auto createHLine = [](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            int y = static_cast<int>(0.5 * (bounds.top + bounds.bottom));
            CRect lineBounds(bounds.left, y, bounds.right, y + 1);
            CViewContainer* hline = new CViewContainer(lineBounds);
            hline->setBackgroundColor(CColor(0xff, 0xff, 0xff, 0xff));
            return hline;
        };
        auto createValueLabel = [this, &palette](const CRect& bounds, int, const char* label, CHoriTxtAlign align, int fontsize) {
            CTextLabel* lbl = new CTextLabel(bounds, label);
            lbl->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            OnThemeChanged.push_back([lbl, palette]() {
                lbl->setFontColor(palette->text);
            });
            lbl->setHoriAlign(align);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            lbl->setFont(font);
            return lbl;
        };
        auto createBadge = [this, &palette](const CRect& bounds, int, const char* label, CHoriTxtAlign align, int fontsize) {
            CTextLabel* lbl = new CTextLabel(bounds, label);
            lbl->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            OnThemeChanged.push_back([lbl, palette]() {
                lbl->setBackColor(palette->valueBackground);
                lbl->setFontColor(palette->valueText);
            });
            lbl->setHoriAlign(align);
            lbl->setStyle(CParamDisplay::kRoundRectStyle);
            lbl->setRoundRectRadius(5.0);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            lbl->setFont(font);
            return lbl;
        };
        auto createVMeter = [](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            // TODO the volume meter...
            CViewContainer* container = new CViewContainer(bounds);
            container->setBackgroundColor(CColor(0x00, 0x00, 0x00, 0x00));
            return container;
        };
#if 0
        auto createButton = [this](const CRect& bounds, int tag, const char* label, CHoriTxtAlign align, int fontsize) {
            CTextButton* button = new CTextButton(bounds, this, tag, label);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            button->setFont(font);
            button->setTextAlignment(align);
            return button;
        };
#endif
        auto createClickableLabel = [this, &palette](const CRect& bounds, int tag, const char* label, CHoriTxtAlign align, int fontsize) {
            STextButton* button = new STextButton(bounds, this, tag, label);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            button->setFont(font);
            button->setTextAlignment(align);
            OnThemeChanged.push_back([button, palette]() {
                button->setTextColor(palette->text);
                button->setInactiveColor(palette->inactiveText);
                button->setHighlightColor(palette->highlightedText);
            });
            button->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            button->setFrameColorHighlighted(CColor(0x00, 0x00, 0x00, 0x00));
            SharedPointer<CGradient> gradient = owned(CGradient::create(0.0, 1.0, CColor(0x00, 0x00, 0x00, 0x00), CColor(0x00, 0x00, 0x00, 0x00)));
            button->setGradient(gradient);
            button->setGradientHighlighted(gradient);
            return button;
        };
        auto createValueButton = [this, &palette](const CRect& bounds, int tag, const char* label, CHoriTxtAlign align, int fontsize) {
            STextButton* button = new STextButton(bounds, this, tag, label);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            button->setFont(font);
            button->setTextAlignment(align);
            OnThemeChanged.push_back([button, palette]() {
                button->setTextColor(palette->valueText);
                button->setInactiveColor(palette->inactiveText);
                button->setHighlightColor(palette->highlightedText);
                SharedPointer<CGradient> gradient = owned(CGradient::create(0.0, 1.0, palette->valueBackground, palette->valueBackground));
                button->setGradient(gradient);
                button->setGradientHighlighted(gradient);
            });
            button->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            button->setFrameColorHighlighted(CColor(0x00, 0x00, 0x00, 0x00));
            return button;
        };
        auto createValueMenu = [this, &palette](const CRect& bounds, int tag, const char*, CHoriTxtAlign align, int fontsize) {
            SValueMenu* vm = new SValueMenu(bounds, this, tag);
            vm->setHoriAlign(align);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            vm->setFont(font);
            OnThemeChanged.push_back([vm, palette]() {
                vm->setFontColor(palette->valueText);
                vm->setBackColor(palette->valueBackground);
            });
            vm->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            vm->setStyle(CParamDisplay::kRoundRectStyle);
            vm->setRoundRectRadius(5.0);
            return vm;
        };
        auto createOptionMenu = [this, &palette](const CRect& bounds, int tag, const char*, CHoriTxtAlign align, int fontsize) {
            auto* cb = new COptionMenu(bounds, this, tag);
            cb->setHoriAlign(align);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            cb->setFont(font);
            OnThemeChanged.push_back([cb, palette]() {
                cb->setFontColor(palette->valueText);
                cb->setBackColor(palette->valueBackground);
            });
            cb->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            cb->setStyle(CParamDisplay::kRoundRectStyle);
            cb->setRoundRectRadius(5.0);
            return cb;
        };
        auto createGlyphButton = [this, &palette](UTF8StringPtr glyph, const CRect& bounds, int tag, int fontsize) {
            STextButton* btn = new STextButton(bounds, this, tag, glyph);
            btn->setFont(makeOwned<CFontDesc>("Sfizz Fluent System F20", fontsize));
            OnThemeChanged.push_back([btn, palette]() {
                btn->setTextColor(palette->icon);
                btn->setHighlightColor(palette->iconHighlight);
            });
            btn->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            btn->setFrameColorHighlighted(CColor(0x00, 0x00, 0x00, 0x00));
            btn->setGradient(nullptr);
            btn->setGradientHighlighted(nullptr);
            return btn;
        };
        auto createHomeButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            return createGlyphButton(u8"\ue1d6", bounds, tag, fontsize);
        };
        auto createCCButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            // return createGlyphButton(u8"\ue240", bounds, tag, fontsize);
            return createGlyphButton(u8"\ue253", bounds, tag, fontsize);
        };
        auto createSettingsButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            return createGlyphButton(u8"\ue2e4", bounds, tag, fontsize);
        };
#if 0
        auto createEditFileButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            return createGlyphButton(u8"\ue148", bounds, tag, fontsize);
        };
        auto createLoadFileButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            return createGlyphButton(u8"\ue1a3", bounds, tag, fontsize);
        };
#endif
        auto createPreviousFileButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            return createGlyphButton(u8"\ue0d9", bounds, tag, fontsize);
        };
        auto createNextFileButton = [&createGlyphButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            return createGlyphButton(u8"\ue0da", bounds, tag, fontsize);
        };
        auto createResetSomethingButton = [&createValueButton](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            STextButton* btn = createValueButton(bounds, tag, u8"\ue13a", kCenterText, fontsize);
            btn->setFont(makeOwned<CFontDesc>("Sfizz Fluent System F20", fontsize));
            return btn;
        };
        auto createPiano = [this, &palette](const CRect& bounds, int, const char*, CHoriTxtAlign, int fontsize) {
            SPiano* piano = new SPiano(bounds);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            piano->setFont(font);
            OnThemeChanged.push_back([piano, palette]() {
                piano->setFontColor(palette->text);
                piano->setBackColor(palette->boxBackground);
            });
            return piano;
        };
        auto createChevronDropDown = [this, &palette](const CRect& bounds, int, const char*, CHoriTxtAlign, int fontsize) {
            SActionMenu* menu = new SActionMenu(bounds, this);
            menu->setTitle(u8"\ue0d7");
            menu->setFont(makeOwned<CFontDesc>("Sfizz Fluent System F20", fontsize));
            OnThemeChanged.push_back([menu, palette]() {
                menu->setFontColor(palette->icon);
                menu->setHoverColor(palette->iconHighlight);
            });
            menu->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            menu->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            return menu;
        };
        auto createChevronValueDropDown = [this, &palette](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int fontsize) {
            SValueMenu* menu = new SValueMenu(bounds, this, tag);
            menu->setValueToStringFunction2([](float, std::string& result, CParamDisplay*) -> bool {
                result = u8"\ue0d7";
                return true;
            });
            menu->setFont(makeOwned<CFontDesc>("Sfizz Fluent System F20", fontsize));
            OnThemeChanged.push_back([menu, palette]() {
                menu->setFontColor(palette->icon);
                menu->setHoverColor(palette->iconHighlight);
            });
            menu->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            menu->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            return menu;
        };
        auto createKnob48 = [this, &knob48](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int) {
            return new CAnimKnob(bounds, this, tag, 31, 48, knob48);
        };
        auto createStyledKnob = [this, &palette](const CRect& bounds, int tag, const char*, CHoriTxtAlign, int) {
            SStyledKnob* knob = new SStyledKnob(bounds, this, tag);
            OnThemeChanged.push_back([knob, palette]() {
                knob->setActiveTrackColor(palette->knobActiveTrack);
                knob->setInactiveTrackColor(palette->knobInactiveTrack);
                knob->setLineIndicatorColor(palette->knobLineIndicator);
            });
            return knob;
        };
        auto createKnobCCBox = [this, &palette](const CRect& bounds, int tag, const char* label, CHoriTxtAlign, int fontsize) {
            SKnobCCBox* box = new SKnobCCBox(bounds, this, tag);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            box->setNameLabelText(label);
            box->setNameLabelFont(font);
            box->setKnobFont(font);
            box->setCCLabelText(label);
            box->setCCLabelFont(font);
            OnThemeChanged.push_back([box, palette]() {
                box->setNameLabelFontColor(palette->knobText);
                box->setCCLabelFontColor(palette->knobLabelText);
                box->setCCLabelBackColor(palette->knobLabelBackground);
                box->setKnobFontColor(palette->knobText);
                box->setKnobLineIndicatorColor(palette->knobLineIndicator);
                box->setKnobActiveTrackColor(palette->knobActiveTrack);
                box->setKnobInactiveTrackColor(palette->knobInactiveTrack);
            });
            box->setValueToStringFunction([](float value, std::string& text) -> bool {
                text = std::to_string(std::lround(value * 127));
                return true;
            });
            return box;
        };
        auto createBackground = [&background](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            CViewContainer* container = new CViewContainer(bounds);
            container->setBackground(background);
            return container;
        };
        auto createControlsPanel = [this, &palette](const CRect& bounds, int, const char*, CHoriTxtAlign, int fontsize) {
            auto* panel = new SControlsPanel(bounds);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            panel->setNameLabelFont(font);
            panel->setKnobFont(font);
            panel->setCCLabelFont(font);
            OnThemeChanged.push_back([panel, palette]() {
                panel->setNameLabelFontColor(palette->knobText);
                panel->setCCLabelFontColor(palette->knobLabelText);
                panel->setCCLabelBackColor(palette->knobLabelBackground);
                panel->setKnobFontColor(palette->knobText);
                panel->setKnobLineIndicatorColor(palette->knobLineIndicator);
                panel->setKnobActiveTrackColor(palette->knobActiveTrack);
                panel->setKnobInactiveTrackColor(palette->knobInactiveTrack);
            });
            return panel;
        };

        #include "layout/main.hpp"

        OnThemeChanged.push_back([mainView, theme]() {
            mainView->setBackgroundColor(theme->frameBackground);
        });

#if LINUX
        if (!isZenityAvailable()) {
            CRect bounds = mainView->getViewSize();

            CViewContainer* box = new CViewContainer(bounds);
            mainView->addView(box);
            box->setBackgroundColor(CColor(0x00, 0x00, 0x00, 0xc0));

            CRect textSize = CRect(0, 0, 400, 80).centerInside(bounds);
            CMultiLineTextLabel* textLabel = new CMultiLineTextLabel(textSize);
            box->addView(textLabel);
            textLabel->setTextInset(CPoint(10.0, 10.0));
            textLabel->setStyle(CParamDisplay::kRoundRectStyle);
            textLabel->setRoundRectRadius(10.0);
            textLabel->setFrameColor(CColor(0xb2, 0xb2, 0xb2));
            textLabel->setBackColor(CColor(0x2e, 0x34, 0x36));
            auto font = makeOwned<CFontDesc>("Roboto", 16.0);
            textLabel->setFont(font);
            textLabel->setLineLayout(CMultiLineTextLabel::LineLayout::wrap);
            textLabel->setText(
                "The required program \"zenity\" is missing.\n"
                "Install this software package first, and restart sfizz.");
        }
#endif

        mainView_ = owned(mainView);
    }

    ///
    currentThemeName_ = theme->loadCurrentName();
    theme->load(currentThemeName_);

    ///
    SAboutDialog* aboutDialog = new SAboutDialog(mainView->getViewSize());
    mainView->addView(aboutDialog);
    aboutDialog_ = aboutDialog;
    aboutDialog->setVisible(false);

    ///
    SharedPointer<SFileDropTarget> fileDropTarget = owned(new SFileDropTarget);

    fileDropTarget->setFileDropFunction([this](const std::string& file) {
        changeSfzFile(file);
    });

    mainView_->setDropTarget(fileDropTarget);

    ///
    adjustMinMaxToEditRange(volumeSlider_, EditId::Volume);
    adjustMinMaxToEditRange(numVoicesSlider_, EditId::Polyphony);
    adjustMinMaxToEditRange(oversamplingSlider_, EditId::Oversampling);
    adjustMinMaxToEditRange(preloadSizeSlider_, EditId::PreloadSize);
    if (scalaRootKeySlider_) {
        scalaRootKeySlider_->setMin(0.0);
        scalaRootKeySlider_->setMax(11.0);
        scalaRootKeySlider_->setDefaultValue(
            static_cast<int>(EditRange::get(EditId::ScalaRootKey).def) % 12);
    }
    if (scalaRootOctaveSlider_) {
        scalaRootOctaveSlider_->setMin(0.0);
        scalaRootOctaveSlider_->setMax(10.0);
        scalaRootOctaveSlider_->setDefaultValue(
            static_cast<int>(EditRange::get(EditId::ScalaRootKey).def) / 12);
    }
    adjustMinMaxToEditRange(tuningFrequencySlider_, EditId::TuningFrequency);
    tuningFrequencySlider_->setWheelInc(0.1f / EditRange::get(EditId::TuningFrequency).extent());
    adjustMinMaxToEditRange(stretchedTuningSlider_, EditId::StretchTuning);
    adjustMinMaxToEditRange(sampleQualitySlider_, EditId::SampleQuality);
    adjustMinMaxToEditRange(oscillatorQualitySlider_, EditId::OscillatorQuality);

    for (int value : {1, 2, 4, 8, 16, 32, 64, 96, 128, 160, 192, 224, 256})
        numVoicesSlider_->addEntry(std::to_string(value), value);

    for (int log2value = 0; log2value <= 3; ++log2value) {
        int value = 1 << log2value;
        oversamplingSlider_->addEntry(std::to_string(value) + "x", log2value);
    }
    oversamplingSlider_->setValueToStringFunction2(
        [](float value, std::string& result, CParamDisplay*) -> bool
        {
            result = std::to_string(1 << static_cast<int32_t>(value)) + "x";
            return true;
        });

    for (int log2value = 10; log2value <= 16; ++log2value) {
        int value = 1 << log2value;
        char text[256];
        sprintf(text, "%lu kB", static_cast<unsigned long>(value / 1024 * sizeof(float)));
        text[sizeof(text) - 1] = '\0';
        preloadSizeSlider_->addEntry(text, value);
    }
    preloadSizeSlider_->setValueToStringFunction2(
        [](float value, std::string& result, CParamDisplay*) -> bool
        {
            result = std::to_string(static_cast<int>(std::round(value * (1.0 / 1024 * sizeof(float))))) + " kB";
            return true;
        });

    static const std::pair<float, const char*> tuningFrequencies[] = {
        {380.0f, "English pitchpipe 380 (1720)"},
        {409.0f, "Handel fork 409 (1780)"},
        {415.0f, "Baroque 415"},
        {422.5f, "Handel fork 422.5 (1740)"},
        {423.2f, "Dresden opera 423.2 (1815)"},
        {435.0f, "French Law 435 (1859)"},
        {439.0f, "British Phil 439 (1896)"},
        {440.0f, "International 440"},
        {442.0f, "European 442"},
        {445.0f, "Germany, China 445"},
        {451.0f, "La Scala in Milan 451 (18th)"},
    };

    for (std::pair<float, const char*> value : tuningFrequencies)
        tuningFrequencySlider_->addEntry(value.second, value.first);
    tuningFrequencySlider_->setValueToStringFunction(
        [](float value, char result[256], CParamDisplay*) -> bool
        {
            sprintf(result, "%.1f Hz", value);
            return true;
        });

    static const char* notesInOctave[12] = {
        "C", "C#", "D", "D#", "E",
        "F", "F#", "G", "G#", "A", "A#", "B",
    };
    for (int note = 0; note < 12; ++note)
        scalaRootKeySlider_->addEntry(notesInOctave[note], note);
    for (int octave = 0; octave <= 10; ++octave)
        scalaRootOctaveSlider_->addEntry(std::to_string(octave - 1), octave);
    scalaRootKeySlider_->setValueToStringFunction2(
        [](float value, std::string& result, CParamDisplay*) -> bool
        {
            result = notesInOctave[std::max(0, static_cast<int>(value)) % 12];
            return true;
        });
    scalaRootOctaveSlider_->setValueToStringFunction2(
        [](float value, std::string& result, CParamDisplay*) -> bool
        {
            result = std::to_string(static_cast<int>(value) - 1);
            return true;
        });

    if (SActionMenu* menu = fileOperationsMenu_) {
        menu->addEntry("Load file", kTagLoadSfzFile);
        menu->addEntry("Edit file", kTagEditSfzFile);
        menu->addEntry("Create new file", kTagCreateNewSfzFile);
        menu->addEntry("Open SFZ folder", kTagOpenSfzFolder);
    }

    if (SValueMenu *menu = sampleQualitySlider_) {
        static const std::array<const char*, 11> labels {{
            "Nearest", "Linear", "Polynomial",
            "Sinc 8", "Sinc 12", "Sinc 16", "Sinc 24",
            "Sinc 36", "Sinc 48", "Sinc 60", "Sinc 72",
        }};
        for (size_t i = 0; i < labels.size(); ++i)
            menu->addEntry(labels[i], float(i));
        menu->setValueToStringFunction2([](float value, std::string& result, CParamDisplay*) -> bool {
            int index = int(value);
            if (index < 0 || unsigned(index) >= labels.size())
                return false;
            result = labels[unsigned(index)];
            return true;
        });
    }
    if (SValueMenu *menu = oscillatorQualitySlider_) {
        static const std::array<const char*, 4> labels {{
            "Nearest", "Linear", "High", "Dual-High",
        }};
        for (size_t i = 0; i < labels.size(); ++i)
            menu->addEntry(labels[i], float(i));
        menu->setValueToStringFunction2([](float value, std::string& result, CParamDisplay*) -> bool {
            int index = int(value);
            if (index < 0 || unsigned(index) >= labels.size())
                return false;
            result = labels[unsigned(index)];
            return true;
        });
    }

    if (SPiano* piano = piano_) {
        piano->onKeyPressed = [this](unsigned key, float vel) {
            uint8_t msg[3];
            msg[0] = 0x90;
            msg[1] = static_cast<uint8_t>(key);
            msg[2] = static_cast<uint8_t>(std::max(1, static_cast<int>(vel * 127)));
            ctrl_->uiSendMIDI(msg, sizeof(msg));
        };
        piano->onKeyReleased = [this](unsigned key, float vel) {
            uint8_t msg[3];
            msg[0] = 0x80;
            msg[1] = static_cast<uint8_t>(key);
            msg[2] = static_cast<uint8_t>(vel * 127);
            ctrl_->uiSendMIDI(msg, sizeof(msg));
        };
    }

    if (SControlsPanel* panel = controlsPanel_) {
        panel->ValueChangeFunction = [this](uint32_t cc, float value) {
            performCCValueChange(cc, value);
            updateCCValue(cc, value);
        };
        panel->BeginEditFunction = [this](uint32_t cc) {
            performCCBeginEdit(cc);
        };
        panel->EndEditFunction = [this](uint32_t cc) {
            performCCEndEdit(cc);
        };
    }

    if (SKnobCCBox* box = volumeCCKnob_) {
        unsigned ccNumber = 7;
        box->setCCLabelText(("CC " + std::to_string(ccNumber)).c_str());
    }
    if (SKnobCCBox* box = panCCKnob_) {
        unsigned ccNumber = 10;
        box->setCCLabelText(("CC " + std::to_string(ccNumber)).c_str());
    }

    updateKeyswitchNameLabel();

    ///
    CViewContainer* panel;
    activePanel_ = 0;

    // all panels
    for (unsigned currentPanel = 0; currentPanel < kNumPanels; ++currentPanel) {
        panel = subPanels_[currentPanel];

        if (!panel)
            continue;

        panel->setVisible(currentPanel == activePanel_);
    }

    setupCurrentPanel();

    if (COptionMenu* menu = themeMenu_) {
        const std::vector<std::string>& names = Theme::getAvailableNames();
        size_t index = ~size_t(0);
        for (size_t i = 0, n = names.size(); i < n; ++i) {
            const std::string& name = names[i];
            menu->addEntry(UTF8String(name));
            if (name == currentThemeName_)
                index = i;
        }
        if (index != ~size_t(0))
            menu->setCurrent(index);
    }
}

void Editor::Impl::chooseSfzFile()
{
    SharedPointer<CNewFileSelector> fs = owned(CNewFileSelector::create(frame_));

    fs->setTitle("Load SFZ file");
    fs->addFileExtension(CFileExtension("SFZ", "sfz"));

    // also add extensions of importable files
    fs->addFileExtension(CFileExtension("WAV", "wav"));
    fs->addFileExtension(CFileExtension("FLAC", "flac"));
    fs->addFileExtension(CFileExtension("OGG", "ogg"));
    fs->addFileExtension(CFileExtension("MP3", "mp3"));
    fs->addFileExtension(CFileExtension("AIF", "aif"));
    fs->addFileExtension(CFileExtension("AIFF", "aiff"));
    fs->addFileExtension(CFileExtension("AIFC", "aifc"));
    // Decent samples
    fs->addFileExtension(CFileExtension("DSPRESET", "dspreset"));

    std::string initialDir = getFileChooserInitialDir(currentSfzFile_);
    if (!initialDir.empty())
        fs->setInitialDirectory(initialDir.c_str());

    frameDisabler_->disable();
    bool runOk = fs->runModal();
    frameDisabler_->enable();

    if (runOk) {
        UTF8StringPtr file = fs->getSelectedFile(0);
        if (file)
            changeSfzFile(file);
    }
}

///
static const char defaultSfzText[] =
    "<region>sample=*sine" "\n"
    "ampeg_attack=0.02 ampeg_release=0.1" "\n";

static void createDefaultSfzFileIfNotExisting(const fs::path& path)
{
    if (!fs::exists(path))
        fs::ofstream { path } << defaultSfzText;
}

///
void Editor::Impl::createNewSfzFile()
{
    SharedPointer<CNewFileSelector> fs = owned(CNewFileSelector::create(frame_, CNewFileSelector::kSelectSaveFile));

    fs->setTitle("Create SFZ file");
    fs->addFileExtension(CFileExtension("SFZ", "sfz"));

    std::string initialDir = getFileChooserInitialDir(currentSfzFile_);
    if (!initialDir.empty())
        fs->setInitialDirectory(initialDir.c_str());

    frameDisabler_->disable();
    bool runOk = fs->runModal();
    frameDisabler_->enable();

    if (runOk) {
        UTF8StringPtr file = fs->getSelectedFile(0);
        std::string fileStr;
        if (file && !absl::EndsWithIgnoreCase(file, ".sfz")) {
            fileStr = std::string(file) + ".sfz";
            file = fileStr.c_str();
        }
        if (file) {
            createDefaultSfzFileIfNotExisting(fs::u8path(file));
            changeSfzFile(file);
            openFileInExternalEditor(file);
        }
    }
}

void Editor::Impl::changeSfzFile(const std::string& filePath)
{
    ctrl_->uiSendValue(EditId::SfzFile, filePath);
    currentSfzFile_ = filePath;
    updateSfzFileLabel(filePath);
}

void Editor::Impl::changeToNextSfzFile(long offset)
{
    if (currentSfzFile_.empty())
        return;

    const fs::path filePath = fs::u8path(currentSfzFile_);
    const fs::path dirPath = filePath.parent_path();

    // extract file names of regular files from the sfz directory
    std::vector<fs::path> fileNames;
    fileNames.reserve(64);

    auto fileFilter = [](const fs::path &name) -> bool {
        std::string ext = name.extension().u8string();
        absl::AsciiStrToLower(&ext);
        return ext == ".sfz";
    };

    if (!scanDirectoryFiles(dirPath, fileFilter, fileNames))
        return;

    // sort file names
    const size_t size = fileNames.size();
    if (size == 0)
        return;

    std::sort(fileNames.begin(), fileNames.end());

    // find our current position in the file name list
    size_t currentIndex = 0;
    const fs::path currentFileName = filePath.filename();

    while (currentIndex + 1 < size && fileNames[currentIndex] < currentFileName)
        ++currentIndex;

    // advance to the next or previous item
    typedef typename std::make_signed<size_t>::type signed_size_t;

    size_t newIndex = static_cast<signed_size_t>(currentIndex) + offset;
    if (static_cast<signed_size_t>(newIndex) < 0)
        newIndex = static_cast<signed_size_t>(newIndex) %
            static_cast<signed_size_t>(size) + size;
    newIndex %= size;

    if (newIndex != currentIndex) {
        const fs::path newFilePath = dirPath / fileNames[newIndex];
        changeSfzFile(newFilePath.u8string());
    }
}

void Editor::Impl::chooseScalaFile()
{
    SharedPointer<CNewFileSelector> fs = owned(CNewFileSelector::create(frame_));

    fs->setTitle("Load Scala file");
    fs->addFileExtension(CFileExtension("SCL", "scl"));

    std::string initialDir = getFileChooserInitialDir(currentScalaFile_);
    if (!initialDir.empty())
        fs->setInitialDirectory(initialDir.c_str());

    frameDisabler_->disable();
    bool runOk = fs->runModal();
    frameDisabler_->enable();

    if (runOk) {
        UTF8StringPtr file = fs->getSelectedFile(0);
        if (file)
            changeScalaFile(file);
    }
}

void Editor::Impl::changeScalaFile(const std::string& filePath)
{
    ctrl_->uiSendValue(EditId::ScalaFile, filePath);
    currentScalaFile_ = filePath;
    updateScalaFileLabel(filePath);
}

void Editor::Impl::chooseUserFilesDir()
{
    SharedPointer<CNewFileSelector> fs = owned(
        CNewFileSelector::create(frame_, CNewFileSelector::kSelectDirectory));

    fs->setTitle("Set user files directory");

    frameDisabler_->disable();
    bool runOk = fs->runModal();
    frameDisabler_->enable();

    if (runOk) {
        UTF8StringPtr dir = fs->getSelectedFile(0);
        if (dir) {
            userFilesDir_ = std::string(dir);
            updateUserFilesDirLabel(userFilesDir_);
            ctrl_->uiSendValue(EditId::UserFilesDir, userFilesDir_);
        }
    }
}

std::string Editor::Impl::getFileChooserInitialDir(const std::string& previousFilePath) const
{
    fs::path initialPath;

    if (!previousFilePath.empty())
        initialPath = fs::u8path(previousFilePath).parent_path();
    else if (!userFilesDir_.empty())
        initialPath = fs::u8path(userFilesDir_);
    else if (!fallbackFilesDir_.empty())
        initialPath = fs::u8path(fallbackFilesDir_);

    std::string initialDir = initialPath.u8string();
    if (!initialDir.empty())
        initialDir.push_back('/');

    return initialDir;
}

bool Editor::Impl::scanDirectoryFiles(const fs::path& dirPath, std::function<bool(const fs::path&)> filter, std::vector<fs::path>& fileNames)
{
    std::error_code ec;
    fs::directory_iterator it { dirPath, ec };

    if (ec)
        return false;

    fileNames.clear();

    while (!ec && it != fs::directory_iterator()) {
        const fs::directory_entry& ent = *it;

        std::error_code fileEc;
        const fs::file_status status = ent.status(fileEc);
        if (fileEc)
            continue;

        if (status.type() == fs::file_type::regular) {
            fs::path fileName = ent.path().filename();
            if (!filter || filter(fileName))
                fileNames.push_back(std::move(fileName));
        }

        it.increment(ec);
    }

    if (ec)
        return false;

    return true;
}

absl::string_view Editor::Impl::simplifiedFileName(absl::string_view path, absl::string_view removedSuffix, absl::string_view ifEmpty)
{
    if (path.empty())
        return ifEmpty;

#if defined (_WIN32)
    size_t pos = path.find_last_of("/\\");
#else
    size_t pos = path.rfind('/');
#endif
    path = (pos != path.npos) ? path.substr(pos + 1) : path;

    if (!removedSuffix.empty() && absl::EndsWithIgnoreCase(path, removedSuffix))
        path.remove_suffix(removedSuffix.size());

    return path;
}

void Editor::Impl::updateSfzFileLabel(const std::string& filePath)
{
    updateButtonWithFileName(sfzFileLabel_, filePath, ".sfz");
}

void Editor::Impl::updateScalaFileLabel(const std::string& filePath)
{
    updateLabelWithFileName(scalaFileLabel_, filePath, ".scl");
    updateButtonWithFileName(scalaFileButton_, filePath, ".scl");
}

void Editor::Impl::updateUserFilesDirLabel(const std::string& filePath)
{
    updateButtonWithFileName(userFilesDirButton_, filePath, {});
}

void Editor::Impl::updateLabelWithFileName(CTextLabel* label, const std::string& filePath, absl::string_view removedSuffix)
{
    if (!label)
        return;

    std::string fileName = std::string(simplifiedFileName(filePath, removedSuffix, "<No file>"));
    label->setText(fileName.c_str());
}

void Editor::Impl::updateButtonWithFileName(STextButton* button, const std::string& filePath, absl::string_view removedSuffix)
{
    if (!button)
        return;

    std::string fileName = std::string(simplifiedFileName(filePath, removedSuffix, {}));
    if (!fileName.empty()) {
        button->setTitle(fileName.c_str());
        button->setInactive(false);
    }
    else {
        button->setTitle("No file");
        button->setInactive(true);
    }
}

void Editor::Impl::updateVolumeLabel(float volume)
{
    CTextLabel* label = volumeLabel_;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%.1f dB", volume);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void Editor::Impl::updateNumVoicesLabel(int numVoices)
{
    CTextLabel* label = numVoicesLabel_;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%d", numVoices);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void Editor::Impl::updateOversamplingLabel(int oversamplingLog2)
{
    CTextLabel* label = oversamplingLabel_;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%dx", 1 << oversamplingLog2);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void Editor::Impl::updatePreloadSizeLabel(int preloadSize)
{
    CTextLabel* label = preloadSizeLabel_;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%d kB", static_cast<int>(std::round(preloadSize * (1.0 / 1024))));
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void Editor::Impl::updateScalaRootKeyLabel(int rootKey)
{
    CTextLabel* label = scalaRootKeyLabel_;
    if (!label)
        return;

    static const char *octNoteNames[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
    };

    auto noteName = [](int key) -> std::string
    {
        int octNum;
        int octNoteNum;
        if (key >= 0) {
            octNum = key / 12 - 1;
            octNoteNum = key % 12;
        }
        else {
            octNum = -2 - (key + 1) / -12;
            octNoteNum = (key % 12 + 12) % 12;
        }
        return std::string(octNoteNames[octNoteNum]) + std::to_string(octNum);
    };

    label->setText(noteName(rootKey));
}

void Editor::Impl::updateTuningFrequencyLabel(float tuningFrequency)
{
    CTextLabel* label = tuningFrequencyLabel_;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%.1f", tuningFrequency);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void Editor::Impl::updateStretchedTuningLabel(float stretchedTuning)
{
    CTextLabel* label = stretchedTuningLabel_;
    if (!label)
        return;

    char text[64];
    sprintf(text, "%.3f", stretchedTuning);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

absl::string_view Editor::Impl::getCurrentKeyswitchName() const
{
    int sw = currentKeyswitch_;
    if (sw == -1)
        return {};

    auto it = keyswitchNames_.find(static_cast<unsigned>(sw));
    if (it == keyswitchNames_.end())
        return {};

    return it->second;
}

void Editor::Impl::updateKeyswitchNameLabel()
{
    CTextLabel* label = keyswitchLabel_;
    CTextLabel* badge = keyswitchBadge_;
    CTextLabel* inactiveLabel = keyswitchInactiveLabel_;

    int sw = currentKeyswitch_;
    const std::string name { getCurrentKeyswitchName() };

    if (sw == -1) {
        if (badge)
            badge->setVisible(false);
        if (label)
            label->setVisible(false);
        if (inactiveLabel)
            inactiveLabel->setVisible(true);
    }
    else {
        if (badge) {
            badge->setText(getUnicodeNoteName(sw));
            badge->setVisible(true);
        }
        if (label) {
            label->setText(name.c_str());
            label->setVisible(true);
        }
        if (inactiveLabel)
            inactiveLabel->setVisible(false);
    }
}

void Editor::Impl::updateKeyUsed(unsigned key, bool used)
{
    if (SPiano* piano = piano_)
        piano->setKeyUsed(key, used);
}

void Editor::Impl::updateKeyLabel(unsigned key, const char* label)
{
    // TODO nothing done with this info currently
    (void)key;
    (void)label;
}

void Editor::Impl::updateKeyswitchUsed(unsigned key, bool used)
{
    if (SPiano* piano = piano_)
        piano->setKeyswitchUsed(key, used);
}

void Editor::Impl::updateCCUsed(unsigned cc, bool used)
{
    if (SControlsPanel* panel = controlsPanel_)
        panel->setControlUsed(cc, used);
}

void Editor::Impl::updateCCValue(unsigned cc, float value)
{
    if (SControlsPanel* panel = controlsPanel_)
        panel->setControlValue(cc, value);

    if (CControl* other = getSecondaryCCControl(cc)) {
        other->setValue(value);
        other->invalid();
    }
}

void Editor::Impl::updateCCDefaultValue(unsigned cc, float value)
{
    if (SControlsPanel* panel = controlsPanel_)
        panel->setControlDefaultValue(cc, value);

    if (CControl* other = getSecondaryCCControl(cc))
        other->setDefaultValue(value);
}

void Editor::Impl::updateCCLabel(unsigned cc, const char* label)
{
    if (SControlsPanel* panel = controlsPanel_)
        panel->setControlLabelText(cc, label);
}

void Editor::Impl::updateSWLastCurrent(int sw)
{
    if (currentKeyswitch_ == sw)
        return;
    currentKeyswitch_ = sw;
    updateKeyswitchNameLabel();
}

void Editor::Impl::updateSWLastLabel(unsigned sw, const char* label)
{
    keyswitchNames_[sw].assign(label);
    if ((unsigned)currentKeyswitch_ == sw)
        updateKeyswitchNameLabel();
}

void Editor::Impl::updateBackgroundImage(const char* filepath)
{
    backgroundBitmap_ = loadAnyFormatImage(filepath);

    if (!backgroundBitmap_)
        backgroundBitmap_ = defaultBackgroundBitmap_;

    applyBackgroundForCurrentPanel();
}

void Editor::Impl::setupCurrentPanel()
{
    for (unsigned i = 0; i < kNumPanels; ++i) {
        if (STextButton* button = panelButtons_[i])
            button->setHighlighted(i == activePanel_);
    }

    applyBackgroundForCurrentPanel();
}

void Editor::Impl::applyBackgroundForCurrentPanel()
{
    CBitmap* bitmap;
    if (activePanel_ == kPanelGeneral)
        bitmap = backgroundBitmap_;
    else
        bitmap = defaultBackgroundBitmap_;

    downscaleToWidthAndHeight(bitmap, imageContainer_->getViewSize().getSize());
    imageContainer_->setBackground(bitmap);
}

void Editor::Impl::updateMemoryUsed(uint64_t mem)
{
    if (CTextLabel* label = memoryLabel_) {
        double value = mem / 1e3;
        const char* unit = "kB";
        int precision = 0;
        if (value >= 1e3) {
            value /= 1e3;
            unit = "MB";
        }
        if (value >= 1e3) {
            value /= 1e3;
            unit = "GB";
            precision = 1;
        }
        char textbuf[128];
        snprintf(textbuf, sizeof(textbuf), "%.*f %s", precision, value, unit);
        label->setText(textbuf);
    }
}

void Editor::Impl::performCCValueChange(unsigned cc, float value)
{
    EditorController& ctrl = *ctrl_;
    ctrl.uiSendValue(editIdForCC(int(cc)), value);
}

void Editor::Impl::performCCBeginEdit(unsigned cc)
{
    // TODO(jpc) CC as parameters and automation
    (void)cc;
}

void Editor::Impl::performCCEndEdit(unsigned cc)
{
    // TODO(jpc) CC as parameters and automation
    (void)cc;
}

void Editor::Impl::setActivePanel(unsigned panelId)
{
    panelId = std::max(0, std::min(kNumPanels - 1, static_cast<int>(panelId)));

    if (activePanel_ != panelId) {
        if (subPanels_[activePanel_])
            subPanels_[activePanel_]->setVisible(false);
        if (subPanels_[panelId])
            subPanels_[panelId]->setVisible(true);
        activePanel_ = panelId;
        setupCurrentPanel();
    }
}

void Editor::Impl::formatLabel(CTextLabel* label, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vformatLabel(label, fmt, ap);
    va_end(ap);
}

void Editor::Impl::vformatLabel(CTextLabel* label, const char* fmt, va_list ap)
{
    char text[256];
    vsprintf(text, fmt, ap);
    text[sizeof(text) - 1] = '\0';
    label->setText(text);
}

void Editor::Impl::valueChanged(CControl* ctl)
{
    int32_t tag = ctl->getTag();
    float value = ctl->getValue();
    EditorController& ctrl = *ctrl_;

    switch (tag) {
    case kTagLoadSfzFile:
        if (value != 1)
            break;

        Call::later([this]() { chooseSfzFile(); });
        break;

    case kTagEditSfzFile:
        if (value != 1)
            break;

        if (!currentSfzFile_.empty())
            openFileInExternalEditor(currentSfzFile_.c_str());
        break;

    case kTagCreateNewSfzFile:
        if (value != 1)
            break;

        Call::later([this]() { createNewSfzFile(); });
        break;

    case kTagOpenSfzFolder:
        if (value != 1)
            break;

        if (!userFilesDir_.empty())
            openDirectoryInExplorer(userFilesDir_.c_str());
        else if (!fallbackFilesDir_.empty())
            openDirectoryInExplorer(fallbackFilesDir_.c_str());
        break;

    case kTagPreviousSfzFile:
        if (value != 1)
            break;

        Call::later([this]() { changeToNextSfzFile(-1); });
        break;

    case kTagNextSfzFile:
        if (value != 1)
            break;

        Call::later([this]() { changeToNextSfzFile(+1); });
        break;

    case kTagLoadScalaFile:
        if (value != 1)
            break;

        Call::later([this]() { chooseScalaFile(); });
        break;

    case kTagResetScalaFile:
        if (value != 1)
            break;

        changeScalaFile(std::string());
        break;

    case kTagSetMainVolume:
        ctrl.uiSendValue(EditId::Volume, value);
        updateVolumeLabel(value);
        break;

    case kTagSetCCVolume:
        performCCValueChange(7, value);
        updateCCValue(7, value);
        break;

    case kTagSetCCPan:
        performCCValueChange(10, value);
        updateCCValue(10, value);
        break;

    case kTagSetNumVoices:
        ctrl.uiSendValue(EditId::Polyphony, value);
        updateNumVoicesLabel(static_cast<int>(value));
        break;

    case kTagSetOversampling:
        ctrl.uiSendValue(EditId::Oversampling, static_cast<float>(1 << static_cast<int>(value)));
        updateOversamplingLabel(static_cast<int>(value));
        break;

    case kTagSetPreloadSize:
        ctrl.uiSendValue(EditId::PreloadSize, value);
        updatePreloadSizeLabel(static_cast<int>(value));
        break;

    case kTagSetScalaRootKey:
        {
            if (scalaRootKeySlider_ && scalaRootOctaveSlider_) {
                int key = static_cast<int>(scalaRootKeySlider_->getValue());
                int octave = static_cast<int>(scalaRootOctaveSlider_->getValue());
                int midiKey = key + 12 * octave;
                ctrl.uiSendValue(EditId::ScalaRootKey, midiKey);
                updateScalaRootKeyLabel(midiKey);
            }
        }
        break;

    case kTagSetTuningFrequency:
        ctrl.uiSendValue(EditId::TuningFrequency, value);
        updateTuningFrequencyLabel(value);
        break;

    case kTagSetSampleQuality:
        ctrl.uiSendValue(EditId::SampleQuality, value);
        break;

    case kTagSetOscillatorQuality:
        ctrl.uiSendValue(EditId::OscillatorQuality, value);
        break;

    case kTagSetStretchedTuning:
        ctrl.uiSendValue(EditId::StretchTuning, value);
        updateStretchedTuningLabel(value);
        break;

    case kTagChooseUserFilesDir:
        if (value != 1)
            break;

        Call::later([this]() { chooseUserFilesDir(); });
        break;

    case kTagAbout:
        if (value != 1)
            break;

        Call::later([this]() { aboutDialog_->setVisible(true); });
        break;

    case kTagThemeMenu:
        {
            currentThemeName_ = Theme::getAvailableNames()[int(value)];
            Theme::storeCurrentName(currentThemeName_);
            theme_->load(currentThemeName_);
        }
        break;

    default:
        if (tag >= kTagFirstChangePanel && tag <= kTagLastChangePanel) {
            int panelId = tag - kTagFirstChangePanel;
            ctrl.uiSendValue(EditId::UIActivePanel, static_cast<float>(panelId));
            setActivePanel(panelId);
        }
        break;
    }
}

void Editor::Impl::enterOrLeaveEdit(CControl* ctl, bool enter)
{
    int32_t tag = ctl->getTag();
    EditId id;

    switch (tag) {
    case kTagSetMainVolume: id = EditId::Volume; break;
    case kTagSetNumVoices: id = EditId::Polyphony; break;
    case kTagSetOversampling: id = EditId::Oversampling; break;
    case kTagSetPreloadSize: id = EditId::PreloadSize; break;
    case kTagSetScalaRootKey: id = EditId::ScalaRootKey; break;
    case kTagSetTuningFrequency: id = EditId::TuningFrequency; break;
    case kTagSetStretchedTuning: id = EditId::StretchTuning; break;
    case kTagSetCCVolume: id = editIdForCC(7); break;
    case kTagSetCCPan: id = editIdForCC(10); break;
    default: return;
    }

    EditorController& ctrl = *ctrl_;
    if (enter)
        ctrl.uiBeginSend(id);
    else
        ctrl.uiEndSend(id);
}

void Editor::Impl::controlBeginEdit(CControl* ctl)
{
    enterOrLeaveEdit(ctl, true);
}

void Editor::Impl::controlEndEdit(CControl* ctl)
{
    enterOrLeaveEdit(ctl, false);
}

void Editor::Impl::onThemeChanged()
{
    for (std::function<void()> &function : OnThemeChanged) {
        if (function)
            function();
    }
}
