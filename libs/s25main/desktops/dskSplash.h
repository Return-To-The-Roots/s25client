// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Desktop.h"
#include <memory>
class MouseCoords;

/// Klasse des Splashscreen Desktops.
class dskSplash : public Desktop
{
public:
    dskSplash(std::unique_ptr<glArchivItem_Bitmap> splashImg);
    ~dskSplash() override;

    void SetActive(bool activate) override;

private:
    void Msg_Timer(unsigned ctrl_id) override;
    bool Msg_LeftDown(const MouseCoords& mc) override;
    void LoadFiles();

    std::unique_ptr<glArchivItem_Bitmap> splashImg;
    bool isLoading, isLoaded;
};
