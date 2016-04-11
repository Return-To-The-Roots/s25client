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
#include "nobUsual.h"

#include "figures/nofBuildingWorker.h"
#include "figures/nofPigbreeder.h"
#include "Ware.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "GameClient.h"
#include "SerializedGameData.h"
#include "Loader.h"
#include <numeric>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class noFigure;
class noRoadNode;

nobUsual::nobUsual(BuildingType type,
                   MapPoint pos,
                   unsigned char player,
                   Nation nation)
    : noBuilding(type, pos, player, nation),
      worker(NULL), disable_production(false), disable_production_virtual(false),
      last_ordered_ware(0), orderware_ev(NULL), productivity_ev(NULL), is_working(false)
{
    std::fill(wares.begin(), wares.end(), 0);

    // Entsprechend so viele Listen erstellen, wie nötig sind, bei Bergwerken 3
    if(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count)
        ordered_wares.resize(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count);
    else
        ordered_wares.clear();

    // Arbeiter bestellen
    GameClientPlayer& owner = gwg->GetPlayer(player);
    owner.AddJobWanted(USUAL_BUILDING_CONSTS[type_ - 10].job, this);

    // Tür aufmachen,bis Gebäude besetzt ist
    OpenDoor();

    const std::list<nobUsual*>& otherBlds = owner.GetBuildings(type);
    if(otherBlds.empty())
        productivity = 0;
    else
    {
        // New building gets half the average productivity from all buildings of the same type
        int sumProductivity = 0;
        for(std::list<nobUsual*>::const_iterator it = otherBlds.begin(); it != otherBlds.end(); ++it)
            sumProductivity += (*it)->GetProductivity();
        productivity = sumProductivity / otherBlds.size() / 2;
    }
    // Set last productivities to current to avoid resetting it on first recalculation event
    std::fill(last_productivities.begin(), last_productivities.end(), productivity);

    // Gebäude in den Index eintragen, damit die Wirtschaft auch Bescheid weiß
    // Do this AFTER the productivity calculation or we will get this building too!
    owner.AddUsualBuilding(this);
}

nobUsual::nobUsual(SerializedGameData& sgd, const unsigned int obj_id)
    : noBuilding(sgd, obj_id),
      worker(sgd.PopObject<nofBuildingWorker>(GOT_UNKNOWN)),
      productivity(sgd.PopUnsignedShort()),
      disable_production(sgd.PopBool()),
      disable_production_virtual(sgd.PopBool()),
      last_ordered_ware(sgd.PopUnsignedChar()),
      orderware_ev(sgd.PopObject<EventManager::Event>(GOT_EVENT)),
      productivity_ev(sgd.PopObject<EventManager::Event>(GOT_EVENT)),
      is_working(sgd.PopBool())
{
    for(unsigned i = 0; i < 3; ++i)
        wares[i] = sgd.PopUnsignedChar();

    if(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count)
        ordered_wares.resize(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count);
    else
        ordered_wares.clear();

    for(unsigned i = 0; i < USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count; ++i)
        sgd.PopObjectContainer(ordered_wares[i], GOT_WARE);
    for(unsigned i = 0; i < LAST_PRODUCTIVITIES_COUNT; ++i)
        last_productivities[i] = sgd.PopUnsignedShort();

    // Visuellen Produktionszustand dem realen anpassen
    disable_production_virtual = disable_production;
}

nobUsual::~nobUsual()
{}

void nobUsual::Destroy_nobUsual()
{
    // Arbeiter Bescheid sagen
    if(worker)
    {
        worker->LostWork();
        worker = NULL;
    }else
        gwg->GetPlayer(player).JobNotWanted(this);

    // Bestellte Waren Bescheid sagen
    for(unsigned i = 0; i < USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count; ++i)
    {
        for(std::list<Ware*>::iterator it = ordered_wares[i].begin(); it != ordered_wares[i].end(); ++it)
            WareNotNeeded((*it));
        ordered_wares[i].clear();
    }

    // Events löschen
    em->RemoveEvent(orderware_ev);
    em->RemoveEvent(productivity_ev);
    orderware_ev = NULL;
    productivity_ev = NULL;

    // Gebäude wieder aus der Liste entfernen
    gwg->GetPlayer(player).RemoveUsualBuilding(this);

    // Inventur entsprechend verringern wegen den Waren, die vernichtetet werden
    for(unsigned i = 0; i < USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count; ++i)
        gwg->GetPlayer(player).DecreaseInventoryWare(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed[i], wares[i]);

    Destroy_noBuilding();
}

