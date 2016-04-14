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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "ctrlGroup.h"
#include "drivers/ScreenResizeEvent.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class MouseCoords;
struct KeyEvent;

ctrlGroup::ctrlGroup(Window* parent,
                     unsigned int id,
                     bool scale)
    : Window(0, 0, id, parent)
{
    SetScale(scale);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichenmethode
 *
 *  @author FloSoft
 */
bool ctrlGroup::Draw_()
{
    // Steuerelemente zeichnen
    DrawControls();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/*
 *  Reagiert auf Spielfenstergrößenänderung
 *
 *  @author Divan
 */
void ctrlGroup::Msg_ScreenResize(const ScreenResizeEvent& sr)
{
// Keep the following block the same as in Desktop class:
    // Für skalierte Desktops ist alles einfach, die brauchen im besten Fall gar nichts selbst implementieren
    if (scale_)
    {
        //Zunächst an die Kinder weiterleiten
        for(std::map<unsigned int, Window*>::iterator it = childIdToWnd_.begin(); it != childIdToWnd_.end(); ++it)
        {
            if(!it->second)
                continue;
            Window* ctrl = it->second;
            // unskalierte Position und Größe bekommen
            unsigned realX = ctrl->GetX() * 800 / sr.oldWidth;
            unsigned realY = ctrl->GetY() * 600 / sr.oldHeight;
            unsigned realWidth = ctrl->GetWidth() * 800 / sr.oldWidth;
            unsigned realHeight = ctrl->GetHeight() * 600 / sr.oldHeight;
            // Rundungsfehler?
            if(realX * sr.oldWidth / 800 < ctrl->GetX()) ++realX;
            if(realY * sr.oldHeight / 600 < ctrl->GetY()) ++realY;
            if(realWidth  * sr.oldWidth / 800 < ctrl->GetWidth())  ++realWidth;
            if(realHeight * sr.oldHeight / 600 < ctrl->GetHeight()) ++realHeight;
            // Und los
            ctrl->Move(realX * sr.newWidth / 800, realY * sr.newHeight / 600);
            ctrl->Msg_ScreenResize(sr);
            ctrl->Resize(realWidth * sr.newWidth / 800, realHeight * sr.newHeight / 600);
        }
    }
}
void ctrlGroup::Msg_ButtonClick(const unsigned int ctrl_id)
{
    parent_->Msg_Group_ButtonClick(this->id_, ctrl_id);
}

void ctrlGroup::Msg_EditEnter(const unsigned int ctrl_id)
{
    parent_->Msg_Group_EditEnter(this->id_, ctrl_id);
}

void ctrlGroup::Msg_EditChange(const unsigned int ctrl_id)
{
    parent_->Msg_Group_EditChange(this->id_, ctrl_id);
}

void ctrlGroup::Msg_TabChange(const unsigned int ctrl_id, const unsigned short tab_id)
{
    parent_->Msg_Group_TabChange(this->id_, ctrl_id, tab_id);
}

void ctrlGroup::Msg_ListSelectItem(const unsigned int ctrl_id, const int selection)
{
    parent_->Msg_Group_ListSelectItem(this->id_, ctrl_id, selection);
}

void ctrlGroup::Msg_ComboSelectItem(const unsigned int ctrl_id, const int selection)
{
    parent_->Msg_Group_ComboSelectItem(this->id_, ctrl_id, selection);
}

void ctrlGroup::Msg_CheckboxChange(const unsigned int ctrl_id, const bool checked)
{
    parent_->Msg_Group_CheckboxChange(this->id_, ctrl_id, checked);
}

void ctrlGroup::Msg_ProgressChange(const unsigned int ctrl_id, const unsigned short position)
{
    parent_->Msg_Group_ProgressChange(this->id_, ctrl_id, position);
}

void ctrlGroup::Msg_ScrollShow(const unsigned int ctrl_id, const bool visible)
{
    parent_->Msg_Group_ScrollShow(this->id_, ctrl_id, visible);
}

void ctrlGroup::Msg_OptionGroupChange(const unsigned int ctrl_id, const int selection)
{
    parent_->Msg_Group_OptionGroupChange(this->id_, ctrl_id, selection);
}

void ctrlGroup::Msg_Timer(const unsigned int ctrl_id)
{
    parent_->Msg_Group_Timer(this->id_, ctrl_id);
}

void ctrlGroup::Msg_TableSelectItem(const unsigned int ctrl_id, const int selection)
{
    parent_->Msg_Group_TableSelectItem(this->id_, ctrl_id, selection);
}

void ctrlGroup::Msg_TableRightButton(const unsigned int ctrl_id, const int selection)
{
    parent_->Msg_Group_TableRightButton(this->id_, ctrl_id, selection);
}

void ctrlGroup::Msg_TableLeftButton(const unsigned int ctrl_id, const int selection)
{
    parent_->Msg_Group_TableLeftButton(this->id_, ctrl_id, selection);
}

bool ctrlGroup::Msg_LeftDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

bool ctrlGroup::Msg_RightDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_RightDown, mc);
}

