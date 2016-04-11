// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "gameData/NationConsts.h"
#include "gameTypes/BuildingTypes.h"
#include "gameTypes/TextureColor.h"
#include "Rect.h"
#include <map>
#include <vector>

class ctrlBuildingIcon;
class ctrlTextButton;
class ctrlColorButton;
class ctrlImageButton;
class ctrlChat;
class ctrlCheck;
class ctrlComboBox;
class ctrlDeepening;
class ctrlColorDeepening;
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
        friend class WindowManager;

    public:
        Window();
        Window(unsigned short x, unsigned short y, unsigned int id, Window* parent, unsigned short width = 0, unsigned short height = 0, const std::string& tooltip = "");
        virtual ~Window();
        /// zeichnet das Fenster.
        bool Draw();
        /// liefert die X-Koordinate.
        unsigned short GetX(bool absolute = true) const;
        /// liefert die Y-Koordinate.
        unsigned short GetY(bool absolute = true) const;
        /// liefert die Breite des Fensters.
        unsigned short GetWidth(const bool scale = false) const { return (scale) ? ScaleX(width_) : width_; }
        /// liefert die Höhe des Fensters.
        unsigned short GetHeight(const bool scale = false) const { return (scale) ? ScaleY(height_) : height_; }
        /// setzt die Größe des Fensters
        void Resize(unsigned short width, unsigned short height) { Resize_(width, height); this->width_ = width; this->height_ = height; }
        /// setzt die Breite des Fensters
        void SetWidth(unsigned short width)   { Resize(width, this->height_); }
        /// setzt die Höhe des Fensters
        void SetHeight(unsigned short height) { Resize(this->width_, height); }
        /// Sendet eine Tastaturnachricht an die Steuerelemente.
        bool RelayKeyboardMessage(bool (Window::*msg)(const KeyEvent&), const KeyEvent& ke);
        /// Sendet eine Mausnachricht weiter an alle Steuerelemente
        bool RelayMouseMessage(bool (Window::*msg)(const MouseCoords&), const MouseCoords& mc);
        /// aktiviert das Fenster.
        virtual void SetActive(bool activate = true);
        /// aktiviert die Steuerelemente des Fensters.
        void ActivateControls(bool activate = true);
        /// Sperrt eine bestimmte Region für Mausereignisse.
        void LockRegion(Window* window, const Rect& rect);
        /// Gibt eine gesperrte Region wieder frei.
        void FreeRegion(Window* window);
        /// Größe verändern oder überhaupt setzen

        /// setzt das Parentfenster.
        void SetParent(Window* parent) { this->parent_ = parent; }
        /// verschiebt das Fenster.
        void Move(short x, short y, bool absolute = true) { this->x_ = (absolute ? x : this->x_ + x); this->y_ = (absolute ? y : this->y_ + y); }

        // macht das Fenster sichtbar oder blendet es aus
        virtual void SetVisible(bool visible) { this->visible_ = visible; }
        /// Ist das Fenster sichtbar?
        bool GetVisible() const { return visible_; }
        /// Ist das Fenster aktiv?
        bool GetActive() const { return active_; }
        /// liefert das übergeordnete Fenster
        Window* GetParent() const { return parent_; }
        const unsigned int GetID() const { return id_; }

        template<typename T>
        T* GetCtrl(unsigned int id)
        {
            std::map<unsigned int, Window*>::iterator it = childIdToWnd_.find(id);
            if(it == childIdToWnd_.end())
                return NULL;

            return dynamic_cast<T*>( it->second );
        }

        template<typename T>
        const T* GetCtrl(unsigned int id) const
        {
            std::map<unsigned int, Window*>::const_iterator it = childIdToWnd_.find(id);
            if(it == childIdToWnd_.end())
                return NULL;

            return dynamic_cast<T*>( it->second );
        }

        void DeleteCtrl(unsigned int id)
        {
            std::map<unsigned int, Window*>::iterator it = childIdToWnd_.find(id);

            if(it == childIdToWnd_.end())
                return;

            delete it->second;

            childIdToWnd_.erase(it);
        }

        /// fügt ein BuildingIcon hinzu.
        ctrlBuildingIcon* AddBuildingIcon(unsigned int id_, unsigned short x_, unsigned short y_, BuildingType type, const Nation nation, unsigned short size = 36, const std::string& tooltip_ = "");
        /// fügt einen Text-ButtonCtrl hinzu.
        ctrlTextButton* AddTextButton(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, const TextureColor tc, const std::string& text,  glArchivItem_Font* font, const std::string& tooltip = "");
        /// fügt einen Color-ButtonCtrl hinzu.
        ctrlColorButton* AddColorButton(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, const TextureColor tc, const unsigned int fillColor, const std::string& tooltip = "");
        /// fügt einen Image-ButtonCtrl hinzu.
        ctrlImageButton* AddImageButton(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, const TextureColor tc, glArchivItem_Bitmap* const image, const std::string& tooltip = "");

        //ctrlButton *AddButton(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, bool type, const char *text, const char *tooltip, glArchivItem_Font *font, glArchivItem_Bitmap *image = NULL, bool border = true);
        /// fügt ein ChatCtrl hinzu.
        ctrlChat* AddChatCtrl(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, glArchivItem_Font* font);
        /// fügt ein CheckBoxCtrl hinzu.
        ctrlCheck* AddCheckBox(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, const std::string& text, glArchivItem_Font* font, bool readonly = false);
        /// fügt eine ComboBox hinzu.
        ctrlComboBox* AddComboBox(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, glArchivItem_Font* font, unsigned short max_list_height, bool readonly = false);
        /// fügt ein vertieftes TextCtrl hinzu.
        ctrlDeepening* AddDeepening(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, const std::string& text, glArchivItem_Font* font, unsigned int color);
        /// Deepening fille with a color
        ctrlColorDeepening* AddColorDeepening(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, unsigned int fillColor);
        /// fügt ein EditCtrl hinzu.
        ctrlEdit* AddEdit(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, glArchivItem_Font* font, unsigned short maxlength = 0, bool password = false, bool disabled = false, bool notify = false);
        /// fügt eine Gruppe hinzu.
        ctrlGroup* AddGroup(unsigned int id, bool scale = false);
        /// fügt ein ImageCtrl hinzu.
        ctrlImage* AddImage(unsigned int id, unsigned short x, unsigned short y, glArchivItem_Bitmap* image, const std::string& tooltip = "");
        /// fügt eine ListenCtrl hinzu.
        ctrlList* AddList(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, glArchivItem_Font* font);
        /// fügt ein mehrzeilen EditCtrl hinzu.
        ctrlMultiline* AddMultiline(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, glArchivItem_Font* font, unsigned int format = 0);
        /// fügt eine OptionenGruppe hinzu.
        ctrlOptionGroup* AddOptionGroup(unsigned int id, int select_type, bool scale = false);
        /// fügt eine MultiSelectGruppe hinzu.
        ctrlMultiSelectGroup* AddMultiSelectGroup(unsigned int id, int select_type, bool scale = false);
        /// fügt eine prozentuale ProgressBar hinzu.
        ctrlPercent* AddPercent(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, unsigned int text_color, glArchivItem_Font* font, const unsigned short* percentage);
        /// fügt eine ProgressBar hinzu.
        ctrlProgress* AddProgress(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, unsigned short button_minus, unsigned short button_plus, unsigned short maximum,
                                  const std::string& tooltip  = "", unsigned short x_padding = 0, unsigned short y_padding = 0, unsigned int force_color = 0, const std::string& button_minus_tooltip = "", const std::string& button_plus_tooltip = "");
        /// fügt eine Scrollbar hinzu.
        ctrlScrollBar* AddScrollBar(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, unsigned short button_height, TextureColor tc, unsigned short page_size);
        /// fügt ein TabCtrl hinzu.
        ctrlTab* AddTabCtrl(unsigned int id, unsigned short x, unsigned short y, unsigned short width);
        /// fügt eine Tabelle hinzu.
        ctrlTable* AddTable(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, glArchivItem_Font* font, unsigned int columns, ...);
        /// fügt ein TextCtrl hinzu.
        ctrlText* AddText(unsigned int id, unsigned short x, unsigned short y, const std::string& text, unsigned int color, unsigned int format, glArchivItem_Font* font);
        /// fügt einen Timer hinzu.
        ctrlTimer* AddTimer(unsigned int id, unsigned int timeout);
        /// fügt ein vertieftes variables TextCtrl hinzu.
        /// var parameters are pointers to int, unsigned or const char and must be valid for the lifetime of the var text!
        ctrlVarDeepening* AddVarDeepening(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, const std::string& formatstr, glArchivItem_Font* font, unsigned int color, unsigned int parameters, ...);
        /// fügt ein variables TextCtrl hinzu.
        /// var parameters are pointers to int, unsigned or const char and must be valid for the lifetime of the var text!
        ctrlVarText* AddVarText(unsigned int id, unsigned short x, unsigned short y,  const std::string& formatstr, unsigned int color, unsigned int format, glArchivItem_Font* font, unsigned int parameters, ...);
        /// Fügt eine MapPreview hinzu
        ctrlPreviewMinimap* AddPreviewMinimap(const unsigned id,
                                              unsigned short x,
                                              unsigned short y,
                                              unsigned short width,
                                              unsigned short height,
                                              glArchivItem_Map* const map);

        /// Zeichnet einen 3D-Rahmen.
        static void Draw3D(const unsigned short x, const unsigned short y, const unsigned short width, unsigned short height, const TextureColor tc, const unsigned short type, const bool illuminated = false, const bool draw_content = true);
        /// Zeichnet ein Rechteck
        static void DrawRectangle(unsigned short x, unsigned short y, unsigned short width, unsigned short height, unsigned int color);
        /// Zeichnet eine Linie
        static void DrawLine(unsigned short ax, unsigned short ay, unsigned short bx, unsigned short by, unsigned short width, unsigned int color);

        // GUI-Notify-Messages

        // Nachrichten, die von oben (WindowManager) nach unten (zu Controls) gereicht werden
        virtual void Msg_PaintBefore(){}
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
        virtual void Msg_ScreenResize(const ScreenResizeEvent& sr){}

        // Nachrichten, die von unten (Controls) nach oben (Fenster) gereicht werden
        virtual void Msg_ButtonClick(const unsigned int ctrl_id){}
        virtual void Msg_EditEnter(const unsigned int ctrl_id){}
        virtual void Msg_EditChange(const unsigned int ctrl_id){}
        virtual void Msg_TabChange(const unsigned int ctrl_id, const unsigned short tab_id){}
        virtual void Msg_ListSelectItem(const unsigned int ctrl_id, const int selection){}
        virtual void Msg_ListChooseItem(const unsigned int ctrl_id, const unsigned selection){}
        virtual void Msg_ComboSelectItem(const unsigned int ctrl_id, const int selection){}
        virtual void Msg_CheckboxChange(const unsigned int ctrl_id, const bool checked){}
        virtual void Msg_ProgressChange(const unsigned int ctrl_id, const unsigned short position){}
        virtual void Msg_ScrollChange(const unsigned int ctrl_id, const unsigned short position){}
        virtual void Msg_ScrollShow(const unsigned int ctrl_id, const bool visible){}
        virtual void Msg_OptionGroupChange(const unsigned int ctrl_id, const int selection){}
        virtual void Msg_Timer(const unsigned int ctrl_id){}
        virtual void Msg_TableSelectItem(const unsigned int ctrl_id, const int selection){}
        virtual void Msg_TableChooseItem(const unsigned ctrl_id, const unsigned selection){}
        virtual void Msg_TableRightButton(const unsigned int ctrl_id, const int selection){}
        virtual void Msg_TableLeftButton(const unsigned int ctrl_id, const int selection){}

        // Sonstiges
        virtual void Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr){}

        // Nachrichten, die von Controls von ctrlGroup weitergeleitet werden
        virtual void Msg_Group_ButtonClick(const unsigned int group_id, const unsigned int ctrl_id){}
        virtual void Msg_Group_EditEnter(const unsigned int group_id, const unsigned int ctrl_id){}
        virtual void Msg_Group_EditChange(const unsigned int group_id, const unsigned int ctrl_id){}
        virtual void Msg_Group_TabChange(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short tab_id){}
        virtual void Msg_Group_ListSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const int selection){}
        virtual void Msg_Group_ComboSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const int selection){}
        virtual void Msg_Group_CheckboxChange(const unsigned int group_id, const unsigned int ctrl_id, const bool checked){}
        virtual void Msg_Group_ProgressChange(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short position){}
        virtual void Msg_Group_ScrollShow(const unsigned int group_id, const unsigned int ctrl_id, const bool visible){}
        virtual void Msg_Group_OptionGroupChange(const unsigned int group_id, const unsigned int ctrl_id, const int selection){}
        virtual void Msg_Group_Timer(const unsigned int group_id, const unsigned int ctrl_id){}
        virtual void Msg_Group_TableSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const int selection){}
        virtual void Msg_Group_TableRightButton(const unsigned int group_id, const unsigned int ctrl_id, const int selection){}
        virtual void Msg_Group_TableLeftButton(const unsigned int group_id, const unsigned int ctrl_id, const int selection){}

    protected:

        /// gets the extent of the window
        Rect GetRect() const { return Rect(x_, y_, GetWidth(), GetHeight()); }
        /// scales X- und Y values to fit the screen
        unsigned short ScaleX(const unsigned short val) const;
        unsigned short ScaleY(const unsigned short val) const;
        /// setzt Scale-Wert, ob neue Controls skaliert werden sollen oder nicht.
        void SetScale(bool scale = true) { this->scale_ = scale; }
        /// zeichnet die Steuerelemente.
        void DrawControls();
        /// prüft ob Mauskoordinaten in einer gesperrten Region liegt.
        bool TestWindowInRegion(Window* window, const MouseCoords& mc) const;
        /// zeichnet das Fenster. (virtuelle Memberfunktion)
        virtual bool Draw_() = 0;
        /// Weiterleitung von Nachrichten von abgeleiteten Klassen erlaubt oder nicht?
        virtual bool IsMessageRelayAllowed() const;
        /// Auf Größe verändern evtl. auch individuell reagieren?
        virtual void Resize_(unsigned short  /*width*/, unsigned short  /*height*/) {}

        template <typename T>
        T* AddCtrl(unsigned int id, T* ctrl)
        {
            RTTR_Assert(childIdToWnd_.find(id) == childIdToWnd_.end());
            // ID auf control mappen
            childIdToWnd_.insert(std::make_pair(id, ctrl));

            // scale-Eigenschaft weitervererben
            ctrl->scale_ = scale_;

            return ctrl;
        }

    protected:
        enum ButtonState
        {
            BUTTON_UP = 0,
            BUTTON_HOVER,
            BUTTON_PRESSED,
            BUTTON_UNKNOWN = 0xFF
        };

        unsigned short x_;         /// X-Position des Fensters.
        unsigned short y_;         /// Y-Position des Fensters.
        unsigned short width_;     /// Breite des Fensters.
        unsigned short height_;    /// Höhe des Fensters.
        unsigned int id_;          /// ID des Fensters.
        Window* parent_;           /// Handle auf das Parentfenster.
        bool active_;              /// Fenster aktiv?
        bool visible_;             /// Fenster sichtbar?
        bool scale_;               /// Sollen Controls an Fenstergröße angepasst werden?
        std::string tooltip_;      /// Tooltip des Fensters (nur bei Controls benutzt)

        std::map<Window*, Rect> lockedAreas_;       /// gesperrte Regionen des Fensters.
        std::vector<Window*> tofreeAreas_;
        bool isInMouseRelay;
        std::map<unsigned int, Window*> childIdToWnd_; /// Die Steuerelemente des Fensters.
};

#endif // !WINDOW_H_INCLUDED
