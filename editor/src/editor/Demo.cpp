// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Demo.h"

constexpr el::color DemoKnobsAndSliders::bkd_color;

DemoKnobsAndSliders::DemoKnobsAndSliders(el::view& group)
{
    group.content(
        hold(make_controls()),
        background
        );
    link_controls(group);
}

template <bool is_vertical>
el::element_ptr DemoKnobsAndSliders::make_markers()
{
    auto track = el::basic_track<5, is_vertical>();
    return share(el::slider_labels<10>(
       el::slider_marks<40>(track),         // Track with marks
       0.8,                             // Label font size (relative size)
       "0", "1", "2", "3", "4",         // Labels
       "5", "6", "7", "8", "9", "10"
    ));
}

el::element_ptr DemoKnobsAndSliders::make_hslider(int index)
{
    hsliders[index] = share(slider(
       el::basic_thumb<25>(),
       hold(make_markers<false>()),
       (index + 1) * 0.25
    ));
    return share(align_middle(xside_margin({ 20, 20 }, hold(hsliders[index]))));
}

el::element_ptr DemoKnobsAndSliders::make_hsliders()
{
    return share(hmin_size(300,
       vtile(
          hold(make_hslider(0)),
          hold(make_hslider(1)),
          hold(make_hslider(2))
       )
    ));
}

el::element_ptr DemoKnobsAndSliders::make_vslider(int index)
{
    vsliders[index] = share(slider(
       el::basic_thumb<25>(),
       hold(make_markers<true>()),
       (index + 1) * 0.25
    ));
    return share(align_center(yside_margin({ 20, 20 }, hold(vsliders[index]))));
}

el::element_ptr DemoKnobsAndSliders::make_vsliders()
{
    return share(hmin_size(300,
       htile(
          hold(make_vslider(0)),
          hold(make_vslider(1)),
          hold(make_vslider(2))
       )
    ));
}

el::element_ptr DemoKnobsAndSliders::make_dial(int index)
{
    dials[index] = share(
       dial(
          el::radial_marks<20>(el::basic_knob<50>()),
          (index + 1) * 0.25
       )
    );

    auto markers = el::radial_labels<15>(
       hold(dials[index]),
       0.7,                                   // Label font size (relative size)
       "0", "1", "2", "3", "4",               // Labels
       "5", "6", "7", "8", "9", "10"
    );

    return share(align_center_middle(markers));
}

el::element_ptr DemoKnobsAndSliders::make_dials()
{
    return share(xside_margin(20,
       vtile(
          hold(make_dial(0)),
          hold(make_dial(1)),
          hold(make_dial(2))
       )
    ));
}

el::element_ptr DemoKnobsAndSliders::make_controls()
{
   return
      share(margin({ 20, 10, 20, 10 },
         vmin_size(400,
            htile(
               margin({ 20, 20, 20, 20 }, pane("Vertical Sliders", hold(make_vsliders()), 0.8f)),
               margin({ 20, 20, 20, 20 }, pane("Horizontal Sliders", hold(make_hsliders()), 0.8f)),
               hstretch(0.5, margin({ 20, 20, 20, 20 }, pane("Knobs", hold(make_dials()), 0.8f)))
            )
         )
      ));
}

void DemoKnobsAndSliders::link_control(int index, el::view& view_)
{
    vsliders[index]->on_change =
       [this, index, &view_](double val) {
           hsliders[index]->slider_base::value(val);
           dials[index]->dial_base::value(val);
           view_.refresh(*hsliders[index]);
           view_.refresh(*dials[index]);
       };

    hsliders[index]->on_change =
       [this, index, &view_](double val) {
           vsliders[index]->slider_base::value(val);
           dials[index]->dial_base::value(val);
           view_.refresh(*vsliders[index]);
           view_.refresh(*dials[index]);
       };

    dials[index]->on_change =
       [this, index, &view_](double val) {
           vsliders[index]->slider_base::value(val);
           hsliders[index]->slider_base::value(val);
           view_.refresh(*vsliders[index]);
           view_.refresh(*hsliders[index]);
       };
}

void DemoKnobsAndSliders::link_controls(el::view& view_)
{
    link_control(0, view_);
    link_control(1, view_);
    link_control(2, view_);
}
