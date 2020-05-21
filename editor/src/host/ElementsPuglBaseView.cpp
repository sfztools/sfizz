// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "PuglHelpers.h"
#include "NativeWindowHelpers.h"
#include <elements.hpp>
#include <pugl/pugl_cairo.h>

namespace cycfi {
namespace elements {

struct PuglHostView {
    SingletonPuglWorld_s world;
    PuglView_u view;
    bool ready = false;
    point cursorPosition {};
    int buttonState = 0;
    mouse_button::what dragButton = static_cast<mouse_button::what>(-1);
};

static int convertModifiers(PuglMods mods)
{
    int modifiers = 0;
    if (mods & PUGL_MOD_SHIFT)
        modifiers |= mod_shift;
    if (mods & PUGL_MOD_CTRL)
        modifiers |= mod_control;
    if (mods & PUGL_MOD_ALT)
        modifiers |= mod_alt;
    if (mods & PUGL_MOD_SUPER)
        modifiers |= mod_super;
    return modifiers;
}

static mouse_button::what convertButton(uint32_t button)
{
    switch (button) {
    case 1: return mouse_button::left;
    case 2: return mouse_button::middle;
    case 3: return mouse_button::right;
    default: return static_cast<mouse_button::what>(-1);
    }
}

static PuglStatus onEvent(PuglView *view, const PuglEvent *event)
{
    //fprintf(stderr, "Base view Event: %d\n", event->type);

    base_view* self = reinterpret_cast<base_view*>(puglGetHandle(view));
    PuglHostView* host = reinterpret_cast<PuglHostView*>(self->host());

    if (!host || !host->ready)
        return PUGL_SUCCESS;

    switch (event->type) {
    default:
        break;

    case PUGL_MAP:
        puglGrabFocus(view);
        break;

    case PUGL_BUTTON_PRESS:
    case PUGL_BUTTON_RELEASE:
        {
            if (event->type == PUGL_BUTTON_PRESS)
                puglGrabFocus(view);

            mouse_button b;

            b.state = convertButton(event->button.button);
            if (b.state == static_cast<mouse_button::what>(-1))
                break;

            b.down = event->type == PUGL_BUTTON_PRESS;
            b.num_clicks = 1; // TODO double click
            b.modifiers = convertModifiers(event->button.state);
            b.pos.x = event->button.x;
            b.pos.y = event->button.y;

            if (b.down) {
                host->buttonState |= 1 << b.state;
                host->dragButton = b.state;
            }
            else {
                host->buttonState &= ~(1 << b.state);
                host->dragButton = static_cast<mouse_button::what>(-1);
            }

            self->click(b);
        }
        break;

    case PUGL_MOTION:
         {
             host->cursorPosition.x = event->motion.x;
             host->cursorPosition.y = event->motion.y;

             mouse_button b;

             b.num_clicks = 1; // TODO double click
             b.modifiers = convertModifiers(event->motion.state);
             b.pos.x = event->motion.x;
             b.pos.y = event->motion.y;

             b.state = host->dragButton;
             b.down = b.state != static_cast<mouse_button::what>(-1);

             if (b.down)
                 self->drag(b);
             else
                 self->cursor(host->cursorPosition, cursor_tracking::hovering);
         }
         break;

    case PUGL_SCROLL:
        {
            point delta;
            point origin;

            delta.x = event->scroll.dx;
            delta.y = event->scroll.dy;
            origin.x = event->scroll.x;
            origin.y = event->scroll.y;

            self->scroll(delta, origin);
        }
        break;

    case PUGL_KEY_PRESS:
    case PUGL_KEY_RELEASE:
        {
            key_info k;

            k.modifiers = convertModifiers(event->key.state);

            if (event->type == PUGL_KEY_PRESS)
                k.action = key_action::press;
            else if (event->type == PUGL_KEY_PRESS)
                k.action = key_action::release;

            auto translate_pugl_key = [](uint32_t unicode, uint32_t /*keycode*/) -> key_code {
                if (unicode >= 32 && unicode <= 126) {
                    uint32_t upper = unicode;
                    if (unicode >= 97 && unicode <= 122)
                        upper = unicode - 97 + 64;
                    return static_cast<key_code>(upper);
                }

                switch (unicode) {
                case PUGL_KEY_BACKSPACE: return key_code::backspace;
                case PUGL_KEY_ESCAPE: return key_code::escape;
                case PUGL_KEY_DELETE: return key_code::_delete;
                case PUGL_KEY_F1: return key_code::f1;
                case PUGL_KEY_F2: return key_code::f2;
                case PUGL_KEY_F3: return key_code::f3;
                case PUGL_KEY_F4: return key_code::f4;
                case PUGL_KEY_F5: return key_code::f5;
                case PUGL_KEY_F6: return key_code::f6;
                case PUGL_KEY_F7: return key_code::f7;
                case PUGL_KEY_F8: return key_code::f8;
                case PUGL_KEY_F9: return key_code::f9;
                case PUGL_KEY_F10: return key_code::f10;
                case PUGL_KEY_F11: return key_code::f11;
                case PUGL_KEY_F12: return key_code::f12;
                case PUGL_KEY_LEFT: return key_code::left;
                case PUGL_KEY_UP: return key_code::up;
                case PUGL_KEY_RIGHT: return key_code::right;
                case PUGL_KEY_DOWN: return key_code::down;
                case PUGL_KEY_PAGE_UP: return key_code::page_up;
                case PUGL_KEY_PAGE_DOWN: return key_code::page_down;
                case PUGL_KEY_HOME: return key_code::home;
                case PUGL_KEY_END: return key_code::end;
                case PUGL_KEY_INSERT: return key_code::insert;
                case PUGL_KEY_SHIFT_L: return key_code::left_shift;
                case PUGL_KEY_SHIFT_R: return key_code::right_shift;
                case PUGL_KEY_CTRL_L: return key_code::left_control;
                case PUGL_KEY_CTRL_R: return key_code::right_control;
                case PUGL_KEY_ALT_L: return key_code::left_alt;
                case PUGL_KEY_ALT_R: return key_code::right_alt;
                case PUGL_KEY_SUPER_L: return key_code::left_super;
                case PUGL_KEY_SUPER_R: return key_code::right_super;
                case PUGL_KEY_MENU: return key_code::menu;
                case PUGL_KEY_CAPS_LOCK: return key_code::caps_lock;
                case PUGL_KEY_SCROLL_LOCK: return key_code::scroll_lock;
                case PUGL_KEY_NUM_LOCK: return key_code::num_lock;
                case PUGL_KEY_PRINT_SCREEN: return key_code::print_screen;
                case PUGL_KEY_PAUSE: return key_code::pause;
                }

                return key_code::unknown;
            };

            k.key = translate_pugl_key(event->key.key, event->key.keycode);

            self->key(k);
        }
        break;

    case PUGL_EXPOSE:
        {
            cairo_t* cr = reinterpret_cast<cairo_t*>(puglGetContext(view));
            cairo_save(cr);

            rect dirty;
            dirty.left = event->expose.x;
            dirty.top = event->expose.y;
            dirty.right = dirty.left + event->expose.width;
            dirty.bottom = dirty.top + event->expose.height;

            self->draw(cr, dirty);

            cairo_restore(cr);
        }
        break;
    }

    return PUGL_SUCCESS;
}

static PuglHostView* makeWindow(base_view* self, void* parentWindowId, PuglRect frame)
{
    std::unique_ptr<PuglHostView> host { new PuglHostView };

    SingletonPuglWorld_s world = SingletonPuglWorld::instance();
    if (!world)
        return nullptr;
    host->world = world;

    PuglView *view = puglNewView(world->get());
    if (!view)
        return nullptr;
    host->view.reset(view);
    puglSetHandle(view, self);

    if (puglSetEventFunc(view, &onEvent) != PUGL_SUCCESS)
        return nullptr;

    if (puglSetBackend(view, puglCairoBackend()) != PUGL_SUCCESS)
        return nullptr;

    if (puglSetDefaultSize(view, frame.width, frame.height) != PUGL_SUCCESS)
        return nullptr;

    if (puglSetFrame(view, frame) != PUGL_SUCCESS)
        return nullptr;

    if (puglSetParentWindow(view, reinterpret_cast<PuglNativeView>(parentWindowId)) != PUGL_SUCCESS)
        return nullptr;

    if (puglRealize(view) != PUGL_SUCCESS)
        return nullptr;

    puglShowWindow(view);

    return host.release();
}

static base_view* gCurrentBaseView = nullptr;

base_view::base_view(extent size_)
{
    PuglRect frame;
    frame.x = 0;
    frame.y = 0;
    frame.width = size_.x;
    frame.height = size_.y;
    _view = makeWindow(this, nullptr, frame);
    static_cast<PuglHostView*>(_view)->ready = true;
}

base_view::base_view(host_window_handle h)
{
    PuglRect frame = NativeWindows::getFrame(h);
    frame.x = 0;
    frame.y = 0;
    _view = makeWindow(this, h, frame);
    static_cast<PuglHostView*>(_view)->ready = true;
}

base_view::~base_view()
{
    if (gCurrentBaseView == this)
        gCurrentBaseView = nullptr;

    PuglHostView* host = reinterpret_cast<PuglHostView*>(this->host());
    delete host;
}

point base_view::cursor_pos() const
{
    PuglHostView* host = reinterpret_cast<PuglHostView*>(this->host());
    if (!host || !host->view)
        return {};

    return host->cursorPosition;
}

extent base_view::size() const
{
    PuglHostView* host = reinterpret_cast<PuglHostView*>(this->host());
    if (!host || !host->view)
        return extent();

    PuglRect frame = puglGetFrame(host->view.get());
    return point(frame.width, frame.height);
}

void base_view::size(extent p)
{
    PuglHostView* host = reinterpret_cast<PuglHostView*>(this->host());
    if (!host || !host->view)
        return;

    PuglRect frame = puglGetFrame(host->view.get());
    frame.width = p.x;
    frame.height = p.y;
    puglSetFrame(host->view.get(), frame);
}

void base_view::refresh()
{
    PuglHostView* host = reinterpret_cast<PuglHostView*>(this->host());
    if (!host || !host->view)
        return;

    puglPostRedisplay(host->view.get());
}

void base_view::refresh(rect area)
{
    PuglHostView* host = reinterpret_cast<PuglHostView*>(this->host());
    if (!host || !host->view)
        return;

    PuglRect rect;
    rect.x = area.left;
    rect.y = area.top;
    rect.width = area.right - area.left;
    rect.height = area.bottom - area.top;
    puglPostRedisplayRect(host->view.get(), rect);
}

std::string clipboard()
{
    base_view* self = gCurrentBaseView;
    if (!self)
        return {};

    PuglHostView* host = reinterpret_cast<PuglHostView*>(self->host());
    if (!host || !host->view)
        return {};

    const char* type = nullptr;
    size_t size = 0;
    const char* data = reinterpret_cast<const char*>(
        puglGetClipboard(host->view.get(), &type, &size));

    if (!data || !type || strcmp(type, "text/plain") != 0)
        return {};

    return std::string(data, size);
}

void clipboard(std::string const& text)
{
    base_view* self = gCurrentBaseView;
    if (!self)
        return;

    PuglHostView* host = reinterpret_cast<PuglHostView*>(self->host());
    if (!host || !host->view)
        return;

    puglSetClipboard(host->view.get(), nullptr, text.data(), text.size());
}

void set_cursor(cursor_type type)
{
    #pragma message("TODO(pugl) not implemented")
}

///

void show_window(base_view& view)
{
    PuglHostView* host = reinterpret_cast<PuglHostView*>(view.host());
    if (!host || !host->view)
        return;

    puglShowWindow(host->view.get());
    puglGrabFocus(host->view.get());
}

void hide_window(base_view& view)
{
    PuglHostView* host = reinterpret_cast<PuglHostView*>(view.host());
    if (!host || !host->view)
        return;

    puglHideWindow(host->view.get());
}

void process_events(base_view& view)
{
    SingletonPuglWorld_s world = SingletonPuglWorld::instance();

    gCurrentBaseView = &view;
    view.poll();
    puglUpdate(world->get(), 0.0);
    gCurrentBaseView = nullptr;
}

} // namespace cycfi
} // namespace elements
