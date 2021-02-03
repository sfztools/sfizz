// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "GUIComponents.h"
#include <complex>
#include <cmath>

#include "utility/vstgui_before.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/cframe.h"
#include "utility/vstgui_after.h"

///
SBoxContainer::SBoxContainer(const CRect& size)
    : CViewContainer(size)
{
    CViewContainer::setBackgroundColor(CColor(0, 0, 0, 0));
}

void SBoxContainer::setCornerRadius(CCoord radius)
{
    cornerRadius_ = radius;
    invalid();
}

void SBoxContainer::setBackgroundColor(const CColor& color)
{
    backgroundColor_ = color;
    invalid();
}

CColor SBoxContainer::getBackgroundColor() const
{
    return backgroundColor_;
}

void SBoxContainer::drawRect(CDrawContext* dc, const CRect& updateRect)
{
    CRect bounds = getViewSize();

    dc->setDrawMode(kAntiAliasing);

    SharedPointer<CGraphicsPath> path = owned(dc->createGraphicsPath());
    path->addRoundRect(bounds, cornerRadius_);

    dc->setFillColor(backgroundColor_);
    dc->drawGraphicsPath(path.get(), CDrawContext::kPathFilled);

    CViewContainer::drawRect(dc, updateRect);
}

///
STitleContainer::STitleContainer(const CRect& size, UTF8StringPtr text)
    : SBoxContainer(size), text_(text ? text : ""), titleFont_(kNormalFont)
{
}

void STitleContainer::setTitleFont(CFontRef font)
{
    titleFont_ = font;
    invalid();
}

void STitleContainer::setTitleFontColor(CColor color)
{
    titleFontColor_ = color;
    invalid();
}

void STitleContainer::setTitleBackgroundColor(CColor color)
{
    titleBackgroundColor_ = color;
    invalid();
}

void STitleContainer::drawRect(CDrawContext* dc, const CRect& updateRect)
{
    SBoxContainer::drawRect(dc, updateRect);

    CRect bounds = getViewSize();
    CCoord cornerRadius = cornerRadius_;

    dc->setDrawMode(kAntiAliasing);

    CCoord fontHeight = titleFont_->getSize();
    CCoord titleHeight = fontHeight + 8.0;

    CRect titleBounds = bounds;
    titleBounds.bottom = titleBounds.top + titleHeight;

    SharedPointer<CGraphicsPath> path = owned(dc->createGraphicsPath());
    path->beginSubpath(titleBounds.getBottomRight());
    path->addLine(titleBounds.getBottomLeft());
    path->addArc(CRect(titleBounds.left, titleBounds.top, titleBounds.left + 2.0 * cornerRadius, titleBounds.top + 2.0 * cornerRadius), 180., 270., true);
    path->addArc(CRect(titleBounds.right - 2.0 * cornerRadius, titleBounds.top, titleBounds.right, titleBounds.top + 2.0 * cornerRadius), 270., 360., true);
    path->closeSubpath();

    dc->setFillColor(titleBackgroundColor_);
    dc->drawGraphicsPath(path, CDrawContext::kPathFilled);

    dc->setFont(titleFont_);
    dc->setFontColor(titleFontColor_);
    dc->drawString(text_.c_str(), titleBounds, kCenterText);
}

///
void SFileDropTarget::setFileDropFunction(FileDropFunction f)
{
    dropFunction_ = std::move(f);
}

DragOperation SFileDropTarget::onDragEnter(DragEventData data)
{
    op_ = isFileDrop(data.drag) ?
        DragOperation::Copy : DragOperation::None;
    return op_;
}

DragOperation SFileDropTarget::onDragMove(DragEventData data)
{
    (void)data;
    return op_;
}

void SFileDropTarget::onDragLeave(DragEventData data)
{
    (void)data;
    op_ = DragOperation::None;
}

bool SFileDropTarget::onDrop(DragEventData data)
{
    if (op_ != DragOperation::Copy || !isFileDrop(data.drag))
        return false;

    IDataPackage::Type type;
    const void* bytes;
    uint32_t size = data.drag->getData(0, bytes, type);
    std::string path(reinterpret_cast<const char*>(bytes), size);

    if (dropFunction_)
        dropFunction_(path);

    return true;
}

bool SFileDropTarget::isFileDrop(IDataPackage* package)
{
    return package->getCount() == 1 &&
        package->getDataType(0) == IDataPackage::kFilePath;
}

///
SValueMenu::SValueMenu(const CRect& bounds, IControlListener* listener, int32_t tag)
    : CParamDisplay(bounds), menuListener_(owned(new MenuListener(*this)))
{
    setListener(listener);
    setTag(tag);
    setWheelInc(0.0f);
}

