// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <bitset>
#include <vector>
#include <memory>
#include <functional>

#include "utility/vstgui_before.h"
#include "vstgui/lib/controls/cslider.h"
#include "vstgui/lib/controls/cknob.h"
#include "vstgui/lib/controls/ctextlabel.h"
#include "vstgui/lib/controls/ctextedit.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "vstgui/lib/controls/coptionmenu.h"
#include "vstgui/lib/controls/cscrollbar.h"
#include "vstgui/lib/controls/icontrollistener.h"
#include "vstgui/lib/cviewcontainer.h"
#include "vstgui/lib/cscrollview.h"
#include "vstgui/lib/ccolor.h"
#include "vstgui/lib/dragging.h"
#include "utility/vstgui_after.h"

using namespace VSTGUI;

///
class SBoxContainer : public CViewContainer {
public:
    explicit SBoxContainer(const CRect& size);
    virtual ~SBoxContainer() {}
    void setCornerRadius(CCoord radius);
    void setBackgroundColor(const CColor& color) override;
    CColor getBackgroundColor() const override;

protected:
    void drawRect(CDrawContext* dc, const CRect& updateRect) override;

protected:
    CCoord cornerRadius_ = 0.0;
    CColor backgroundColor_;
};

///
class STitleContainer : public SBoxContainer {
public:
    explicit STitleContainer(const CRect& size, UTF8StringPtr text = nullptr);
    ~STitleContainer() {}

    void setTitleFont(CFontRef font);
    CFontRef getTitleFont() { return titleFont_; }

    void setTitleFontColor(CColor color);
    CColor getTitleFontColor() const { return titleFontColor_; }
    void setTitleBackgroundColor(CColor color);
    CColor getTitleBackgroundColor() const { return titleBackgroundColor_; }

protected:
    void drawRect(CDrawContext* dc, const CRect& updateRect) override;

private:
    std::string text_;
    CColor titleFontColor_;
    CColor titleBackgroundColor_;
    SharedPointer<CFontDesc> titleFont_;
};

///
class SFileDropTarget : public IDropTarget,
                        public NonAtomicReferenceCounted {
public:
    typedef std::function<void(const std::string&)> FileDropFunction;
    void setFileDropFunction(FileDropFunction f);

protected:
    DragOperation onDragEnter(DragEventData data) override;
    DragOperation onDragMove(DragEventData data) override;
    void onDragLeave(DragEventData data) override;
    bool onDrop(DragEventData data) override;

private:
    static bool isFileDrop(IDataPackage* package);

private:
    DragOperation op_ = DragOperation::None;
    FileDropFunction dropFunction_;
};

///
class SValueMenu : public CParamDisplay {
public:
    explicit SValueMenu(const CRect& bounds, IControlListener* listener, int32_t tag);
    CColor getHoverColor() const { return hoverColor_; }
    void setHoverColor(const CColor& color);
    CMenuItem* addEntry(CMenuItem* item, float value, int32_t index = -1);
    CMenuItem* addEntry(const UTF8String& title, float value, int32_t index = -1, int32_t itemFlags = CMenuItem::kNoFlags);
    CMenuItem* addSeparator(int32_t index = -1);
    int32_t getNbEntries() const;

protected:
    void draw(CDrawContext* dc) override;
    CMouseEventResult onMouseEntered(CPoint& where, const CButtonState& buttons) override;
    CMouseEventResult onMouseExited(CPoint& where, const CButtonState& buttons) override;
    CMouseEventResult onMouseDown(CPoint& where, const CButtonState& buttons) override;
    bool onWheel(const CPoint& where, const CMouseWheelAxis& axis, const float& distance, const CButtonState& buttons) override;

private:
    CColor hoverColor_;
    bool hovered_ = false;

    //
    class MenuListener;

    //
    void onItemClicked(int32_t index);

    //
    CMenuItemList menuItems_;
    std::vector<float> menuItemValues_;
    SharedPointer<MenuListener> menuListener_;

    //
    class MenuListener : public IControlListener, public NonAtomicReferenceCounted {
    public:
        explicit MenuListener(SValueMenu& menu) : menu_(menu) {}
        void valueChanged(CControl* control) override
        {
            menu_.onItemClicked(static_cast<int32_t>(control->getValue()));
        }
    private:
        SValueMenu& menu_;
    };
};

