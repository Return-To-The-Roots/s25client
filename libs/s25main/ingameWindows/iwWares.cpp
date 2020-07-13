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

#include "iwWares.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlVarText.h"
#include "iwHelp.h"
#include "ogl/FontStyle.h"
#include "gameData/JobConsts.h"
#include "gameData/ShieldConsts.h"

// 167, 416
iwWares::iwWares(unsigned id, const DrawPoint& pos, const Extent& size, const std::string& title, bool allow_outhousing, const glFont* font,
                 const Inventory& inventory, const GamePlayer& player)
    : IngameWindow(id, pos, size, title, LOADER.GetImageN("io", 5)), inventory(inventory), player(player), curPage_(0), numPages(0)
{
    if(!font)
        font = SmallFont;

    // Zuordnungs-IDs
    const helpers::MultiArray<unsigned short, 2, 31> INVENTORY_IDS = {{
      {// Waren
       22, 23, 24, 33, 27, 18, 19, 32, 20, 11, 0, 31, 30, 29, 17, 28, 1, 3, 4, 5, 2, 6, 7, 8, 9, 12, 13, 14, 16, GD_SHIELDROMANS,
       15}, // GD_SHIELDROMANS = Völkerspezifisches Schild

      {// Figuren
       0,  19, 20, 1,  3, 5, 2, 6, 4, 7, 13, 14, 8, 9, 10, 12, 11, 15, 18, 16, 17, 27, 26, 28, 29, JOB_CHARBURNER,
       21, 22, 23, 24, 25}, // 0xFFFF = unused
    }};

    // Warenseite hinzufügen
    ctrlGroup& wares = AddPage();
    pageWares = wares.GetID() - 100;
    // Figurenseite hinzufügen
    ctrlGroup& figures = AddPage();
    pagePeople = figures.GetID() - 100;

    bool four = true;
    unsigned short ware_idx = 0;
    for(int x = 0, y = 0; y < 7; ++x, ++ware_idx)
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
        Extent btSize(26, 26);
        DrawPoint btPos((four ? btSize.x + 1 : btSize.x / 2) + x * 28, 21 + y * 42);
        if(allow_outhousing)
        {
            ctrlButton* b = wares.AddImageButton(100 + INVENTORY_IDS[0][ware_idx], btPos, btSize, TC_GREY, LOADER.GetMapImageN(2298),
                                                 _(WARE_NAMES[INVENTORY_IDS[0][ware_idx]]));
            b->SetBorder(false);
        } else
            wares.AddImage(100 + INVENTORY_IDS[0][ware_idx], btPos + btSize / 2, LOADER.GetMapImageN(2298),
                           _(WARE_NAMES[INVENTORY_IDS[0][ware_idx]]));

        if(INVENTORY_IDS[1][ware_idx] != 0xFFFF)
        {
            if(allow_outhousing)
            {
                ctrlButton* b = figures.AddImageButton(100 + INVENTORY_IDS[1][ware_idx], btPos, btSize, TC_GREY, LOADER.GetMapImageN(2298),
                                                       _(JOB_NAMES[INVENTORY_IDS[1][ware_idx]]));
                b->SetBorder(false);
            } else
            {
                figures.AddImage(100 + INVENTORY_IDS[1][ware_idx], btPos + btSize / 2, LOADER.GetMapImageN(2298),
                                 _(JOB_NAMES[INVENTORY_IDS[1][ware_idx]]));
            }
        }

        // Hintergrundbild hinter Anzahl
        DrawPoint bgCtPos = btPos + DrawPoint(btSize.x / 2, 32);
        wares.AddImage(200 + INVENTORY_IDS[0][ware_idx], bgCtPos, LOADER.GetMapImageN(2299));
        if(INVENTORY_IDS[1][ware_idx] != 0xFFFF)
            figures.AddImage(200 + INVENTORY_IDS[1][ware_idx], bgCtPos, LOADER.GetMapImageN(2299));

        // die jeweilige Ware
        DrawPoint warePos = btPos + btSize / 2;
        const GoodType ware =
          INVENTORY_IDS[0][ware_idx] == GD_SHIELDROMANS ? SHIELD_TYPES[player.nation] : GoodType(INVENTORY_IDS[0][ware_idx]);
        wares.AddImage(300 + INVENTORY_IDS[0][ware_idx], warePos, LOADER.GetMapImageN(2250 + ware));
        if(INVENTORY_IDS[1][ware_idx] != 0xFFFF)
        {
            glArchivItem_Bitmap* image;
            // Exception: charburner
            if(INVENTORY_IDS[1][ware_idx] != JOB_CHARBURNER)
                image = LOADER.GetMapImageN(2300 + INVENTORY_IDS[1][ware_idx]);
            else
                image = LOADER.GetImageN("io_new", 5);

            figures.AddImage(300 + INVENTORY_IDS[1][ware_idx], warePos, image);
        }

        // Overlay für "Nicht Einlagern"
        DrawPoint overlayPos = warePos - DrawPoint(0, 4);
        ctrlImage* image = wares.AddImage(400 + INVENTORY_IDS[0][ware_idx], overlayPos, LOADER.GetImageN("io", 222));
        image->SetVisible(false);
        if(INVENTORY_IDS[1][ware_idx] != 0xFFFF)
        {
            image = figures.AddImage(400 + INVENTORY_IDS[1][ware_idx], overlayPos, LOADER.GetImageN("io", 222));
            image->SetVisible(false);
        }

        // Overlay für "Auslagern"
        overlayPos = warePos + DrawPoint(0, 10);
        image = wares.AddImage(500 + INVENTORY_IDS[0][ware_idx], overlayPos, LOADER.GetImageN("io", 221));
        image->SetVisible(false);
        if(INVENTORY_IDS[1][ware_idx] != 0xFFFF)
        {
            image = figures.AddImage(500 + INVENTORY_IDS[1][ware_idx], overlayPos, LOADER.GetImageN("io", 221));
            image->SetVisible(false);
        }

        // Overlay für "Einlagern"
        image = wares.AddImage(700 + INVENTORY_IDS[0][ware_idx], overlayPos, LOADER.GetImageN("io_new", 3));
        image->SetVisible(false);
        if(INVENTORY_IDS[1][ware_idx] != 0xFFFF)
        {
            image = figures.AddImage(700 + INVENTORY_IDS[1][ware_idx], overlayPos, LOADER.GetImageN("io_new", 3));
            image->SetVisible(false);
        }

        // die jeweilige Anzahl (Texte)
        DrawPoint txtPos = btPos + DrawPoint(btSize.x, 40);
        wares.AddVarText(600 + INVENTORY_IDS[0][ware_idx], txtPos, _("%d"), COLOR_YELLOW, FontStyle::RIGHT | FontStyle::BOTTOM, font, 1,
                         &inventory.goods[INVENTORY_IDS[0][ware_idx]]);
        if(INVENTORY_IDS[1][ware_idx] != 0xFFFF)
            figures.AddVarText(600 + INVENTORY_IDS[1][ware_idx], txtPos, _("%d"), COLOR_YELLOW, FontStyle::RIGHT | FontStyle::BOTTOM, font,
                               1, &inventory.people[INVENTORY_IDS[1][ware_idx]]);
    }

    // "Blättern"
    AddImageButton(0, DrawPoint(52, GetSize().y - 47), Extent(66, 32), TC_GREY, LOADER.GetImageN("io", 84), _("Next page"));
    // Hilfe
    AddImageButton(12, DrawPoint(16, GetSize().y - 47), Extent(32, 32), TC_GREY, LOADER.GetImageN("io", 225), _("Help"));

    wares.SetVisible(true);
}