CMenuItem* SValueMenu::addEntry(CMenuItem* item, float value, int32_t index)
{
    if (index < 0 || index > getNbEntries()) {
        menuItems_.emplace_back(owned(item));
        menuItemValues_.emplace_back(value);
    }
    else
    {
        menuItems_.insert(menuItems_.begin() + index, owned(item));
        menuItemValues_.insert(menuItemValues_.begin() + index, value);
    }
    return item;
}

CMenuItem* SValueMenu::addEntry(const UTF8String& title, float value, int32_t index, int32_t itemFlags)
{
    if (title == "-")
        return addSeparator(index);
    CMenuItem* item = new CMenuItem(title, nullptr, 0, nullptr, itemFlags);
    return addEntry(item, value, index);
}

CMenuItem* SValueMenu::addSeparator(int32_t index)
{
    CMenuItem* item = new CMenuItem("", nullptr, 0, nullptr, CMenuItem::kSeparator);
    return addEntry(item, 0.0f, index);
}

int32_t SValueMenu::getNbEntries() const
{
    return static_cast<int32_t>(menuItems_.size());
}

CMouseEventResult SValueMenu::onMouseDown(CPoint& where, const CButtonState& buttons)
{
    (void)where;

    if (buttons & (kLButton|kRButton|kApple)) {
        CFrame* frame = getFrame();
        CRect bounds = getViewSize();

        CPoint frameWhere = bounds.getBottomLeft();
        this->localToFrame(frameWhere);

        auto self = shared(this);
        frame->doAfterEventProcessing([self, frameWhere]() {
            if (CFrame* frame = self->getFrame()) {
                SharedPointer<COptionMenu> menu = owned(new COptionMenu(CRect(), self->menuListener_, -1, nullptr, nullptr, COptionMenu::kPopupStyle));
                for (const SharedPointer<CMenuItem>& item : self->menuItems_) {
                    menu->addEntry(item);
                    item->remember(); // above call does not increment refcount
                }
                menu->setFont(self->getFont());
                menu->setFontColor(self->getFontColor());
                menu->setBackColor(self->getBackColor());
                menu->popup(frame, frameWhere + CPoint(0.0, 1.0));
            }
        });
        return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
    }

    return kMouseEventNotHandled;
}

bool SValueMenu::onWheel(const CPoint& where, const CMouseWheelAxis& axis, const float& distance, const CButtonState& buttons)
{
    (void)where;
    (void)buttons;

    if (axis != kMouseWheelAxisY)
        return false;

    float wheelInc = getWheelInc();
    if (wheelInc != 0) {
        float oldValue = getValue();
        setValueNormalized(getValueNormalized() + distance * wheelInc);
        if (getValue() != oldValue) {
            valueChanged();
            invalid();
        }
    }
    return true;
}

void SValueMenu::onItemClicked(int32_t index)
{
    float oldValue = getValue();
    setValue(menuItemValues_[index]);
    if (getValue() != oldValue) {
        valueChanged();
        invalid();
    }
}

///
SActionMenu::SActionMenu(const CRect& bounds, IControlListener* listener)
    : CParamDisplay(bounds), menuListener_(owned(new MenuListener(*this)))
{
    setListener(listener);

    auto toString = [](float, std::string& result, CParamDisplay* display) {
        result = static_cast<SActionMenu*>(display)->getTitle();
        return true;
    };

    setValueToStringFunction2(toString);
}

void SActionMenu::setTitle(std::string title)
{
    title_ = std::move(title);
    invalid();
}

void SActionMenu::setHoverColor(const CColor& color)
{
    hoverColor_ = color;
    invalid();
}

CMenuItem* SActionMenu::addEntry(CMenuItem* item, int32_t tag, int32_t index)
{
    if (index < 0 || index > getNbEntries()) {
        menuItems_.emplace_back(owned(item));
        menuItemTags_.emplace_back(tag);
    }
    else
    {
        menuItems_.insert(menuItems_.begin() + index, owned(item));
        menuItemTags_.insert(menuItemTags_.begin() + index, tag);
    }
    return item;
}

CMenuItem* SActionMenu::addEntry(const UTF8String& title, int32_t tag, int32_t index, int32_t itemFlags)
{
    if (title == "-")
        return addSeparator(index);
    CMenuItem* item = new CMenuItem(title, nullptr, 0, nullptr, itemFlags);
    return addEntry(item, tag, index);
}

