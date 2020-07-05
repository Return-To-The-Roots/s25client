// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "nobUsual.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "addons/const_addons.h"
#include "figures/nofBuildingWorker.h"
#include "figures/nofPigbreeder.h"
#include "helpers/containerUtils.h"
#include "network/GameClient.h"
#include "notifications/BuildingNote.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "world/GameWorldGame.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include <numeric>

nobUsual::nobUsual(BuildingType type, MapPoint pos, unsigned char player, Nation nation)
    : noBuilding(type, pos, player, nation), worker(nullptr), disable_production(false), disable_production_virtual(false),
      last_ordered_ware(0), orderware_ev(nullptr), productivity_ev(nullptr), numGfNotWorking(0), since_not_working(0xFFFFFFFF),
      outOfRessourcesMsgSent(false), is_working(false)
{
    std::fill(numWares.begin(), numWares.end(), 0);

    ordered_wares.resize(BLD_WORK_DESC[bldType_].waresNeeded.getNum());

    // Tür aufmachen,bis Gebäude besetzt ist
    OpenDoor();

    GamePlayer& owner = gwg->GetPlayer(player);
    // New building gets half the average productivity from all buildings of the same type
    productivity = owner.GetBuildingRegister().CalcAverageProductivity(type) / 2u;
    // Set last productivities to current to avoid resetting it on first recalculation event
    std::fill(last_productivities.begin(), last_productivities.end(), productivity);
}

nobUsual::nobUsual(SerializedGameData& sgd, const unsigned obj_id)
    : noBuilding(sgd, obj_id), worker(sgd.PopObject<nofBuildingWorker>(GOT_UNKNOWN)), productivity(sgd.PopUnsignedShort()),
      disable_production(sgd.PopBool()), disable_production_virtual(disable_production), last_ordered_ware(sgd.PopUnsignedChar()),
      orderware_ev(sgd.PopEvent()), productivity_ev(sgd.PopEvent()), numGfNotWorking(sgd.PopUnsignedShort()),
      since_not_working(sgd.PopUnsignedInt()), outOfRessourcesMsgSent(sgd.PopBool()), is_working(sgd.PopBool())
{
    for(unsigned i = 0; i < 3; ++i)
        numWares[i] = sgd.PopUnsignedChar();

    ordered_wares.resize(BLD_WORK_DESC[bldType_].waresNeeded.getNum());

    for(std::list<Ware*>& orderedWare : ordered_wares)
        sgd.PopObjectContainer(orderedWare, GOT_WARE);
    for(unsigned short& last_productivitie : last_productivities)
        last_productivitie = sgd.PopUnsignedShort();
}

void nobUsual::Serialize_nobUsual(SerializedGameData& sgd) const
{
    Serialize_noBuilding(sgd);

    sgd.PushObject(worker, false);
    sgd.PushUnsignedShort(productivity);
    sgd.PushBool(disable_production);
    sgd.PushUnsignedChar(last_ordered_ware);
    sgd.PushEvent(orderware_ev);
    sgd.PushEvent(productivity_ev);
    sgd.PushUnsignedShort(numGfNotWorking);
    sgd.PushUnsignedInt(since_not_working);
    sgd.PushBool(outOfRessourcesMsgSent);
    sgd.PushBool(is_working);

    for(unsigned i = 0; i < 3; ++i)
        sgd.PushUnsignedChar(numWares[i]);
    for(const std::list<Ware*>& orderedWare : ordered_wares)
        sgd.PushObjectContainer(orderedWare, true);
    for(unsigned short last_productivitie : last_productivities)
        sgd.PushUnsignedShort(last_productivitie);
}

nobUsual::~nobUsual() = default;

void nobUsual::DestroyBuilding()
{
    // Arbeiter Bescheid sagen
    if(worker)
    {
        worker->LostWork();
        worker = nullptr;
    } else
        gwg->GetPlayer(player).JobNotWanted(this);

    // Bestellte Waren Bescheid sagen
    for(std::list<Ware*>& orderedWare : ordered_wares)
    {
        for(Ware* ware : orderedWare)
            WareNotNeeded(ware);
        orderedWare.clear();
    }

    // Events löschen
    GetEvMgr().RemoveEvent(orderware_ev);
    GetEvMgr().RemoveEvent(productivity_ev);

    // Inventur entsprechend verringern wegen den Waren, die vernichtetet werden
    for(unsigned i = 0; i < BLD_WORK_DESC[bldType_].waresNeeded.size(); ++i)
    {
        GoodType ware = BLD_WORK_DESC[bldType_].waresNeeded[i];
        if(ware == GD_NOTHING)
            break;
        gwg->GetPlayer(player).DecreaseInventoryWare(ware, numWares[i]);
    }
}

