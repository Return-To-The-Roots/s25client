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
#include <boost/optional/optional_fwd.hpp>
#include <boost/range/adaptor/map.hpp>
#include <chrono>
#include <map>
#include <memory>
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
    void Draw();
    /// Get the current position relative to the parent window
    DrawPoint GetPos() const;
    /// Get the absolute position for drawing
    DrawPoint GetDrawPos() const;
    /// Get the size of the window
    Extent GetSize() const;
    /// Get the extent of the window in absolute coordinates
    Rect GetDrawRect() const;
    /// Get the actual extents of the rect
    /// (might be different to the draw rect if the window resizes according to content)
    virtual Rect GetBoundaryRect() const;
    /// Change the size
    virtual void Resize(const Extent& newSize) { size_ = newSize; }
    /// Change only the width
    void SetWidth(unsigned width) { Resize(Extent(width, size_.y)); }
    /// Change only the height
    void SetHeight(unsigned height) { Resize(Extent(size_.x, height)); }
    /// Send a keyboard message to all controls, return true if handled
    bool RelayKeyboardMessage(KeyboardMsgHandler msg, const KeyEvent& ke);
    /// Send a mouse message to all controls, return true if handled
    bool RelayMouseMessage(MouseMsgHandler msg, const MouseCoords& mc);
    /// Make the window active or inactive. Inactive controls e.g. don't react to events
    virtual void SetActive(bool activate = true);
    /// Activate/deactivate only the elements of the window
    void ActivateControls(bool activate = true);
    /// Lock a region which won't react to mouse events anymore except for the given window/control.
    /// Only a single region can be locked per window.
    void LockRegion(Window* window, const Rect& rect);
    /// Release the region locked for the given window/control.
    void FreeRegion(Window* window);
    /// Check if the given point is in a region locked by any window other than exception
    bool IsInLockedRegion(const Position& pos, const Window* exception = nullptr) const;
    /// Check if the mouse is hovering over this control, i.e. inside its boundary.
    bool IsMouseOver() const;
    /// Check if the given mouse position inside the boundary of this control.
    bool IsMouseOver(const MouseCoords& mousePos) const;

    /// Set the position for the window
    void SetPos(const DrawPoint& newPos);

    // Make the window visible or hide it
    virtual void SetVisible(bool visible) { visible_ = visible; }
    bool IsVisible() const { return visible_; }
    bool IsActive() const { return active_; }
    /// Get the parent window (containing this) or nullptr if this is a top-level window
    Window* GetParent() const { return parent_; }
    unsigned GetID() const { return id_; }
    /// Get control with given ID of given type or nullptr if not found or other type
    template<typename T>
    T* GetCtrl(unsigned id);
    /// Get control with given ID of given type or nullptr if not found or other type
    template<typename T>
    const T* GetCtrl(unsigned id) const;

    /// Get all controls of given type
    template<typename T>
    std::vector<T*> GetCtrls();
    /// Get all controls of given type
    template<typename T>
    std::vector<const T*> GetCtrls() const;

    void DeleteCtrl(unsigned id);

    AnimationManager& GetAnimationManager() { return animations_; }

    template<typename T>
    T* AddCtrl(std::unique_ptr<T> ctrl);

    ctrlBuildingIcon* AddBuildingIcon(unsigned id, const DrawPoint& pos, BuildingType type, Nation nation,
                                      unsigned short size = 36, const std::string& tooltip = "");
    ctrlButton* AddTextButton(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                              const std::string& text, const glFont* font, const std::string& tooltip = "");
    ctrlButton* AddColorButton(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                               unsigned fillColor, const std::string& tooltip = "");
    ctrlButton* AddImageButton(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, ITexture* image,
                               const std::string& tooltip = "");
    ctrlButton* AddImageButton(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                               glArchivItem_Bitmap* image, const std::string& tooltip = "");
    ctrlChat* AddChatCtrl(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font);
    ctrlCheck* AddCheckBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                           const std::string& text, const glFont* font, bool readonly = false);
    ctrlComboBox* AddComboBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                              const glFont* font, unsigned short max_list_height, bool readonly = false);
    ctrlDeepening* AddTextDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                    const std::string& text, const glFont* font, unsigned color,
                                    FontStyle style = FontStyle::CENTER | FontStyle::VCENTER);
    ctrlDeepening* AddColorDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                     unsigned fillColor);
    ctrlDeepening* AddImageDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                     ITexture* image);
    ctrlDeepening* AddImageDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                     glArchivItem_Bitmap* image);

    ctrlEdit* AddEdit(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
                      unsigned short maxlength = 0, bool password = false, bool disabled = false, bool notify = false);
    ctrlGroup* AddGroup(unsigned id);
    ctrlImage* AddImage(unsigned id, const DrawPoint& pos, ITexture* image, const std::string& tooltip = "");
    ctrlImage* AddImage(unsigned id, const DrawPoint& pos, glArchivItem_Bitmap* image, const std::string& tooltip = "");
    ctrlList* AddList(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font);
    ctrlMultiline* AddMultiline(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                const glFont* font, FontStyle format = {});
    ctrlOptionGroup* AddOptionGroup(unsigned id, GroupSelectType select_type);
    ctrlMultiSelectGroup* AddMultiSelectGroup(unsigned id, GroupSelectType select_type);
    ctrlPercent* AddPercent(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, unsigned text_color,
                            const glFont* font, const unsigned short* percentage);
    ctrlProgress* AddProgress(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                              unsigned short button_minus, unsigned short button_plus, unsigned short maximum,
                              const std::string& tooltip = "", const Extent& padding = Extent(0, 0),
                              unsigned force_color = 0, const std::string& button_minus_tooltip = "",
                              const std::string& button_plus_tooltip = "");
    ctrlScrollBar* AddScrollBar(unsigned id, const DrawPoint& pos, const Extent& size, unsigned short button_height,
                                TextureColor tc, unsigned short page_size);
    ctrlTab* AddTabCtrl(unsigned id, const DrawPoint& pos, unsigned short width);
    ctrlTable* AddTable(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
                        std::vector<TableColumn> columns);
    /// Add text
    /// @param color    Text color (ARGB)
    /// @param format   can be a combination of FontStyle::LEFT/CENTER/RIGHT and FontStyle::TOP/VCENTER/BOTTOM and
    /// FontStyle::OUTLINE/NO_OUTLINE
    ///                 Alignment specifies how the position is treated, i.e. where relative to the text it will be.
    ctrlText* AddText(unsigned id, const DrawPoint& pos, const std::string& text, unsigned color, FontStyle format,
                      const glFont* font);
    ctrlMapSelection* AddMapSelection(unsigned id, const DrawPoint& pos, const Extent& size,
                                      const SelectionMapInputData& inputData);
    TextFormatSetter AddFormattedText(unsigned id, const DrawPoint& pos, const std::string& text, unsigned color,
                                      FontStyle format, const glFont* font);
    ctrlTimer* AddTimer(unsigned id, std::chrono::milliseconds timeout);
    /// Add a 3D text control with a variable text. The text is formatted like printf but with pointers
    /// to int (%d), unsigned (%u) or const char (%s) which must be valid for the lifetime of the var text!
    ctrlVarDeepening* AddVarDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                      const std::string& formatstr, const glFont* font, unsigned color,
                                      unsigned parameters, ...);
    /// Add a text control with a variable text. The text is formatted like printf but with pointers
    /// to int (%d), unsigned (%u) or const char (%s) which must be valid for the lifetime of the var text!
    ctrlVarText* AddVarText(unsigned id, const DrawPoint& pos, const std::string& formatstr, unsigned color,
                            FontStyle format, const glFont* font, unsigned parameters, ...);
    ctrlPreviewMinimap* AddPreviewMinimap(unsigned id, const DrawPoint& pos, const Extent& size,
                                          libsiedler2::ArchivItem_Map* map);

    /// Draw a 3D rectangle (e.g. button)
    static void Draw3D(const Rect& rect, TextureColor tc, bool elevated, bool highlighted = false,
                       bool illuminated = false, unsigned contentColor = COLOR_WHITE);
    static void Draw3DBorder(const Rect& rect, TextureColor tc, bool elevated);
    static void Draw3DContent(const Rect& rect, TextureColor tc, bool elevated, bool highlighted = false,
                              bool illuminated = false, unsigned contentColor = COLOR_WHITE);
    static void DrawRectangle(const Rect& rect, unsigned color);
    static void DrawLine(DrawPoint pt1, DrawPoint pt2, unsigned short width, unsigned color);

    // GUI-Notify-Messages

    // These messages get passed downwards (WindowManager to controls)
    // Return true if the message was handled
    virtual void Msg_PaintBefore();
    virtual void Msg_PaintAfter();
    virtual bool Msg_LeftDown(const MouseCoords&) { return false; }
    virtual bool Msg_RightDown(const MouseCoords&) { return false; }
    virtual bool Msg_MiddleDown(const MouseCoords&) { return false; }
    virtual bool Msg_LeftUp(const MouseCoords&) { return false; }
    virtual bool Msg_RightUp(const MouseCoords&) { return false; }
    virtual bool Msg_MiddleUp(const MouseCoords&) { return false; }
    virtual bool Msg_WheelUp(const MouseCoords&) { return false; }
    virtual bool Msg_WheelDown(const MouseCoords&) { return false; }
    virtual bool Msg_MouseMove(const MouseCoords&) { return false; }
    virtual bool Msg_KeyDown(const KeyEvent&) { return false; }
    virtual void Msg_ScreenResize(const ScreenResizeEvent& sr);

    // Callback messages that are passed upwards (from controls to window)
    virtual void Msg_ButtonClick(unsigned /*ctrl_id*/) {}
    virtual void Msg_EditEnter(unsigned /*ctrl_id*/) {}
    virtual void Msg_EditChange(unsigned /*ctrl_id*/) {}
    virtual void Msg_TabChange(unsigned /*ctrl_id*/, unsigned short /*tab_id*/) {}
    virtual void Msg_ListSelectItem(unsigned /*ctrl_id*/, int /*selection*/) {}
    virtual void Msg_ListChooseItem(unsigned /*ctrl_id*/, unsigned /*selection*/) {}
    virtual void Msg_ComboSelectItem(unsigned /*ctrl_id*/, unsigned /*selection*/) {}
    virtual void Msg_CheckboxChange(unsigned /*ctrl_id*/, bool /*checked*/) {}
    virtual void Msg_ProgressChange(unsigned /*ctrl_id*/, unsigned short /*position*/) {}
    virtual void Msg_ScrollChange(unsigned /*ctrl_id*/, unsigned short /*position*/) {}
    virtual void Msg_ScrollShow(unsigned /*ctrl_id*/, bool /*visible*/) {}
    virtual void Msg_OptionGroupChange(unsigned /*ctrl_id*/, unsigned /*selection*/) {}
    virtual void Msg_Timer(unsigned /*ctrl_id*/) {}
    virtual void Msg_TableSelectItem(unsigned /*ctrl_id*/, const boost::optional<unsigned>& /*selection*/) {}
    virtual void Msg_TableChooseItem(unsigned /*ctrl_id*/, unsigned /*selection*/) {}
    virtual void Msg_TableRightButton(unsigned /*ctrl_id*/, const boost::optional<unsigned>& /*selection*/) {}
    virtual void Msg_TableLeftButton(unsigned /*ctrl_id*/, const boost::optional<unsigned>& /*selection*/) {}

    /// Callback of a message box when closed
    virtual void Msg_MsgBoxResult(unsigned /*msgbox_id*/, MsgboxResult /*mbr*/) {}

    // Callbacks triggered by controls of ctrlGroup
    virtual void Msg_Group_ButtonClick(unsigned /*group_id*/, unsigned /*ctrl_id*/) {}
    virtual void Msg_Group_EditEnter(unsigned /*group_id*/, unsigned /*ctrl_id*/) {}
    virtual void Msg_Group_EditChange(unsigned /*group_id*/, unsigned /*ctrl_id*/) {}
    virtual void Msg_Group_TabChange(unsigned /*group_id*/, unsigned /*ctrl_id*/, unsigned short /*tab_id*/) {}
    virtual void Msg_Group_ListSelectItem(unsigned /*group_id*/, unsigned /*ctrl_id*/, int /*selection*/) {}
    virtual void Msg_Group_ComboSelectItem(unsigned /*group_id*/, unsigned /*ctrl_id*/, unsigned /*selection*/) {}
    virtual void Msg_Group_CheckboxChange(unsigned /*group_id*/, unsigned /*ctrl_id*/, bool /*checked*/) {}
    virtual void Msg_Group_ProgressChange(unsigned /*group_id*/, unsigned /*ctrl_id*/, unsigned short /*position*/) {}
    virtual void Msg_Group_ScrollShow(unsigned /*group_id*/, unsigned /*ctrl_id*/, bool /*visible*/) {}
    virtual void Msg_Group_OptionGroupChange(unsigned /*group_id*/, unsigned /*ctrl_id*/, unsigned /*selection*/) {}
    virtual void Msg_Group_Timer(unsigned /*group_id*/, unsigned /*ctrl_id*/) {}
    virtual void Msg_Group_TableSelectItem(unsigned /*group_id*/, unsigned /*ctrl_id*/,
                                           const boost::optional<unsigned>& /*selection*/)
    {}
    virtual void Msg_Group_TableRightButton(unsigned /*group_id*/, unsigned /*ctrl_id*/,
                                            const boost::optional<unsigned>& /*selection*/)
    {}
    virtual void Msg_Group_TableLeftButton(unsigned /*group_id*/, unsigned /*ctrl_id*/,
                                           const boost::optional<unsigned>& /*selection*/)
    {}

