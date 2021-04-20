// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Desktop.h"

class glArchivItem_Bitmap;

/// Base class for all the desktops making up the menus
/// Adds the basic background logo and title and version texts at the bottom
class dskMenuBase : public Desktop
{
public:
    /// Create using the basic menu background
    dskMenuBase();
    /// Create using the specified background
    dskMenuBase(glArchivItem_Bitmap* background);

    enum ControlIds
    {
        ID_txtVersion,
        ID_txtURL,
        ID_txtCopyright,
        /// First free ID to use for own controls
        ID_FIRST_FREE
    };

private:
    void AddBottomTexts();
};
