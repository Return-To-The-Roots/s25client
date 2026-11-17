// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwBuilding.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "WindowManager.h"
#include "buildings/nobShipYard.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlPercent.h"
#include "controls/ctrlText.h"
#include "factories/GameCommandFactory.h"
#include "helpers/containerUtils.h"
#include "iwDemolishBuilding.h"
#include "iwHelp.h"
#include "iwTempleBuilding.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glFont.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/const_gui_ids.h"
#include <sstream>

/// IDs in der IO_DAT von Boot und Schiffs-Bild für den Umschaltebutton beim Schiffsbauer
const unsigned IODAT_BOAT_ID = 219;
const unsigned IODAT_SHIP_ID = 218;

iwBuilding::iwBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobUsual* const building, Extent extent)
    : IngameWindow(CGI_BUILDING + MapBase::createGUIID(building->getPos()), IngameWindow::posAtMouse, extent,
                   _(BUILDING_NAMES[building->getBuildingType()]), LOADER.getImageN("resource", 41)),
      gwv(gwv), gcFactory(gcFactory), building(building), productivity(building->getProductivity())
{
    // Arbeitersymbol
    addImage(0, DrawPoint(28, 39), LOADER.getMapTexture(2298));

    if(const auto job = BLD_WORK_DESC[building->getBuildingType()].job)
        addImage(13, DrawPoint(28, 39), LOADER.getJobTex(*job));

    // Gebäudesymbol
    addImage(1, DrawPoint(117, 114), &building->getBuildingImage());

    // Symbol der produzierten Ware (falls hier was produziert wird)
    const auto& producedWare = BLD_WORK_DESC[building->getBuildingType()].producedWare;
    ITexture* tex = visit(
      composeVisitor([](GoodType gt) -> ITexture* { return gt != GoodType::Nothing ? LOADER.getWareTex(gt) : nullptr; },
                     [](Job job) -> ITexture* { return LOADER.getJobTex(job); },
                     [](std::nullopt_t) -> ITexture* { return nullptr; }),
      producedWare);
    if(tex)
    {
        addImage(2, DrawPoint(196, 39), LOADER.getMapTexture(2298));
        addImage(3, DrawPoint(196, 39), tex);
    }

    // Info
    addImageButton(4, DrawPoint(16, extent.y - 47), Extent(30, 32), TextureColor::Grey, LOADER.getImageN("io", 225),
                   _("Help"));
    // Abreißen
    addImageButton(5, DrawPoint(50, extent.y - 47), Extent(34, 32), TextureColor::Grey, LOADER.getImageN("io", 23),
                   _("Demolish house"));
    // Produktivität einstellen (196,197) (bei Spähturm ausblenden)
    Window* enable_productivity = addImageButton(
      6, DrawPoint(90, extent.y - 47), Extent(34, 32), TextureColor::Grey,
      LOADER.getImageN("io", ((building->isProductionDisabledVirtual()) ? 197 : 196)), _("Production on/off"));
    if(building->getBuildingType() == BuildingType::LookoutTower)
        enable_productivity->setVisible(false);
    // Bei Bootsbauer Button zum Umwählen von Booten und Schiffen
    if(building->getBuildingType() == BuildingType::Shipyard)
    {
        // Jenachdem Boot oder Schiff anzeigen
        unsigned ioDatId =
          (static_cast<nobShipYard*>(building)->getMode() == nobShipYard::Mode::Boats) ? IODAT_BOAT_ID : IODAT_SHIP_ID;
        addImageButton(11, DrawPoint(130, extent.y - 47), Extent(43, 32), TextureColor::Grey,
                       LOADER.getImageN("io", ioDatId), _("Ships/Boats"));
    }

    // "Gehe Zum Ort"
    addImageButton(7, DrawPoint(179, extent.y - 47), Extent(30, 32), TextureColor::Grey, LOADER.getImageN("io", 107),
                   _("Go to place"));

    // Produktivitätsanzeige (bei Katapulten und Spähtürmen ausblenden)
    Window* productivity =
      addPercent(9, DrawPoint(59, 31), Extent(106, 16), TextureColor::Grey, 0xFFFFFF00, SmallFont, &this->productivity);
    if(building->getBuildingType() == BuildingType::Catapult
       || building->getBuildingType() == BuildingType::LookoutTower)
        productivity->setVisible(false);

    addText(10, DrawPoint(113, 50), _("(House unoccupied)"), COLOR_RED, FontStyle::CENTER, NormalFont);

    // "Go to next" (building of same type)
    addImageButton(12, DrawPoint(179, extent.y - 79), Extent(30, 32), TextureColor::Grey,
                   LOADER.getImageN("io_new", 11), _("Go to next building of same type"));
}

void iwBuilding::msgPaintBefore()
{
    IngameWindow::msgPaintBefore();
    productivity = building->getProductivity();

    // Haus unbesetzt ggf ausblenden
    getCtrl<ctrlText>(10)->setVisible(!building->hasWorker());
}

