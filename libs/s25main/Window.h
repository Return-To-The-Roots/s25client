// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "Msgbox.h"
#include "Rect.h"
#include "TextFormatSetter.h"
#include "animation/AnimationManager.h"
#include "ogl/FontStyle.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/Nation.h"
#include "gameTypes/TextureColor.h"
#include "s25util/colors.h"
#include <boost/range/adaptor/map.hpp>
#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <vector>

class ctrlBuildingIcon;
class ctrlButton;
class ctrlChat;
class ctrlCheck;
class ctrlComboBox;
class ctrlDeepening;
class ctrlEdit;
class ctrlGroup;
class ctrlImage;
class ctrlList;
class ctrlMapSelection;
class ctrlMultiline;
class ctrlMultiSelectGroup;
class ctrlOptionGroup;
class ctrlPercent;
class ctrlPreviewMinimap;
class ctrlProgress;
class ctrlScrollBar;
class ctrlTab;
class ctrlTable;
class ctrlText;
class ctrlTimer;
class ctrlVarDeepening;
class ctrlVarText;
class glArchivItem_Bitmap;
class glFont;
class ITexture;
struct MouseCoords;
enum class GroupSelectType : unsigned;
struct KeyEvent;
struct ScreenResizeEvent;
struct TableColumn;
struct SelectionMapInputData;

namespace libsiedler2 {
class ArchivItem_Map;
}

/// Base class for windows and controls
class Window
{
public:
    using KeyboardMsgHandler = bool (Window::*)(const KeyEvent&);
    using MouseMsgHandler = bool (Window::*)(const MouseCoords&);

    Window(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size = Extent(0, 0));
    virtual ~Window();
    /// Draw all contained controls if the window is visible
    void draw();
    /// Get the current position relative to the parent window
    DrawPoint getPos() const;
    /// Get the absolute position for drawing
    DrawPoint getDrawPos() const;
    /// Get the size of the window
    Extent getSize() const;
    /// Get the extent of the window in absolute coordinates
    Rect getDrawRect() const;
    /// Get the actual extents of the rect
    /// (might be different to the draw rect if the window resizes according to content)
    virtual Rect getBoundaryRect() const;
    /// Change the size
    virtual void resize(const Extent& newSize) { size_ = newSize; }
    /// Change only the width
    void setWidth(unsigned width) { resize(Extent(width, size_.y)); }
    /// Change only the height
    void setHeight(unsigned height) { resize(Extent(size_.x, height)); }
    /// Send a keyboard message to all controls, return true if handled
    bool relayKeyboardMessage(KeyboardMsgHandler msg, const KeyEvent& ke);
    /// Send a mouse message to all controls, return true if handled
    bool relayMouseMessage(MouseMsgHandler msg, const MouseCoords& mc);
    /// Make the window active or inactive. Inactive controls e.g. don't react to events
    virtual void setActive(bool activate = true);
    /// Activate/deactivate only the elements of the window
    void activateControls(bool activate = true);
    /// Lock a region which won't react to mouse events anymore except for the given window/control.
    /// Only a single region can be locked per window.
    void lockRegion(Window* window, const Rect& rect);
    /// Release the region locked for the given window/control.
    void freeRegion(Window* window);
    /// Check if the given point is in a region locked by any window other than exception
    bool isInLockedRegion(const Position& pos, const Window* exception = nullptr) const;
    /// Check if the mouse is hovering over this control, i.e. inside its boundary.
    bool isMouseOver() const;
    /// Check if the given mouse position inside the boundary of this control.
    bool isMouseOver(const MouseCoords& mousePos) const;

    /// Set the position for the window
    void setPos(const DrawPoint& newPos);

    // Make the window visible or hide it
    virtual void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    bool IsActive() const { return active_; }
    /// Get the parent window (containing this) or nullptr if this is a top-level window
    Window* getParent() const { return parent_; }
    unsigned getID() const { return id_; }
    /// Get control with given ID of given type or nullptr if not found or other type
    template<typename T>
    T* getCtrl(unsigned id);
    /// Get control with given ID of given type or nullptr if not found or other type
    template<typename T>
    const T* getCtrl(unsigned id) const;

    /// Get all controls of given type
    template<typename T>
    std::vector<T*> getCtrls();
    /// Get all controls of given type
    template<typename T>
    std::vector<const T*> getCtrls() const;

    void deleteCtrl(unsigned id);

    AnimationManager& getAnimationManager() { return animations_; }

    template<typename T>
    T* addCtrl(std::unique_ptr<T> ctrl);