///
class SActionMenu : public CParamDisplay {
public:
    explicit SActionMenu(const CRect& bounds, IControlListener* listener);
    std::string getTitle() const { return title_; }
    void setTitle(std::string title);
    CColor getHoverColor() const { return hoverColor_; }
    void setHoverColor(const CColor& color);
    CMenuItem* addEntry(CMenuItem* item, int32_t tag, int32_t index = -1);
    CMenuItem* addEntry(const UTF8String& title, int32_t tag, int32_t index = -1, int32_t itemFlags = CMenuItem::kNoFlags);
    CMenuItem* addSeparator(int32_t index = -1);
    int32_t getNbEntries() const;

protected:
    void draw(CDrawContext* dc) override;
    CMouseEventResult onMouseEntered(CPoint& where, const CButtonState& buttons) override;
    CMouseEventResult onMouseExited(CPoint& where, const CButtonState& buttons) override;
    CMouseEventResult onMouseDown(CPoint& where, const CButtonState& buttons) override;

private:
    std::string title_;
    CColor hoverColor_;
    bool hovered_ = false;

    class MenuListener;

    //
    void onItemClicked(int32_t index);

    //
    CMenuItemList menuItems_;
    std::vector<int32_t> menuItemTags_;
    SharedPointer<MenuListener> menuListener_;

    //
    class MenuListener : public IControlListener, public NonAtomicReferenceCounted {
    public:
        explicit MenuListener(SActionMenu& menu) : menu_(menu) {}
        void valueChanged(CControl* control) override
        {
            menu_.onItemClicked(static_cast<int32_t>(control->getValue()));
        }
    private:
        SActionMenu& menu_;
    };
};

///
class STextButton: public CTextButton {
public:
    STextButton(const CRect& size, IControlListener* listener = nullptr, int32_t tag = -1, UTF8StringPtr title = nullptr)
    : CTextButton(size, listener, tag, title) {}

    CColor getHighlightColor() const { return highlightColor_; }
    void setHighlightColor(const CColor& color);
    CColor getInactiveColor() const { return inactiveColor_; }
    void setInactiveColor(const CColor& color);
    bool isInactive() const { return inactive_; }
    void setInactive(bool b);
    bool isHighlighted() const { return highlighted_; }
    void setHighlighted(bool b);
    CMouseEventResult onMouseEntered (CPoint& where, const CButtonState& buttons) override;
    CMouseEventResult onMouseExited (CPoint& where, const CButtonState& buttons) override;
    void draw(CDrawContext* context) override;

    std::function<void()> OnHoverEnter;
    std::function<void()> OnHoverLeave;

private:
    CColor highlightColor_;
    bool hovered_ { false };
    bool highlighted_ { false };
    CColor inactiveColor_;
    bool inactive_ { false };
};

///
class SStyledKnob : public CKnobBase {
public:
    SStyledKnob(const CRect& size, IControlListener* listener, int32_t tag);

    const CColor& getActiveTrackColor() const { return activeTrackColor_; }
    void setActiveTrackColor(const CColor& color);

    const CColor& getInactiveTrackColor() const { return inactiveTrackColor_; }
    void setInactiveTrackColor(const CColor& color);

    const CColor& getLineIndicatorColor() const { return lineIndicatorColor_; }
    void setLineIndicatorColor(const CColor& color);

    void setFont(CFontRef font);
    CFontRef getFont() const { return font_; }

    void setFontColor(CColor fontColor);
    CColor getFontColor() const { return fontColor_; }

    using ValueToStringFunction = std::function<bool(float value, std::string& result)>;
    void setValueToStringFunction(ValueToStringFunction func);

    void setHideValue(bool hide) { hideValue_ = hide; invalid(); }
    bool getHideValue() const { return hideValue_; }

    CLASS_METHODS(SStyledKnob, CKnobBase)
protected:
    void draw(CDrawContext* dc) override;

private:
    CColor activeTrackColor_;
    CColor inactiveTrackColor_;
    CColor lineIndicatorColor_;
    bool hideValue_ { false };

    SharedPointer<CFontDesc> font_ = kNormalFont;
    CColor fontColor_ { 0x00, 0x00, 0x00 };

    ValueToStringFunction valueToStringFunction_;
};

class CFilledRect : public CView
{
public:
    explicit CFilledRect(const CRect& size)
    : CView(size) {}

    void setRadius(CCoord radius) { radius_ = radius; invalid(); }
    CCoord getRadius() const { return radius_; }