void nobUsual::Draw(DrawPoint drawPt)
{
    // Gebäude an sich zeichnen
    DrawBaseBuilding(drawPt);

    // Wenn Produktion gestoppt ist, Schild außen am Gebäude zeichnen zeichnen
    if(disable_production_virtual)
        LOADER.GetMapImageN(46)->DrawFull(drawPt + BUILDING_SIGN_CONSTS[nation][bldType_]);

    // Rauch zeichnen

    // Raucht dieses Gebäude und ist es in Betrieb? (nur arbeitende Gebäude rauchen schließlich)
    if(is_working && BUILDING_SMOKE_CONSTS[nation][bldType_].type)
    {
        // Dann Qualm zeichnen (damit Qualm nicht synchron ist, x- und y- Koordinate als Unterscheidung
        LOADER
          .GetMapImageN(692 + BUILDING_SMOKE_CONSTS[nation][bldType_].type * 8
                        + GAMECLIENT.GetGlobalAnimation(8, 5, 2, (GetX() + GetY()) * 100))
          ->DrawFull(drawPt + BUILDING_SMOKE_CONSTS[nation][bldType_].offset, 0x99EEEEEE);
    }

    // TODO: zusätzliche Dinge wie Mühlenräder, Schweinchen etc bei bestimmten Gebäuden zeichnen

    // Bei Mühle, wenn sie nicht arbeitet, immer Mühlenräder (nichtdrehend) zeichnen
    if(bldType_ == BLD_MILL && !is_working)
    {
        // Flügel der Mühle
        LOADER.GetNationImage(nation, 250 + 5 * 49)->DrawFull(drawPt);
        // Schatten der Flügel
        LOADER.GetNationImage(nation, 250 + 5 * 49 + 1)->DrawFull(drawPt, COLOR_SHADOW);
    }
    // Esel in den Kammer bei Eselzucht zeichnen
    else if(bldType_ == BLD_DONKEYBREEDER)
    {
        // Für alle Völker jeweils
        // X-Position der Esel
        const helpers::MultiArray<DrawPoint, NUM_NATIONS, 3> DONKEY_OFFSETS = {{{{13, -9}, {26, -9}, {39, -9}},
                                                                                {{3, -17}, {16, -17}, {30, -17}},
                                                                                {{2, -21}, {15, -21}, {29, -21}},
                                                                                {{7, -17}, {18, -17}, {30, -17}},
                                                                                {{3, -22}, {16, -22}, {30, -22}}}};
        // Animations-IDS des Esels
        const std::array<unsigned char, 25> DONKEY_ANIMATION = {
          {0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 6, 5, 4, 4, 5, 6, 5, 7, 6, 5, 4, 3, 2, 1, 0}};

        // Die drei Esel zeichnen mithilfe von Globalanimation
        // Anzahl hängt von Produktivität der Eselzucht ab:
        // 0-29 - kein Esel
        // 30-60 - 1 Esel
        // 60-90 - 2 Esel
        // 90-100 - 3 Esel
        if(productivity >= 30)
            LOADER
              .GetMapImageN(2180 + DONKEY_ANIMATION[GAMECLIENT.GetGlobalAnimation(DONKEY_ANIMATION.size(), 5, 2, GetX() * (player + 2))])
              ->DrawFull(drawPt + DONKEY_OFFSETS[nation][0]);
        if(productivity >= 60)
            LOADER.GetMapImageN(2180 + DONKEY_ANIMATION[GAMECLIENT.GetGlobalAnimation(DONKEY_ANIMATION.size(), 5, 2, GetY())])
              ->DrawFull(drawPt + DONKEY_OFFSETS[nation][1]);
        if(productivity >= 90)
            LOADER
              .GetMapImageN(
                2180 + DONKEY_ANIMATION[GAMECLIENT.GetGlobalAnimation(DONKEY_ANIMATION.size(), 5, 2, GetX() + GetY() * (nation + 1))])
              ->DrawFull(drawPt + DONKEY_OFFSETS[nation][2]);
    }
    // Bei Katapulthaus Katapult oben auf dem Dach zeichnen, falls er nicht "arbeitet"
    else if(bldType_ == BLD_CATAPULT && !is_working)
    {
        LOADER.GetPlayerImage("rom_bobs", 1776)->DrawFull(drawPt - DrawPoint(7, 19));
    }

    // Bei Schweinefarm Schweinchen auf dem Hof zeichnen
    else if(bldType_ == BLD_PIGFARM && this->HasWorker())
    {
        // Position der 5 Schweinchen für alle 4 Völker (1. ist das große Schwein)
        const helpers::MultiArray<DrawPoint, NUM_NATIONS, 5> PIG_POSITIONS = {{
          //  gr. S. 1.klS 2. klS usw
          {{3, -8}, {17, 3}, {-12, 4}, {-2, 10}, {-22, 11}},    // Afrikaner
          {{-16, 0}, {-37, 0}, {-32, 8}, {-16, 10}, {-22, 18}}, // Japaner
          {{-15, 0}, {-4, 9}, {-22, 10}, {2, 19}, {-15, 20}},   // Römer
          {{5, -5}, {25, -12}, {-7, 7}, {-23, 11}, {-10, 14}},  // Wikinger
          {{-16, 5}, {-37, 5}, {-32, -1}, {-16, 15}, {-27, 18}} // Babylonier
        }};

        /// Großes Schwein zeichnen
        LOADER.GetMapImageN(2160)->DrawFull(drawPt + PIG_POSITIONS[nation][0], COLOR_SHADOW);
        LOADER.GetMapImageN(2100 + GAMECLIENT.GetGlobalAnimation(12, 3, 1, GetX() + GetY() + GetObjId()))
          ->DrawFull(drawPt + PIG_POSITIONS[nation][0]);

        // Die 4 kleinen Schweinchen, je nach Produktivität
        for(unsigned i = 1; i < std::min(unsigned(productivity) / 20u + 1u, 5u); ++i)
        {
            // A random (really, dice-rolled by hand:) ) order of the four possible pig animations, with eating three times as much as the
            // others ones  To get random-looking, non synchronous, sweet little pigs
            const std::array<unsigned char, 63> smallpig_animations = {0, 0, 3, 2, 0, 0, 1, 3, 0, 3, 1, 3, 2, 0, 0, 1, 0, 0, 1, 3, 2,
                                                                       0, 1, 1, 0, 0, 2, 1, 0, 1, 0, 2, 2, 0, 0, 2, 2, 0, 1, 0, 3, 1,
                                                                       2, 0, 1, 2, 2, 0, 0, 0, 3, 0, 2, 0, 3, 0, 3, 0, 1, 1, 0, 3, 0};
            const unsigned short animpos =
              GAMECLIENT.GetGlobalAnimation(63 * 12, 63 * 4 - i * 5, 1, 183 * i + GetX() * GetObjId() + GetY() * i);
            LOADER.GetMapImageN(2160)->DrawFull(drawPt + PIG_POSITIONS[nation][i], COLOR_SHADOW);
            LOADER.GetMapImageN(2112 + smallpig_animations[animpos / 12] * 12 + animpos % 12)->DrawFull(drawPt + PIG_POSITIONS[nation][i]);
        }

        // Ggf. Sounds abspielen (oink oink), da soll sich der Schweinezüchter drum kümmen
        dynamic_cast<nofPigbreeder*>(worker)->MakePigSounds(); //-V522
    }
    // Bei nubischen Bergwerken das Feuer vor dem Bergwerk zeichnen
    else if(BuildingProperties::IsMine(GetBuildingType()) && worker && nation == NAT_AFRICANS)
        LOADER.GetMapPlayerImage(740 + GAMECLIENT.GetGlobalAnimation(8, 5, 2, GetObjId() + GetX() + GetY()))
          ->DrawFull(drawPt + NUBIAN_MINE_FIRE[bldType_ - BLD_GRANITEMINE]);
}