    ctrlBuildingIcon* addBuildingIcon(unsigned id, const DrawPoint& pos, BuildingType type, Nation nation,
                                      unsigned short size = 36, const std::string& tooltip = "");
    ctrlButton* addTextButton(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                              const std::string& text, const glFont* font, const std::string& tooltip = "");
    ctrlButton* addColorButton(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                               unsigned fillColor, const std::string& tooltip = "");
    ctrlButton* addImageButton(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, ITexture* image,
                               const std::string& tooltip = "");
    ctrlButton* addImageButton(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                               glArchivItem_Bitmap* image, const std::string& tooltip = "");
    ctrlChat* addChatCtrl(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font);
    ctrlCheck* addCheckBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                           const std::string& text, const glFont* font, bool readonly = false);
    ctrlComboBox* addComboBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                              const glFont* font, unsigned short max_list_height, bool readonly = false);
    ctrlDeepening* addTextDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                    const std::string& text, const glFont* font, unsigned color,
                                    FontStyle style = FontStyle::CENTER | FontStyle::VCENTER);
    ctrlDeepening* addColorDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                     unsigned fillColor);
    ctrlDeepening* addImageDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                     ITexture* image);
    ctrlDeepening* addImageDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                     glArchivItem_Bitmap* image);

    ctrlEdit* addEdit(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
                      unsigned short maxlength = 0, bool password = false, bool disabled = false, bool notify = false);
    ctrlGroup* addGroup(unsigned id);
    ctrlImage* addImage(unsigned id, const DrawPoint& pos, ITexture* image, const std::string& tooltip = "");
    ctrlImage* addImage(unsigned id, const DrawPoint& pos, glArchivItem_Bitmap* image, const std::string& tooltip = "");
    ctrlList* addList(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font);
    ctrlMultiline* addMultiline(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                const glFont* font, FontStyle format = {});
    ctrlOptionGroup* addOptionGroup(unsigned id, GroupSelectType select_type);
    ctrlMultiSelectGroup* addMultiSelectGroup(unsigned id, GroupSelectType select_type);
    ctrlPercent* addPercent(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, unsigned text_color,
                            const glFont* font, const unsigned short* percentage);
    ctrlProgress* addProgress(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                              unsigned short button_minus, unsigned short button_plus, unsigned short maximum,
                              const std::string& tooltip = "", const Extent& padding = Extent(0, 0),
                              unsigned force_color = 0, const std::string& button_minus_tooltip = "",
                              const std::string& button_plus_tooltip = "");
    ctrlScrollBar* addScrollBar(unsigned id, const DrawPoint& pos, const Extent& size, unsigned short button_height,
                                TextureColor tc, unsigned short page_size);
    ctrlTab* addTabCtrl(unsigned id, const DrawPoint& pos, unsigned short width);
    ctrlTable* addTable(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
                        std::vector<TableColumn> columns);
    /// Add text
    /// @param color    Text color (ARGB)
    /// @param format   can be a combination of FontStyle::LEFT/CENTER/RIGHT and FontStyle::TOP/VCENTER/BOTTOM and
    /// FontStyle::OUTLINE/NO_OUTLINE
    ///                 Alignment specifies how the position is treated, i.e. where relative to the text it will be.
    ctrlText* addText(unsigned id, const DrawPoint& pos, const std::string& text, unsigned color, FontStyle format,
                      const glFont* font);
    ctrlMapSelection* addMapSelection(unsigned id, const DrawPoint& pos, const Extent& size,
                                      const SelectionMapInputData& inputData);
    TextFormatSetter addFormattedText(unsigned id, const DrawPoint& pos, const std::string& text, unsigned color,
                                      FontStyle format, const glFont* font);
    ctrlTimer* addTimer(unsigned id, std::chrono::milliseconds timeout);
    /// Add a 3D text control with a variable text. The text is formatted like printf but with pointers
    /// to int (%d), unsigned (%u) or const char (%s) which must be valid for the lifetime of the var text!
    ctrlVarDeepening* addVarDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                      const std::string& formatstr, const glFont* font, unsigned color,
                                      unsigned parameters, ...);
    /// Add a text control with a variable text. The text is formatted like printf but with pointers
    /// to int (%d), unsigned (%u) or const char (%s) which must be valid for the lifetime of the var text!
    ctrlVarText* addVarText(unsigned id, const DrawPoint& pos, const std::string& formatstr, unsigned color,
                            FontStyle format, const glFont* font, unsigned parameters, ...);
    ctrlPreviewMinimap* addPreviewMinimap(unsigned id, const DrawPoint& pos, const Extent& size,
                                          libsiedler2::ArchivItem_Map* map);

    /// Draw a 3D rectangle (e.g. button)
    static void draw3D(const Rect& rect, TextureColor tc, bool elevated, bool highlighted = false,
                       bool illuminated = false, unsigned contentColor = COLOR_WHITE);
    static void draw3DBorder(const Rect& rect, TextureColor tc, bool elevated);
    static void draw3DContent(const Rect& rect, TextureColor tc, bool elevated, bool highlighted = false,
                              bool illuminated = false, unsigned contentColor = COLOR_WHITE);
    static void drawRectangle(const Rect& rect, unsigned color);
    static void drawLine(DrawPoint pt1, DrawPoint pt2, unsigned short width, unsigned color);

    // GUI-Notify-Messages

    // These messages get passed downwards (WindowManager to controls)
    // Return true if the message was handled
    virtual void msgPaintBefore();
    virtual void msgPaintAfter();
    virtual bool msgLeftDown(const MouseCoords&) { return false; }
    virtual bool msgRightDown(const MouseCoords&) { return false; }
    virtual bool msgMiddleDown(const MouseCoords&) { return false; }
    virtual bool msgLeftUp(const MouseCoords&) { return false; }
    virtual bool msgRightUp(const MouseCoords&) { return false; }
    virtual bool msgMiddleUp(const MouseCoords&) { return false; }
    virtual bool msgWheelUp(const MouseCoords&) { return false; }
    virtual bool msgWheelDown(const MouseCoords&) { return false; }
    virtual bool msgMouseMove(const MouseCoords&) { return false; }
    virtual bool msgKeyDown(const KeyEvent&) { return false; }
    virtual void msgScreenResize(const ScreenResizeEvent& sr);

    // Callback messages that are passed upwards (from controls to window)
    virtual void msgButtonClick(unsigned /*ctrl_id*/) {}
    virtual void msgEditEnter(unsigned /*ctrl_id*/) {}
    virtual void msgEditChange(unsigned /*ctrl_id*/) {}
    virtual void msgTabChange(unsigned /*ctrl_id*/, unsigned short /*tab_id*/) {}
    virtual void msgListSelectItem(unsigned /*ctrl_id*/, int /*selection*/) {}
    virtual void msgListChooseItem(unsigned /*ctrl_id*/, unsigned /*selection*/) {}
    virtual void msgComboSelectItem(unsigned /*ctrl_id*/, unsigned /*selection*/) {}
    virtual void msgCheckboxChange(unsigned /*ctrl_id*/, bool /*checked*/) {}
    virtual void msgProgressChange(unsigned /*ctrl_id*/, unsigned short /*position*/) {}
    virtual void msgScrollChange(unsigned /*ctrl_id*/, unsigned short /*position*/) {}
    virtual void msgScrollShow(unsigned /*ctrl_id*/, bool /*visible*/) {}
    virtual void msgOptionGroupChange(unsigned /*ctrl_id*/, unsigned /*selection*/) {}
    virtual void msgTimer(unsigned /*ctrl_id*/) {}
    virtual void msgTableSelectItem(unsigned /*ctrl_id*/, const std::optional<unsigned>& /*selection*/) {}
    virtual void msgTableChooseItem(unsigned /*ctrl_id*/, unsigned /*selection*/) {}
    virtual void msgTableRightButton(unsigned /*ctrl_id*/, const std::optional<unsigned>& /*selection*/) {}
    virtual void msgTableLeftButton(unsigned /*ctrl_id*/, const std::optional<unsigned>& /*selection*/) {}

    /// Callback of a message box when closed
    virtual void msgMsgBoxResult(unsigned /*msgbox_id*/, MsgboxResult /*mbr*/) {}

    // Callbacks triggered by controls of ctrlGroup
    virtual void msgGroupButtonClick(unsigned /*group_id*/, unsigned /*ctrl_id*/) {}
    virtual void msgGroupEditEnter(unsigned /*group_id*/, unsigned /*ctrl_id*/) {}
    virtual void msgGroupEditChange(unsigned /*group_id*/, unsigned /*ctrl_id*/) {}
    virtual void msgGroupTabChange(unsigned /*group_id*/, unsigned /*ctrl_id*/, unsigned short /*tab_id*/) {}
    virtual void msgGroupListSelectItem(unsigned /*group_id*/, unsigned /*ctrl_id*/, int /*selection*/) {}
    virtual void msgGroupComboSelectItem(unsigned /*group_id*/, unsigned /*ctrl_id*/, unsigned /*selection*/) {}
    virtual void msgGroupCheckboxChange(unsigned /*group_id*/, unsigned /*ctrl_id*/, bool /*checked*/) {}
    virtual void msgGroupProgressChange(unsigned /*group_id*/, unsigned /*ctrl_id*/, unsigned short /*position*/) {}
    virtual void msgGroupScrollShow(unsigned /*group_id*/, unsigned /*ctrl_id*/, bool /*visible*/) {}
    virtual void msgGroupOptionGroupChange(unsigned /*group_id*/, unsigned /*ctrl_id*/, unsigned /*selection*/) {}
    virtual void msgGroupTimer(unsigned /*group_id*/, unsigned /*ctrl_id*/) {}
    virtual void msgGroupTableSelectItem(unsigned /*group_id*/, unsigned /*ctrl_id*/,
                                         const std::optional<unsigned>& /*selection*/)
    {}
    virtual void msgGroupTableRightButton(unsigned /*group_id*/, unsigned /*ctrl_id*/,
                                          const std::optional<unsigned>& /*selection*/)
    {}
    virtual void msgGroupTableLeftButton(unsigned /*group_id*/, unsigned /*ctrl_id*/,
                                         const std::optional<unsigned>& /*selection*/)
    {}

    enum class ButtonState
    {
        Up,
        Hover,
        Pressed
    };
    friend constexpr auto maxEnumValue(ButtonState) { return ButtonState::Pressed; }

