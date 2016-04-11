// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h" // IWYU pragma: keep
#include "iwWares.h"
#include "Loader.h"
#include "GameClient.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlVarText.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/JobConsts.h"
#include "gameData/ShieldConsts.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class glArchivItem_Bitmap;

//167, 416
iwWares::iwWares(unsigned int id, unsigned short x , unsigned short y,
                 const unsigned short width, const unsigned short height,
                 const std::string& title, unsigned char page_count,
                 bool allow_outhousing, glArchivItem_Font* font, const Inventory& inventory)
    : IngameWindow(id, x, y, width, height, title, LOADER.GetImageN("io", 5)),
      inventory(inventory), page(0), page_count(page_count)
{
    if(!font)
        font = SmallFont;

    // Zuordnungs-IDs
    const unsigned short INVENTORY_IDS[2][31] =
    {
        {
            // Waren
            22, 23, 24, 33,
            27, 18, 19, 32, 20,
            11,  0, 31, 30,
            29, 17, 28,  1,  3,
            4,  5,  2,  6,
            7,  8,  9, 12, 13,
            14, 16, GD_SHIELDROMANS, 15
        }, // GD_SHIELDROMANS = Völkerspezifisches Schild

        {
            // Figuren
            0, 19, 20,  1,
            3,  5,  2,  6,  4,
            7, 13, 14,  8,
            9, 10, 12, 11, 15,
            18, 16, 17, 27,
            26, 28, 29, JOB_CHARBURNER, 21,
            22, 23, 24, 25
        }, // 0xFFFF = unused
    };

    // Warenseite hinzufügen
    ctrlGroup* wares = AddGroup(100);
    // Figurenseite hinzufügen
    ctrlGroup* figures = AddGroup(101);

    GameClientPlayer& player = GAMECLIENT.GetLocalPlayer();
    bool four = true;
    unsigned short ware_id = 0;
    for(int x = 0, y = 0; y < 7; ++x, ++ware_id)
    {
        // 4er und 5er Block abwechselnd
        if(x >= (four ? 4 : 5))
        {
            x = 0;
            ++y;
            if(y == 7)
                break;

            four = !four;
        }


        // Hintergrundbutton oder -bild hinter Ware, nur beim Auslagern ein Button
        if(allow_outhousing)
        {
            ctrlButton* b = wares->AddImageButton(100 + INVENTORY_IDS[0][ware_id], (four ? 27 : 13) + x * 28, 21 + y * 42, 26, 26, TC_GREY, LOADER.GetMapImageN(2298), _(WARE_NAMES[INVENTORY_IDS[0][ware_id]]));
            b->SetBorder(false);
        }
        else
            wares->AddImage(100 + INVENTORY_IDS[0][ware_id], (four ? 27 : 13) + x * 28 + 13, 21 + y * 42 + 13, LOADER.GetMapImageN(2298), _(WARE_NAMES[INVENTORY_IDS[0][ware_id]]));

        if(INVENTORY_IDS[1][ware_id] != 0xFFFF)
        {
            if(allow_outhousing)
            {
                ctrlButton* b = figures->AddImageButton(100 + INVENTORY_IDS[1][ware_id], (four ? 27 : 13) + x * 28, 21 + y * 42, 26, 26, TC_GREY, LOADER.GetMapImageN(2298), _(JOB_NAMES[INVENTORY_IDS[1][ware_id]]));
                b->SetBorder(false);
            }
            else
            {

                figures->AddImage(100 + INVENTORY_IDS[1][ware_id], (four ? 27 : 13) + x * 28 + 13, 21 + y * 42 + 13, LOADER.GetMapImageN(2298), _(JOB_NAMES[INVENTORY_IDS[1][ware_id]]));
            }
        }

        // Hintergrundbild hinter Anzahl
        wares->AddImage(200 + INVENTORY_IDS[0][ware_id], (four ? 40 : 26) + x * 28, 53 + y * 42, LOADER.GetMapImageN(2299));
        if(INVENTORY_IDS[1][ware_id] != 0xFFFF)
            figures->AddImage(200 + INVENTORY_IDS[1][ware_id], (four ? 40 : 26) + x * 28, 53 + y * 42, LOADER.GetMapImageN(2299));

        // die jeweilige Ware
        wares->AddImage(300 + INVENTORY_IDS[0][ware_id], (four ? 40 : 26) + x * 28, 34 + y * 42, LOADER.GetMapImageN(2250 + (INVENTORY_IDS[0][ware_id] == GD_SHIELDROMANS ? SHIELD_TYPES[player.nation] : INVENTORY_IDS[0][ware_id])));
        if(INVENTORY_IDS[1][ware_id] != 0xFFFF)
        {
            glArchivItem_Bitmap* image;
            // Exception: charburner
            if(INVENTORY_IDS[1][ware_id] != JOB_CHARBURNER)
                image = LOADER.GetMapImageN(2300 + INVENTORY_IDS[1][ware_id]);
            else
                image = LOADER.GetImageN("io_new", 5);

            figures->AddImage(300 + INVENTORY_IDS[1][ware_id], (four ? 40 : 26) + x * 28, 34 + y * 42, image);
        }

        // Overlay für "Nicht Einlagern"

        ctrlImage* image = wares->AddImage(400 + INVENTORY_IDS[0][ware_id], (four ? 40 : 26) + x * 28, 30 + y * 42, LOADER.GetImageN("io", 222));
        image->SetVisible(false);
        if(INVENTORY_IDS[1][ware_id] != 0xFFFF)
        {
            image = figures->AddImage(400 + INVENTORY_IDS[1][ware_id], (four ? 40 : 26) + x * 28, 30 + y * 42, LOADER.GetImageN("io", 222));
            image->SetVisible(false);
        }

        // Overlay für "Auslagern"
        image = wares->AddImage(500 + INVENTORY_IDS[0][ware_id], (four ? 40 : 26) + x * 28, 44 + y * 42, LOADER.GetImageN("io", 221));
        image->SetVisible(false);
        if(INVENTORY_IDS[1][ware_id] != 0xFFFF)
        {
            image = figures->AddImage(500 + INVENTORY_IDS[1][ware_id], (four ? 40 : 26) + x * 28, 44 + y * 42, LOADER.GetImageN("io", 221));
            image->SetVisible(false);
        }

        // die jeweilige Anzahl (Texte)
        wares->AddVarText(600 + INVENTORY_IDS[0][ware_id], (four ? 53 : 39) + x * 28, 61 + y * 42, _("%d"), COLOR_YELLOW,
                          glArchivItem_Font::DF_BOTTOM | glArchivItem_Font::DF_RIGHT, font, 1,
                          &inventory.goods[INVENTORY_IDS[0][ware_id]]);
        if(INVENTORY_IDS[1][ware_id] != 0xFFFF)
            figures->AddVarText(600 + INVENTORY_IDS[1][ware_id], (four ? 53 : 39) + x * 28, 61 + y * 42, _("%d"), COLOR_YELLOW,
                                glArchivItem_Font::DF_BOTTOM | glArchivItem_Font::DF_RIGHT, font, 1,
                                &inventory.people[INVENTORY_IDS[1][ware_id]]);

        // Overlay für "Einlagern"
        image = wares->AddImage(700 + INVENTORY_IDS[0][ware_id], (four ? 40 : 26) + x * 28, 44 + y * 42, LOADER.GetImageN("io_new", 3));
        image->SetVisible(false);
        if(INVENTORY_IDS[1][ware_id] != 0xFFFF)
        {
            image = figures->AddImage(700 + INVENTORY_IDS[1][ware_id], (four ? 40 : 26) + x * 28, 44 + y * 42, LOADER.GetImageN("io_new", 3));
            image->SetVisible(false);
        }
    }

    wares->SetVisible(true);
    figures->SetVisible(false);

    // "Blättern"
    AddImageButton(0, 52, height - 47, 66, 32, TC_GREY, LOADER.GetImageN("io", 84), _("Next page"));
    // Hilfe
    AddImageButton(12,  16, height - 47, 32, 32, TC_GREY, LOADER.GetImageN("io", 21), _("Help"));
}

