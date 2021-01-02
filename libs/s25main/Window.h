// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
class ctrlPreviewMinimap;
class ctrlMultiline;
class ctrlOptionGroup;
class ctrlPercent;
class ctrlProgress;
class ctrlScrollBar;
class ctrlTab;
class ctrlTable;
class ctrlText;
class ctrlTimer;
class ctrlVarDeepening;
class ctrlVarText;
class ctrlMultiSelectGroup;
struct TableColumn;

class glArchivItem_Bitmap;
class glArchivItem_Map;
class glFont;
class ITexture;

struct KeyEvent;
class MouseCoords;
struct ScreenResizeEvent;
enum class GroupSelectType : unsigned;

/// Die Basisklasse der Fenster.
class Window
{
public:
    using KeyboardMsgHandler = bool (Window::*)(const KeyEvent&);
    using MouseMsgHandler = bool (Window::*)(const MouseCoords&);

    Window(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size = Extent(0, 0));
    virtual ~Window();
    /// zeichnet das Fenster.
    void Draw();
    /// Get the current position
    DrawPoint GetPos() const;
    /// Get the absolute (X,Y) position as when calling GetX/GetY for drawing
    DrawPoint GetDrawPos() const;
    /// Get the size of the window
    Extent GetSize() const;
    /// gets the extent of the window in absolute coordinates
    Rect GetDrawRect() const;
    /// Get the actual extents of the rect (might be different to the draw rect if the window resizes according to
    /// content)
    virtual Rect GetBoundaryRect() const;
    /// setzt die Größe des Fensters
    virtual void Resize(const Extent& newSize) { size_ = newSize; }
    /// setzt die Breite des Fensters
    void SetWidth(unsigned width) { Resize(Extent(width, size_.y)); }
    /// setzt die Höhe des Fensters
    void SetHeight(unsigned height) { Resize(Extent(size_.x, height)); }
    /// Sendet eine Tastaturnachricht an die Steuerelemente.
    bool RelayKeyboardMessage(KeyboardMsgHandler msg, const KeyEvent& ke);
    /// Sendet eine Mausnachricht weiter an alle Steuerelemente
    bool RelayMouseMessage(MouseMsgHandler msg, const MouseCoords& mc);
    /// aktiviert das Fenster.
    virtual void SetActive(bool activate = true);
    /// aktiviert die Steuerelemente des Fensters.
    void ActivateControls(bool activate = true);
    /// Sperrt eine bestimmte Region für Mausereignisse.
    void LockRegion(Window* window, const Rect& rect);
    /// Gibt eine gesperrte Region wieder frei.
    void FreeRegion(Window* window);
    /// Check if the gicen point is in a region locked by any window other than exception
    bool IsInLockedRegion(const Position& pos, const Window* exception = nullptr) const;

    /// Set the position for the window
    void SetPos(const DrawPoint& newPos);

