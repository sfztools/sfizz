/*=============================================================================


    Modified include/elements/element/gallery/menu.hpp selection_menu
    to act as a simil-combobox by adding a scrollarea in the menu

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#pragma once

#include "elements.hpp"
#include <functional>

namespace el = cycfi::elements;

std::pair<el::basic_menu, std::shared_ptr<el::basic_label>>
combo_box(std::string init)
{
    auto btn_text = el::share(el::label(std::move(init)).relative_font_size(1.0));

    auto menu_btn = el::text_button<el::basic_menu>(
        el::margin(
            el::get_theme().button_margin,
            el::htile(
                el::align_left(el::hold(btn_text)),
                el::align_right(el::left_margin(12, el::icon(el::icons::down_dir, 1.0))))));

    menu_btn.position(el::menu_position::bottom_right);
    return { std::move(menu_btn), btn_text };
}
template <typename T>
std::pair<el::basic_menu, std::shared_ptr<el::basic_label>>
combo_box(
    std::function<void(cycfi::string_view item)> on_select, std::initializer_list<T> const& list)
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
    return combo_box(on_select, init_list_menu_selector { list });
}
std::pair<el::basic_menu, std::shared_ptr<el::basic_label>>
combo_box(
    std::function<void(cycfi::string_view item)> on_select, el::menu_selector const& items)
{
    auto r = combo_box(items.size() ? std::string(items[0]) : "");

    if (items.size()) {
        el::vtile_composite list;
        for (std::size_t i = 0; i != items.size(); ++i) {
            auto e = el::menu_item(std::string(items[i]));
            e.on_click = [btn_text = r.second, on_select, text = items[i]]() {
                btn_text->set_text(text);
                on_select(text);
            };
            list.push_back(share(e));
        }
        auto menu = el::layer(
            el::vsize(114, el::vscroller(list)),
            el::panel {});

        r.first.menu(menu);
    }
    return r;
}