void nobUsual::HandleEvent(const unsigned id)
{
    if(id)
    {
        const unsigned short current_productivity = CalcProductivity();
        // Sum over all last productivities and current (as start value)
        productivity = std::accumulate(last_productivities.begin(), last_productivities.end(), current_productivity);
        // Produktivität "verrücken"
        for(unsigned short i = last_productivities.size() - 1; i >= 1; --i)
            last_productivities[i] = last_productivities[i - 1];
        last_productivities[0] = current_productivity;

        // Durschnitt ausrechnen der letzten Produktivitäten PLUS der aktuellen!
        productivity /= (last_productivities.size() + 1);

        // Event für nächste Abrechnung
        productivity_ev = GetEvMgr().AddEvent(this, 400, 1);
    } else
    {
        // Ware bestellen (falls noch Platz ist) und nicht an Betriebe, die stillgelegt wurden!
        if(!disable_production)
        {
            const BldWorkDescription& workDesc = BLD_WORK_DESC[bldType_];
            RTTR_Assert(workDesc.waresNeeded[last_ordered_ware] != GD_NOTHING);
            // How many wares can we have of each type?
            unsigned wareSpaces = workDesc.numSpacesPerWare;

            if(numWares[last_ordered_ware] + ordered_wares[last_ordered_ware].size() < wareSpaces)
            {
                Ware* w = gwg->GetPlayer(player).OrderWare(workDesc.waresNeeded[last_ordered_ware], this);
                if(w)
                    RTTR_Assert(helpers::contains(ordered_wares[last_ordered_ware], w));
            }

            ++last_ordered_ware;
            if(last_ordered_ware >= workDesc.waresNeeded.size() || workDesc.waresNeeded[last_ordered_ware] == GD_NOTHING)
                last_ordered_ware = 0;
        }

        // Nach ner bestimmten Zeit dann nächste Ware holen
        orderware_ev = GetEvMgr().AddEvent(this, 210);
    }
}

