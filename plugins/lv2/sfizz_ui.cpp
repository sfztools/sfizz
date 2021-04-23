/*
  SPDX-License-Identifier: ISC

  Sfizz LV2 plugin

  Copyright 2019-2020, Paul Ferrand <paul@ferrand.cc>

  This file was based on skeleton and example code from the LV2 plugin
  distribution available at http://lv2plug.in/

  The LV2 sample plugins have the following copyright and notice, which are
  extended to the current work:
  Copyright 2011-2016 David Robillard <d@drobilla.net>
  Copyright 2011 Gabriel M. Beddingfield <gabriel@teuton.org>
  Copyright 2011 James Morris <jwm.art.net@gmail.com>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  Compiling this plugin statically against libsndfile implies distributing it
  under the terms of the LGPL v3 license. See the LICENSE.md file for more
  information. If you did not receive a LICENSE.md file, inform the current
  maintainer.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "sfizz_lv2.h"

#include "editor/Editor.h"
#include "editor/EditorController.h"
#include "editor/EditIds.h"
#include "plugin/InstrumentDescription.h"
#include <lv2/ui/ui.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/midi/midi.h>
#include <lv2/patch/patch.h>
#include <lv2/urid/urid.h>
#include <lv2/instance-access/instance-access.h>
#include <ghc/fs_std.hpp>
#include <string>
#include <memory>
#include <cstring>
#include <cstdio>

#include "vstgui_helpers.h"

#include "editor/utility/vstgui_before.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/platform/iplatformframe.h"
#include "vstgui/lib/platform/platformfactory.h"
#if defined(_WIN32)
#include "vstgui/lib/platform/platform_win32.h"
#include "vstgui/lib/platform/win32/win32factory.h"
#endif
#include "editor/utility/vstgui_after.h"
using namespace VSTGUI;

///
struct FrameHolderDeleter {
    void operator()(CFrame* frame) const
    {
        if (frame->getNbReference() != 1)
            frame->forget();
        else
            frame->close();
    }
};
typedef std::unique_ptr<CFrame, FrameHolderDeleter> FrameHolder;

///
struct sfizz_ui_t : EditorController, VSTGUIEditorInterface {
#if LINUX
    SoHandleInitializer soHandleInitializer;
#endif
#if MAC
    BundleRefInitializer bundleRefInitializer;
#endif
    VSTGUIInitializer vstguiInitializer;
    LV2UI_Write_Function write = nullptr;
    LV2UI_Controller con = nullptr;
    LV2_URID_Map *map = nullptr;
    LV2_URID_Unmap *unmap = nullptr;
    LV2UI_Resize *resize = nullptr;
    LV2UI_Touch *touch = nullptr;
    sfizz_plugin_t *plugin = nullptr;
    FrameHolder uiFrame;
    std::unique_ptr<Editor> editor;
#if LINUX
    SharedPointer<Lv2IdleRunLoop> runLoop;
#endif

    /// VSTGUIEditorInterface
    CFrame* getFrame() const override { return uiFrame.get(); }

    LV2_Atom_Forge atom_forge;
    LV2_URID atom_event_transfer_uri;
    LV2_URID atom_object_uri;
    LV2_URID atom_float_uri;
    LV2_URID atom_path_uri;
    LV2_URID atom_urid_uri;
    LV2_URID midi_event_uri;
    LV2_URID patch_get_uri;
    LV2_URID patch_set_uri;
    LV2_URID patch_property_uri;
    LV2_URID patch_value_uri;
    LV2_URID sfizz_sfz_file_uri;
    LV2_URID sfizz_scala_file_uri;
    LV2_URID sfizz_osc_blob_uri;
    std::unique_ptr<sfizz_lv2_ccmap, sfizz_lv2_ccmap_delete> ccmap;

    uint8_t osc_temp[OSC_TEMP_SIZE];
    alignas(LV2_Atom) uint8_t atom_temp[ATOM_TEMP_SIZE];

    int sfz_serial = 0;
    bool valid_sfz_serial = false;

protected:
    void uiSendValue(EditId id, const EditValue& v) override;
    void uiBeginSend(EditId id) override;
    void uiEndSend(EditId id) override;
    void uiSendMIDI(const uint8_t* msg, uint32_t len) override;
    void uiSendMessage(const char* path, const char* sig, const sfizz_arg_t* args) override;

private:
    void uiTouch(EditId id, bool t);
};

#if defined(_WIN32)
static bool fixBundlePath(std::string& path)
{
    // go up some directories until reaching the *.lv2 directory
    bool valid = false;
    while (!valid && !path.empty()) {
        if (path.back() == '\\' || path.back() == '/')
            path.pop_back();
        else if (path.size() > 4 && !memcmp(".lv2", path.data() + path.size() - 4, 4))
            valid = true;
        else {
            path.pop_back();
            while (!path.empty() && path.back() != '\\' && path.back() != '/')
                path.pop_back();
        }
    }
    return valid;
}
#endif

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor,
            const char *plugin_uri,
            const char *bundle_path,
            LV2UI_Write_Function write_function,
            LV2UI_Controller controller,
            LV2UI_Widget *widget,
            const LV2_Feature * const *features)
{
    std::unique_ptr<sfizz_ui_t> self { new sfizz_ui_t };

    (void)descriptor;
    (void)plugin_uri;
    (void)bundle_path;

    self->write = write_function;
    self->con = controller;

    void *parentWindowId = nullptr;

    LV2_URID_Map *map = nullptr;
    LV2_URID_Unmap *unmap = nullptr;

    for (const LV2_Feature *const *f = features; *f; f++)
    {
        if (!strcmp((**f).URI, LV2_URID__map))
            self->map = map = (LV2_URID_Map *)(**f).data;
        else if (!strcmp((**f).URI, LV2_URID__unmap))
            self->unmap = unmap = (LV2_URID_Unmap *)(**f).data;
        else if (!strcmp((**f).URI, LV2_UI__resize))
            self->resize = (LV2UI_Resize *)(**f).data;
        else if (!strcmp((**f).URI, LV2_UI__touch))
            self->touch = (LV2UI_Touch*)(**f).data;
        else if (!strcmp((**f).URI, LV2_UI__parent))
            parentWindowId = (**f).data;
        else if (!strcmp((**f).URI, LV2_INSTANCE_ACCESS_URI))
            self->plugin = (sfizz_plugin_t *)(**f).data;
    }

    // The map feature is required
    if (!map || !unmap)
        return nullptr;

    // The instance-access feature is required
    if (!self->plugin)
        return nullptr;

    LV2_Atom_Forge *forge = &self->atom_forge;
    lv2_atom_forge_init(forge, map);
    self->atom_event_transfer_uri = map->map(map->handle, LV2_ATOM__eventTransfer);
    self->atom_object_uri = map->map(map->handle, LV2_ATOM__Object);
    self->atom_float_uri = map->map(map->handle, LV2_ATOM__Float);
    self->atom_path_uri = map->map(map->handle, LV2_ATOM__Path);
    self->atom_urid_uri = map->map(map->handle, LV2_ATOM__URID);
    self->midi_event_uri = map->map(map->handle, LV2_MIDI__MidiEvent);
    self->patch_get_uri = map->map(map->handle, LV2_PATCH__Get);
    self->patch_set_uri = map->map(map->handle, LV2_PATCH__Set);
    self->patch_property_uri = map->map(map->handle, LV2_PATCH__property);
    self->patch_value_uri = map->map(map->handle, LV2_PATCH__value);
    self->sfizz_sfz_file_uri = map->map(map->handle, SFIZZ__sfzFile);
    self->sfizz_scala_file_uri = map->map(map->handle, SFIZZ__tuningfile);
    self->sfizz_osc_blob_uri = map->map(map->handle, SFIZZ__OSCBlob);
    self->ccmap.reset(sfizz_lv2_ccmap_create(map));

    // set up the resource path
    // * on Linux, this is determined by going 2 folders back from the SO path
    //     name, and appending "Contents/Resources" (not overridable)
    // * on Windows, the folder is set programmatically
    // * on macOS, resource files are looked up using CFBundle APIs

#if defined(_WIN32)
    // some hosts give us the DLL path instead of the bundle path,
    // so we have to work around that.
    std::string realBundlePath { bundle_path };
    if (!fixBundlePath(realBundlePath))
        return nullptr;

    const Win32Factory* winFactory = VSTGUI::getPlatformFactory().asWin32Factory();
    std::string resourcePath = realBundlePath + "\\Contents\\Resources\\";
    winFactory->setResourceBasePath(resourcePath.c_str());
#endif

    // makes labels refresh correctly
    CView::kDirtyCallAlwaysOnMainThread = true;

    const CRect uiBounds(0, 0, Editor::viewWidth, Editor::viewHeight);
    CFrame* uiFrame = new CFrame(uiBounds, self.get());
    self->uiFrame.reset(uiFrame);

    IPlatformFrameConfig* config = nullptr;
#if LINUX
    SharedPointer<Lv2IdleRunLoop> runLoop = owned(new Lv2IdleRunLoop);
    self->runLoop = runLoop;
    VSTGUI::X11::FrameConfig x11Config;
    x11Config.runLoop = runLoop;
    config = &x11Config;
#endif

    if (!uiFrame->open(parentWindowId, PlatformType::kDefaultNative, config))
        return nullptr;

    Editor *editor = new Editor(*self);
    self->editor.reset(editor);
    editor->open(*uiFrame);

    // let the editor know about plugin format
    self->uiReceiveValue(EditId::PluginFormat, std::string("LV2"));

    // user files dir is not relevant to LV2 (not yet?)
    // LV2 has its own path management mechanism
    self->uiReceiveValue(EditId::CanEditUserFilesDir, 0);

    *widget = reinterpret_cast<LV2UI_Widget>(uiFrame->getPlatformFrame()->getPlatformRepresentation());

    if (self->resize)
        self->resize->ui_resize(self->resize->handle, Editor::viewWidth, Editor::viewHeight);

    // send a request to receive all parameters
    uint8_t buffer[256];
    lv2_atom_forge_set_buffer(forge, buffer, sizeof(buffer));
    LV2_Atom_Forge_Frame frame;
    LV2_Atom *msg = (LV2_Atom *)lv2_atom_forge_object(forge, &frame, 0, self->patch_get_uri);
    lv2_atom_forge_pop(forge, &frame);
    write_function(controller, 0, lv2_atom_total_size(msg), self->atom_event_transfer_uri, msg);

    return self.release();
}

static void
cleanup(LV2UI_Handle ui)
{
    sfizz_ui_t *self = (sfizz_ui_t *)ui;
    delete self;
}

static void
port_event(LV2UI_Handle ui,
           uint32_t port_index,
           uint32_t buffer_size,
           uint32_t format,
           const void *buffer)
{
    sfizz_ui_t *self = (sfizz_ui_t *)ui;

    if (format == 0) {
        const float v = *reinterpret_cast<const float*>(buffer);

        switch (port_index) {
        case SFIZZ_VOLUME:
            self->uiReceiveValue(EditId::Volume, v);
            break;
        case SFIZZ_POLYPHONY:
            self->uiReceiveValue(EditId::Polyphony, v);
            break;
        case SFIZZ_OVERSAMPLING:
            self->uiReceiveValue(EditId::Oversampling, v);
            break;
        case SFIZZ_PRELOAD:
            self->uiReceiveValue(EditId::PreloadSize, v);
            break;
        case SFIZZ_SCALA_ROOT_KEY:
            self->uiReceiveValue(EditId::ScalaRootKey, v);
            break;
        case SFIZZ_TUNING_FREQUENCY:
            self->uiReceiveValue(EditId::TuningFrequency, v);
            break;
        case SFIZZ_STRETCH_TUNING:
            self->uiReceiveValue(EditId::StretchTuning, v);
            break;
        case SFIZZ_SAMPLE_QUALITY:
            self->uiReceiveValue(EditId::SampleQuality, v);
            break;
        case SFIZZ_OSCILLATOR_QUALITY:
            self->uiReceiveValue(EditId::OscillatorQuality, v);
            break;
        case SFIZZ_ACTIVE_VOICES:
            self->uiReceiveValue(EditId::UINumActiveVoices, v);
            break;
        case SFIZZ_NUM_CURVES:
            self->uiReceiveValue(EditId::UINumCurves, v);
            break;
        case SFIZZ_NUM_MASTERS:
            self->uiReceiveValue(EditId::UINumMasters, v);
            break;
        case SFIZZ_NUM_GROUPS:
            self->uiReceiveValue(EditId::UINumGroups, v);
            break;
        case SFIZZ_NUM_REGIONS:
            self->uiReceiveValue(EditId::UINumRegions, v);
            break;
        case SFIZZ_NUM_SAMPLES:
            self->uiReceiveValue(EditId::UINumPreloadedSamples, v);
            break;
        }
    }
    else if (format == self->atom_event_transfer_uri) {
        auto *atom = reinterpret_cast<const LV2_Atom *>(buffer);

        if (atom->type == self->atom_object_uri) {
            const LV2_Atom *prop = nullptr;
            const LV2_Atom *value = nullptr;

            lv2_atom_object_get(
                reinterpret_cast<const LV2_Atom_Object *>(atom),
                self->patch_property_uri, &prop, self->patch_value_uri, &value, 0);

            if (prop && value && prop->type == self->atom_urid_uri) {
                const LV2_URID prop_uri = reinterpret_cast<const LV2_Atom_URID *>(prop)->body;
                auto *value_body = reinterpret_cast<const char *>(LV2_ATOM_BODY_CONST(value));

                int cc = sfizz_lv2_ccmap_unmap(self->ccmap.get(), prop_uri);

                if (cc != -1) {
                    if (value->type == self->atom_float_uri) {
                        float ccvalue = *reinterpret_cast<const float*>(value_body);
                        self->uiReceiveValue(editIdForCC(cc), ccvalue);
                    }
                }
                else if (prop_uri == self->sfizz_sfz_file_uri && value->type == self->atom_path_uri) {
                    std::string path(value_body, strnlen(value_body, value->size));
                    self->uiReceiveValue(EditId::SfzFile, path);
                }
                else if (prop_uri == self->sfizz_scala_file_uri && value->type == self->atom_path_uri) {
                    std::string path(value_body, strnlen(value_body, value->size));
                    self->uiReceiveValue(EditId::ScalaFile, path);
                }
            }
        }
        else if (atom->type == self->sfizz_osc_blob_uri) {
            const char *path;
            const char *sig;
            const sfizz_arg_t *args;
            uint8_t buffer[1024];
            if (sfizz_extract_message(LV2_ATOM_BODY_CONST(atom), atom->size, buffer, sizeof(buffer), &path, &sig, &args) > 0)
                self->uiReceiveMessage(path, sig, args);
        }
    }

    (void)buffer_size;
}

static void
sfizz_ui_update_description(sfizz_ui_t *self, const InstrumentDescription& desc)
{
    self->uiReceiveValue(EditId::UINumCurves, desc.numCurves);
    self->uiReceiveValue(EditId::UINumMasters, desc.numMasters);
    self->uiReceiveValue(EditId::UINumGroups, desc.numGroups);
    self->uiReceiveValue(EditId::UINumRegions, desc.numRegions);
    self->uiReceiveValue(EditId::UINumPreloadedSamples, desc.numSamples);

    const fs::path rootPath = fs::u8path(desc.rootPath);
    const fs::path imagePath = rootPath / fs::u8path(desc.image);
    self->uiReceiveValue(EditId::BackgroundImage, imagePath.u8string());

    for (unsigned key = 0; key < 128; ++key) {
        bool keyUsed = desc.keyUsed.test(key);
        bool keyswitchUsed = desc.keyswitchUsed.test(key);
        self->uiReceiveValue(editIdForKeyUsed(key), float(keyUsed));
        self->uiReceiveValue(editIdForKeyswitchUsed(key), float(keyswitchUsed));
        if (keyUsed)
            self->uiReceiveValue(editIdForKeyLabel(key), desc.keyLabel[key]);
        if (keyswitchUsed)
            self->uiReceiveValue(editIdForKeyswitchLabel(key), desc.keyswitchLabel[key]);
    }

    for (unsigned cc = 0; cc < sfz::config::numCCs; ++cc) {
        bool ccUsed = desc.ccUsed.test(cc);
        self->uiReceiveValue(editIdForCCUsed(cc), float(ccUsed));
        if (ccUsed) {
            self->uiReceiveValue(editIdForCCDefault(cc), desc.ccDefault[cc]);
            self->uiReceiveValue(editIdForCCLabel(cc), desc.ccLabel[cc]);
        }
    }
}

static void
sfizz_ui_check_sfz_update(sfizz_ui_t *self)
{
    uint8_t *data = nullptr;
    uint32_t size = 0;
    int new_serial = 0;
    const int *serial = self->valid_sfz_serial ? &self->sfz_serial : nullptr;

    bool update = sfizz_lv2_fetch_description(
        self->plugin, serial, &data, &size, &new_serial);

    if (update) {
        std::unique_ptr<uint8_t[]> cleanup(data);
        self->sfz_serial = new_serial;
        self->valid_sfz_serial = true;

        const InstrumentDescription desc = parseDescriptionBlob(
            absl::string_view(reinterpret_cast<char*>(data), size));
        sfizz_ui_update_description(self, desc);
    }
}

static int
idle(LV2UI_Handle ui)
{
    sfizz_ui_t *self = (sfizz_ui_t *)ui;

    // check if there are news regarding the current SFZ
    sfizz_ui_check_sfz_update(self);

#if LINUX
   self->runLoop->execIdle();
#else
   (void)self;
#endif

    return 0;
}

static const LV2UI_Idle_Interface idle_interface = {
    &idle,
};

static int
show(LV2UI_Handle ui)
{
    sfizz_ui_t *self = (sfizz_ui_t *)ui;
    self->uiFrame->setVisible(true);
    return 0;
}

static int
hide(LV2UI_Handle ui)
{
    sfizz_ui_t *self = (sfizz_ui_t *)ui;
    self->uiFrame->setVisible(false);
    return 0;
}

static const LV2UI_Show_Interface show_interface = {
    &show,
    &hide,
};

const void *
extension_data(const char *uri)
{
    if (!strcmp(uri, LV2_UI__idleInterface))
        return &idle_interface;

    if (!strcmp(uri, LV2_UI__showInterface))
        return &show_interface;

    return nullptr;
}

static const LV2UI_Descriptor descriptor = {
    SFIZZ_UI_URI,
    instantiate,
    cleanup,
    port_event,
    extension_data,
};

LV2_SYMBOL_EXPORT
const LV2UI_Descriptor *
lv2ui_descriptor(uint32_t index)
{
    switch (index)
    {
    case 0:
        return &descriptor;
    default:
        return nullptr;
    }
}

///
void sfizz_ui_t::uiSendValue(EditId id, const EditValue& v)
{
    auto sendFloat = [this](int port, float value) {
        write(con, port, sizeof(float), 0, &value);
    };

    auto sendPath = [this](LV2_URID property, const std::string& value) {
        LV2_Atom_Forge *forge = &atom_forge;
        LV2_Atom_Forge_Frame frame;
        auto *atom = reinterpret_cast<const LV2_Atom *>(atom_temp);
        lv2_atom_forge_set_buffer(forge, atom_temp, sizeof(atom_temp));
        if (lv2_atom_forge_object(forge, &frame, 0, patch_set_uri) &&
            lv2_atom_forge_key(forge, patch_property_uri) &&
            lv2_atom_forge_urid(forge, property) &&
            lv2_atom_forge_key(forge, patch_value_uri) &&
            lv2_atom_forge_path(forge, value.data(), value.size()))
        {
            lv2_atom_forge_pop(forge, &frame);
            write(con, SFIZZ_CONTROL, lv2_atom_total_size(atom), atom_event_transfer_uri, atom);
        }
    };

    auto sendController = [this](LV2_URID property, float value) {
        LV2_Atom_Forge *forge = &atom_forge;
        LV2_Atom_Forge_Frame frame;
        auto *atom = reinterpret_cast<const LV2_Atom *>(atom_temp);
        lv2_atom_forge_set_buffer(forge, atom_temp, sizeof(atom_temp));
        if (lv2_atom_forge_object(forge, &frame, 0, patch_set_uri) &&
            lv2_atom_forge_key(forge, patch_property_uri) &&
            lv2_atom_forge_urid(forge, property) &&
            lv2_atom_forge_key(forge, patch_value_uri) &&
            lv2_atom_forge_float(forge, value))
        {
            lv2_atom_forge_pop(forge, &frame);
            write(con, SFIZZ_CONTROL, lv2_atom_total_size(atom), atom_event_transfer_uri, atom);
        }
    };

    switch (id) {
    case EditId::Volume:
        sendFloat(SFIZZ_VOLUME, v.to_float());
        break;
    case EditId::Polyphony:
        sendFloat(SFIZZ_POLYPHONY, v.to_float());
        break;
    case EditId::Oversampling:
        sendFloat(SFIZZ_OVERSAMPLING, v.to_float());
        break;
    case EditId::PreloadSize:
        sendFloat(SFIZZ_PRELOAD, v.to_float());
        break;
    case EditId::ScalaRootKey:
        sendFloat(SFIZZ_SCALA_ROOT_KEY, v.to_float());
        break;
    case EditId::TuningFrequency:
        sendFloat(SFIZZ_TUNING_FREQUENCY, v.to_float());
        break;
    case EditId::StretchTuning:
        sendFloat(SFIZZ_STRETCH_TUNING, v.to_float());
        break;
    case EditId::SampleQuality:
        sendFloat(SFIZZ_SAMPLE_QUALITY, v.to_float());
        break;
    case EditId::OscillatorQuality:
        sendFloat(SFIZZ_OSCILLATOR_QUALITY, v.to_float());
        break;
    case EditId::SfzFile:
        sendPath(sfizz_sfz_file_uri, v.to_string());
        break;
    case EditId::ScalaFile:
        sendPath(sfizz_scala_file_uri, v.to_string());
        break;
    default:
        if (editIdIsCC(id)) {
            int cc = ccForEditId(id);
            LV2_URID urid = sfizz_lv2_ccmap_map(ccmap.get(), cc);
            sendController(urid, v.to_float());
        }
        break;
    }
}

void sfizz_ui_t::uiBeginSend(EditId id)
{
    uiTouch(id, true);
}

void sfizz_ui_t::uiEndSend(EditId id)
{
    uiTouch(id, false);
}

void sfizz_ui_t::uiTouch(EditId id, bool t)
{
    if (!touch)
        return;

    switch (id) {
    case EditId::Volume:
        touch->touch(touch->handle, SFIZZ_VOLUME, t);
        break;
    case EditId::Polyphony:
        touch->touch(touch->handle, SFIZZ_POLYPHONY, t);
        break;
    case EditId::Oversampling:
        touch->touch(touch->handle, SFIZZ_OVERSAMPLING, t);
        break;
    case EditId::PreloadSize:
        touch->touch(touch->handle, SFIZZ_PRELOAD, t);
        break;
    case EditId::ScalaRootKey:
        touch->touch(touch->handle, SFIZZ_SCALA_ROOT_KEY, t);
        break;
    case EditId::TuningFrequency:
        touch->touch(touch->handle, SFIZZ_TUNING_FREQUENCY, t);
        break;
    case EditId::StretchTuning:
        touch->touch(touch->handle, SFIZZ_STRETCH_TUNING, t);
        break;
    case EditId::SampleQuality:
        touch->touch(touch->handle, SFIZZ_SAMPLE_QUALITY, t);
        break;
    case EditId::OscillatorQuality:
        touch->touch(touch->handle, SFIZZ_OSCILLATOR_QUALITY, t);
        break;
    default:
        break;
    }
}

void sfizz_ui_t::uiSendMIDI(const uint8_t* msg, uint32_t len)
{
    LV2_Atom_Forge *forge = &atom_forge;
    auto *atom = reinterpret_cast<const LV2_Atom *>(atom_temp);
    lv2_atom_forge_set_buffer(forge, atom_temp, sizeof(atom_temp));
    if (lv2_atom_forge_atom(forge, len, midi_event_uri) &&
        lv2_atom_forge_write(forge, msg, len))
    {
        write(con, SFIZZ_CONTROL, lv2_atom_total_size(atom), atom_event_transfer_uri, atom);
    }
}

void sfizz_ui_t::uiSendMessage(const char* path, const char* sig, const sfizz_arg_t* args)
{
    uint8_t *osc_temp = this->osc_temp;
    uint32_t osc_size = sfizz_prepare_message(osc_temp, OSC_TEMP_SIZE, path, sig, args);

    if (osc_size > OSC_TEMP_SIZE)
        return;

    LV2_Atom_Forge *forge = &atom_forge;
    auto *atom = reinterpret_cast<const LV2_Atom *>(atom_temp);
    lv2_atom_forge_set_buffer(forge, atom_temp, sizeof(atom_temp));

    if (lv2_atom_forge_atom(forge, osc_size, sfizz_osc_blob_uri) &&
        lv2_atom_forge_raw(forge, osc_temp, osc_size))
    {
        write(con, SFIZZ_CONTROL, lv2_atom_total_size(atom), atom_event_transfer_uri, atom);
    }
}
