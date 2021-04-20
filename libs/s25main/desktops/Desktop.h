// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