void iwWares::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // "Blättern"
        {
            SetPage(curPage_ + 1);
        }
        break;
        case 12: // Hilfe
        {
            WINDOWMANAGER.ReplaceWindow(std::make_unique<iwHelp>(_("Here you will find a list of your entire stores of "
                                                                   "merchandise and all the inhabitants of your realm.")));
        }
        break;
    }
}

void iwWares::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();

    // Farben ggf. aktualisieren

    if(curPage_ != pagePeople && curPage_ != pageWares)
        return;

    auto* group = GetCtrl<ctrlGroup>(100 + curPage_);
    if(group)
    {
        unsigned count = (curPage_ == pageWares) ? 36 : 32;

        for(unsigned i = 0; i < count; ++i)
        {
            auto* text = group->GetCtrl<ctrlVarText>(600 + i);
            if(text)
                text->SetTextColor((((curPage_ == pageWares) ? inventory.goods[i] : inventory.people[i]) == 0) ? COLOR_RED : COLOR_YELLOW);
        }
    }
}

/**
 *  bestimmte Inventurseite zeigen.
 *
 *  @param[in] page Die neue Seite
 */
void iwWares::SetPage(unsigned page)
{
    // alte Page verstecken
    auto* group = GetCtrl<ctrlGroup>(100 + curPage_);
    if(group)
        group->SetVisible(false);

    // neue Page setzen
    curPage_ = page % numPages;

    // neue Page anzeigen
    group = GetCtrl<ctrlGroup>(100 + curPage_);
    if(group)
        group->SetVisible(true);
}

ctrlGroup& iwWares::AddPage()
{
    ctrlGroup& grp = *AddGroup(100 + numPages);
    numPages++;
    grp.SetVisible(false);
    return grp;
}
