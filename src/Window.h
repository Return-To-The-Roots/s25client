// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#pragma once

#include "Msgbox.h"
#include "animation/AnimationManager.h"
#include "gameData/NationConsts.h"
#include "gameTypes/BuildingTypes.h"
#include "gameTypes/TextureColor.h"
#include "Rect.h"
#include "DrawPoint.h"
#include "libutil/src/colors.h"
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

class glArchivItem_Map;
class glArchivItem_Font;
class glArchivItem_Bitmap;

struct KeyEvent;
class MouseCoords;
struct ScreenResizeEvent;

/// Die Basisklasse der Fenster.
class Window
{
    public:
        typedef bool (Window::*KeyboardMsgHandler)(const KeyEvent&);
        typedef bool (Window::*MouseMsgHandler)(const MouseCoords&);

        Window(Window* parent, unsigned id, const DrawPoint& position, const Extent& size = Extent(0, 0));
        virtual ~Window();
        /// zeichnet das Fenster.
        void Draw();
        /// Get the current position
        DrawPoint GetPos() const;
        /// Get the absolute (X,Y) position as when calling GetX/GetY for drawing
        DrawPoint GetDrawPos() const;
        /// Get the size of the window
        Extent GetSize(bool scale = false) const;
        /// gets the extent of the window in absolute coordinates
        Rect GetDrawRect() const;
        /// Get the actual extents of the rect (might be different to the draw rect if the window resizes according to content)
        virtual Rect GetBoundaryRect() const;
        /// setzt die Größe des Fensters
        virtual void Resize(const Extent& newSize) { size_ = newSize; }
        /// setzt die Breite des Fensters
        void SetWidth(unsigned width)   { Resize(Extent(width, size_.y)); }
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
        /// Größe verändern oder überhaupt setzen

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
        const unsigned GetID() const { return id_; }
        /// Get control with given ID of given type or NULL if not found or other type
        template<typename T>
        T* GetCtrl(unsigned id);
        /// Get control with given ID of given type or NULL if not found or other type
        template<typename T>
        const T* GetCtrl(unsigned id) const;

        /// Get all controls of given type
        template<typename T>
        std::vector<T*> GetCtrls();
        /// Get all controls of given type
        template<typename T>
        std::vector<const T*> GetCtrls() const;

        void DeleteCtrl(unsigned id);

        AnimationManager& GetAnimationManager(){ return animations_; }

