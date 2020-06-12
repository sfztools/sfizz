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
#include <lv2/urid/urid.h>
#include <string>
#include <memory>
#include <cstring>
#include <cstdio>

struct sfizz_ui_t : EditorController {
    LV2UI_Write_Function write = nullptr;
    LV2UI_Controller con = nullptr;
    LV2_URID_Map *map = nullptr;
    LV2UI_Resize *resize = nullptr;
    LV2UI_Touch *touch = nullptr;
    std::unique_ptr<Editor> editor;

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

    for (const LV2_Feature *const *f = features; *f; f++)
    {
        if (!strcmp((**f).URI, LV2_URID__map))
            self->map = (LV2_URID_Map *)(**f).data;
        else if (!strcmp((**f).URI, LV2_UI__resize))
            self->resize = (LV2UI_Resize *)(**f).data;
        else if (!strcmp((**f).URI, LV2_UI__touch))
            self->touch = (LV2UI_Touch*)(**f).data;
        else if (!strcmp((**f).URI, LV2_UI__parent))
            parentWindowId = (**f).data;
    }

    // The map feature is required
    if (!self->map)
        return nullptr;

    Res::initializeRootPath((std::string(bundle_path) + "/Resources").c_str());

    Editor *editor = new Editor(*self);
    self->editor.reset(editor);

    if (!editor->open(parentWindowId))
        return nullptr;

    *widget = reinterpret_cast<LV2UI_Widget>(editor->getNativeWindowId());

    if (self->resize)
        self->resize->ui_resize(self->resize->handle, Editor::fixedWidth, Editor::fixedHeight);

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
    else {
        (void)buffer_size;
    }
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
    // TODO
    switch (id) {
    case EditId::SfzFile:
        
        break;
    case EditId::ScalaFile:
        
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
    // TODO
}