protected:
    enum class ButtonState
    {
        Up,
        Hover,
        Pressed
    };
    friend constexpr auto maxEnumValue(ButtonState) { return ButtonState::Pressed; }
    using ControlMap = std::map<unsigned, std::unique_ptr<Window>>;

    /// Scale the value from the reference coordinates to current render size
    template<class T_Pt>
    static T_Pt Scale(const T_Pt& pt);
    /// Scales the value when scale_ is true, else returns the value unchanged
    template<class T_Pt>
    T_Pt ScaleIf(const T_Pt& pt) const;
    /// Set whether controls of this window shall be scaled
    void SetScale(bool scale = true) { scale_ = scale; }
    /// Implementation of drawing the window, derived classes can override this to draw custom backgrounds or similar
    virtual void Draw_();
    /// Shall messages be relayed to the controls of this window?
    virtual bool IsMessageRelayAllowed() const;

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
T* Window::AddCtrl(std::unique_ptr<T> ctrl)
{
    RTTR_Assert(ctrl);
    RTTR_Assert(childIdToWnd_.find(ctrl->GetID()) == childIdToWnd_.end());

    T* ctrlPtr = ctrl.get();
    ctrl->scale_ = scale_;
    childIdToWnd_.emplace(ctrl->GetID(), std::move(ctrl));

    ctrlPtr->SetActive(active_);
    return ctrlPtr;
}

template<typename T>
T* Window::GetCtrl(unsigned id)
{
    return const_cast<T*>(static_cast<const Window&>(*this).GetCtrl<T>(id));
}

template<typename T>
const T* Window::GetCtrl(unsigned id) const
{
    auto it = childIdToWnd_.find(id);
    if(it == childIdToWnd_.end())
        return nullptr;

    return dynamic_cast<T*>(it->second.get());
}

template<typename T>
std::vector<T*> Window::GetCtrls()
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
std::vector<const T*> Window::GetCtrls() const
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