CMenuItem* SActionMenu::addSeparator(int32_t index)
{
    CMenuItem* item = new CMenuItem("", nullptr, 0, nullptr, CMenuItem::kSeparator);
    return addEntry(item, 0.0f, index);
}

int32_t SActionMenu::getNbEntries() const
{
    return static_cast<int32_t>(menuItems_.size());
}

void SActionMenu::draw(CDrawContext* dc)
{
    CColor backupColor = fontColor;
    if (hovered_)
        fontColor = hoverColor_;
    CParamDisplay::draw(dc);
    if (hovered_)
        fontColor = backupColor;
}

CMouseEventResult SActionMenu::onMouseEntered(CPoint& where, const CButtonState& buttons)
{
    hovered_ = true;
    invalid();
    return CParamDisplay::onMouseEntered(where, buttons);
}

CMouseEventResult SActionMenu::onMouseExited(CPoint& where, const CButtonState& buttons)
{
    hovered_ = false;
    invalid();
    return CParamDisplay::onMouseExited(where, buttons);
}

CMouseEventResult SActionMenu::onMouseDown(CPoint& where, const CButtonState& buttons)
{
    (void)where;

    if (buttons & (kLButton|kRButton|kApple)) {
        CFrame* frame = getFrame();
        CRect bounds = getViewSize();

        CPoint frameWhere = bounds.getBottomLeft();
        this->localToFrame(frameWhere);

        auto self = shared(this);
        frame->doAfterEventProcessing([self, frameWhere]() {
            if (CFrame* frame = self->getFrame()) {
                SharedPointer<COptionMenu> menu = owned(new COptionMenu(CRect(), self->menuListener_, -1, nullptr, nullptr, COptionMenu::kPopupStyle));
                for (const SharedPointer<CMenuItem>& item : self->menuItems_) {
                    menu->addEntry(item);
                    item->remember(); // above call does not increment refcount
                }
                menu->setFont(self->getFont());
                menu->setFontColor(self->getFontColor());
                menu->setBackColor(self->getBackColor());
                menu->popup(frame, frameWhere + CPoint(0.0, 1.0));
            }
        });
        return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
    }

    return kMouseEventNotHandled;
}

void SActionMenu::onItemClicked(int32_t index)
{
    setTag(menuItemTags_[index]);
    setValue(1.0f);
    if (listener)
        listener->valueChanged(this);
    setValue(0.0f);
    if (listener)
        listener->valueChanged(this);
}

///
void STextButton::setHoverColor(const CColor& color)
{
    hoverColor_ = color;
    invalid();
}

void STextButton::setInactiveColor(const CColor& color)
{
    inactiveColor_ = color;
    invalid();
}

void STextButton::setInactive(bool b)
{
    inactive_ = b;
    invalid();
}

void STextButton::draw(CDrawContext* context)
{
    CColor backupColor = textColor;
    if (hovered_)
        textColor = hoverColor_; // textColor is protected
    else if (inactive_)
        textColor = inactiveColor_;
    CTextButton::draw(context);
    textColor = backupColor;
}


CMouseEventResult STextButton::onMouseEntered (CPoint& where, const CButtonState& buttons)
{
    hovered_ = true;
    invalid();
    return CTextButton::onMouseEntered(where, buttons);
}

CMouseEventResult STextButton::onMouseExited (CPoint& where, const CButtonState& buttons)
{
    hovered_ = false;
    invalid();
    return CTextButton::onMouseExited(where, buttons);
}

///
SStyledKnob::SStyledKnob(const CRect& size, IControlListener* listener, int32_t tag)
    : CKnobBase(size, listener, tag, nullptr)
{
}

void SStyledKnob::setActiveTrackColor(const CColor& color)
{
    if (activeTrackColor_ == color)
        return;
    activeTrackColor_ = color;
    invalid();
}

void SStyledKnob::setInactiveTrackColor(const CColor& color)
{
    if (inactiveTrackColor_ == color)
        return;
    inactiveTrackColor_ = color;
    invalid();
}

void SStyledKnob::setLineIndicatorColor(const CColor& color)
{
    if (lineIndicatorColor_ == color)
        return;
    lineIndicatorColor_ = color;
    invalid();
}