void nobUsual::Serialize_nobUsual(SerializedGameData& sgd) const
{
    Serialize_noBuilding(sgd);

    sgd.PushObject(worker, false);
    sgd.PushUnsignedShort(productivity);
    sgd.PushBool(disable_production);
    sgd.PushBool(disable_production_virtual);
    sgd.PushUnsignedChar(last_ordered_ware);
    sgd.PushObject(orderware_ev, true);
    sgd.PushObject(productivity_ev, true);
    sgd.PushBool(is_working);

    for(unsigned i = 0; i < 3; ++i)
        sgd.PushUnsignedChar(wares[i]);
    for(unsigned i = 0; i < USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count; ++i)
        sgd.PushObjectContainer(ordered_wares[i], true);
    for(unsigned i = 0; i < LAST_PRODUCTIVITIES_COUNT; ++i)
        sgd.PushUnsignedShort(last_productivities[i]);
}

void nobUsual::Draw(int x, int y)
{
    // Gebäude an sich zeichnen
    DrawBaseBuilding(x, y);

    // Wenn Produktion gestoppt ist, Schild außen am Gebäude zeichnen zeichnen
    if(disable_production_virtual)
        LOADER.GetMapImageN(46)->Draw(x + BUILDING_SIGN_CONSTS[nation][type_].x, y + BUILDING_SIGN_CONSTS[nation][type_].y, 0, 0, 0, 0, 0, 0);

    // Rauch zeichnen

    // Raucht dieses Gebäude und ist es in Betrieb? (nur arbeitende Gebäude rauchen schließlich)
    if(BUILDING_SMOKE_CONSTS[nation][type_ - 10].type && is_working)
    {
        // Dann Qualm zeichnen (damit Qualm nicht synchron ist, x- und y- Koordinate als Unterscheidung
        LOADER.GetMapImageN(692 + BUILDING_SMOKE_CONSTS[nation][type_ - 10].type * 8 + GAMECLIENT.GetGlobalAnimation(8, 5, 2, (GetX() + GetY()) * 100))
        ->Draw(x + BUILDING_SMOKE_CONSTS[nation][type_ - 10].x, y + BUILDING_SMOKE_CONSTS[nation][type_ - 10].y, 0, 0, 0, 0, 0, 0, 0x99EEEEEE);
    }

    // TODO: zusätzliche Dinge wie Mühlenräder, Schweinchen etc bei bestimmten Gebäuden zeichnen

    // Bei Mühle, wenn sie nicht arbeitet, immer Mühlenräder (nichtdrehend) zeichnen
    if(type_ == BLD_MILL && !is_working)
    {
        // Flügel der Mühle
        LOADER.GetNationImage(nation, 250 + 5 * 49)->Draw(x, y);
        // Schatten der Flügel
        LOADER.GetNationImage(nation, 250 + 5 * 49 + 1)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
    }
    // Esel in den Kammer bei Eselzucht zeichnen
    else if(type_ == BLD_DONKEYBREEDER)
    {
        // Für alle Völker jeweils
        // X-Position der Esel
        const int DONKEY_X[NAT_COUNT][3] = {{13, 26, 39}, {3, 16, 30}, {2, 15, 29}, {7, 18, 30}, {3, 16, 30}};
        // Y-Position
        const int DONKEY_Y[NAT_COUNT] = { -9, -17, -21, -17, -22};
        // Animations-IDS des Esels
        const unsigned char DONKEY_ANIMATION[] =
        { 0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 6, 5, 4, 4, 5, 6, 5, 7, 6, 5, 4, 3, 2, 1, 0 };

        // Die drei Esel zeichnen mithilfe von Globalanimation
        // Anzahl hängt von Produktivität der Eselzucht ab:
        // 0-29 - kein Esel
        // 30-60 - 1 Esel
        // 60-90 - 2 Esel
        // 90-100 - 3 Esel
        if(productivity >= 30) LOADER.GetMapImageN(2180 + DONKEY_ANIMATION[GAMECLIENT.GetGlobalAnimation(sizeof(DONKEY_ANIMATION), 5, 2, GetX() * (player + 2))])->Draw(x + DONKEY_X[nation][0], y + DONKEY_Y[nation]);
        if(productivity >= 60) LOADER.GetMapImageN(2180 + DONKEY_ANIMATION[GAMECLIENT.GetGlobalAnimation(sizeof(DONKEY_ANIMATION), 5, 2, GetY())])->Draw(x + DONKEY_X[nation][1], y + DONKEY_Y[nation]);
        if(productivity >= 90) LOADER.GetMapImageN(2180 + DONKEY_ANIMATION[GAMECLIENT.GetGlobalAnimation(sizeof(DONKEY_ANIMATION), 5, 2, GetX() + GetY() * (nation + 1))])->Draw(x + DONKEY_X[nation][2], y + DONKEY_Y[nation]);
    }
    // Bei Katapulthaus Katapult oben auf dem Dach zeichnen, falls er nicht "arbeitet"
    else if(type_ == BLD_CATAPULT && !is_working)
    {
        LOADER.GetPlayerImage("rom_bobs", 1776)->Draw(x - 7, y - 19, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLOR_WHITE);
    }

    // Bei Schweinefarm Schweinchen auf dem Hof zeichnen
    else if(type_ == BLD_PIGFARM && this->HasWorker())
    {
        // Position der 5 Schweinchen für alle 4 Völker (1. ist das große Schwein)
        const int PIG_POSITIONS[NAT_COUNT][5][2] =
        {
            //  gr. S. 1.klS 2. klS usw
            { {3, -8}, {17, 3}, { -12, 4}, { -2, 10}, { -22, 11} }, // Afrikaner
            { { -16, 0}, { -37, 0}, { -32, 8}, { -16, 10}, { -22, 18} }, // Japaner
            { { -15, 0}, { -4, 9}, { -22, 10}, {2, 19}, { -15, 20} }, // Römer
            { {5, -5}, {25, -12}, { -7, 7}, { -23, 11}, { -10, 14} }, // Wikinger
            { { -16, 5}, { -37, 5}, { -32, -1}, { -16, 15}, { -27, 18} } // Babylonier
        };


        /// Großes Schwein zeichnen
        LOADER.GetMapImageN(2160)->Draw(x + PIG_POSITIONS[nation][0][0], y + PIG_POSITIONS[nation][0][1], 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
        LOADER.GetMapImageN(2100 + GAMECLIENT.GetGlobalAnimation(12, 3, 1, GetX() + GetY() + GetObjId()))->Draw(x + PIG_POSITIONS[nation][0][0], y + PIG_POSITIONS[nation][0][1]);

        // Die 4 kleinen Schweinchen, je nach Produktivität
        for(unsigned i = 1; i < min<unsigned>(unsigned(productivity) / 20 + 1, 5); ++i)
        {
            //A random (really, dice-rolled by hand:) ) order of the four possible pig animations, with eating three times as much as the others ones
            //To get random-looking, non synchronous, sweet little pigs
            const unsigned char smallpig_animations[63] =
            {
                0, 0, 3, 2, 0, 0, 1, 3, 0, 3, 1, 3, 2, 0, 0, 1,
                0, 0, 1, 3, 2, 0, 1, 1, 0, 0, 2, 1, 0, 1, 0, 2,
                2, 0, 0, 2, 2, 0, 1, 0, 3, 1, 2, 0, 1, 2, 2, 0,
                0, 0, 3, 0, 2, 0, 3, 0, 3, 0, 1, 1, 0, 3, 0
            };
            const unsigned short animpos = GAMECLIENT.GetGlobalAnimation(63 * 12, 63 * 4 - i * 5, 1, 183 * i + GetX() * GetObjId() + GetY() * i);
            LOADER.GetMapImageN(2160)->Draw(x + PIG_POSITIONS[nation][i][0], y + PIG_POSITIONS[nation][i][1], 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
            LOADER.GetMapImageN(2112 + smallpig_animations[animpos / 12] * 12 + animpos % 12)->Draw(x + PIG_POSITIONS[nation][i][0], y + PIG_POSITIONS[nation][i][1]);
        }

        // Ggf. Sounds abspielen (oink oink), da soll sich der Schweinezüchter drum kümmen
        dynamic_cast<nofPigbreeder*>(worker)->MakePigSounds();
    }
    // Bei nubischen Bergwerken das Feuer vor dem Bergwerk zeichnen
    else if(IsMine() && worker && nation == NAT_AFRICANS)
        LOADER.GetMapPlayerImage(740 + GAMECLIENT.GetGlobalAnimation(8, 5, 2, GetObjId() + GetX() + GetY()))->
        Draw(x + NUBIAN_MINE_FIRE[type_ - BLD_GRANITEMINE][0], y + NUBIAN_MINE_FIRE[type_ - BLD_GRANITEMINE][1]);
}

void nobUsual::HandleEvent(const unsigned int id)
{
    if(id)
    {
        const unsigned short current_productivity = worker->CalcProductivity();
        // Sum over all last productivities and current (as start value)
        productivity = std::accumulate(last_productivities.begin(), last_productivities.end(), current_productivity);
        // Produktivität "verrücken"
        for(unsigned short i = last_productivities.size() - 1; i >= 1; --i)
            last_productivities[i] = last_productivities[i - 1];
        last_productivities[0] = current_productivity;

        // Durschnitt ausrechnen der letzten Produktivitäten PLUS der aktuellen!
        productivity /= (last_productivities.size() + 1);

        // Event für nächste Abrechnung
        productivity_ev = em->AddEvent(this, 400, 1);
    }
    else
    {
        // Ware bestellen (falls noch Platz ist) und nicht an Betriebe, die stillgelegt wurden!
        if(!disable_production)
        {
            // Maximale Warenanzahlbestimmen
            unsigned wares_count;
            if(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count == 3)
                wares_count = 2;
            else if(type_ == BLD_CATAPULT)
                wares_count = 4;
            else
                wares_count = 6;

            if(wares[last_ordered_ware] + ordered_wares[last_ordered_ware].size() < wares_count)
            {
                Ware* w = gwg->GetPlayer(player).OrderWare(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed[last_ordered_ware], this);
                if(w)
                    RTTR_Assert(helpers::contains(ordered_wares[last_ordered_ware], w));
            }

            // Wenn dieser Betrieb 2 Waren benötigt, muss sich bei den Warentypen abgewechselt werden
            last_ordered_ware = (last_ordered_ware + 1) % USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count;
        }

        // Nach ner bestimmten Zeit dann nächste Ware holen
        orderware_ev = em->AddEvent(this, 210);
    }

}

void nobUsual::AddWare(Ware*& ware)

{
    // Maximale Warenanzahlbestimmen (nur für assert unten)
    /*  unsigned wares_count;
        if(USUAL_BUILDING_CONSTS[this->type-10].wares_needed_count == 3)
            wares_count = 2;
        else if(this->type == BLD_CATAPULT)
            wares_count = 4;
        else
            wares_count = 6;*/

    // Gucken, um was für einen Warentyp es sich handelt und dann dort hinzufügen
    for(unsigned char i = 0; i < USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count; ++i)
    {
        if(ware->type == USUAL_BUILDING_CONSTS[type_ - 10].wares_needed[i])
        {
            ++wares[i];
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
    for(unsigned i = 0; i < USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count; ++i)
    {
        if(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed[i] == ware->type)
        {
            RTTR_Assert(helpers::contains(ordered_wares[i], ware));
            ordered_wares[i].remove(ware);
            break;
        }
    }
}

void nobUsual::GotWorker(Job  /*job*/, noFigure* worker)
{
    this->worker = static_cast<nofBuildingWorker*>(worker);

    if(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed[0] != GD_NOTHING)
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
    em->RemoveEvent(productivity_ev);

    // Waren-Bestell-Event abmelden
    em->RemoveEvent(orderware_ev);

    // neuen Arbeiter bestellen
    worker = NULL;
    gwg->GetPlayer(player).AddJobWanted(USUAL_BUILDING_CONSTS[type_ - 10].job, this);
}

bool nobUsual::WaresAvailable()
{
    switch(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count)
    {
        case 0: return true;
        case 1: return wares[0] != 0;
        case 2: return (wares[0] && wares[1]);
        case 3: return (wares[0] || wares[1] || wares[2]);
        default: return false;
    }
}

void nobUsual::ConsumeWares()
{
    unsigned char ware_type = 0xFF;

    if(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed_count == 3)
    {
        // Bei Bergwerken wird immer nur eine Lebensmittelart "konsumiert" und es wird natürlich immer die genommen, die am meisten vorhanden ist
        unsigned char best = 0;

        for(unsigned char i = 0; i < 3; ++i)
        {
            if(wares[i] > best)
            {
                ware_type = i;
                best = wares[i];
            }
        }
    }
    else
    {
        if(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed[0] != GD_NOTHING)
            ware_type = 0;
        if(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed[1] != GD_NOTHING)
            // 2 Waren verbrauchen!
            ware_type = 3;
    }

    if(ware_type != 0xFF)
    {
        if(ware_type == 3)
        {
            // 2 Waren verbrauchen
            --wares[0];
            --wares[1];
            GameClientPlayer& owner = gwg->GetPlayer(player);
            owner.DecreaseInventoryWare(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed[0], 1);
            owner.DecreaseInventoryWare(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed[1], 1);

            // try to get wares from warehouses
            if(wares[0] < 2)
            {
                Ware* w = owner.OrderWare(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed[0], this);
                if(w)
                    RTTR_Assert(helpers::contains(ordered_wares[0], w));
            }

            if(wares[1] < 2)
            {
                Ware* w = owner.OrderWare(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed[1], this);
                if(w)
                    RTTR_Assert(helpers::contains(ordered_wares[1], w));
            }

        }
        else
        {
            // Bestand verringern
            --wares[ware_type];
            // Inventur entsprechend verringern
            gwg->GetPlayer(player).DecreaseInventoryWare(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed[ware_type], 1);

            // try to get ware from warehouses
            if (wares[ware_type] < 2)
            {
                Ware* w = gwg->GetPlayer(player).OrderWare(USUAL_BUILDING_CONSTS[type_ - 10].wares_needed[ware_type], this);
                if(w)
                    RTTR_Assert(helpers::contains(ordered_wares[ware_type], w));
            }
        }
    }

}

unsigned nobUsual::CalcDistributionPoints(noRoadNode*  /*start*/, const GoodType type)
{
    // Warentyp ermitteln
    unsigned id;
    for(id = 0; id < 3; ++id)
    {
        if(USUAL_BUILDING_CONSTS[this->type_ - 10].wares_needed[id] == type)
            break;
    }

    // Wenn wir Ware nicht brauchen oder Produktion eingestellt wurde --> 0 zurückgeben
    if(id == 3 || disable_production)
        return 0;

    // Maximale Warenanzahlbestimmen
    unsigned wares_count;
    if(USUAL_BUILDING_CONSTS[this->type_ - 10].wares_needed_count == 3)
        wares_count = 2;
    else if(this->type_ == BLD_CATAPULT)
        wares_count = 4;
    else
        wares_count = 6;

    // Wenn wir schon mit der Ware vollgestopft sind (bei Bergwerken nur 2 Waren jeweils!), ebenfalls 0 zurückgeben
    if(unsigned(wares[id]) + ordered_wares[id].size() == wares_count)
        return 0;

    // 10000 als Basis wählen, damit man auch noch was abziehen kann
    unsigned points = 10000;

    // Wenn hier schon Waren drin sind oder welche bestellt sind, wirkt sich das natürlich negativ auf die "Wichtigkeit" aus
    points -= (wares[id] + ordered_wares[id].size()) * 30;

    if (points > 10000) // "underflow" ;)
    {
        return(0);
    }

    return points;
}

void nobUsual::TakeWare(Ware* ware)
{
    // Ware in die Bestellliste aufnehmen
    for(unsigned char i = 0; i < 3; ++i)
    {
        if(ware->type == USUAL_BUILDING_CONSTS[this->type_ - 10].wares_needed[i])
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
    productivity_ev = em->AddEvent(this, 400, 1);
}

void nobUsual::SetProductionEnabled(const bool enabled)
{
    if(disable_production == !enabled)
        return;
    // Umstellen
    disable_production = !enabled;
    // Wenn das von einem fremden Spieler umgestellt wurde (oder vom Replay), muss auch das visuelle umgestellt werden
    if(GAMECLIENT.GetPlayerID() != player || GAMECLIENT.IsReplayModeOn())
        disable_production_virtual = disable_production;

    if(disable_production)
    {
        // Wenn sie deaktiviert wurde, dem Arbeiter Bescheid sagen, damit er entsprechend stoppt, falls er schon
        // auf die Arbeit warteet
        if(worker)
            worker->ProductionStopped();
    }
    else
    {
        // Wenn sie wieder aktiviert wurde, evtl wieder mit arbeiten anfangen, falls es einen Arbeiter gibt
        if(worker)
            worker->GotWareOrProductionAllowed();
    }
}

bool nobUsual::HasWorker() const
{
    return worker && worker->GetState() != nofBuildingWorker::STATE_FIGUREWORK;;
}

void nobUsual::SetProductivityToZero()
{
    productivity = 0;
    std::fill(last_productivities.begin(), last_productivities.end(), 0);
}
