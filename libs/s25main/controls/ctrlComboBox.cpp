// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlComboBox.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "ctrlButton.h"
#include "ctrlList.h"
#include "driver/MouseCoords.h"
#include "ogl/FontStyle.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glFont.h"

ctrlComboBox::ctrlComboBox(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                           const glFont* font, unsigned short max_list_height, bool readonly)
    : Window(parent, id, pos, size), tc(tc), font(font), max_list_height(max_list_height), readonly(readonly),
      suppressSelectEvent(false)
{
    addList(0, DrawPoint(0, size.y), Extent(size.x, 4), tc, font)->setVisible(false);

    if(!readonly)
        addImageButton(1, DrawPoint(size.x - size.y, 0), Extent(size.y, size.y), tc, LOADER.getImageN("io", 34));

    resize(size);
}

void ctrlComboBox::resize(const Extent& newSize)
{
    Window::resize(newSize);

    auto* button = getCtrl<ctrlButton>(1);
    if(button)
    {
        button->setPos(DrawPoint(newSize.x - newSize.y, 0));
        button->resize(Extent(newSize.y, newSize.y));
    }

    auto* list = getCtrl<ctrlList>(0);

    Extent listSize(newSize.x, 4);

    // Langsam die Höhe der maximalen annähern
    for(unsigned i = 0; i < list->getNumLines(); ++i)
    {
        // zu große geworden?
        listSize.y += font->getHeight();
        unsigned short scaledMaxHeight = scaleIf(Extent(0, max_list_height)).y;

        if(listSize.y > scaledMaxHeight)
        {
            // kann nicht mal ein Item aufnehmen, dann raus
            if(i == 0)
                return;

            // Höhe um eins erniedrigen, damits wieder kleiner ist als die maximale
            listSize.y -= font->getHeight();
            break;
        }
    }

    list->setPos(DrawPoint(0, newSize.y));
    list->resize(listSize);
}

std::optional<std::string> ctrlComboBox::getSelectedText() const
{
    const std::optional<unsigned>& selection = getSelection();
    if(selection)
        return getText(*selection);
    else
        return std::nullopt;
}

bool ctrlComboBox::msgMouseMove(const MouseCoords& mc)
{
    // Für Button und Liste weiterleiten
    return relayMouseMessage(&Window::msgMouseMove, mc);
}

bool ctrlComboBox::msgLeftDown(const MouseCoords& mc)
{
    auto* list = getCtrl<ctrlList>(0);

    // Irgendwo anders hingeklickt --> Liste ausblenden
    if(!readonly && !IsPointInRect(mc.pos, getFullDrawRect(list)))
    {
        // Liste wieder ausblenden
        showList(false);
        return false;
    }

    if(!readonly && IsPointInRect(mc.pos, getDrawRect()))
    {
        // Liste wieder ein/ausblenden
        showList(!list->isVisible());
        return true;
    }

    // Für Button und Liste weiterleiten
    return relayMouseMessage(&Window::msgLeftDown, mc);
}

bool ctrlComboBox::msgLeftUp(const MouseCoords& mc)
{
    // Für Button und Liste weiterleiten
    return relayMouseMessage(&Window::msgLeftUp, mc);
}

bool ctrlComboBox::msgRightDown(const MouseCoords& mc)
{
    auto* list = getCtrl<ctrlList>(0);

    // Für Button und Liste weiterleiten (und danach erst schließen)
    bool ret = relayMouseMessage(&Window::msgRightDown, mc);

    // Clicked on list -> close it
    if(!readonly && IsPointInRect(mc.pos, list->getDrawRect()))
    {
        // Liste wieder ausblenden
        showList(false);
    }

    return ret;
}

bool ctrlComboBox::msgWheelUp(const MouseCoords& mc)
{
    if(readonly)
        return false;

    auto* list = getCtrl<ctrlList>(0);
    if(list->isVisible() && IsPointInRect(mc.pos, list->getDrawRect()))
        return relayMouseMessage(&Window::msgWheelUp, mc);

    if(IsPointInRect(mc.pos, getDrawRect()))
    {
        // Don't scroll too far down
        if(list->getSelection().value_or(0u) > 0u)
            list->setSelection(*list->getSelection() - 1u);
        return true;
    }

    return false;
}

bool ctrlComboBox::msgWheelDown(const MouseCoords& mc)
{
    if(readonly)
        return false;

    auto* list = getCtrl<ctrlList>(0);

    if(list->isVisible() && IsPointInRect(mc.pos, list->getDrawRect()))
    {
        // Scrolled in opened list ->
        return relayMouseMessage(&Window::msgWheelDown, mc);
    }

    if(IsPointInRect(mc.pos, getDrawRect()))
    {
        // Will be ignored by the list if to high
        list->setSelection(list->getSelection() ? *list->getSelection() + 1u : 0u);
        return true;
    }

    return false;
}

Rect ctrlComboBox::getFullDrawRect(const ctrlList* list)
{
    Rect myRect = getDrawRect();
    myRect.bottom = list->getDrawRect().bottom;
    return myRect;
}

void ctrlComboBox::msgListSelectItem(unsigned, const int selection)
{
    // Liste wieder ausblenden
    showList(false);

    // ist in der Liste überhaupt was drin?
    if(selection >= 0 && !suppressSelectEvent)
    {
        // Nachricht an übergeordnetes Fenster verschicken
        getParent()->msgComboSelectItem(getID(), selection);
    }
}

void ctrlComboBox::addItem(const std::string& text)
{
    getCtrl<ctrlList>(0)->addItem(text);
    resize(getSize());
}

void ctrlComboBox::deleteAllItems()
{
    getCtrl<ctrlList>(0)->deleteAllItems();
    resize(getSize());
}

void ctrlComboBox::setSelection(unsigned selection)
{
    // Avoid sending the change method when this is invoked intentionally
    suppressSelectEvent = true;
    getCtrl<ctrlList>(0)->setSelection(selection);
    suppressSelectEvent = false;
}

void ctrlComboBox::drawImpl()
{
    auto* liste = getCtrl<ctrlList>(0);

    draw3D(Rect(getDrawPos(), getSize()), tc, false);

    // Show selected item in the box
    if(liste->getNumLines() > 0)
    {
        font->draw(getDrawPos() + DrawPoint(2, getSize().y / 2), liste->getSelItemText(), FontStyle::VCENTER,
                   COLOR_YELLOW, getSize().x - 2 - getSize().y, "");
    }

    // Draw button manually as we can't use DrawControls which would draw the list we do in Msg_PaintAfter
    auto* button = getCtrl<ctrlButton>(1);
    if(button)
        button->draw();
}

void ctrlComboBox::msgPaintAfter()
{
    // Draw list now so it is on top of everything
    getCtrl<ctrlList>(0)->draw();
    Window::msgPaintAfter();
}

void ctrlComboBox::showList(bool show)
{
    auto* list = getCtrl<ctrlList>(0);
    if(list->isVisible() == show)
        return;

    // list field
    list->setVisible(show);
    // Arrow button
    getCtrl<ctrlButton>(1)->setChecked(show);

    // Lock/unlock region of extended list
    if(show)
        getParent()->lockRegion(this, list->getDrawRect());
    else
        getParent()->freeRegion(this);

    LOADER.getSoundN("sound", 113)->play(255, false);
}
