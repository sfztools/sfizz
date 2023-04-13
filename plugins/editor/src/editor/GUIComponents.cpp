// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "GUIDefs.h"
#include "GUIComponents.h"
#include "ColorHelpers.h"
#include <complex>
#include <cmath>

#include "utility/vstgui_before.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/cvstguitimer.h"
#include "vstgui/lib/cframe.h"
#include "utility/vstgui_after.h"
#include "absl/strings/numbers.h"

using namespace gui;

///
SBoxContainer::SBoxContainer(const CRect& size)
    : CViewContainer(size)
{
    CViewContainer::setBackgroundColor(kColorTransparent);
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

void SValueMenu::setHoverColor(const CColor& color)
{
    hoverColor_ = color;
    invalid();
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

void SValueMenu::draw(CDrawContext* dc)
{
    CColor backupColor = fontColor;
    if (hovered_)
        fontColor = hoverColor_;
    CParamDisplay::draw(dc);
    if (hovered_)
        fontColor = backupColor;
}

CMouseEventResult SValueMenu::onMouseEntered(CPoint& where, const CButtonState& buttons)
{
    hovered_ = true;
    invalid();
    return CParamDisplay::onMouseEntered(where, buttons);
}

CMouseEventResult SValueMenu::onMouseExited(CPoint& where, const CButtonState& buttons)
{
    hovered_ = false;
    invalid();
    return CParamDisplay::onMouseExited(where, buttons);
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
CMouseEventResult SHoverButton::onMouseEntered(CPoint& where, const CButtonState& buttons) {
    hovered_ = true;
    if (OnHoverEnter)
        OnHoverEnter();
    invalid();
    return CKickButton::onMouseEntered(where, buttons);
}

CMouseEventResult SHoverButton::onMouseExited(CPoint& where, const CButtonState& buttons) {
    hovered_ = false;
    if (OnHoverLeave)
        OnHoverLeave();
    invalid();
    return CKickButton::onMouseExited(where, buttons);
}

void SHoverButton::draw(CDrawContext* dc) {
    CPoint where(offset.x, offset.y);
    bounceValue();
    if (hovered_)
        where.y += heightOfOneImage;
    if (getDrawBackground())
        getDrawBackground()->draw(dc, getViewSize(), where);
    setDirty(false);
}

///
void STextButton::setHighlightColor(const CColor& color)
{
    highlightColor_ = color;
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

void STextButton::setHighlighted(bool b)
{
    highlighted_ = b;
    invalid();
}

void STextButton::draw(CDrawContext* context)
{
    CColor backupColor = textColor;
    if (inactive_)
        textColor = inactiveColor_; // textColor is protected
    else if (hovered_ || highlighted_)
        textColor = highlightColor_;
    else if (inactive_)
        textColor = inactiveColor_;
    CTextButton::draw(context);
    textColor = backupColor;
}


CMouseEventResult STextButton::onMouseEntered (CPoint& where, const CButtonState& buttons)
{
    hovered_ = true;
    if (OnHoverEnter)
        OnHoverEnter();
    invalid();
    return CTextButton::onMouseEntered(where, buttons);
}

CMouseEventResult STextButton::onMouseExited (CPoint& where, const CButtonState& buttons)
{
    hovered_ = false;
    if (OnHoverLeave)
        OnHoverLeave();
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

void SStyledKnob::setRotatorColor(const CColor &color)
{
    if (rotatorColor_ == color)
        return;
    rotatorColor_ = color;
    invalid();
}

void SStyledKnob::setFont(CFontRef font)
{
    if (font_ == font)
        return;
    font_ = font;
    invalid();
}

void SStyledKnob::setFontColor(CColor fontColor)
{
    if (fontColor_ == fontColor)
        return;
    fontColor_ = fontColor;
    invalid();
}

void SStyledKnob::setValueToStringFunction(ValueToStringFunction func)
{
    valueToStringFunction_ = std::move(func);
    invalid();
}

void SStyledKnob::draw(CDrawContext* dc)
{
    const CCoord lineWidth = 4.0;
    const CCoord indicatorLineLength = 8.0;
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

    CRect knobRect(rect);
    knobRect.centerInside(bounds);
    knobRect.extend(-lineWidth, -lineWidth);

    SharedPointer<CGraphicsPath> path;

    // rotator
    path = owned(dc->createGraphicsPath());
    path->addEllipse(knobRect);

    dc->setFillColor(rotatorColor_);
    dc->drawGraphicsPath(path, CDrawContext::kPathFilled);

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

    if (valueToStringFunction_ && fontColor_.alpha > 0 && !hideValue_) {
        std::string text;
        if (valueToStringFunction_(getValue(), text)) {
            dc->setFont(font_);
            dc->setFontColor(fontColor_);
            dc->drawString(text.c_str(), bounds);
        }
    }
}

void CFilledRect::draw(CDrawContext* dc)
{
    CRect bounds = getViewSize();
    dc->setFillColor(color_);
    bool isRounded = radius_ > 0.0;
    if (isRounded) {
        auto roundRect = owned(dc->createRoundRectGraphicsPath(bounds, radius_));
        dc->drawGraphicsPath(roundRect, CDrawContext::kPathFilled);
    } else {
        dc->drawRect(bounds, kDrawFilled);
    }
}

///
SKnobCCBox::SKnobCCBox(const CRect& size, IControlListener* listener, int32_t tag)
    : CViewContainer(size),
      label_(makeOwned<CTextLabel>(CRect())),
      valueEdit_(makeOwned<CTextEdit>(CRect(), listener, tag)),
      knob_(makeOwned<SStyledKnob>(CRect(), listener, tag)),
      ccLabel_(makeOwned<CTextLabel>(CRect())),
      shadingRectangle_(makeOwned<CFilledRect>(CRect())),
      menuEntry_(makeOwned<CMenuItem>("Use HDCC", tag)),
      menuListener_(owned(new MenuListener(*this)))
{
    setBackgroundColor(kColorTransparent);

    label_->setText("Parameter");
    label_->setBackColor(kColorTransparent);
    label_->setFrameColor(kColorTransparent);
    label_->setFontColor(kBlackCColor);
    label_->setStyle(CParamDisplay::kRoundRectStyle);
    label_->setRoundRectRadius(5.0);

    knob_->setLineIndicatorColor(kBlackCColor);

    ccLabel_->setText("CC 1");
    ccLabel_->setStyle(CParamDisplay::kRoundRectStyle);
    ccLabel_->setRoundRectRadius(5.0);
    ccLabel_->setFrameColor(kColorTransparent);
    ccLabel_->setFontColor(kWhiteCColor);

    valueEdit_->setBackColor(kColorTransparent);
    valueEdit_->setFrameColor(kColorTransparent);
    valueEdit_->setFontColor(kBlackCColor);
    valueEdit_->registerViewListener(this);
    setHDMode(false);
    valueEdit_->setVisible(false);

    shadingRectangle_->setVisible(false);

    addView(label_);
    label_->remember();
    addView(knob_);
    knob_->remember();
    addView(shadingRectangle_);
    shadingRectangle_->remember();
    addView(valueEdit_);
    valueEdit_->remember();
    addView(ccLabel_);
    ccLabel_->remember();
    updateViewColors();
    updateViewSizes();
}

SKnobCCBox::~SKnobCCBox()
{
    valueEdit_->unregisterViewListener(this);
}

void SKnobCCBox::setHDMode(bool mode)
{
    if (mode) {
        auto valueToString = [](float value, std::string& text, VSTGUI::CParamDisplay*) -> bool {
            std::string s = std::to_string(value + 0.005f);
            text = s.substr(0, 4);
            return true;
        };
        knob_->setValueToStringFunction([valueToString](float value, std::string& text) {
            return valueToString(value, text, nullptr);
        });
        valueEdit_->setValueToStringFunction2(valueToString);

        valueEdit_->setStringToValueFunction([](UTF8StringPtr txt, float& result, CTextEdit*) -> bool {
            float value;
            if (absl::SimpleAtof(txt, &value)) {
                result = value;
                return true;
            }

            return false;
        });
        menuEntry_->setTitle("Use low-res. CC");
    } else {
        auto valueToString = [](float value, std::string& text, VSTGUI::CParamDisplay*) -> bool {
            text = std::to_string(std::lround(value * 127));
            return true;
        };
        knob_->setValueToStringFunction([valueToString](float value, std::string& text) {
            return valueToString(value, text, nullptr);
        });
        valueEdit_->setValueToStringFunction2(valueToString);

        valueEdit_->setStringToValueFunction([](UTF8StringPtr txt, float& result, CTextEdit*) -> bool {
            float value;
            if (absl::SimpleAtof(txt, &value)) {
                result = value / 127.0f;
                return true;
            }

            return false;
        });
        menuEntry_->setTitle("Use high-res. CC");
    }

    hdMode_ = mode;
    valueEdit_->setValue(valueEdit_->getValue());
    invalid();
}

CMouseEventResult SKnobCCBox::onMouseDown(CPoint& where, const CButtonState& buttons)
{
    if (buttons.isRightButton()) {
        CFrame* frame = getFrame();
        CPoint frameWhere = where;
        frameWhere.offset(-getViewSize().left, -getViewSize().top);
        this->localToFrame(frameWhere);

        auto self = shared(this);
        frame->doAfterEventProcessing([self, frameWhere]() {
            if (CFrame* frame = self->getFrame()) {
                SharedPointer<COptionMenu> menu =
                    owned(new COptionMenu(CRect(), self->menuListener_, -1, nullptr, nullptr, COptionMenu::kPopupStyle));
                menu->addEntry(self->menuEntry_);
                self->menuEntry_->remember(); // above call does not increment refcount

                menu->setFont(self->getValueEditFont());
                menu->setFontColor(self->getValueEditFontColor());
                menu->setBackColor(self->getValueEditBackColor());
                menu->popup(frame, frameWhere);
            }
        });
        return kMouseEventHandled;
    } else if (buttons.isDoubleClick() && !valueEdit_->isVisible()) {
        valueEdit_->setVisible(true);
        shadingRectangle_->setVisible(true);
        knob_->setHideValue(true);
        valueEdit_->takeFocus();
        invalid();
        return kMouseEventHandled;
    }

    return CViewContainer::onMouseDown(where, buttons);
}

void SKnobCCBox::viewLostFocus (CView* view)
{
    if (view == valueEdit_.get()) {
        shadingRectangle_->setVisible(false);
        valueEdit_->setVisible(false);
        knob_->setHideValue(false);
        invalid();
    }
}

void SKnobCCBox::setHue(float hue)
{
    hue_ = hue;
    updateViewColors();
}

void SKnobCCBox::setNameLabelFont(CFontRef font)
{
    label_->setFont(font);
    updateViewSizes();
}

void SKnobCCBox::setValueEditFont(CFontRef font)
{
    label_->setFont(font);
    updateViewSizes();
}

void SKnobCCBox::setCCLabelFont(CFontRef font)
{
    ccLabel_->setFont(font);
    updateViewSizes();
}

void SKnobCCBox::setValue(float value)
{
    float oldValue = knob_->getValue();
    knob_->setValue(value);
    valueEdit_->setValue(value);
    if (value != oldValue)
        invalid();
}

void SKnobCCBox::setDefaultValue(float value)
{
    knob_->setDefaultValue(value);
    valueEdit_->setDefaultValue(value);
}

void SKnobCCBox::updateViewSizes()
{
    const CRect size = getViewSize();
    const CCoord ypad = 4.0;

    const CFontRef nameFont = label_->getFont();
    const CFontRef ccFont = ccLabel_->getFont();
    const CFontRef valueFont = valueEdit_->getFont();

    nameLabelSize_ = CRect(0.0, 0.0, size.getWidth(), nameFont->getSize() + 2 * ypad);
    ccLabelSize_ = CRect(0.0, size.getHeight() - ccFont->getSize() - 2 * ypad, size.getWidth(), size.getHeight());
    knobSize_ = CRect(0.0, nameLabelSize_.bottom, size.getWidth(), ccLabelSize_.top);
    valueEditSize_ = CRect(
        size.getWidth() / 2 - valueFont->getSize(),
        size.getHeight() / 2 - valueFont->getSize() / 2,
        size.getWidth() / 2 + valueFont->getSize(),
        size.getHeight() / 2 + valueFont->getSize() / 2
    );

    // remove knob side areas
    CCoord side = std::max(0.0, knobSize_.getWidth() - knobSize_.getHeight());
    knobSize_.extend(-0.5 * side, 0.0);
    shadingRectangleSize_ = knobSize_;
    shadingRectangleSize_.bottom -= ypad;

    //
    label_->setViewSize(nameLabelSize_);
    knob_->setViewSize(knobSize_);
    ccLabel_->setViewSize(ccLabelSize_);
    valueEdit_->setViewSize(valueEditSize_);
    shadingRectangle_->setViewSize(shadingRectangleSize_);

    invalid();
}

void SKnobCCBox::updateViewColors()
{
    const float knobLuma = 0.4;
    const float ccLuma = 0.25;

    SColorHCY knobActiveTrackColor { hue_, 1.0, knobLuma };
    SColorHCY knobInactiveTrackColor { 0.0, 0.0, knobLuma };
    knob_->setActiveTrackColor(knobActiveTrackColor.toColor());
    knob_->setInactiveTrackColor(knobInactiveTrackColor.toColor());

    SColorHCY ccLabelColor { hue_, 1.0, ccLuma };
    ccLabel_->setBackColor(ccLabelColor.toColor());

    invalid();
}

///
SControlsPanel::SControlsPanel(const CRect& size)
    : CScrollView(
        size, CRect(),
        CScrollView::kVerticalScrollbar|CScrollView::kDontDrawFrame|CScrollView::kAutoHideScrollbars),
      listener_(new ControlSlotListener(this))
{
    // slot 0 always exists, keep the default style on the views there
    getOrCreateSlot(0);

    setBackgroundColor(kColorTransparent);

    setScrollbarWidth(10.0);

    relayoutTrigger_ = makeOwned<CVSTGUITimer>(
        [this](CVSTGUITimer* timer) { timer->stop(); updateLayout(); },
        1, false);
}

void SControlsPanel::setControlUsed(uint32_t index, bool used)
{
    ControlSlot* slot = getSlot(index);
    if (!slot && !used)
        return;
    if (!slot)
        slot = getOrCreateSlot(index);
    if (used != slot->used) {
        slot->used = used;
        relayoutTrigger_->start();
    }
}

std::string SControlsPanel::getDefaultLabelText(uint32_t index)
{
    (void)index;
    return {};
}

SControlsPanel::ControlSlot* SControlsPanel::getSlot(uint32_t index)
{
    ControlSlot* slot = nullptr;
    if (index < slots_.size())
        slot = slots_[index].get();
    return slot;
}

SControlsPanel::ControlSlot* SControlsPanel::getOrCreateSlot(uint32_t index)
{
    ControlSlot* slot = getSlot(index);
    if (slot)
        return slot;

    if (index + 1 > slots_.size())
        slots_.resize(index + 1);

    slot = new ControlSlot;
    slots_[index].reset(slot);

    CRect boxSize { 0.0, 0.0, 120.0, 90.0 };
    SharedPointer<SKnobCCBox> box = makeOwned<SKnobCCBox>(boxSize, listener_.get(), index);
    slot->box = box;
    slot->box->setCCLabelText(("CC " + std::to_string(index)).c_str());

    syncSlotStyle(index);

    return slot;
}

void SControlsPanel::setControlValue(uint32_t index, float value)
{
    ControlSlot* slot = getOrCreateSlot(index);
    SKnobCCBox* box = slot->box;
    float oldValue = box->getValue();
    box->setValue(value);
    if (box->getValue() != oldValue)
        box->invalid();
}

void SControlsPanel::setControlDefaultValue(uint32_t index, float value)
{
    ControlSlot* slot = getOrCreateSlot(index);
    SKnobCCBox* box = slot->box;
    box->setDefaultValue(value);
}

void SControlsPanel::setControlLabelText(uint32_t index, UTF8StringPtr text)
{
    ControlSlot* slot = getOrCreateSlot(index);
    SKnobCCBox* box = slot->box;
    if (text && text[0] != '\0')
        box->setNameLabelText(text);
    else
        box->setNameLabelText(getDefaultLabelText(index).c_str());
    box->invalid();
}

void SControlsPanel::setNameLabelFont(CFontRef font)
{
    slots_[0]->box->setNameLabelFont(font);
    slots_[0]->box->setValueEditFont(font);
    syncAllSlotStyles();
}

void SControlsPanel::setNameLabelFontColor(CColor color)
{
    slots_[0]->box->setNameLabelFontColor(color);
    slots_[0]->box->setValueEditFontColor(color);
    syncAllSlotStyles();
}

void SControlsPanel::setNameLabelBackColor(CColor color)
{
    slots_[0]->box->setNameLabelBackColor(color);
    slots_[0]->box->setValueEditBackColor(color);
    syncAllSlotStyles();
}

void SControlsPanel::setCCLabelFont(CFontRef font)
{
    slots_[0]->box->setCCLabelFont(font);
    syncAllSlotStyles();
}

void SControlsPanel::setCCLabelBackColor(CColor color)
{
    slots_[0]->box->setCCLabelBackColor(color);
    syncAllSlotStyles();
}

void SControlsPanel::setCCLabelFontColor(CColor color)
{
    slots_[0]->box->setCCLabelFontColor(color);
    syncAllSlotStyles();
}

void SControlsPanel::setValueEditBackColor(CColor color)
{
    slots_[0]->box->setValueEditBackColor(color);
    syncAllSlotStyles();
}

void SControlsPanel::setShadingRectangleColor(CColor color)
{
    slots_[0]->box->setShadingRectangleColor(color);
    syncAllSlotStyles();
}

void SControlsPanel::setValueEditFontColor(CColor color)
{
    slots_[0]->box->setValueEditFontColor(color);
    syncAllSlotStyles();
}

void SControlsPanel::setKnobActiveTrackColor(CColor color)
{
    slots_[0]->box->setKnobActiveTrackColor(color);
    syncAllSlotStyles();
}

void SControlsPanel::setKnobInactiveTrackColor(CColor color)
{
    slots_[0]->box->setKnobInactiveTrackColor(color);
    syncAllSlotStyles();
}

void SControlsPanel::setKnobLineIndicatorColor(CColor color)
{
    slots_[0]->box->setKnobLineIndicatorColor(color);
    syncAllSlotStyles();
}

void SControlsPanel::setKnobRotatorColor(CColor color)
{
    slots_[0]->box->setKnobRotatorColor(color);
    syncAllSlotStyles();
}

void SControlsPanel::setKnobFont(CFontRef font)
{
    slots_[0]->box->setKnobFont(font);
    syncAllSlotStyles();
}

void SControlsPanel::setKnobFontColor(CColor color)
{
    slots_[0]->box->setKnobFontColor(color);
    syncAllSlotStyles();
}

void SControlsPanel::recalculateSubViews()
{
    CScrollView::recalculateSubViews();

    // maybe the operation just created the scroll bar
    if (CScrollbar* vsb = getVerticalScrollbar()) {
        // update scrollbar style
        vsb->setFrameColor(kColorTransparent);
        vsb->setBackgroundColor(kColorTransparent);
        vsb->setScrollerColor(kColorControlsScrollerTransparency);
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
    const CCoord horizontalPadding = 4.0;
    CCoord verticalPadding = 4.0;
    CCoord interRowPadding = {};
    CCoord interColumnPadding = 8.0;

    int currentRow = 0;
    int currentColumn = 0;
    int containerBottom = 0;

    uint32_t numSlots = static_cast<uint32_t>(slots_.size());
    for (uint32_t i = 0; i < numSlots; ++i) {
        ControlSlot* slot = slots_[i].get();
        if (!slot || !slot->used)
            continue;

        CViewContainer* box = slot->box;

        if (isFirstSlot) {
            itemWidth = box->getWidth();
            itemHeight = box->getHeight();
            isFirstSlot = false;
            numColumns = int((viewBounds.getWidth() - horizontalPadding) /
                             (itemWidth + interColumnPadding));
            numColumns = std::max(1, numColumns);
            itemOffsetX = (viewBounds.getWidth() - horizontalPadding -
                           numColumns * (itemWidth + interColumnPadding)) / 2.0;

            int maxRowsShown = int((viewBounds.getHeight() - 2 * verticalPadding) / itemHeight);
            if (maxRowsShown > 1)
                interRowPadding = (viewBounds.getHeight() - 2 * verticalPadding - itemHeight * maxRowsShown) / (maxRowsShown - 1);
        }

        CRect itemBounds = box->getViewSize();
        itemBounds.moveTo(
            itemOffsetX + horizontalPadding + currentColumn * (interColumnPadding + itemWidth),
            verticalPadding + currentRow * (interRowPadding + itemHeight));

        box->setViewSize(itemBounds);

        containerBottom = itemBounds.bottom;

        addView(box);
        box->remember();

        if (++currentColumn == numColumns) {
            currentColumn = 0;
            ++currentRow;
        }
    }

    setContainerSize(CRect(0.0, 0.0, viewBounds.getWidth(), containerBottom + verticalPadding));

    invalid();
}

void SControlsPanel::syncAllSlotStyles()
{
    uint32_t count = static_cast<uint32_t>(slots_.size());
    for (uint32_t index = 0; index < count; ++index)
        syncSlotStyle(index);
}

void SControlsPanel::syncSlotStyle(uint32_t index)
{
    if (index >= slots_.size())
        return;

    const SKnobCCBox* ref = slots_[0]->box;
    SKnobCCBox* cur = slots_[index]->box;

    if (!cur)
        return;

    if (cur != ref) {
        cur->setNameLabelFont(ref->getNameLabelFont());
        cur->setNameLabelFontColor(ref->getNameLabelFontColor());
        cur->setNameLabelBackColor(ref->getNameLabelBackColor());

        cur->setValueEditFont(ref->getValueEditFont());
        cur->setValueEditFontColor(ref->getValueEditFontColor());

        cur->setShadingRectangleColor(ref->getShadingRectangleColor());

        cur->setCCLabelFont(ref->getCCLabelFont());
        cur->setCCLabelFontColor(ref->getCCLabelFontColor());
        cur->setCCLabelBackColor(ref->getCCLabelBackColor());

        cur->setKnobActiveTrackColor(ref->getKnobActiveTrackColor());
        cur->setKnobInactiveTrackColor(ref->getKnobInactiveTrackColor());
        cur->setKnobLineIndicatorColor(ref->getKnobLineIndicatorColor());
        cur->setKnobRotatorColor(ref->getKnobRotatorColor());
        cur->setKnobFont(ref->getKnobFont());
        cur->setKnobFontColor(ref->getKnobFontColor());
    }

    cur->invalid();
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
SLevelMeter::SLevelMeter(const CRect& size)
    : CView(size)
{
}

void SLevelMeter::setValue(float value)
{
    if (value_ == value)
        return;

    value_ = value;

    // instantiate the timer lazily
    if (!timer_) {
        const uint32_t interval = 10;
        timer_ = makeOwned<CVSTGUITimer>(
            [this](CVSTGUITimer* timer) {
                timer->stop();
                timerArmed_ = false;
                invalid();
            }, interval, false);
    }

    // defer the update, but do not rearm the timer
    if (!timerArmed_) {
        timerArmed_ = true;
        timer_->start();
    }
}

void SLevelMeter::draw(CDrawContext* dc)
{
    float dbValue = 20.0f * std::log10(value_);
    float fill = (dbValue - dbMin_) / (dbMax_ - dbMin_);
    fill = (fill < 0.0f) ? 0.0f : fill;
    fill = (fill > 1.0f) ? 1.0f : fill;

    CRect largeBounds = getViewSize();
    CRect fillBounds = largeBounds;
    fillBounds.top = largeBounds.bottom - fill * largeBounds.getHeight();

    const CColor safeColor = safeFillColor_;
    const CColor dangerColor = dangerFillColor_;

    CColor fillColor;
    if (safeColor == dangerColor) {
        fillColor = safeColor;
    }
    else {
        float thres = dangerThreshold_;
        float mix = (fill - thres) / (1.0f - thres);
        mix = (mix < 0.0f) ? 0.0f : mix;

        CCoord safeH, safeS, safeV, safeA;
        CCoord dangerH, dangerS, dangerV, dangerA;
        safeColor.toHSV(safeH, safeS, safeV);
        dangerColor.toHSV(dangerH, dangerS, dangerV);
        safeA = safeColor.alpha / 255.0;
        dangerA = dangerColor.alpha / 255.0;

        CCoord H, S, V, A;
        H = safeH + mix * (dangerH - safeH);
        S = safeS + mix * (dangerS - safeS);
        V = safeV + mix * (dangerV - safeV);
        A = safeA + mix * (dangerA - safeA);

        fillColor.fromHSV(H, S, V);
        fillColor.alpha = static_cast<uint8_t>(A * 255.0);
    }

    CCoord radius = radius_;
    bool isRounded = radius > 0.0;

    dc->setDrawMode(isRounded ? kAntiAliasing : kAliasing);

    SharedPointer<CGraphicsPath> largeRoundRect;
    SharedPointer<CGraphicsPath> fillRoundRect;

    if (isRounded) {
        largeRoundRect = owned(dc->createRoundRectGraphicsPath(largeBounds, radius));
        fillRoundRect = owned(dc->createRoundRectGraphicsPath(fillBounds, radius));
    }

    if (backColor_.alpha > 0) {
        dc->setFillColor(backColor_);
        if (!isRounded)
            dc->drawRect(largeBounds, kDrawFilled);
        else
            dc->drawGraphicsPath(largeRoundRect, CDrawContext::kPathFilled);
    }

    dc->setFrameColor(frameColor_);
    dc->setFillColor(fillColor);

    if (!isRounded) {
        if (fill > 0)
            dc->drawRect(fillBounds, kDrawFilled);
        dc->drawRect(largeBounds);
    }
    else {
        if (fill > 0 && fillBounds.getHeight() >= radius)
            dc->drawGraphicsPath(fillRoundRect, CDrawContext::kPathFilled);
        dc->drawGraphicsPath(largeRoundRect, CDrawContext::kPathStroked);
    }
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
