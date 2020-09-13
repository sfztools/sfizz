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
    setDirty();
}

void SBoxContainer::setBackgroundColor(const CColor& color)
{
    backgroundColor_ = color;
    setDirty();
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
    setDirty();
}

void STitleContainer::setTitleFontColor(CColor color)
{
    titleFontColor_ = color;
    setDirty();
}

void STitleContainer::setTitleBackgroundColor(CColor color)
{
    titleBackgroundColor_ = color;
    setDirty();
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
SPiano::SPiano(const CRect& bounds)
    : CView(bounds), font_(kNormalFont)
{
}

void SPiano::setFont(CFontRef font)
{
    font_ = font;
    setDirty();
}

void SPiano::clearKeyRanges()
{
    keyInRange_.reset();
}

void SPiano::addKeyRange(int start, int end)
{
    start = std::min(127, std::max(0, start));
    end = std::min(127, std::max(0, end));

    for (int x = start; x <= end; ++x)
        keyInRange_.set(x);
}

CCoord SPiano::getKeyWidth()
{
    return 6.0;
}

CCoord SPiano::getKeySwitchesHeight()
{
    return 20.0;
}

CCoord SPiano::getKeyRangesHeight()
{
    return 11.0;
}

CCoord SPiano::getKeysHeight() const
{
    return getHeight() -
        (getKeySwitchesHeight() + getKeyRangesHeight() + getOctavesHeight());
}

CCoord SPiano::getOctavesHeight() const
{
    return font_->getSize();
}

void SPiano::getZoneDimensions(
    CRect* pKeySwitches,
    CRect* pKeyboard,
    CRect* pKeyRanges,
    CRect* pOctaves)
{
    CRect bounds = getViewSize();

    CRect keySwitches(bounds);
    keySwitches.setHeight(getKeySwitchesHeight());

    CRect keyboard(bounds);
    keyboard.top = keySwitches.bottom;
    keyboard.setHeight(getKeysHeight());

    CRect keyRanges(bounds);
    keyRanges.top = keyboard.bottom;
    keyRanges.setHeight(getKeyRangesHeight());

    CRect octaves(bounds);
    octaves.top = keyRanges.bottom;
    octaves.setHeight(getOctavesHeight());

    // apply some paddings
    keySwitches.extend(-2.0, -2.0);
    keyboard.extend(-2.0, -2.0);
    keyRanges.extend(-2.0, -4.0);
    octaves.extend(-2.0, -2.0);

    // offsets for centered keyboard
    CCoord keyWidth = getKeyWidth();
    CCoord offset = std::round((keyboard.getWidth() - (128.0 * keyWidth)) * 0.5);
    if (offset > 0) {
        keySwitches.extend(-offset, 0.0);
        keyboard.extend(-offset, 0.0);
        keyRanges.extend(-offset, 0.0);
        octaves.extend(-offset, 0.0);
    }

    //
    if (pKeySwitches)
        *pKeySwitches = keySwitches;
    if (pKeyboard)
        *pKeyboard = keyboard;
    if (pKeyRanges)
        *pKeyRanges = keyRanges;
    if (pOctaves)
        *pOctaves = octaves;
}

void SPiano::draw(CDrawContext* dc)
{
    CRect bounds = getViewSize();

    dc->setDrawMode(kAntiAliasing);

    SharedPointer<CGraphicsPath> path;

    path = owned(dc->createGraphicsPath());
    path->addRoundRect(bounds, 5.0);
    dc->setFillColor(CColor(0xca, 0xca, 0xca));
    dc->drawGraphicsPath(path, CDrawContext::kPathFilled);

    //
    CRect rectKeySwitches;
    CRect rectKeyboard;
    CRect rectKeyRanges;
    CRect rectOctaves;
    getZoneDimensions(&rectKeySwitches, &rectKeyboard, &rectKeyRanges, &rectOctaves);

    //
    path = owned(dc->createGraphicsPath());
    path->addRoundRect(rectKeyboard, 1.0);
    dc->setFillColor(CColor(0xff, 0xff, 0xff));
    dc->drawGraphicsPath(path, CDrawContext::kPathFilled);

    CCoord keyWidth = getKeyWidth();
    for (int key = 0; key < 128; ++key) {
        CCoord keyX = rectKeyboard.left + key * keyWidth;
        int key12 = key % 12;
        if (key12 == 1 || key12 == 3 ||
            key12 == 6 || key12 == 8 || key12 == 10)
        {
            CRect blackRect(keyX, rectKeyboard.top + 2, keyX + keyWidth, rectKeyboard.bottom - 2);
            path = owned(dc->createGraphicsPath());
            path->addRoundRect(blackRect, 1.0);
            dc->setFillColor(CColor(0x02, 0x02, 0x02));
            dc->drawGraphicsPath(path, CDrawContext::kPathFilled);
        }
        if (key != 0 && key12 == 0) {
            dc->setLineWidth(1.5);
            dc->setFrameColor(CColor(0x63, 0x63, 0x63));
            dc->drawLine(CPoint(keyX, rectKeyboard.top), CPoint(keyX, rectKeyboard.bottom));
        }
        if (key12 == 5) {
            CCoord pad = rectKeyboard.getHeight() * 0.4;
            dc->setLineWidth(1.0);
            dc->setFrameColor(CColor(0x63, 0x63, 0x63));
            dc->drawLine(CPoint(keyX, rectKeyboard.top + pad), CPoint(keyX, rectKeyboard.bottom - pad));
        }
    }

    //

    for (int rangeStart = 0; rangeStart < 128;)
    {
        if (!keyInRange_[rangeStart]) {
            ++rangeStart;
        }
        else {
            int rangeEnd = rangeStart;
            while (rangeEnd + 1 < 128 && keyInRange_[rangeEnd + 1])
                ++rangeEnd;

            CCoord rangeStartX = rectKeyRanges.left + rangeStart * keyWidth;
            CCoord rangeEndX = rectKeyRanges.left + (rangeEnd + 1.0) * keyWidth;
            CRect rectRange(rangeStartX, rectKeyRanges.top, rangeEndX, rectKeyRanges.bottom);

            path = owned(dc->createGraphicsPath());
            path->addRoundRect(rectRange, 2.0);
            dc->setFillColor(CColor(0x0f, 0x0f, 0x0f));
            dc->drawGraphicsPath(path, CDrawContext::kPathFilled);

            rangeStart = rangeEnd + 1;
        }
    }

    //

    for (int key = 0; key < 128; ++key) {
        CCoord keyX = rectOctaves.left + key * keyWidth;
        int key12 = key % 12;
        if (key12 == 0) {
            CRect textRect(keyX, rectOctaves.top, keyX + 12 * keyWidth, rectOctaves.bottom);
            dc->setFont(font_);
            dc->setFontColor(CColor(0x63, 0x63, 0x63));
            dc->drawString(std::to_string(key / 12 - 1).c_str(), textRect, kLeftText);
        }
    }

    //
}

///
SValueMenu::SValueMenu(const CRect& bounds, IControlListener* listener, int32_t tag)
    : CParamDisplay(bounds), menuListener_(owned(new MenuListener(*this)))
{
    setListener(listener);
    setTag(tag);
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

void SValueMenu::onItemClicked(int32_t index)
{
    float oldValue = getValue();
    setValue(menuItemValues_[index]);
    if (getValue() != oldValue)
        valueChanged();
}

///
void STextButton::setHoverColor (const CColor& color)
{
    hoverColor_ = color;
}

void STextButton::draw(CDrawContext* context)
{
    CColor backupColor = textColor;
    if (hovered) {
        textColor = hoverColor_; // textColor is protected
    }
    CTextButton::draw(context);
    if (hovered)
        textColor = backupColor;
}


CMouseEventResult STextButton::onMouseEntered (CPoint& where, const CButtonState& buttons)
{
    hovered = true;
    setDirty();
    return CTextButton::onMouseEntered(where, buttons);
}

CMouseEventResult STextButton::onMouseExited (CPoint& where, const CButtonState& buttons)
{
    hovered = false;
    setDirty();
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
    setDirty();
}

void SStyledKnob::setInactiveTrackColor(const CColor& color)
{
    if (inactiveTrackColor_ == color)
        return;
    inactiveTrackColor_ = color;
    setDirty();
}

void SStyledKnob::setLineIndicatorColor(const CColor& color)
{
    if (lineIndicatorColor_ == color)
        return;
    lineIndicatorColor_ = color;
    setDirty();
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
