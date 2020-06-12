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

#include "editor/Editor.h"
#include "editor/EditorController.h"
#include "editor/Res.h"
#include <lv2/ui/ui.h>
#include <lv2/urid/urid.h>
#include <string>
#include <memory>
#include <cstring>
#include <cstdio>

#define SFIZZ_UI_URI "http://sfztools.github.io/sfizz#ui"

struct sfizz_ui_t : EditorController {
    LV2_URID_Map *map = nullptr;
    LV2UI_Resize *resize = nullptr;
    std::unique_ptr<Editor> editor;

protected:
    void uiSendNumber(EditId id, float v) override;
    void uiSendString(EditId id, absl::string_view v) override;
    void uiBeginSend(EditId id) override;
    void uiEndSend(EditId id) override;
    void uiSendMIDI(const uint8_t* msg, uint32_t len) override;
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

    void *parentWindowId = nullptr;

    for (const LV2_Feature *const *f = features; *f; f++)
    {
        if (!strcmp((**f).URI, LV2_URID__map))
            self->map = (LV2_URID_Map *)(**f).data;
        else if (!strcmp((**f).URI, LV2_UI__resize))
            self->resize = (LV2UI_Resize *)(**f).data;
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
    // TODO
    switch (id) {
        
    default:
        break;
    }
}

void sfizz_ui_t::uiSendString(EditId id, absl::string_view v)
{
    // TODO
    switch (id) {
        
    default:
        break;
    }
}

void sfizz_ui_t::uiBeginSend(EditId id)
{
    // TODO
    switch (id) {
        
    default:
        break;
    }
}

void sfizz_ui_t::uiEndSend(EditId id)
{
    // TODO
    switch (id) {
        
    default:
        break;
    }
}

void sfizz_ui_t::uiSendMIDI(const uint8_t* msg, uint32_t len)
{
    // TODO
}