        ctrlBuildingIcon* AddBuildingIcon(unsigned id, const DrawPoint& pos, BuildingType type, const Nation nation, unsigned short size = 36, const std::string& tooltip_ = "");
        ctrlButton* AddTextButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc, const std::string& text,  glArchivItem_Font* font, const std::string& tooltip = "");
        ctrlButton* AddColorButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc, const unsigned fillColor, const std::string& tooltip = "");
        ctrlButton* AddImageButton(unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc, glArchivItem_Bitmap* const image, const std::string& tooltip = "");
        ctrlChat* AddChatCtrl(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font);
        ctrlCheck* AddCheckBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const std::string& text, glArchivItem_Font* font, bool readonly = false);
        ctrlComboBox* AddComboBox(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font, unsigned short max_list_height, bool readonly = false);
        ctrlDeepening* AddTextDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const std::string& text, glArchivItem_Font* font, unsigned color);
        ctrlDeepening* AddColorDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, unsigned fillColor);
        ctrlEdit* AddEdit(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font, unsigned short maxlength = 0, bool password = false, bool disabled = false, bool notify = false);
        ctrlGroup* AddGroup(unsigned id);
        ctrlImage* AddImage(unsigned id, const DrawPoint& pos, glArchivItem_Bitmap* image, const std::string& tooltip = "");
        ctrlList* AddList(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font);
        ctrlMultiline* AddMultiline(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font, unsigned format = 0);
        ctrlOptionGroup* AddOptionGroup(unsigned id, int select_type);
        ctrlMultiSelectGroup* AddMultiSelectGroup(unsigned id, int select_type);
        ctrlPercent* AddPercent(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, unsigned text_color, glArchivItem_Font* font, const unsigned short* percentage);
        ctrlProgress* AddProgress(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, unsigned short button_minus, unsigned short button_plus, unsigned short maximum,
                                  const std::string& tooltip  = "", const Extent& padding = Extent(0, 0), unsigned force_color = 0, const std::string& button_minus_tooltip = "", const std::string& button_plus_tooltip = "");
        ctrlScrollBar* AddScrollBar(unsigned id, const DrawPoint& pos, const Extent& size, unsigned short button_height, TextureColor tc, unsigned short page_size);
        ctrlTab* AddTabCtrl(unsigned id, const DrawPoint& pos, unsigned short width);
        ctrlTable* AddTable(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font, unsigned columns, ...);
        ctrlText* AddText(unsigned id, const DrawPoint& pos, const std::string& text, unsigned color, unsigned format, glArchivItem_Font* font);
        ctrlTimer* AddTimer(unsigned id, unsigned timeout);
        /// fügt ein vertieftes variables TextCtrl hinzu.
        /// var parameters are pointers to int, unsigned or const char and must be valid for the lifetime of the var text!
        ctrlVarDeepening* AddVarDeepening(unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const std::string& formatstr, glArchivItem_Font* font, unsigned color, unsigned parameters, ...);
        /// fügt ein variables TextCtrl hinzu.
        /// var parameters are pointers to int, unsigned or const char and must be valid for the lifetime of the var text!
        ctrlVarText* AddVarText(unsigned id, const DrawPoint& pos,  const std::string& formatstr, unsigned color, unsigned format, glArchivItem_Font* font, unsigned parameters, ...);
        ctrlPreviewMinimap* AddPreviewMinimap(const unsigned id, const DrawPoint& pos, const Extent& size, glArchivItem_Map* const map);

        /// Zeichnet einen 3D-Rahmen.
        /// type 0 / 1 elevated border
        /// type 2: deepened border
        /// type 1 content texture is lighter than the other 2
        static void Draw3D(const Rect& rect, TextureColor tc, unsigned short type, bool illuminated = false, bool drawContent = true, unsigned color = COLOR_WHITE);
        /// Zeichnet ein Rechteck
        static void DrawRectangle(const Rect& rect, unsigned color);
        /// Zeichnet eine Linie
        static void DrawLine(DrawPoint pt1, DrawPoint pt2, unsigned short width, unsigned color);

        // GUI-Notify-Messages

        // Nachrichten, die von oben (WindowManager) nach unten (zu Controls) gereicht werden
        virtual void Msg_PaintBefore();
        virtual void Msg_PaintAfter(){}
        virtual bool Msg_LeftDown(const MouseCoords& mc){ return false; }
        virtual bool Msg_RightDown(const MouseCoords& mc){ return false; }
        virtual bool Msg_LeftUp(const MouseCoords& mc){ return false; }
        virtual bool Msg_RightUp(const MouseCoords& mc){ return false; }
        virtual bool Msg_WheelUp(const MouseCoords& mc){ return false; }
        virtual bool Msg_WheelDown(const MouseCoords& mc){ return false; }
        virtual bool Msg_MouseMove(const MouseCoords& mc){ return false; }
        virtual bool Msg_KeyDown(const KeyEvent& ke){ return false; }
        // Wird aufgerufen, nachdem schon ein Mausklick behandelt wurde
        // NUR VORÜBERGEHEND für Edit-Controls, bis richtiger Steuerelement-Fokus
        // eingebaut wurde!
        virtual bool Msg_LeftDown_After(const MouseCoords& mc){ return false; }
        virtual void Msg_ScreenResize(const ScreenResizeEvent& sr);

        // Nachrichten, die von unten (Controls) nach oben (Fenster) gereicht werden
        virtual void Msg_ButtonClick(const unsigned ctrl_id){}
        virtual void Msg_EditEnter(const unsigned ctrl_id){}
        virtual void Msg_EditChange(const unsigned ctrl_id){}
        virtual void Msg_TabChange(const unsigned ctrl_id, const unsigned short tab_id){}
        virtual void Msg_ListSelectItem(const unsigned ctrl_id, const int selection){}
        virtual void Msg_ListChooseItem(const unsigned ctrl_id, const unsigned selection){}
        virtual void Msg_ComboSelectItem(const unsigned ctrl_id, const int selection){}
        virtual void Msg_CheckboxChange(const unsigned ctrl_id, const bool checked){}
        virtual void Msg_ProgressChange(const unsigned ctrl_id, const unsigned short position){}
        virtual void Msg_ScrollChange(const unsigned ctrl_id, const unsigned short position){}
        virtual void Msg_ScrollShow(const unsigned ctrl_id, const bool visible){}
        virtual void Msg_OptionGroupChange(const unsigned ctrl_id, const int selection){}
        virtual void Msg_Timer(const unsigned ctrl_id){}
        virtual void Msg_TableSelectItem(const unsigned ctrl_id, const int selection){}
        virtual void Msg_TableChooseItem(const unsigned ctrl_id, const unsigned selection){}
        virtual void Msg_TableRightButton(const unsigned ctrl_id, const int selection){}
        virtual void Msg_TableLeftButton(const unsigned ctrl_id, const int selection){}

        // Sonstiges
        virtual void Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr){}

        // Nachrichten, die von Controls von ctrlGroup weitergeleitet werden
        virtual void Msg_Group_ButtonClick(const unsigned group_id, const unsigned ctrl_id){}
        virtual void Msg_Group_EditEnter(const unsigned group_id, const unsigned ctrl_id){}
        virtual void Msg_Group_EditChange(const unsigned group_id, const unsigned ctrl_id){}
        virtual void Msg_Group_TabChange(const unsigned group_id, const unsigned ctrl_id, const unsigned short tab_id){}
        virtual void Msg_Group_ListSelectItem(const unsigned group_id, const unsigned ctrl_id, const int selection){}
        virtual void Msg_Group_ComboSelectItem(const unsigned group_id, const unsigned ctrl_id, const int selection){}
        virtual void Msg_Group_CheckboxChange(const unsigned group_id, const unsigned ctrl_id, const bool checked){}
        virtual void Msg_Group_ProgressChange(const unsigned group_id, const unsigned ctrl_id, const unsigned short position){}
        virtual void Msg_Group_ScrollShow(const unsigned group_id, const unsigned ctrl_id, const bool visible){}
        virtual void Msg_Group_OptionGroupChange(const unsigned group_id, const unsigned ctrl_id, const int selection){}
        virtual void Msg_Group_Timer(const unsigned group_id, const unsigned ctrl_id){}
        virtual void Msg_Group_TableSelectItem(const unsigned group_id, const unsigned ctrl_id, const int selection){}
        virtual void Msg_Group_TableRightButton(const unsigned group_id, const unsigned ctrl_id, const int selection){}
        virtual void Msg_Group_TableLeftButton(const unsigned group_id, const unsigned ctrl_id, const int selection){}

    protected:
        enum ButtonState
        {
            BUTTON_UP = 0,
            BUTTON_HOVER,
            BUTTON_PRESSED,
            BUTTON_UNKNOWN = 0xFF
        };
        typedef std::map<unsigned, Window*> ControlMap;

        /// scales X- und Y values to fit the screen
        template<class T_Pt>
        T_Pt Scale(const T_Pt& pt) const;
        /// Scales the value when scale_ is true, else returns the value
        template<class T_Pt>
        T_Pt ScaleIf(const T_Pt& pt) const;
        /// setzt Scale-Wert, ob neue Controls skaliert werden sollen oder nicht.
        void SetScale(bool scale = true) { this->scale_ = scale; }
        /// zeichnet die Steuerelemente.
        void DrawControls();
        /// prüft ob Mauskoordinaten in einer gesperrten Region liegt.
        bool TestWindowInRegion(Window* window, const MouseCoords& mc) const;
        /// zeichnet das Fenster. (virtuelle Memberfunktion)
        virtual void Draw_() = 0;
        /// Weiterleitung von Nachrichten von abgeleiteten Klassen erlaubt oder nicht?
        virtual bool IsMessageRelayAllowed() const;

        template <typename T>
        T* AddCtrl(T* ctrl);

    private:
        Window* const parent_;/// Handle auf das Parentfenster.
        const unsigned id_;   /// ID des Fensters.
        DrawPoint pos_;       /// Position des Fensters.
        Extent size_;         /// Höhe des Fensters.
        bool active_;         /// Fenster aktiv?
        bool visible_;        /// Fenster sichtbar?
        bool scale_;          /// Sollen Controls an Fenstergröße angepasst werden?

        std::map<Window*, Rect> lockedAreas_;       /// gesperrte Regionen des Fensters.
        std::vector<Window*> tofreeAreas_;
        bool isInMouseRelay;
        ControlMap childIdToWnd_; /// Die Steuerelemente des Fensters.
        AnimationManager animations_;

        friend class WindowManager;
};

