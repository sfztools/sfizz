// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "PageSettings.h"
#include "parts/formatters.hpp"
#include "parts/dials.hpp"
#include "parts/misc.hpp"
#include "native/FileDialog.h"
#include <combobox.hpp>

struct PageSettings::Impl {
    PageSettings* self_ = nullptr;

    void init();
    el::basic_menu makeScalaCenterMenu();
    el::basic_menu makeScalaTuningMenu();
    void askToChooseScalaFile();

    el::element_ptr contents_;
    std::shared_ptr<FileDialog> fileDialog_;
    std::shared_ptr<el::basic_input_box> txtScala_;
    value_dial_ptr knbStretchTuning_;

    std::shared_ptr<el::basic_label> lblScalaRootKey_;
    std::shared_ptr<el::basic_label> lblTuningFrequency_;
};

///
struct TuningItem {
    float value;
    cycfi::string_view name;
};

static const TuningItem tuningItems[] = {
    { 380.0, "English pitchpipe 380 (1720)"},
    { 409.0, "Handel fork1 409 (1780)"},
    { 415.0, "Baroque 415"},
    { 422.5, "Handel fork 422.5 (1740)"},
    { 423.2, "Dresden opera 423.2 (1815)"},
    { 435.0, "French Law 435 (1859)"},
    { 439.0, "British Phil 439 (1896)"},
    { 440.0, "International 440"},
    { 442.0, "European 442"},
    { 445.0, "Germany}, China 445"},
    { 451.0, "La Scala in Milan 451 (18th)"},
};

static constexpr std::size_t numTuningItems =
    sizeof(tuningItems) / sizeof(tuningItems[0]);

///
static std::string midiKeyNumberToName(int key)
{
    static const char *octNoteNames[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
    };
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
}

///

PageSettings::PageSettings(el::view& view_)
    : impl_(new Impl)
{
    impl_->self_ = this;
    impl_->init();
}

PageSettings::~PageSettings()
{
}

const el::element& PageSettings::subject() const
{
    return *impl_->contents_;
}

el::element& PageSettings::subject()
{
    return *impl_->contents_;
}

void PageSettings::updateScalaFile(cycfi::string_view v)
{
    impl_->txtScala_->value(v);
}

void PageSettings::updateScalaRootKey(float v)
{
    impl_->lblScalaRootKey_->set_text(midiKeyNumberToName(static_cast<int>(v)));
}

void PageSettings::updateTuningFrequency(float v)
{
    for (const TuningItem& t : tuningItems) {
        if (t.value == v) {
            impl_->lblTuningFrequency_->set_text(t.name);
            return;
        }
    }
    impl_->lblTuningFrequency_->set_text(strprintf(256, "%.1f Hz", v));
}

void PageSettings::updateStretchTuning(float v)
{
    impl_->knbStretchTuning_->value(v);
}

void PageSettings::Impl::init()
{
    auto ibScala = el::input_box(""); // FIXME: "Select a scala file..." inputbox is messed up
    txtScala_ = ibScala.second;

    const float dialScale = 1.0 / 4;
    auto dialSprite = el::share(el::sprite { "knob.png", 128 * dialScale, dialScale });

    knbStretchTuning_ = value_dial(el::hold(dialSprite), { 0.0, 1.0 });
    knbStretchTuning_->formatter(
        [f = create_integer_printf_formatter("%d %%")](double v) -> std::string {
            return f(static_cast<int>(std::lround(100 * v)));
        });
    knbStretchTuning_->on_change = [this](double v) { self_->on_change_stretch_tuning(v); };

    el::layered_button btnScala = el::button("...");

    btnScala.on_click = [this](bool) { askToChooseScalaFile(); };

    auto scalaGroup = el::group("Scala Tuning",
                            el::margin({ 10, 10, 10, 10 },
                                el::top_margin(26,
                                    el::vtile(
                                        el::htile(
                                            ibScala.first,
                                            el::left_margin(10,
                                                el::hsize(30,
                                                    btnScala))),
                                        el::top_margin(10,
                                            el::htile(
                                                makeScalaCenterMenu(),
                                                el::left_margin(10,
                                                    makeScalaTuningMenu()),
                                                el::left_margin(10,
                                                    top_labeled("Stretch",
                                                        el::hold(knbStretchTuning_)))))))));

    contents_
        = el::share(
            el::layer(
                el::margin({ 10, 10, 10, 10 },
                    el::vtile(
                        std::move(scalaGroup)))));
}

el::basic_menu PageSettings::Impl::makeScalaCenterMenu()
{
    static const std::vector<int> keyList = []() {
        std::vector<int> keys;
        keys.reserve(32);
        keys.push_back(33);
        for (int key = 60; key <= 84; ++key)
            keys.push_back(key);
        return keys;
    }();

    static const std::vector<std::string> keyNames = []() {
        std::vector<std::string> names;
        names.resize(keyList.size());
        for (size_t i = 0, n = names.size(); i < n; ++i)
            names[i] = midiKeyNumberToName(keyList[i]);
        return names;
    }();

    //
    struct selector : el::menu_selector {
        selector(const std::vector<std::string>& items) : items_(items) {}
        std::size_t size() const override { return items_.size(); }
        cycfi::string_view operator[](std::size_t index) const override { return items_[index]; }
        const std::vector<std::string>& items_;
    };

    //
    auto popup = sfizz::combo_box(
        [this](std::size_t index) {
            if (self_->on_change_scala_root_key)
                self_->on_change_scala_root_key(keyList[index]);
        },
        selector(keyNames)
    );

    lblScalaRootKey_ = popup.second;
    return std::move(popup.first);
}

el::basic_menu PageSettings::Impl::makeScalaTuningMenu()
{
    struct selector : el::menu_selector {
        selector(const TuningItem* items, std::size_t size) : items_(items), size_(size) {}
        std::size_t size() const override { return size_; }
        cycfi::string_view operator[](std::size_t index) const override { return items_[index].name; }
        const TuningItem* items_ {};
        size_t size_ {};
    };

    auto popup = sfizz::combo_box(
        [this](std::size_t index) {
            if (self_->on_change_tuning_frequency)
                self_->on_change_tuning_frequency(tuningItems[index].value);
        },
        selector(tuningItems, numTuningItems)
    );

    lblTuningFrequency_ = popup.second;
    return std::move(popup.first);
}

void PageSettings::Impl::askToChooseScalaFile()
{
    FileDialog* dlg = fileDialog_.get();
    if (dlg)
        return;

    dlg = new FileDialog;
    fileDialog_.reset(dlg);

    dlg->setMode(FileDialog::Mode::Open);
    dlg->setTitle("Open a scala file...");
    dlg->addFilter(FileDialog::Filter { "Scala Files", { "*.scl" } });

    dlg->onFileChosen = [this](absl::string_view fileName) {
         if (!fileName.empty()) {
             txtScala_->set_text(std::string(fileName));
             if (self_->on_change_scala_file)
                 self_->on_change_scala_file(fileName);
         }
         fileDialog_.reset();
    };

    if (!dlg->chooseFile())
        fileDialog_.reset();
}