void SStyledKnob::draw(CDrawContext* dc)
{
    const CCoord lineWidth = 4.0;
    const CCoord indicatorLineLength = 10.0;
    const CCoord angleSpread = 250.0;
    const CCoord angle1 = 270.0 - 0.5 * angleSpread;
    const CCoord angle2 = 270.0 + 0.5 * angleSpread;

    dc->setDrawMode(kAntiAliasing);

    const CRect bounds = getViewSize();

    // compute inner bounds
    CRect rect(bounds);
    rect.setWidth(std::min(rect.getWidth(), rect.getHeight()));
    rect.setHeight(rect.getWidth());
    rect.centerInside(bounds);
    rect.extend(-lineWidth, -lineWidth);

    SharedPointer<CGraphicsPath> path;

    // inactive track
    path = owned(dc->createGraphicsPath());
    path->addArc(rect, angle1, angle2, true);

    dc->setFrameColor(inactiveTrackColor_);
    dc->setLineWidth(lineWidth);
    dc->setLineStyle(kLineSolid);
    dc->drawGraphicsPath(path, CDrawContext::kPathStroked);

    // active track
    const CCoord v = getValueNormalized();
    const CCoord vAngle = angle1 + v * angleSpread;
    path = owned(dc->createGraphicsPath());
    path->addArc(rect, angle1, vAngle, true);

    dc->setFrameColor(activeTrackColor_);
    dc->setLineWidth(lineWidth + 0.5);
    dc->setLineStyle(kLineSolid);
    dc->drawGraphicsPath(path, CDrawContext::kPathStroked);

    // indicator line
    {
        CCoord module1 = 0.5 * rect.getWidth() - indicatorLineLength;
        CCoord module2 = 0.5 * rect.getWidth();
        std::complex<CCoord> c1 = std::polar(module1, vAngle * (M_PI / 180.0));
        std::complex<CCoord> c2 = std::polar(module2, vAngle * (M_PI / 180.0));

        CPoint p1(c1.real(), c1.imag());
        CPoint p2(c2.real(), c2.imag());
        p1.offset(rect.getCenter());
        p2.offset(rect.getCenter());

        dc->setFrameColor(lineIndicatorColor_);
        dc->setLineWidth(1.0);
        dc->setLineStyle(kLineSolid);
        dc->drawLine(p1, p2);
    }
}

///
SControlsPanel::SControlsPanel(const CRect& size)
    : CScrollView(
        size, CRect(),
        CScrollView::kVerticalScrollbar|CScrollView::kDontDrawFrame|CScrollView::kAutoHideScrollbars),
      listener_(new ControlSlotListener(this))
{
    setBackgroundColor(CColor(0x00, 0x00, 0x00, 0x00));

    setScrollbarWidth(10.0);
}

void SControlsPanel::setControlUsed(uint32_t index, bool used)
{
    bool changed = false;

    if (used) {
        if (index + 1 > slots_.size())
            slots_.resize(index + 1);
        ControlSlot* slot = slots_[index].get();
        changed = !slot;
        if (changed) {
            slot = new ControlSlot;
            slots_[index].reset(slot);

            // create controls etc...
            CCoord knobWidth = 48.0;
            CCoord knobHeight = knobWidth;
            CCoord labelWidth = 96.0;
            CCoord labelHeight = 24.0;
            CCoord verticalPadding = 0.0;

            CCoord totalWidth = std::max(knobWidth, labelWidth);
            CCoord knobX = (totalWidth - knobWidth) / 2.0;
            CCoord labelX = (totalWidth - labelWidth) / 2.0;

            CRect knobBounds(knobX, 0.0, knobX + knobWidth, knobHeight);
            CRect labelBounds(labelX, knobHeight + verticalPadding, labelX + labelWidth, knobHeight + verticalPadding + labelHeight);
            CRect boxBounds = CRect(knobBounds).unite(labelBounds);

            SharedPointer<SStyledKnob> knob = owned(new SStyledKnob(knobBounds, listener_.get(), index));
            SharedPointer<CTextLabel> label = owned(new CTextLabel(labelBounds));
            SharedPointer<CViewContainer> box = owned(new CViewContainer(boxBounds));

            box->addView(knob);
            knob->remember();
            box->addView(label);
            label->remember();

            box->setBackgroundColor(CColor(0x00, 0x00, 0x00, 0x00));
            label->setStyle(CTextLabel::kRoundRectStyle);
            label->setRoundRectRadius(5.0);
            label->setBackColor(CColor(0x2e, 0x34, 0x36));
            label->setText(getDefaultLabelText(index));
            knob->setActiveTrackColor(CColor(0x00, 0xb6, 0x2a));
            knob->setInactiveTrackColor(CColor(0x30, 0x30, 0x30));
            knob->setLineIndicatorColor(CColor(0x00, 0x00, 0x00));

            slot->knob = knob;
            slot->label = label;
            slot->box = box;
        }
    }
    else {
        if (index < slots_.size() && slots_[index]) {
            changed = true;
            slots_[index].reset();
        }
    }

    if (changed)
        updateLayout();
}