bool ctrlGroup::Msg_LeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

bool ctrlGroup::Msg_RightUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_RightUp, mc);
}

bool ctrlGroup::Msg_WheelUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelUp, mc);
}

bool ctrlGroup::Msg_WheelDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelDown, mc);
}

bool ctrlGroup::Msg_MouseMove(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}

bool ctrlGroup::Msg_KeyDown(const KeyEvent& ke)
{
    return RelayKeyboardMessage(&Window::Msg_KeyDown, ke);
}


void ctrlGroup::Msg_Group_ButtonClick(const unsigned int  /*group_id*/, const unsigned int ctrl_id)
{
    parent_->Msg_Group_ButtonClick(this->id_, ctrl_id);
}

void ctrlGroup::Msg_Group_EditEnter(const unsigned int  /*group_id*/, const unsigned int ctrl_id)
{
    parent_->Msg_Group_EditEnter(this->id_, ctrl_id);
}

void ctrlGroup::Msg_Group_EditChange(const unsigned int  /*group_id*/, const unsigned int ctrl_id)
{
    parent_->Msg_Group_EditChange(this->id_, ctrl_id);
}

void ctrlGroup::Msg_Group_TabChange(const unsigned int  /*group_id*/, const unsigned int ctrl_id, const unsigned short tab_id)
{
    parent_->Msg_Group_TabChange(this->id_, ctrl_id, tab_id);
}

void ctrlGroup::Msg_Group_ListSelectItem(const unsigned int  /*group_id*/, const unsigned int ctrl_id, const int selection)
{
    parent_->Msg_Group_ListSelectItem(this->id_, ctrl_id, selection);
}

void ctrlGroup::Msg_Group_ComboSelectItem(const unsigned int  /*group_id*/, const unsigned int ctrl_id, const int selection)
{
    parent_->Msg_Group_ComboSelectItem(this->id_, ctrl_id, selection);
}

void ctrlGroup::Msg_Group_CheckboxChange(const unsigned int  /*group_id*/, const unsigned int ctrl_id, const bool checked)
{
    parent_->Msg_Group_CheckboxChange(this->id_, ctrl_id, checked);
}

void ctrlGroup::Msg_Group_ProgressChange(const unsigned int  /*group_id*/, const unsigned int ctrl_id, const unsigned short position)
{
    parent_->Msg_Group_ProgressChange(this->id_, ctrl_id, position);
}

void ctrlGroup::Msg_Group_ScrollShow(const unsigned int  /*group_id*/, const unsigned int ctrl_id, const bool visible)
{
    parent_->Msg_Group_ScrollShow(this->id_, ctrl_id, visible);
}

void ctrlGroup::Msg_Group_OptionGroupChange(const unsigned int  /*group_id*/, const unsigned int ctrl_id, const int selection)
{
    parent_->Msg_Group_OptionGroupChange(this->id_, ctrl_id, selection);
}

void ctrlGroup::Msg_Group_Timer(const unsigned int  /*group_id*/, const unsigned int ctrl_id)
{
    parent_->Msg_Group_Timer(this->id_, ctrl_id);
}

void ctrlGroup::Msg_Group_TableSelectItem(const unsigned int  /*group_id*/, const unsigned int ctrl_id, const int selection)
{
    parent_->Msg_Group_TableSelectItem(this->id_, ctrl_id, selection);
}

void ctrlGroup::Msg_Group_TableRightButton(const unsigned int  /*group_id*/, const unsigned int ctrl_id, const int selection)
{
    parent_->Msg_Group_TableRightButton(this->id_, ctrl_id, selection);
}

void ctrlGroup::Msg_Group_TableLeftButton(const unsigned int  /*group_id*/, const unsigned int ctrl_id, const int selection)
{
    parent_->Msg_Group_TableLeftButton(this->id_, ctrl_id, selection);
}