void nobUsual::AddWare(Ware*& ware)
{
    // Gucken, um was für einen Warentyp es sich handelt und dann dort hinzufügen
    const BldWorkDescription& workDesc = BLD_WORK_DESC[bldType_];
    for(unsigned char i = 0; i < workDesc.waresNeeded.size(); ++i)
    {
        if(ware->type == workDesc.waresNeeded[i])
        {
            ++numWares[i];
            RTTR_Assert(helpers::contains(ordered_wares[i], ware));
            ordered_wares[i].remove(ware);
            break;
        }
    }

    // Ware vernichten
    gwg->GetPlayer(player).RemoveWare(ware);
    deletePtr(ware);

    // Arbeiter Bescheid sagen, dass es neue Waren gibt
    if(worker)
        worker->GotWareOrProductionAllowed();
}

bool nobUsual::FreePlaceAtFlag()
{
    // Arbeiter Bescheid sagen, falls es noch keinen gibt, brauch keine Ware rausgetragen werden
    if(worker)
        return worker->FreePlaceAtFlag();
    else
        return false;
}

void nobUsual::WareLost(Ware* ware)
{
    // Ware konnte nicht kommen --> raus damit
    const BldWorkDescription& workDesc = BLD_WORK_DESC[bldType_];
    for(unsigned char i = 0; i < workDesc.waresNeeded.size(); ++i)
    {
        if(ware->type == workDesc.waresNeeded[i])
        {
            RTTR_Assert(helpers::contains(ordered_wares[i], ware));
            ordered_wares[i].remove(ware);
            break;
        }
    }
}

void nobUsual::GotWorker(Job /*job*/, noFigure* worker)
{
    this->worker = static_cast<nofBuildingWorker*>(worker);

    if(BLD_WORK_DESC[bldType_].waresNeeded[0] != GD_NOTHING)
        // erste Ware bestellen
        HandleEvent(0);
}