std::string SControlsPanel::getDefaultLabelText(uint32_t index)
{
    return "CC " + std::to_string(index);
}

void SControlsPanel::setControlValue(uint32_t index, float value)
{
    if (index >= slots_.size())
        return;

    ControlSlot* slot = slots_[index].get();
    if (!slot)
        return;

    slot->knob->setValue(value);
    slot->knob->invalid();
}

void SControlsPanel::setControlDefaultValue(uint32_t index, float value)
{
    if (index >= slots_.size())
        return;

    ControlSlot* slot = slots_[index].get();
    if (!slot)
        return;

    slot->knob->setDefaultValue(value);
}

void SControlsPanel::setControlLabelText(uint32_t index, UTF8StringPtr text)
{
    if (index >= slots_.size())
        return;

    ControlSlot* slot = slots_[index].get();
    if (!slot)
        return;

    if (text && text[0] != '\0')
        slot->label->setText(text);
    else
        slot->label->setText(getDefaultLabelText(index).c_str());
    slot->label->invalid();
}

void SControlsPanel::recalculateSubViews()
{
    CScrollView::recalculateSubViews();

    // maybe the operation just created the scroll bar
    if (CScrollbar* vsb = getVerticalScrollbar()) {
        // update scrollbar style
        vsb->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
        vsb->setBackgroundColor(CColor(0x00, 0x00, 0x00, 0x00));
        vsb->setScrollerColor(CColor(0x00, 0x00, 0x00, 0x80));
    }
}

void SControlsPanel::updateLayout()
{
    removeAll();

    const CRect viewBounds = getViewSize();

    bool isFirstSlot = true;
    CCoord itemWidth {};
    CCoord itemHeight {};
    CCoord itemOffsetX {};

    int numColumns {};
    const CCoord horizontalPadding = 24.0;
    const CCoord verticalPadding = 18.0;

    int currentRow = 0;
    int currentColumn = 0;
    int containerBottom = 0;

    uint32_t numSlots = static_cast<uint32_t>(slots_.size());
    for (uint32_t i = 0; i < numSlots; ++i) {
        ControlSlot* slot = slots_[i].get();
        if (!slot)
            continue;

        CViewContainer* box = slot->box;

        if (isFirstSlot) {
            itemWidth = box->getWidth();
            itemHeight = box->getHeight();
            isFirstSlot = false;
            numColumns = int((viewBounds.getWidth() - horizontalPadding) /
                             (itemWidth + horizontalPadding));
            numColumns = std::max(1, numColumns);
            itemOffsetX = (viewBounds.getWidth() - horizontalPadding -
                           numColumns * (itemWidth + horizontalPadding)) / 2.0;
        }

        CRect itemBounds = box->getViewSize();
        itemBounds.moveTo(
            itemOffsetX + horizontalPadding + currentColumn * (horizontalPadding + itemWidth),
            verticalPadding + currentRow * (verticalPadding + itemHeight));

        box->setViewSize(itemBounds);

        containerBottom = itemBounds.bottom;

        addView(box);
        box->remember();

        if (++currentColumn == numColumns) {
            currentColumn = 0;
            ++currentRow;
        }
    }

    CRect containerSize = getContainerSize();
    containerSize.bottom = containerBottom;
    setContainerSize(CRect(0.0, 0.0, viewBounds.getWidth(), containerBottom + verticalPadding));

    invalid();
}

void SControlsPanel::ControlSlotListener::valueChanged(CControl* pControl)
{
    if (panel_->ValueChangeFunction)
        panel_->ValueChangeFunction(pControl->getTag(), pControl->getValue());
}

void SControlsPanel::ControlSlotListener::controlBeginEdit(CControl* pControl)
{
    if (panel_->BeginEditFunction)
        panel_->BeginEditFunction(pControl->getTag());
}

void SControlsPanel::ControlSlotListener::controlEndEdit(CControl* pControl)
{
    if (panel_->EndEditFunction)
        panel_->EndEditFunction(pControl->getTag());
}

///
SPlaceHolder::SPlaceHolder(const CRect& size, const CColor& color)
    : CView(size), color_(color)
{
}

void SPlaceHolder::draw(CDrawContext* dc)
{
    const CRect bounds = getViewSize();
    dc->setDrawMode(kAliasing);
    dc->setFrameColor(color_);
    dc->drawRect(bounds);
    dc->drawLine(bounds.getTopLeft(), bounds.getBottomRight());
    dc->drawLine(bounds.getTopRight(), bounds.getBottomLeft());
}
