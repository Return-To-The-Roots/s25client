// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlProgress.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "WindowManager.h"
#include "helpers/mathFuncs.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"

ctrlProgress::ctrlProgress(Window* parent, const unsigned id, const DrawPoint& pos, const Extent& size,
                           const TextureColor tc, unsigned short button_minus, unsigned short button_plus,
                           const unsigned short maximum, const Extent& padding, const unsigned force_color,
                           const std::string& tooltip, const std::string& button_minus_tooltip,
                           const std::string& button_plus_tooltip, unsigned short* const /*write_val*/)
    : Window(parent, id, pos, size), ctrlBaseTooltip(tooltip), tc(tc), position(0), maximum(maximum), padding_(padding),
      force_color(force_color)
{
    ResourceId btMinusRes = "io", btPlusRes = "io";
    if(button_minus >= 1000)
    {
        btMinusRes = "io_new";
        button_minus -= 1000;
    }
    if(button_plus >= 1000)
    {
        btPlusRes = "io_new";
        button_plus -= 1000;
    }

    Extent btSize = Extent::all(size.y);
    addImageButton(0, DrawPoint(0, 0), btSize, tc, LOADER.getImageN(btMinusRes, button_minus),
                   (!button_minus_tooltip.empty() ? button_minus_tooltip : _("Less")));
    addImageButton(1, DrawPoint(size.x - btSize.x, 0), btSize, tc, LOADER.getImageN(btPlusRes, button_plus),
                   (!button_plus_tooltip.empty() ? button_plus_tooltip : _("More")));

    // Hide left and right 3D border by making the buttons overlap the bar
    padding_.x -= 2;
}

/**
 *  setzt die Position an den angegebenen Wert.
 */
void ctrlProgress::setPosition(unsigned short position)
{
    this->position = std::min(position, maximum);
}

/**
 *  Zeichenmethode.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void ctrlProgress::drawImpl()
{
    DrawPoint barPos = getDrawPos() + DrawPoint(padding_);
    // Offset by button size
    barPos.x += getSize().y;
    Extent barSize(calcBarWidth(), getSize().y - 2 * padding_.y);
    draw3D(Rect(barPos, barSize), tc, false);

    // Buttons
    Window::drawImpl();

    const DrawPoint innerPadding(4, 4);
    unsigned percentage = position * 100 / maximum;
    unsigned progress = (calcBarWidth() - innerPadding.x * 2) * position / maximum;

    // Farbe herausfinden
    unsigned color = 0xFFD70000;

    // Feste Farbe?
    if(force_color)
        color = force_color;
    else
    {
        // Farbe wählen je nachdem wie viel Prozent
        if(percentage >= 60)
            color = 0xFF71B63C;
        else if(percentage >= 30)
            color = 0xFFFFBF33;
        else if(percentage >= 20)
            color = 0xFFDB7428;
    }

    // Leiste
    drawRectangle(Rect(barPos + innerPadding, progress, getSize().y - 2 * (innerPadding.y + padding_.y)), color);

    // Prozentzahlen zeichnen
    SmallFont->draw(getDrawPos() + DrawPoint(getSize()) / 2, std::to_string(percentage) + "%",
                    FontStyle::CENTER | FontStyle::VCENTER, COLOR_YELLOW);
}

void ctrlProgress::resize(const Extent& newSize)
{
    Window::resize(newSize);

    auto* lessBt = getCtrl<Window>(0);
    auto* moreBt = getCtrl<Window>(1);
    Extent btSize(newSize.y, newSize.y);
    lessBt->resize(btSize);
    moreBt->resize(btSize);
    moreBt->setPos(DrawPoint(newSize.x - btSize.x, 0));
}

unsigned ctrlProgress::calcBarWidth() const
{
    return getSize().x - 2 * (padding_.x + getSize().y);
}

void ctrlProgress::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // Minus
        {
            if(position)
                --position;
            if(getParent())
                getParent()->msgProgressChange(getID(), position);
        }
        break;
        case 1: // Plus
        {
            if(position < maximum)
                ++position;
            if(getParent())
                getParent()->msgProgressChange(getID(), position);
        }
        break;
    }
}

bool ctrlProgress::msgLeftDown(const MouseCoords& mc)
{
    // Test if clicked on progress bar
    DrawPoint progressOrigin = getDrawPos() + DrawPoint(padding_) + DrawPoint(getSize().y + 2, 4);
    Extent progressSize = getSize() - Extent((getSize().y + 1) * 2, 8) - padding_ * 2u;
    if(IsPointInRect(mc.pos, Rect(progressOrigin, progressSize)))
    {
        position =
          helpers::iround<uint16_t>(static_cast<double>((mc.pos.x - progressOrigin.x) * maximum) / progressSize.x);

        if(getParent())
            getParent()->msgProgressChange(getID(), position);
        return true;
    } else
        return relayMouseMessage(&Window::msgLeftDown, mc);
}

bool ctrlProgress::msgLeftUp(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgLeftUp, mc);
}

bool ctrlProgress::msgWheelUp(const MouseCoords& mc)
{
    // If mouse is over the controls, simulate button click
    if(IsPointInRect(mc.pos, getDrawRect()))
    {
        msgButtonClick(1);
        return true;
    }
    return false;
}

bool ctrlProgress::msgWheelDown(const MouseCoords& mc)
{
    // If mouse is over the controls, simulate button click
    if(IsPointInRect(mc.pos, getDrawRect()))
    {
        msgButtonClick(0);
        return true;
    }
    return false;
}

bool ctrlProgress::msgMouseMove(const MouseCoords& mc)
{
    // an Buttons weiterleiten
    relayMouseMessage(&Window::msgMouseMove, mc);

    Extent offset(getSize().y + padding_.x, 0);
    DrawPoint rectOrig = getDrawPos() + DrawPoint(offset);
    Extent rectSize = getSize() - offset * 2u;
    if(IsPointInRect(mc.pos, Rect(rectOrig, rectSize)))
    {
        WINDOWMANAGER.setToolTip(this, tooltip_);
        return true;
    } else
    {
        WINDOWMANAGER.setToolTip(this, "");
        return false;
    }
}