void nobUsual::WorkerLost()
{
    // Check if worker is or was here (e.g. hunter could currently be outside)
    if(HasWorker())
    {
        // If we have a worker, we must be producing something
        RTTR_Assert(productivity_ev);
        // Open the door till we get a new worker
        OpenDoor();
    }
    // Produktivitätsevent ggf. abmelden
    GetEvMgr().RemoveEvent(productivity_ev);

    // Waren-Bestell-Event abmelden
    GetEvMgr().RemoveEvent(orderware_ev);

    // neuen Arbeiter bestellen
    worker = nullptr;
    gwg->GetPlayer(player).AddJobWanted(BLD_WORK_DESC[bldType_].job.value(), this);
}

bool nobUsual::WaresAvailable()
{
    const BldWorkDescription& workDesc = BLD_WORK_DESC[bldType_];
    if(workDesc.useOneWareEach)
    {
        // Any ware not there -> false, else true
        for(unsigned char i = 0; i < workDesc.waresNeeded.size(); ++i)
        {
            if(workDesc.waresNeeded[i] == GD_NOTHING)
                break;
            if(numWares[i] == 0)
                return false;
        }
        return true;
    } else
    {
        // Any ware there -> true else false
        for(unsigned char i = 0; i < workDesc.waresNeeded.size(); ++i)
        {
            if(workDesc.waresNeeded[i] == GD_NOTHING)
                break;
            if(numWares[i] != 0)
                return true;
        }
        return false;
    }
}

void nobUsual::ConsumeWares()
{
    const BldWorkDescription& workDesc = BLD_WORK_DESC[bldType_];
    unsigned numWaresNeeded = workDesc.waresNeeded.getNum();
    if(numWaresNeeded == 0)
        return;

    // Set to first ware (default)
    unsigned wareIdxToUse = 0;
    if(!workDesc.useOneWareEach)
    {
        // Use only 1 ware -> Get the one with the most in store
        unsigned numBestWare = 0;
        for(unsigned i = 0; i < numWaresNeeded; ++i)
        {
            if(numWares[i] > numBestWare)
            {
                wareIdxToUse = i;
                numBestWare = numWares[i];
            }
        }
        // And tell that we only consume 1
        numWaresNeeded = 1;
    }

    GamePlayer& owner = gwg->GetPlayer(player);
    for(unsigned i = 0; i < numWaresNeeded; i++)
    {
        RTTR_Assert(numWares[wareIdxToUse] != 0);
        // Bestand verringern
        --numWares[wareIdxToUse];
        // Inventur entsprechend verringern
        owner.DecreaseInventoryWare(workDesc.waresNeeded[wareIdxToUse], 1);

        // try to get ware from warehouses
        if(numWares[wareIdxToUse] < 2)
        {
            Ware* w = gwg->GetPlayer(player).OrderWare(workDesc.waresNeeded[wareIdxToUse], this);
            if(w)
                RTTR_Assert(helpers::contains(ordered_wares[wareIdxToUse], w));
        }
        // Set to value of next iteration. Note: It might have been not 0 for useOneWareEach == false
        wareIdxToUse = i + 1;
    }
}

unsigned nobUsual::CalcDistributionPoints(noRoadNode* /*start*/, const GoodType type)
{
    // No production -> nothing needed
    if(disable_production)
        return 0;

    const BldWorkDescription& workDesc = BLD_WORK_DESC[bldType_];
    // Warentyp ermitteln
    unsigned id;
    for(id = 0; id < workDesc.waresNeeded.size(); ++id)
    {
        if(workDesc.waresNeeded[id] == type)
            break;
    }

    // Don't need this ware
    if(id == workDesc.waresNeeded.size())
        return 0;

    // Got enough? -> Don't request more
    if(numWares[id] + ordered_wares[id].size() == workDesc.numSpacesPerWare)
        return 0;

    // 10000 as base points then subtract some
    // Note: Subtracted points must be at most this.
    unsigned points = 10000;

    // Every ware we have or is on the way reduces rating
    // Note: maxValue is numSpacesPerWare * 30 which is <= 60*30=1800 (< 10000 base value)
    points -= (numWares[id] + ordered_wares[id].size()) * 30;

    RTTR_Assert(points <= 10000);

    return points;
}

