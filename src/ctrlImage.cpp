// $Id: ctrlImage.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "ctrlImage.h"

#include "WindowManager.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlText.
 *
 *  @author OLiver
 */
ctrlImage::ctrlImage(Window* parent,
                     unsigned int id,
                     unsigned short x,
                     unsigned short y,
                     glArchivItem_Bitmap* image,
                     const std::string& tooltip)
    : Window(x, y, id, parent),
      image(image), tooltip(tooltip)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
ctrlImage::~ctrlImage()
{
    WindowManager::inst().SetToolTip(this, "");
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeichnet das Fenster.
 *
 *  @author OLiver
 */
bool ctrlImage::Draw_()
{
    // gültiges Bild?
    if(image)
        image->Draw(GetX(), GetY(), 0, 0, 0, 0, 0, 0);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
bool ctrlImage::Msg_MouseMove(const MouseCoords& mc)
{
    // gültiges Bildz?
    if(image)
    {
        // Jeweils Tooltip ein- und ausblenden, wenn die Maus über dem Bild ist
        if(Coll(mc.x, mc.y, GetX() - image->getNx(), GetY() - image->getNy(), image->getWidth(), image->getHeight()))
            WindowManager::inst().SetToolTip(this, tooltip);
        else
            WindowManager::inst().SetToolTip(this, "");
    }

    return false;
}
