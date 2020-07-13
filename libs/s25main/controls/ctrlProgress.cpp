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

#include "ctrlProgress.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "WindowManager.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"
#include <cmath>

ctrlProgress::ctrlProgress(Window* parent, const unsigned id, const DrawPoint& pos, const Extent& size, const TextureColor tc,
                           unsigned short button_minus, unsigned short button_plus, const unsigned short maximum, const Extent& padding,
                           const unsigned force_color, const std::string& tooltip, const std::string& button_minus_tooltip,
                           const std::string& button_plus_tooltip, unsigned short* const /*write_val*/)
    : Window(parent, id, pos, size), ctrlBaseTooltip(tooltip), tc(tc), position(0), maximum(maximum), padding_(padding),
      force_color(force_color)
{
    const char *str1 = "io", *str2 = "io";
    if(button_minus >= 1000)
    {
        str1 = "io_new";
        button_minus -= 1000;
    }
    if(button_plus >= 1000)
    {
        str2 = "io_new";
        button_plus -= 1000;
    }

    Extent btSize = Extent::all(size.y);
    AddImageButton(0, DrawPoint(0, 0), btSize, tc, LOADER.GetImageN(str1, button_minus),
                   (button_minus_tooltip.length() ? button_minus_tooltip : _("Less")));
    AddImageButton(1, DrawPoint(size.x - btSize.x, 0), btSize, tc, LOADER.GetImageN(str2, button_plus),
                   (button_plus_tooltip.length() ? button_plus_tooltip : _("More")));

    // Hide left and right 3D border by making the buttons overlap the bar
    padding_.x -= 2;
}

/**
 *  setzt die Position an den angegebenen Wert.
 */
void ctrlProgress::SetPosition(unsigned short position)
{
    this->position = std::min(position, maximum);
}

/**
 *  Zeichenmethode.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void ctrlProgress::Draw_()
{
    DrawPoint barPos = GetDrawPos() + DrawPoint(padding_);
    // Offset by button size
    barPos.x += GetSize().y;
    Extent barSize(CalcBarWidth(), GetSize().y - 2 * padding_.y);
    Draw3D(Rect(barPos, barSize), tc, false);

    // Buttons
    Window::Draw_();

    const DrawPoint innerPadding(4, 4);
    unsigned percentage = position * 100 / maximum;
    unsigned progress = (CalcBarWidth() - innerPadding.x * 2) * position / maximum;

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
    DrawRectangle(Rect(barPos + innerPadding, progress, GetSize().y - 2 * (innerPadding.y + padding_.y)), color);

    // Prozentzahlen zeichnen
    SmallFont->Draw(GetDrawPos() + DrawPoint(GetSize()) / 2, std::to_string(percentage) + "%", FontStyle::CENTER | FontStyle::VCENTER,
                    COLOR_YELLOW);
}

void ctrlProgress::Resize(const Extent& newSize)
{
    Window::Resize(newSize);

    auto* lessBt = GetCtrl<Window>(0);
    auto* moreBt = GetCtrl<Window>(1);
    Extent btSize(newSize.y, newSize.y);
    lessBt->Resize(btSize);
    moreBt->Resize(btSize);
    moreBt->SetPos(DrawPoint(newSize.x - btSize.x, 0));
}

unsigned ctrlProgress::CalcBarWidth() const
{
    return GetSize().x - 2 * (padding_.x + GetSize().y);
}

void ctrlProgress::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // Minus
        {
            if(position)
                --position;
            if(GetParent())
                GetParent()->Msg_ProgressChange(GetID(), position);
        }
        break;
        case 1: // Plus
        {
            if(position < maximum)
                ++position;
            if(GetParent())
                GetParent()->Msg_ProgressChange(GetID(), position);
        }
        break;
    }
}

bool ctrlProgress::Msg_LeftDown(const MouseCoords& mc)
{
    // Test if clicked on progress bar
    DrawPoint progressOrigin = GetDrawPos() + DrawPoint(padding_) + DrawPoint(GetSize().y + 2, 4);
    Extent progressSize = GetSize() - Extent((GetSize().y + 1) * 2, 8) - padding_ * 2u;
    if(IsPointInRect(mc.GetPos(), Rect(progressOrigin, progressSize)))
    {
        position = static_cast<uint16_t>(std::lround(static_cast<double>((mc.pos.x - progressOrigin.x) * maximum) / progressSize.x));

        if(GetParent())
            GetParent()->Msg_ProgressChange(GetID(), position);
        return true;
    } else
        return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

bool ctrlProgress::Msg_LeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

bool ctrlProgress::Msg_WheelUp(const MouseCoords& mc)
{
    // If mouse is over the controls, simulate button click
    if(IsPointInRect(mc.GetPos(), GetDrawRect()))
    {
        Msg_ButtonClick(1);
        return true;
    }
    return false;
}

bool ctrlProgress::Msg_WheelDown(const MouseCoords& mc)
{
    // If mouse is over the controls, simulate button click
    if(IsPointInRect(mc.GetPos(), GetDrawRect()))
    {
        Msg_ButtonClick(0);
        return true;
    }
    return false;
}

bool ctrlProgress::Msg_MouseMove(const MouseCoords& mc)
{
    // an Buttons weiterleiten
    RelayMouseMessage(&Window::Msg_MouseMove, mc);

    Extent offset(GetSize().y + padding_.x, 0);
    DrawPoint rectOrig = GetDrawPos() + DrawPoint(offset);
    Extent rectSize = GetSize() - offset * 2u;
    if(IsPointInRect(mc.GetPos(), Rect(rectOrig, rectSize)))
    {
        WINDOWMANAGER.SetToolTip(this, tooltip_);
        return true;
    } else
    {
        WINDOWMANAGER.SetToolTip(this, "");
        return false;
    }
}