void iwBuilding::msgPaintAfter()
{
    IngameWindow::msgPaintAfter();
    const auto& bldWorkDesk = BLD_WORK_DESC[building->getBuildingType()];
    if(BuildingProperties::isMine(building->getBuildingType()))
    {
        // Bei Bergwerken sieht die Nahrungsanzeige ein wenig anders aus (3x 2)

        // "Schwarzer Rahmen"
        drawRectangle(Rect(getDrawPos() + DrawPoint(40, 60), Extent(144, 24)), 0x80000000);
        DrawPoint curPos = getDrawPos() + DrawPoint(52, 72);
        for(unsigned char i = 0; i < bldWorkDesk.waresNeeded.size(); ++i)
        {
            for(unsigned char z = 0; z < bldWorkDesk.numSpacesPerWare; ++z)
            {
                LOADER.getWareTex(bldWorkDesk.waresNeeded[i])
                  ->drawFull(curPos, (z < building->getNumWares(i) ? 0xFFFFFFFF : 0xFF404040));
                curPos.x += 24;
            }
        }
    } else
    {
        DrawPoint curPos = getDrawPos() + DrawPoint(getSize().x / 2, 60);
        for(unsigned char i = 0; i < bldWorkDesk.waresNeeded.size(); ++i)
        {
            const unsigned waresCount = bldWorkDesk.numSpacesPerWare;

            // "Schwarzer Rahmen"
            DrawPoint waresPos = curPos - DrawPoint(24 * waresCount / 2, 0);
            drawRectangle(Rect(waresPos, Extent(24 * waresCount, 24)), 0x80000000);
            waresPos += DrawPoint(12, 12);

            for(unsigned char z = 0; z < waresCount; ++z)
            {
                LOADER.getWareTex(bldWorkDesk.waresNeeded[i])
                  ->drawFull(waresPos, (z < building->getNumWares(i) ? COLOR_WHITE : 0xFF404040));
                waresPos.x += 24;
            }

            std::stringstream text;
            text << (unsigned)building->getNumWares(i) << "/" << waresCount;
            NormalFont->draw(curPos + DrawPoint(0, 12), text.str(), FontStyle::CENTER | FontStyle::VCENTER);
            curPos.y += 29;
        }
    }
}

void iwBuilding::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 4: // Hilfe
        {
            WINDOWMANAGER.replaceWindow(
              std::make_unique<iwHelp>(_(BUILDING_HELP_STRINGS[building->getBuildingType()])));
        }
        break;
        case 5: // Gebäude abbrennen
        {
            // Abreißen?
            close();
            WINDOWMANAGER.show(std::make_unique<iwDemolishBuilding>(gwv, building));
        }
        break;
        case 6:
        {
            // Produktion einstellen/fortführen
            // NC senden
            if(gcFactory.SetProductionEnabled(building->getPos(), building->isProductionDisabledVirtual()))
            {
                // visuell anzeigen, falls erfolgreich
                building->toggleProductionVirtual();

                // anderes Bild auf dem Button
                if(building->isProductionDisabledVirtual())
                    getCtrl<ctrlImageButton>(6)->setImage(LOADER.getImageN("io", 197));
                else
                    getCtrl<ctrlImageButton>(6)->setImage(LOADER.getImageN("io", 196));

                auto* text = getCtrl<ctrlText>(10);
                if(building->isProductionDisabledVirtual() && building->hasWorker())
                    text->setText(_("(House unoccupied)"));
                else if(building->hasWorker())
                    text->setVisible(false);
            }
        }
        break;
        case 7: // "Gehe Zum Ort"
        {
            gwv.moveToMapPt(building->getPos());
        }
        break;
        case 11: // Schiff/Boot umstellen bei Schiffsbauer
        {
            if(gcFactory.SetShipYardMode(building->getPos(), static_cast<const nobShipYard*>(building)->getMode()
                                                               == nobShipYard::Mode::Boats))
            {
                // Auch optisch den Button umstellen
                auto* button = getCtrl<ctrlImageButton>(11);
                if(button->getImage() == LOADER.getImageN("io", IODAT_BOAT_ID))
                    button->setImage(LOADER.getImageN("io", IODAT_SHIP_ID));
                else
                    button->setImage(LOADER.getImageN("io", IODAT_BOAT_ID));
            }
        }
        break;
        case 12: // go to next of same type
        {
            const std::list<nobUsual*>& buildings = gwv.getWorld()
                                                      .getPlayer(building->getPlayer())
                                                      .getBuildingRegister()
                                                      .getBuildings(building->getBuildingType());
            // go through list once we get to current building -> open window for the next one and go to next location
            auto it = helpers::findIf(buildings,
                                      [bldPos = building->getPos()](const auto* it) { return it->getPos() == bldPos; });
            if(it != buildings.end()) // got to current building in the list?
            {
                // close old window, open new window (todo: only open if it isnt already open), move to location of next
                // building
                close();
                ++it;
                if(it == buildings.end()) // was last entry in list -> goto first
                    it = buildings.begin();
                gwv.moveToMapPt((*it)->getPos());
                if(building->getBuildingType() == BuildingType::Temple)
                    WINDOWMANAGER.replaceWindow(std::make_unique<iwTempleBuilding>(gwv, gcFactory, *it))
                      .setPos(getPos());
                else
                    WINDOWMANAGER.replaceWindow(std::make_unique<iwBuilding>(gwv, gcFactory, *it)).setPos(getPos());
                break;
            }
        }
        break;
    }
}