protected:
    using ControlMap = std::map<unsigned, std::unique_ptr<Window>>;

    /// Scale the value from the reference coordinates to current render size
    template<class T_Pt>
    static T_Pt scale(const T_Pt& pt);
    /// Scales the value when scale_ is true, else returns the value unchanged
    template<class T_Pt>
    T_Pt scaleIf(const T_Pt& pt) const;
    /// Set whether controls of this window shall be scaled
    void setScale(bool scale = true) { scale_ = scale; }
    /// Implementation of drawing the window, derived classes can override this to draw custom backgrounds or similar
    virtual void drawImpl();
    /// Shall messages be relayed to the controls of this window?
    virtual bool isMessageRelayAllowed() const;

private:
    Window* const parent_; /// Handle to parent window
    unsigned id_;          /// ID of the window, must be unique among siblings
    DrawPoint pos_;        /// Position relative to parent window.
    Extent size_;          /// Size of the window
    bool active_;          /// Window active?
    bool visible_;         /// Window visible?
    bool scale_;           /// Shall the controls of this window be scaled according to the render size?

    /// Locked areas for mouse events.
    /// The key is the window/control for which the area is locked, i.e. which control is the only one getting mouse
    /// events from this region, the value is the locked area relative to this window. Only a single area can be locked
    /// per window/control.
    std::map<Window*, Rect> lockedAreas_;
    std::vector<Window*> tofreeAreas_;
    bool isInMouseRelay;
    ControlMap childIdToWnd_; /// Controls contained in this window, mapped by their ID
    AnimationManager animations_;
};

