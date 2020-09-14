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

#pragma once

#include "Window.h"

class IngameWindow;
class glArchivItem_Bitmap;
struct ScreenResizeEvent;

/// Desktopklasse für Spielmenü-Haupthintergrundflächen.
class Desktop : public Window
{
public:
    Desktop(glArchivItem_Bitmap* background);
    ~Desktop();
    void Msg_ScreenResize(const ScreenResizeEvent& sr) override;
    /// Callback when a window was closed
    virtual void Msg_WindowClosed(IngameWindow&){};
    /// Show or hide the fps
    void SetFpsDisplay(bool show);

    /// ID of the fps display text
    static const unsigned fpsDisplayId;

protected:
    void Draw_() override;

    glArchivItem_Bitmap* background;

private:
    void UpdateFps(unsigned newFps);

    unsigned lastFPS_;
};