    void setColor(CColor color){ color_ = color; invalid(); }
    CColor getColor() { return color_; }
protected:
    void draw(CDrawContext* dc) override;
private:
    CCoord radius_  { 5.0 };
    CColor color_  { 0, 0, 0, 70 };
};

///
class SKnobCCBox : public CViewContainer, ViewListenerAdapter {
public:
    SKnobCCBox(const CRect& size, IControlListener* listener, int32_t tag);
    ~SKnobCCBox();
    void setHue(float hue);

    float getValue() const { return knob_->getValue(); }
    float getDefaultValue() const { return knob_->getDefaultValue(); }
    void setValue(float value);
    void setDefaultValue(float value);

    void setNameLabelText(const UTF8String& name) { label_->setText(name); label_->invalid(); }
    void setCCLabelText(const UTF8String& name) { ccLabel_->setText(name); ccLabel_->invalid(); }

    void setNameLabelFont(CFontRef font);
    CFontRef getNameLabelFont() const { return label_->getFont(); }

    void setNameLabelFontColor(CColor color) { label_->setFontColor(color); label_->invalid(); }
    CColor getNameLabelFontColor() const { return label_->getFontColor(); }

    void setValueEditFont(CFontRef font);
    CFontRef getValueEditFont() const { return label_->getFont(); }

    void setValueEditFontColor(CColor color) { valueEdit_->setFontColor(color); valueEdit_->invalid(); }
    CColor getValueEditFontColor() const { return valueEdit_->getFontColor(); }

    void setValueEditBackColor(CColor color) { valueEdit_->setBackColor(color); valueEdit_->invalid(); }
    CColor getValueEditBackColor() const { return valueEdit_->getBackColor(); }

    void setShadingRectangleColor(CColor color) { shadingRectangle_->setColor(color); shadingRectangle_->invalid(); }
    CColor getShadingRectangleColor() const { return shadingRectangle_->getColor(); }

    void setCCLabelFont(CFontRef font);
    CFontRef getCCLabelFont() const { return ccLabel_->getFont(); }

    void setCCLabelFontColor(CColor color) { ccLabel_->setFontColor(color); ccLabel_->invalid(); }
    CColor getCCLabelFontColor() const { return ccLabel_->getFontColor(); }

    void setCCLabelBackColor(CColor color) { ccLabel_->setBackColor(color); ccLabel_->invalid(); }
    CColor getCCLabelBackColor() const { return ccLabel_->getBackColor(); }

    void setKnobActiveTrackColor(CColor color) { knob_->setActiveTrackColor(color); knob_->invalid(); }
    CColor getKnobActiveTrackColor() const { return knob_->getActiveTrackColor(); }

    void setKnobInactiveTrackColor(CColor color) { knob_->setInactiveTrackColor(color); knob_->invalid(); }
    CColor getKnobInactiveTrackColor() const { return knob_->getInactiveTrackColor(); }

    void setKnobLineIndicatorColor(CColor color) { knob_->setLineIndicatorColor(color); knob_->invalid(); }
    CColor getKnobLineIndicatorColor() const { return knob_->getLineIndicatorColor(); }

    void setKnobFont(CFontRef font) { knob_->setFont(font); knob_->invalid(); }
    CFontRef getKnobFont() const { return knob_->getFont(); }

    void setKnobFontColor(CColor color) { knob_->setFontColor(color); knob_->invalid(); }
    CColor getKnobFontColor() const { return knob_->getFontColor(); }

    // Edit box listener
	void viewLostFocus (CView* view) override;

    bool isHD() const noexcept { return hdMode_; }
    void setHDMode(bool mode);

protected:
    CMouseEventResult onMouseDown(CPoint& where, const CButtonState& buttons) override;

private:
    void updateViewSizes();
    void updateViewColors();
    SharedPointer<CTextLabel> label_;
    SharedPointer<CTextEdit> valueEdit_;
    SharedPointer<SStyledKnob> knob_;
    SharedPointer<CTextLabel> ccLabel_;
    SharedPointer<CFilledRect> shadingRectangle_;
    SharedPointer<CMenuItem> menuEntry_;
    CRect nameLabelSize_;
    CRect knobSize_;
    CRect shadingRectangleSize_;
    CRect ccLabelSize_;
    CRect valueEditSize_;
    CRect rectangleSize_;
    float hue_ = 0.35;

