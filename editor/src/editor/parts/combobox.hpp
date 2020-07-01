// SPDX-License-Identifier: MIT

/*=============================================================================


    Modified include/elements/element/gallery/menu.hpp selection_menu
    to act as a simil-combobox by adding a scrollarea in the menu

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#pragma once
#include "elements.hpp"
#include <functional>

namespace el = cycfi::elements;

namespace sfizz {

namespace detail {

std::pair<el::basic_menu, std::shared_ptr<el::basic_label>>
combo_box(std::string init, el::color color = el::get_theme().default_button_color)
{
    auto btn_text = el::share(el::label(std::move(init)).relative_font_size(1.0));

    auto menu_btn = el::text_button<el::basic_menu>(
        el::layer(
            el::basic_button_body(color),
        el::margin(
            el::get_theme().button_margin,
            el::htile(
                el::align_left(el::hold(btn_text)),
                el::align_right(el::left_margin(12, el::icon(el::icons::down_dir, 1.0)))))));

    menu_btn.position(el::menu_position::bottom_right);
    return { std::move(menu_btn), btn_text };
}

} // namespace detail

template <typename T>
std::pair<el::basic_menu, std::shared_ptr<el::basic_label>>
combo_box(
    std::function<void(std::size_t index)> on_select, std::initializer_list<T> const& list)
{
    struct init_list_menu_selector : el::menu_selector {
        init_list_menu_selector(std::initializer_list<T> const& list_)
            : _list(list_)
        {
        }
        std::size_t
        size() const override
        {
            return _list.size();
        }
        cycfi::string_view
        operator[](std::size_t index) const override
        {
            return *(_list.begin() + index);
        }
        std::initializer_list<T> const& _list;
    };
    return detail::combo_box(on_select, init_list_menu_selector { list });
}

std::pair<el::basic_menu, std::shared_ptr<el::basic_label>>
combo_box(
    std::function<void(std::size_t index)> on_select, el::menu_selector const& items,
    std::function<std::string(std::size_t, cycfi::string_view)> btn_format = {},
    el::color color = el::get_theme().text_box_font_color)
{
    if (!btn_format)
        btn_format = [](std::size_t, cycfi::string_view s) { return std::string(s); };

    auto r = detail::combo_box(
        (items.size() == 0) ? std::string() : btn_format(0, items[0]),
        color);

    if (items.size()) {
        el::vtile_composite list;
        for (std::size_t i = 0; i != items.size(); ++i) {
            auto e = el::menu_item(std::string(items[i]));
            e.on_click = [i, btn_format, btn_text = r.second, on_select, text = items[i]]() {
                btn_text->set_text(btn_format(i, text));
                on_select(i);
            };
            list.push_back(share(e));
        }
        auto menu = el::layer(
            el::vsize(80, el::vscroller(list)),
            el::panel {});

        r.first.menu(menu);
    }
    return r;
}
} // namespace sfizz