template <typename T>
inline T* Window::AddCtrl(T* ctrl)
{
    RTTR_Assert(childIdToWnd_.find(ctrl->GetID()) == childIdToWnd_.end());
    // ID auf control mappen
    childIdToWnd_.insert(std::make_pair(ctrl->GetID(), ctrl));

    // scale-Eigenschaft weitervererben
    ctrl->scale_ = scale_;

    return ctrl;
}

template<typename T>
inline T* Window::GetCtrl(unsigned id)
{
    ControlMap::iterator it = childIdToWnd_.find(id);
    if(it == childIdToWnd_.end())
        return NULL;

    return dynamic_cast<T*>(it->second);
}

template<typename T>
inline const T* Window::GetCtrl(unsigned id) const
{
    ControlMap::const_iterator it = childIdToWnd_.find(id);
    if(it == childIdToWnd_.end())
        return NULL;

    return dynamic_cast<T*>(it->second);
}

template<typename T>
inline std::vector<T*> Window::GetCtrls()
{
    std::vector<T*> result;
    for(ControlMap::iterator it = childIdToWnd_.begin(); it != childIdToWnd_.end(); ++it)
    {
        T* ctrl = dynamic_cast<T*>(it->second);
        if(ctrl)
            result.push_back(ctrl);
    }
    return result;
}

template<typename T>
inline std::vector<const T*> Window::GetCtrls() const
{
    std::vector<const T*> result;
    for(ControlMap::const_iterator it = childIdToWnd_.begin(); it != childIdToWnd_.end(); ++it)
    {
        const T* ctrl = dynamic_cast<const T*>(it->second);
        if(ctrl)
            result.push_back(ctrl);
    }
    return result;
}

#endif // !WINDOW_H_INCLUDED