void iwWares::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // "Blättern"
        {
            SetPage( (page + 1) );
        } break;
    }
}

void iwWares::Msg_PaintBefore()
{
    // Farben ggf. aktualisieren

    // nur Seite 1 und 2 hat sowas.
    if(this->page >= 2)
        return;

    ctrlGroup* group = GetCtrl<ctrlGroup>(100 + page);
    if(group)
    {
        unsigned count = (page == 0) ? 36 : 32;

        for(unsigned int i = 0; i < count; ++i)
        {
            ctrlVarText* text = group->GetCtrl<ctrlVarText>(600 + i);
            if(text)
                text->SetColor( ((((page == 0) ? inventory.goods[i] : inventory.people[i]) == 0) ? COLOR_RED : COLOR_YELLOW) );

        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  bestimmte Inventurseite zeigen.
 *
 *  @param[in] page Die neue Seite
 *
 *  @author FloSoft
 */
void iwWares::SetPage(unsigned char page)
{
    // alte Page verstecken
    ctrlGroup* group = GetCtrl<ctrlGroup>(100 + this->page);
    if(group)
        group->SetVisible(false);

    // neue Page setzen
    this->page = page % page_count;

    // neue Page anzeigen
    group = GetCtrl<ctrlGroup>(100 + this->page);
    if(group)
        group->SetVisible(true);
}
