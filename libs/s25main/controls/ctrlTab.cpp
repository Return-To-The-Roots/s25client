// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlTab.h"
#include "Loader.h"
#include "ctrlButton.h"
#include "ctrlGroup.h"
#include "ogl/glArchivItem_Bitmap.h"
struct MouseCoords;

ctrlTab::ctrlTab(Window* parent, unsigned id, const DrawPoint& pos, unsigned short width)
    : Window(parent, id, pos, Extent(width, 45)), tab_count(0), tab_selection(0)
{}

void ctrlTab::msgButtonClick(const unsigned ctrl_id)
{
    setSelection(ctrl_id, true);
}

bool ctrlTab::msgLeftDown(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgLeftDown, mc);
}

bool ctrlTab::msgLeftUp(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgLeftUp, mc);
}

bool ctrlTab::msgWheelUp(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgWheelUp, mc);
}

bool ctrlTab::msgWheelDown(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgWheelDown, mc);
}

bool ctrlTab::msgMouseMove(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgMouseMove, mc);
}

/**
 *  fügt eine Tab hinzu.
 */
ctrlGroup* ctrlTab::addTab(glArchivItem_Bitmap* image, const std::string& tooltip, const unsigned id)
{
    if(tab_count < tabs.size())
    {
        if(addImageButton(tab_count, DrawPoint(36 * tab_count, 0), Extent(36, 45), TextureColor::Red1, image, tooltip))
        {
            tabs[tab_count++] = id;
            ctrlGroup* group = addGroup(tabs.size() + 1 + id);
            group->setVisible(false);

            return group;
        }
    }

    return nullptr;
}

/**
 *  löscht alle Tabs.
 */
void ctrlTab::deleteAllTabs()
{
    for(unsigned i = 0; i < tab_count; ++i)
        deleteCtrl(i);

    tab_selection = 0;
    tab_count = 0;
}
/**
 *  aktiviert eine bestimmte Tabseite.
 */
void ctrlTab::setSelection(unsigned short nr, bool /*notify*/)
{
    if(nr >= tab_count)
        return;

    /// Eltern informieren, dass Tab geändert wurde
    getParent()->msgTabChange(getID(), tabs[nr]);

    // Farbe des alten Buttons ändern
    ctrlButton* button;

    button = getCtrl<ctrlButton>(tab_selection);
    if(button)
        button->setTexture(TextureColor::Red1);

    // Steuerelemente auf der alten Tabseite ausblenden
    getCtrl<ctrlGroup>(tabs[tab_selection] + tabs.size() + 1)->setVisible(false);

    // Umwählen
    tab_selection = nr;

    // Farbe des neuen Buttons ändern
    button = getCtrl<ctrlButton>(tab_selection);
    if(button)
        button->setTexture(TextureColor::Green1);

    // Steuerelemente auf der neuen Tabseite einblenden
    getCtrl<ctrlGroup>(tabs[nr] + tabs.size() + 1)->setVisible(true);
}

/**
 *  Gibt Tab-Group zurück, über die die Steuerelemente der Tab angesprochen
 *  werden können
 */
ctrlGroup* ctrlTab::getGroup(const unsigned tabId)
{
    return getCtrl<ctrlGroup>(tabs.size() + 1 + tabId);
}

/**
 *  Zeichenmethode
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void ctrlTab::drawImpl()
{
    Window::drawImpl();

    // TODO: What is this really?
    int headerSize = tab_count * 36;
    LOADER.getImageN("io", 3)->drawPart(
      Rect(getDrawPos() + DrawPoint(headerSize, 0), Extent(getSize().x - headerSize, 45)));

    draw3D(Rect(getDrawPos() + DrawPoint(0, 32), Extent(getSize().x, 13)), TextureColor::Green1, true);

    auto* button = getCtrl<ctrlButton>(tab_selection);
    if(button)
        button->draw();
}

void ctrlTab::msgGroupButtonClick(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    getParent()->msgGroupButtonClick(this->getID(), ctrl_id);
}

void ctrlTab::msgGroupEditEnter(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    getParent()->msgGroupEditEnter(this->getID(), ctrl_id);
}

void ctrlTab::msgGroupEditChange(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    getParent()->msgGroupEditChange(this->getID(), ctrl_id);
}

void ctrlTab::msgGroupTabChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned short tabId)
{
    getParent()->msgGroupTabChange(this->getID(), ctrl_id, tabId);
}

void ctrlTab::msgGroupListSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id, const int selection)
{
    getParent()->msgGroupListSelectItem(this->getID(), ctrl_id, selection);
}

void ctrlTab::msgGroupComboSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned selection)
{
    getParent()->msgGroupComboSelectItem(this->getID(), ctrl_id, selection);
}

void ctrlTab::msgGroupCheckboxChange(const unsigned /*group_id*/, const unsigned ctrl_id, const bool checked)
{
    getParent()->msgGroupCheckboxChange(this->getID(), ctrl_id, checked);
}

void ctrlTab::msgGroupProgressChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned short position)
{
    getParent()->msgGroupProgressChange(this->getID(), ctrl_id, position);
}

void ctrlTab::msgGroupScrollShow(const unsigned /*group_id*/, const unsigned ctrl_id, const bool visible)
{
    getParent()->msgGroupScrollShow(this->getID(), ctrl_id, visible);
}

void ctrlTab::msgGroupOptionGroupChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned selection)
{
    getParent()->msgGroupOptionGroupChange(this->getID(), ctrl_id, selection);
}

void ctrlTab::msgGroupTimer(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    getParent()->msgGroupTimer(this->getID(), ctrl_id);
}

void ctrlTab::msgGroupTableSelectItem(const unsigned /*group_id*/, const unsigned ctrl_id,
                                      const std::optional<unsigned>& selection)
{
    getParent()->msgGroupTableSelectItem(this->getID(), ctrl_id, selection);
}

void ctrlTab::msgGroupTableRightButton(const unsigned /*group_id*/, const unsigned ctrl_id,
                                       const std::optional<unsigned>& selection)
{
    getParent()->msgGroupTableRightButton(this->getID(), ctrl_id, selection);
}

void ctrlTab::msgGroupTableLeftButton(const unsigned /*group_id*/, const unsigned ctrl_id,
                                      const std::optional<unsigned>& selection)
{
    getParent()->msgGroupTableLeftButton(this->getID(), ctrl_id, selection);
}
