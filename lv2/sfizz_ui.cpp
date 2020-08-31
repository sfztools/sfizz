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
#include <lv2/ui/ui.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/midi/midi.h>
#include <lv2/patch/patch.h>
#include <lv2/urid/urid.h>
#include <string>
#include <memory>
#include <cstring>
#include <cstdio>

#include "vstgui_helpers.h"

#include "editor/utility/vstgui_before.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/platform/iplatformframe.h"
#if defined(_WIN32)
#include "vstgui/lib/platform/platform_win32.h"
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
    LV2UI_Write_Function write = nullptr;
    LV2UI_Controller con = nullptr;
    LV2_URID_Map *map = nullptr;
    LV2_URID_Unmap *unmap = nullptr;
    LV2UI_Resize *resize = nullptr;
    LV2UI_Touch *touch = nullptr;
    std::unique_ptr<Editor> editor;
    FrameHolder uiFrame;
#if LINUX
    SharedPointer<Lv2IdleRunLoop> runLoop;
#endif

    /// VSTGUIEditorInterface
    CFrame* getFrame() const override { return uiFrame.get(); }

    LV2_Atom_Forge atom_forge;
    LV2_URID atom_event_transfer_uri;
    LV2_URID atom_object_uri;
    LV2_URID atom_path_uri;
    LV2_URID atom_urid_uri;
    LV2_URID midi_event_uri;
    LV2_URID patch_get_uri;
    LV2_URID patch_set_uri;
    LV2_URID patch_property_uri;
    LV2_URID patch_value_uri;
    LV2_URID sfizz_sfz_file_uri;
    LV2_URID sfizz_scala_file_uri;

protected:
    void uiSendValue(EditId id, const EditValue& v) override;
    void uiBeginSend(EditId id) override;
    void uiEndSend(EditId id) override;
    void uiSendMIDI(const uint8_t* msg, uint32_t len) override;

private:
    void uiTouch(EditId id, bool t);
};

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
    }

    // The map feature is required
    if (!map || !unmap)
        return nullptr;

    LV2_Atom_Forge *forge = &self->atom_forge;
    lv2_atom_forge_init(forge, map);
    self->atom_event_transfer_uri = map->map(map->handle, LV2_ATOM__eventTransfer);
    self->atom_object_uri = map->map(map->handle, LV2_ATOM__Object);
    self->atom_path_uri = map->map(map->handle, LV2_ATOM__Path);
    self->atom_urid_uri = map->map(map->handle, LV2_ATOM__URID);
    self->midi_event_uri = map->map(map->handle, LV2_MIDI__MidiEvent);
    self->patch_get_uri = map->map(map->handle, LV2_PATCH__Get);
    self->patch_set_uri = map->map(map->handle, LV2_PATCH__Set);
    self->patch_property_uri = map->map(map->handle, LV2_PATCH__property);
    self->patch_value_uri = map->map(map->handle, LV2_PATCH__value);
    self->sfizz_sfz_file_uri = map->map(map->handle, SFIZZ__sfzFile);
    self->sfizz_scala_file_uri = map->map(map->handle, SFIZZ__tuningfile);

    // set up the resource path
    // * on Linux, this is determined by going 2 folders back from the SO path
    //     name, and appending "Contents/Resources" (not overridable)
    // * on Windows, the folder is set programmatically
    // * on macOS, resource files are looked up using CFBundle APIs
#if defined(_WIN32)
    IWin32PlatformFrame::setResourceBasePath((std::string(bundle_path) + "\\Contents\\Resources\\").c_str());
#elif defined(__APPLE__)
    #pragma message("TODO: make resources work on macOS using bundles")
#endif

    // makes labels refresh correctly
    CView::kDirtyCallAlwaysOnMainThread = true;

    const CRect uiBounds(0, 0, Editor::viewWidth, Editor::viewHeight);
    CFrame* uiFrame = new CFrame(uiBounds, self.get());
    self->uiFrame.reset(uiFrame);

    IPlatformFrameConfig* config = nullptr;
#if LINUX
    SharedPointer<Lv2IdleRunLoop> runLoop = new Lv2IdleRunLoop;
    self->runLoop = runLoop;
    VSTGUI::X11::FrameConfig x11Config;
    x11Config.runLoop = runLoop;
    config = &x11Config;
#endif

    if (!uiFrame->open(parentWindowId, kDefaultNative, config))
        return nullptr;

    Editor *editor = new Editor(*self);
    self->editor.reset(editor);
    editor->open(*uiFrame);

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

                if (prop_uri == self->sfizz_sfz_file_uri && value->type == self->atom_path_uri) {
                    std::string path(value_body, strnlen(value_body, value->size));
                    self->uiReceiveValue(EditId::SfzFile, path);
                }
                else if (prop_uri == self->sfizz_scala_file_uri && value->type == self->atom_path_uri) {
                    std::string path(value_body, strnlen(value_body, value->size));
                    self->uiReceiveValue(EditId::ScalaFile, path);
                }
            }
        }
    }

    (void)buffer_size;
}

static int
idle(LV2UI_Handle ui)
{
    sfizz_ui_t *self = (sfizz_ui_t *)ui;

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
#if LINUX
   VSTGUI::initializeSoHandle();
#endif

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
        alignas(LV2_Atom) uint8_t buffer[MAX_PATH_SIZE + 512];
        auto *atom = reinterpret_cast<const LV2_Atom *>(buffer);
        lv2_atom_forge_set_buffer(forge, (uint8_t *)&buffer, sizeof(buffer));
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
    case EditId::SfzFile:
        sendPath(sfizz_sfz_file_uri, v.to_string());
        break;
    case EditId::ScalaFile:
        sendPath(sfizz_scala_file_uri, v.to_string());
        break;
    default:
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
    default:
        break;
    }
}

void sfizz_ui_t::uiSendMIDI(const uint8_t* msg, uint32_t len)
{
    LV2_Atom_Forge *forge = &atom_forge;
    alignas(LV2_Atom) uint8_t buffer[512];
    auto *atom = reinterpret_cast<const LV2_Atom *>(buffer);
    lv2_atom_forge_set_buffer(forge, (uint8_t *)&buffer, sizeof(buffer));
    if (lv2_atom_forge_atom(forge, len, midi_event_uri) &&
        lv2_atom_forge_write(forge, msg, len))
    {
        write(con, SFIZZ_CONTROL, lv2_atom_total_size(atom), atom_event_transfer_uri, atom);
    }
}