void nobUsual::TakeWare(Ware* ware)
{
    // Ware in die Bestellliste aufnehmen
    const BldWorkDescription& workDesc = BLD_WORK_DESC[bldType_];
    for(unsigned char i = 0; i < workDesc.waresNeeded.size(); ++i)
    {
        if(ware->type == workDesc.waresNeeded[i])
        {
            RTTR_Assert(!helpers::contains(ordered_wares[i], ware));
            ordered_wares[i].push_back(ware);
            return;
        }
    }
}

void nobUsual::WorkerArrived()
{
    // Produktivität in 400 gf ausrechnen
    productivity_ev = GetEvMgr().AddEvent(this, 400, 1);
}

void nobUsual::SetProductionEnabled(const bool enabled)
{
    if(disable_production == !enabled)
        return;
    // Umstellen
    disable_production = !enabled;
    // Wenn das von einem fremden Spieler umgestellt wurde (oder vom Replay), muss auch das visuelle umgestellt werden
    if(GAMECLIENT.GetPlayerId() != player || GAMECLIENT.IsReplayModeOn())
        disable_production_virtual = disable_production;

    if(disable_production)
    {
        // Wenn sie deaktiviert wurde, dem Arbeiter Bescheid sagen, damit er entsprechend stoppt, falls er schon
        // auf die Arbeit warteet
        if(worker)
            worker->ProductionStopped();
    } else
    {
        // Wenn sie wieder aktiviert wurde, evtl wieder mit arbeiten anfangen, falls es einen Arbeiter gibt
        if(worker)
            worker->GotWareOrProductionAllowed();
    }
}

bool nobUsual::HasWorker() const
{
    return worker && worker->GetState() != nofBuildingWorker::STATE_FIGUREWORK;
}

void nobUsual::OnOutOfResources()
{
    // Post verschicken, keine Rohstoffe mehr da
    if(outOfRessourcesMsgSent)
        return;
    outOfRessourcesMsgSent = true;
    productivity = 0;
    std::fill(last_productivities.begin(), last_productivities.end(), 0);

    const char* error;
    if(GetBuildingType() == BLD_WELL)
        error = _("This well has dried out");
    else if(BuildingProperties::IsMine(GetBuildingType()))
        error = _("This mine is exhausted");
    else if(GetBuildingType() == BLD_QUARRY)
        error = _("No more stones in range");
    else if(GetBuildingType() == BLD_FISHERY)
        error = _("No more fishes in range");
    else
        return;

    SendPostMessage(player, new PostMsgWithBuilding(GetEvMgr().GetCurrentGF(), error, PostCategory::Economy, *this));
    gwg->GetNotifications().publish(BuildingNote(BuildingNote::NoRessources, player, GetPos(), GetBuildingType()));

    if(GAMECLIENT.GetPlayerId() == player && gwg->GetGGS().isEnabled(AddonId::DEMOLISH_BLD_WO_RES))
    {
        GAMECLIENT.DestroyBuilding(GetPos());
    }
}

void nobUsual::StartNotWorking()
{
    // Wenn noch kein Zeitpunkt festgesetzt wurde, jetzt merken
    if(since_not_working == 0xFFFFFFFF)
        since_not_working = GetEvMgr().GetCurrentGF();
}

void nobUsual::StopNotWorking()
{
    // Falls wir vorher nicht gearbeitet haben, diese Zeit merken für die Produktivität
    if(since_not_working != 0xFFFFFFFF)
    {
        numGfNotWorking += static_cast<unsigned short>(GetEvMgr().GetCurrentGF() - since_not_working);
        since_not_working = 0xFFFFFFFF;
    }
}

unsigned short nobUsual::CalcProductivity()
{
    if(outOfRessourcesMsgSent)
        return 0;
    // Gucken, ob bis jetzt gearbeitet wurde/wird oder nicht, je nachdem noch was dazuzählen
    if(since_not_working != 0xFFFFFFFF)
    {
        // Es wurde bis jetzt nicht mehr gearbeitet, das also noch dazuzählen
        numGfNotWorking += static_cast<unsigned short>(GetEvMgr().GetCurrentGF() - since_not_working);
        // Zähler zurücksetzen
        since_not_working = GetEvMgr().GetCurrentGF();
    }

    // Produktivität ausrechnen
    unsigned short curProductivity = (400 - numGfNotWorking) / 4;

    // Zähler zurücksetzen
    numGfNotWorking = 0;

    return curProductivity;
}