template<typename T>
T* Window::addCtrl(std::unique_ptr<T> ctrl)
{
    RTTR_Assert(ctrl);
    RTTR_Assert(childIdToWnd_.find(ctrl->getID()) == childIdToWnd_.end());

    T* ctrlPtr = ctrl.get();
    ctrl->scale_ = scale_;
    childIdToWnd_.emplace(ctrl->getID(), std::move(ctrl));

    ctrlPtr->setActive(active_);
    return ctrlPtr;
}

template<typename T>
T* Window::getCtrl(unsigned id)
{
    return const_cast<T*>(static_cast<const Window&>(*this).getCtrl<T>(id));
}

template<typename T>
const T* Window::getCtrl(unsigned id) const
{
    auto it = childIdToWnd_.find(id);
    if(it == childIdToWnd_.end())
        return nullptr;

    return dynamic_cast<T*>(it->second.get());
}

template<typename T>
std::vector<T*> Window::getCtrls()
{
    std::vector<T*> result;
    for(const auto& wnd : childIdToWnd_ | boost::adaptors::map_values)
    {
        T* ctrl = dynamic_cast<T*>(wnd.get());
        if(ctrl)
            result.push_back(ctrl);
    }
    return result;
}

template<typename T>
std::vector<const T*> Window::getCtrls() const
{
    std::vector<const T*> result;
    for(const auto& wnd : childIdToWnd_ | boost::adaptors::map_values)
    {
        const T* ctrl = dynamic_cast<const T*>(wnd.get());
        if(ctrl)
            result.push_back(ctrl);
    }
    return result;
}
