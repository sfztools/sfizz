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
#include "editor/Res.h"
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

struct sfizz_ui_t : EditorController {
    LV2UI_Write_Function write = nullptr;
    LV2UI_Controller con = nullptr;
    LV2_URID_Map *map = nullptr;
    LV2_URID_Unmap *unmap = nullptr;
    LV2UI_Resize *resize = nullptr;
    LV2UI_Touch *touch = nullptr;
    std::unique_ptr<Editor> editor;

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
    void uiSendNumber(EditId id, float v) override;
    void uiSendString(EditId id, absl::string_view v) override;
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
    self->sfizz_scala_file_uri = map->map(map->handle, SFIZZ__scalaFile);

    Res::initializeRootPath((std::string(bundle_path) + "/Resources").c_str());

    Editor *editor = new Editor(*self);
    self->editor.reset(editor);

    if (!editor->open(parentWindowId))
        return nullptr;

    *widget = reinterpret_cast<LV2UI_Widget>(editor->getNativeWindowId());

    if (self->resize)
        self->resize->ui_resize(self->resize->handle, Editor::fixedWidth, Editor::fixedHeight);

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
            self->uiReceiveNumber(EditId::Volume, v);
            break;
        case SFIZZ_POLYPHONY:
            self->uiReceiveNumber(EditId::Polyphony, v);
            break;
        case SFIZZ_OVERSAMPLING:
            self->uiReceiveNumber(EditId::Oversampling, v);
            break;
        case SFIZZ_PRELOAD:
            self->uiReceiveNumber(EditId::PreloadSize, v);
            break;
        case SFIZZ_SCALA_ROOT_KEY:
            self->uiReceiveNumber(EditId::ScalaRootKey, v);
            break;
        case SFIZZ_TUNING_FREQUENCY:
            self->uiReceiveNumber(EditId::TuningFrequency, v);
            break;
        case SFIZZ_STRETCH_TUNING:
            self->uiReceiveNumber(EditId::StretchTuning, v);
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
                    self->uiReceiveString(EditId::SfzFile, path);
                }
                else if (prop_uri == self->sfizz_scala_file_uri && value->type == self->atom_path_uri) {
                    std::string path(value_body, strnlen(value_body, value->size));
                    self->uiReceiveString(EditId::ScalaFile, path);
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
    if (!self->editor->isOpen())
        return 1;

    self->editor->processEvents();
    return 0;
}

static const LV2UI_Idle_Interface idle_interface = {
    &idle,
};

static int
show(LV2UI_Handle ui)
{
    sfizz_ui_t *self = (sfizz_ui_t *)ui;
    self->editor->show();
    return 0;
}

static int
hide(LV2UI_Handle ui)
{
    sfizz_ui_t *self = (sfizz_ui_t *)ui;
    self->editor->hide();
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
void sfizz_ui_t::uiSendNumber(EditId id, float v)
{
    switch (id) {
    case EditId::Volume:
        write(con, SFIZZ_VOLUME, sizeof(float), 0, &v);
        break;
    case EditId::Polyphony:
        write(con, SFIZZ_POLYPHONY, sizeof(float), 0, &v);
        break;
    case EditId::Oversampling:
        write(con, SFIZZ_OVERSAMPLING, sizeof(float), 0, &v);
        break;
    case EditId::PreloadSize:
        write(con, SFIZZ_PRELOAD, sizeof(float), 0, &v);
        break;
    case EditId::ScalaRootKey:
        write(con, SFIZZ_SCALA_ROOT_KEY, sizeof(float), 0, &v);
        break;
    case EditId::TuningFrequency:
        write(con, SFIZZ_TUNING_FREQUENCY, sizeof(float), 0, &v);
        break;
    case EditId::StretchTuning:
        write(con, SFIZZ_STRETCH_TUNING, sizeof(float), 0, &v);
        break;
    default:
        break;
    }
}

void sfizz_ui_t::uiSendString(EditId id, absl::string_view v)
{
    switch (id) {
    case EditId::SfzFile:
        {
            LV2_Atom_Forge *forge = &atom_forge;
            LV2_Atom_Forge_Frame frame;
            alignas(LV2_Atom) uint8_t buffer[MAX_PATH_SIZE + 512];
            auto *atom = reinterpret_cast<const LV2_Atom *>(buffer);
            lv2_atom_forge_set_buffer(forge, (uint8_t *)&buffer, sizeof(buffer));
            if (lv2_atom_forge_object(forge, &frame, 0, patch_set_uri) &&
                lv2_atom_forge_key(forge, patch_property_uri) &&
                lv2_atom_forge_urid(forge, sfizz_sfz_file_uri) &&
                lv2_atom_forge_key(forge, patch_value_uri) &&
                lv2_atom_forge_path(forge, v.data(), v.size()))
            {
                lv2_atom_forge_pop(forge, &frame);
                write(con, SFIZZ_CONTROL, lv2_atom_total_size(atom), atom_event_transfer_uri, atom);
            }
        }
        break;
    case EditId::ScalaFile:
        {
            LV2_Atom_Forge *forge = &atom_forge;
            LV2_Atom_Forge_Frame frame;
            alignas(LV2_Atom) uint8_t buffer[MAX_PATH_SIZE + 512];
            auto *atom = reinterpret_cast<const LV2_Atom *>(buffer);
            lv2_atom_forge_set_buffer(forge, (uint8_t *)&buffer, sizeof(buffer));
            if (lv2_atom_forge_object(forge, &frame, 0, patch_set_uri) &&
                lv2_atom_forge_key(forge, patch_property_uri) &&
                lv2_atom_forge_urid(forge, sfizz_scala_file_uri) &&
                lv2_atom_forge_key(forge, patch_value_uri) &&
                lv2_atom_forge_path(forge, v.data(), v.size()))
            {
                lv2_atom_forge_pop(forge, &frame);
                write(con, SFIZZ_CONTROL, lv2_atom_total_size(atom), atom_event_transfer_uri, atom);
            }
        }
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