    // macht das Fenster sichtbar oder blendet es aus
    virtual void SetVisible(bool visible) { this->visible_ = visible; }
    /// Ist das Fenster sichtbar?
    bool IsVisible() const { return visible_; }
    /// Ist das Fenster aktiv?
    bool IsActive() const { return active_; }
    /// liefert das übergeordnete Fenster
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
    T* AddCtrl(T* ctrl);

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
    ctrlText* AddText(unsigned id, const DrawPoint& pos, const std::string& text, unsigned color, FontStyle format,
                      const glFont* font);
    TextFormatSetter AddFormattedText(unsigned id, const DrawPoint& pos, const std::string& text, unsigned color,
                                      FontStyle format, const glFont* font);
    ctrlTimer* AddTimer(unsigned id, std::chrono::milliseconds timeout);
    /// fügt ein vertieftes variables TextCtrl hinzu.
    /// var parameters are pointers to int, unsigned or const char and must be valid for the lifetime of the var text!
    ctrlVarDeepening* AddVarDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                                      const std::string& formatstr, const glFont* font, unsigned color,
                                      unsigned parameters, ...);
    /// fügt ein variables TextCtrl hinzu.
    /// var parameters are pointers to int, unsigned or const char and must be valid for the lifetime of the var text!
    ctrlVarText* AddVarText(unsigned id, const DrawPoint& pos, const std::string& formatstr, unsigned color,
                            FontStyle format, const glFont* font, unsigned parameters, ...);
    ctrlPreviewMinimap* AddPreviewMinimap(unsigned id, const DrawPoint& pos, const Extent& size, glArchivItem_Map* map);

    /// Draw a 3D rectangle (e.g. button)
    static void Draw3D(const Rect& rect, TextureColor tc, bool elevated, bool highlighted = false,
                       bool illuminated = false, unsigned contentColor = COLOR_WHITE);
    static void Draw3DBorder(const Rect& rect, TextureColor tc, bool elevated);
    static void Draw3DContent(const Rect& rect, TextureColor tc, bool elevated, bool highlighted = false,
                              bool illuminated = false, unsigned contentColor = COLOR_WHITE);
    /// Zeichnet ein Rechteck
    static void DrawRectangle(const Rect& rect, unsigned color);
    /// Zeichnet eine Linie
    static void DrawLine(DrawPoint pt1, DrawPoint pt2, unsigned short width, unsigned color);

    // GUI-Notify-Messages

    // Nachrichten, die von oben (WindowManager) nach unten (zu Controls) gereicht werden
    virtual void Msg_PaintBefore();
    virtual void Msg_PaintAfter();
    virtual bool Msg_LeftDown(const MouseCoords&) { return false; }
    virtual bool Msg_RightDown(const MouseCoords&) { return false; }
    virtual bool Msg_LeftUp(const MouseCoords&) { return false; }
    virtual bool Msg_RightUp(const MouseCoords&) { return false; }
    virtual bool Msg_WheelUp(const MouseCoords&) { return false; }
    virtual bool Msg_WheelDown(const MouseCoords&) { return false; }
    virtual bool Msg_MouseMove(const MouseCoords&) { return false; }
    virtual bool Msg_KeyDown(const KeyEvent&) { return false; }
    virtual void Msg_ScreenResize(const ScreenResizeEvent& sr);

    // Nachrichten, die von unten (Controls) nach oben (Fenster) gereicht werden
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

    // Sonstiges
    virtual void Msg_MsgBoxResult(unsigned /*msgbox_id*/, MsgboxResult /*mbr*/) {}

    // Nachrichten, die von Controls von ctrlGroup weitergeleitet werden
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
    using ControlMap = std::map<unsigned, Window*>;

    /// scales X- und Y values to fit the screen
    template<class T_Pt>
    static T_Pt Scale(const T_Pt& pt);
    /// Scales the value when scale_ is true, else returns the value
    template<class T_Pt>
    T_Pt ScaleIf(const T_Pt& pt) const;
    /// setzt Scale-Wert, ob neue Controls skaliert werden sollen oder nicht.
    void SetScale(bool scale = true) { this->scale_ = scale; }
    /// zeichnet das Fenster.
    virtual void Draw_();
    /// Weiterleitung von Nachrichten von abgeleiteten Klassen erlaubt oder nicht?
    virtual bool IsMessageRelayAllowed() const;

private:
    Window* const parent_; /// Handle auf das Parentfenster.
    unsigned id_;          /// ID des Fensters.
    DrawPoint pos_;        /// Position des Fensters.
    Extent size_;          /// Höhe des Fensters.
    bool active_;          /// Fenster aktiv?
    bool visible_;         /// Fenster sichtbar?
    bool scale_;           /// Sollen Controls an Fenstergröße angepasst werden?

    std::map<Window*, Rect> lockedAreas_; /// gesperrte Regionen des Fensters.
    std::vector<Window*> tofreeAreas_;
    bool isInMouseRelay;
    ControlMap childIdToWnd_; /// Die Steuerelemente des Fensters.
    AnimationManager animations_;
};

template<typename T>
inline T* Window::AddCtrl(T* ctrl)
{
    RTTR_Assert(childIdToWnd_.find(ctrl->GetID()) == childIdToWnd_.end());
    childIdToWnd_.insert(std::make_pair(ctrl->GetID(), ctrl));

    ctrl->scale_ = scale_;
    ctrl->SetActive(active_);

    return ctrl;
}

template<typename T>
inline T* Window::GetCtrl(unsigned id)
{
    auto it = childIdToWnd_.find(id);
    if(it == childIdToWnd_.end())
        return nullptr;

    return dynamic_cast<T*>(it->second);
}

template<typename T>
inline const T* Window::GetCtrl(unsigned id) const
{
    auto it = childIdToWnd_.find(id);
    if(it == childIdToWnd_.end())
        return nullptr;

    return dynamic_cast<T*>(it->second);
}

template<typename T>
inline std::vector<T*> Window::GetCtrls()
{
    std::vector<T*> result;
    for(auto* wnd : childIdToWnd_ | boost::adaptors::map_values)
    {
        T* ctrl = dynamic_cast<T*>(wnd);
        if(ctrl)
            result.push_back(ctrl);
    }
    return result;
}

template<typename T>
inline std::vector<const T*> Window::GetCtrls() const
{
    std::vector<const T*> result;
    for(auto* const wnd : childIdToWnd_ | boost::adaptors::map_values)
    {
        const T* ctrl = dynamic_cast<const T*>(wnd);
        if(ctrl)
            result.push_back(ctrl);
    }
    return result;
}