    class MenuListener : public IControlListener, public NonAtomicReferenceCounted {
    public:
        explicit MenuListener(SKnobCCBox& box) : box_(box) {}
        void valueChanged(CControl*) override
        {
            box_.setHDMode(!box_.isHD());
        }
    private:
        SKnobCCBox& box_;
    };
    SharedPointer<MenuListener> menuListener_;
    bool hdMode_ { false };
};

///
class SControlsPanel : public CScrollView {
public:
    explicit SControlsPanel(const CRect& size);

    void setControlUsed(uint32_t index, bool used);
    void setControlValue(uint32_t index, float value);
    void setControlDefaultValue(uint32_t index, float value);
    void setControlLabelText(uint32_t index, UTF8StringPtr text);

    void setNameLabelFont(CFontRef font);
    void setNameLabelFontColor(CColor color);
    void setCCLabelFont(CFontRef font);
    void setCCLabelBackColor(CColor color);
    void setCCLabelFontColor(CColor color);
    void setValueEditBackColor(CColor color);
    void setValueEditFontColor(CColor color);
    void setShadingRectangleColor(CColor color);
    void setKnobActiveTrackColor(CColor color);
    void setKnobInactiveTrackColor(CColor color);
    void setKnobLineIndicatorColor(CColor color);
    void setKnobFont(CFontRef font);
    void setKnobFontColor(CColor color);

    std::function<void(uint32_t, float)> ValueChangeFunction;
    std::function<void(uint32_t)> BeginEditFunction;
    std::function<void(uint32_t)> EndEditFunction;

protected:
    void recalculateSubViews() override;

private:
    void updateLayout();
    void syncAllSlotStyles();
    void syncSlotStyle(uint32_t index);
    static std::string getDefaultLabelText(uint32_t index);

    struct ControlSlot;
    ControlSlot* getSlot(uint32_t index);
    ControlSlot* getOrCreateSlot(uint32_t index);

private:
    struct ControlSlot {
        bool used = false;
        SharedPointer<SKnobCCBox> box;
    };

    class ControlSlotListener : public IControlListener {
    public:
        explicit ControlSlotListener(SControlsPanel* panel) : panel_(panel) {}
        void valueChanged(CControl* pControl) override;
        void controlBeginEdit(CControl* pControl) override;
        void controlEndEdit(CControl* pControl) override;

    private:
        SControlsPanel* panel_ = nullptr;
    };

    std::vector<std::unique_ptr<ControlSlot>> slots_;
    std::unique_ptr<ControlSlotListener> listener_;
    SharedPointer<CVSTGUITimer> relayoutTrigger_;
};

///
class SLevelMeter : public CView {
public:
    explicit SLevelMeter(const CRect& size);

    float getValue() const { return value_; }
    void setValue(float value);

    float getDangerThreshold() const { return dangerThreshold_; }
    void setDangerThreshold(float thres) { dangerThreshold_ = thres; invalid(); }

    CColor getFrameColor() const { return frameColor_; }
    void setFrameColor(CColor color) { frameColor_ = color; invalid(); }
    CColor getBackColor() const { return backColor_; }
    void setBackColor(CColor color) { backColor_ = color; invalid(); }

    CColor getNormalFillColor() const { return safeFillColor_; }
    void setNormalFillColor(CColor color) { safeFillColor_ = color; invalid(); }
    CColor getDangerFillColor() const { return dangerFillColor_; }
    void setDangerFillColor(CColor color) { dangerFillColor_ = color; invalid(); }

    CCoord getRoundRectRadius() const { return radius_; }
    void setRoundRectRadius(CCoord radius) { radius_ = radius; invalid(); }

protected:
    void draw(CDrawContext* dc) override;

private:
    float value_ = 0;
    float dangerThreshold_ = 0.5;
    float dbMin_ = -40;
    float dbMax_ = 0;
    CColor frameColor_;
    CColor safeFillColor_;
    CColor dangerFillColor_;
    CColor backColor_;
    CCoord radius_ = 5.0;
    SharedPointer<CVSTGUITimer> timer_;
    bool timerArmed_ = false;
};

///
class SPlaceHolder : public CView {
public:
    explicit SPlaceHolder(const CRect& size, const CColor& color = {0xff, 0x00, 0x00, 0xff});

protected:
    void draw(CDrawContext* dc) override;

private:
    CColor color_;
};
