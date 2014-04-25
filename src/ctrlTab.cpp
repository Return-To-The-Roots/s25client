// $Id: ctrlTab.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "ctrlTab.h"

#include "ctrlButton.h"
#include "ctrlGroup.h"

#include "Loader.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlTab.
 *
 *  @author OLiver
 */
ctrlTab::ctrlTab(Window* parent,
                 unsigned int id,
                 unsigned short x,
                 unsigned short y,
                 unsigned short width)
    : Window(x, y, id, parent, width, 45),
      tab_count(0), tab_selection(0)
{
    memset(tabs, 0, MAX_TAB_COUNT * sizeof(unsigned int));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_ButtonClick(const unsigned int ctrl_id)
{
    SetSelection(ctrl_id, true);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlTab::Msg_LeftDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlTab::Msg_LeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author Divan
 */
bool ctrlTab::Msg_WheelUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelUp, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author Divan
 */
bool ctrlTab::Msg_WheelDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_WheelDown, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlTab::Msg_MouseMove(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  fügt eine Tab hinzu.
 *
 *  @author OLiver
 */
ctrlGroup* ctrlTab::AddTab(glArchivItem_Bitmap* image, std::string tooltip, const unsigned int id)
{
    if(tab_count < MAX_TAB_COUNT)
    {
        if(AddImageButton(tab_count, 36 * tab_count, 0, 36, 45, TC_RED1, image, tooltip) != NULL)
        {
            tabs[tab_count++] = id;
            ctrlGroup* group = AddGroup(MAX_TAB_COUNT + 1 + id);
            group->SetVisible(false);

            return group;
        }
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  löscht alle Tabs.
 *
 *  @author OLiver
 */
void ctrlTab::DeleteAllTabs(void)
{
    for(unsigned int i = 0; i < tab_count; ++i)
        DeleteCtrl(i);

    memset(tabs, 0, MAX_TAB_COUNT * sizeof(unsigned int));

    tab_selection = 0;
    tab_count = 0;
}
///////////////////////////////////////////////////////////////////////////////
/**
 *  aktiviert eine bestimmte Tabseite.
 *
 *  @author OLiver
 */
void ctrlTab::SetSelection(unsigned short nr, bool notify)
{
    /// Eltern informieren, dass Tab geändert wurde
    parent->Msg_TabChange(GetID(), tabs[nr]);

    // Farbe des alten Buttons ändern
    ctrlButton* button;

    button = GetCtrl<ctrlButton>(tab_selection);
    if(button)
        button->SetTexture(TC_RED1);

    // Steuerelemente auf der alten Tabseite ausblenden
    GetCtrl<ctrlGroup>(tabs[tab_selection] + MAX_TAB_COUNT + 1)->SetVisible(false);

    // Umwählen
    tab_selection = nr;

    // Farbe des neuen Buttons ändern
    button = GetCtrl<ctrlButton>(tab_selection);
    if(button)
        button->SetTexture(TC_GREEN1);

    // Steuerelemente auf der neuen Tabseite einblenden
    GetCtrl<ctrlGroup>(tabs[nr] + MAX_TAB_COUNT + 1)->SetVisible(true);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Gibt Tab-Group zurück, über die die Steuerelemente der Tab angesprochen
 *  werden können
 *
 *  @author OLiver
 */
ctrlGroup* ctrlTab::GetGroup(const unsigned int tab_id)
{
    //unsigned int real_id = 0xffffffff;
    //
    //for(unsigned short i = 0; i < tab_count; ++i)
    //  if(tabs[i] == tab_id)
    //      real_id = i;
    //
    //if(real_id == 0xffffffff)
    //  return NULL;

    return GetCtrl<ctrlGroup>(MAX_TAB_COUNT + 1 + tab_id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichenmethode
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 *
 *  @author OLiver
 */
bool ctrlTab::Draw_(void)
{
    DrawControls();

    LOADER.GetImageN("io", 3)->Draw(GetX() + tab_count * 36, GetY(), 0, 0, 0, 0, width - tab_count * 36, 45);

    Draw3D(GetX(), GetY() + 32, width, 13, TC_GREEN1, 0);

    ctrlButton* button = GetCtrl<ctrlButton>(tab_selection);
    if(button)
        button->Draw();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_ButtonClick(const unsigned int group_id, const unsigned int ctrl_id)
{
    parent->Msg_Group_ButtonClick(this->id, ctrl_id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_EditEnter(const unsigned int group_id, const unsigned int ctrl_id)
{
    parent->Msg_Group_EditEnter(this->id, ctrl_id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_EditChange(const unsigned int group_id, const unsigned int ctrl_id)
{
    parent->Msg_Group_EditChange(this->id, ctrl_id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_TabChange(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short tab_id)
{
    parent->Msg_Group_TabChange(this->id, ctrl_id, tab_id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_ListSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short selection)
{
    parent->Msg_Group_ListSelectItem(this->id, ctrl_id, selection);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_ComboSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short selection)
{
    parent->Msg_Group_ComboSelectItem(this->id, ctrl_id, selection);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_CheckboxChange(const unsigned int group_id, const unsigned int ctrl_id, const bool checked)
{
    parent->Msg_Group_CheckboxChange(this->id, ctrl_id, checked);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_ProgressChange(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short position)
{
    parent->Msg_Group_ProgressChange(this->id, ctrl_id, position);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_ScrollShow(const unsigned int group_id, const unsigned int ctrl_id, const bool visible)
{
    parent->Msg_Group_ScrollShow(this->id, ctrl_id, visible);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_OptionGroupChange(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short selection)
{
    parent->Msg_Group_OptionGroupChange(this->id, ctrl_id, selection);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_Timer(const unsigned int group_id, const unsigned int ctrl_id)
{
    parent->Msg_Group_Timer(this->id, ctrl_id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_TableSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short selection)
{
    parent->Msg_Group_TableSelectItem(this->id, ctrl_id, selection);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_TableRightButton(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short selection)
{
    parent->Msg_Group_TableRightButton(this->id, ctrl_id, selection);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlTab::Msg_Group_TableLeftButton(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short selection)
{
    parent->Msg_Group_TableLeftButton(this->id, ctrl_id, selection);
}
