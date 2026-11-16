// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Desktop.h"
#include <memory>
struct MouseCoords;

/// Klasse des Splashscreen Desktops.
class dskSplash : public Desktop
{
public:
    dskSplash(std::unique_ptr<glArchivItem_Bitmap> splashImg);
    ~dskSplash() override;

    void setActive(bool activate) override;

private:
    void msgTimer(unsigned ctrl_id) override;
    bool msgLeftDown(const MouseCoords& mc) override;
    void LoadFiles();

    std::unique_ptr<glArchivItem_Bitmap> splashImg;
    bool isLoading, isLoaded;
};
