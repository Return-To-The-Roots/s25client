// $Id: noFlag.cpp 9588 2015-02-01 09:37:32Z marcus $
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
#include "noFlag.h"

#include "GameWorld.h"
#include "Loader.h"
#include "macros.h"
#include "nofCarrier.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "Ware.h"
#include "noBuilding.h"
#include "noBuildingSite.h"
#include "nobMilitary.h"
#include "SerializedGameData.h"
#include "FOWObjects.h"

#include "glSmartBitmap.h"
#include "GameServer.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
noFlag::noFlag(const unsigned short x, const unsigned short y,
               const unsigned char player, const unsigned char dis_dir)
    : noRoadNode(NOP_FLAG, x, y, player),
      ani_offset(rand() % 20000), flagtype(FT_NORMAL)
{
    for(unsigned char i = 0; i < 8; ++i)
        wares[i] = NULL;

    // BWUs nullen
    for(unsigned char i = 0; i < MAX_BWU; ++i)
    {
        bwus[i].id = 0xFFFFFFFF;
        bwus[i].last_gf = 0;
    }

    // Gucken, ob die Flagge auf einen bereits bestehenden Weg gesetzt wurde
    unsigned char dir;
    noFlag* flag = gwg->GetRoadFlag(x, y, dir, dis_dir);

    if(flag)
    {
        if (flag->routes[dir])
        {
            flag->routes[dir]->SplitRoad(this);
        }
    }

    // auf Wasseranteile prüfen
    for(unsigned char i = 0; i < 6; ++i)
    {
        if(gwg->GetTerrainAround(x, y, i) == 14)
            flagtype = FT_WATER;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
noFlag::noFlag(SerializedGameData* sgd, const unsigned int obj_id)
    : noRoadNode(sgd, obj_id),
      ani_offset(rand() % 20000), flagtype(FlagType(sgd->PopUnsignedChar()))
{
    for(unsigned char i = 0; i < 8; ++i)
        wares[i] = sgd->PopObject<Ware>(GOT_WARE);

    // BWUs laden
    for(unsigned char i = 0; i < MAX_BWU; ++i)
    {
        bwus[i].id = sgd->PopUnsignedInt();
        bwus[i].last_gf = sgd->PopUnsignedInt();
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
noFlag::~noFlag()
{
    // Waren vernichten
    for(unsigned char i = 0; i < 8; ++i)
        delete wares[i];
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void noFlag::Destroy_noFlag()
{
    /// Da ist dann nichts
    gwg->SetNO(0, x, y);

    // Waren vernichten
    for(unsigned i = 0; i < 8; ++i)
    {
        if(wares[i])
        {
            // Inventur entsprechend verringern
            wares[i]->WareLost(player);
            delete wares[i];
            wares[i] = NULL;
        }
    }

    // Den Flag-Workern Bescheid sagen, die hier ggf. arbeiten
    gwg->GetPlayer(player)->FlagDestroyed(this);

    Destroy_noRoadNode();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void noFlag::Serialize_noFlag(SerializedGameData* sgd) const
{
    Serialize_noRoadNode(sgd);

    sgd->PushUnsignedChar(static_cast<unsigned char>(flagtype));
    for(unsigned char i = 0; i < 8; ++i)
        sgd->PushObject(wares[i], true);

    // BWUs speichern
    for(unsigned char i = 0; i < MAX_BWU; ++i)
    {
        sgd->PushUnsignedInt(bwus[i].id);
        sgd->PushUnsignedInt(bwus[i].last_gf);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void noFlag::Draw(int x, int y)
{
    // Positionen der Waren an der Flagge relativ zur Flagge
    static const signed char WARES_POS[8][2] =
    {
        {  0,  0},
        { -4,  0},
        {  3, -1},
        { -7, -1},
        {  6, -2},
        { -10, -2},
        {  9, -5},
        { -13, -5}
    };

    unsigned ani_step = GAMECLIENT.GetGlobalAnimation(8, 2, 1, ani_offset);

    Loader::flag_cache[gwg->GetPlayer(player)->nation][flagtype][ani_step].draw(x, y, 0xFFFFFFFF, COLORS[gwg->GetPlayer(player)->color]);

    // Waren (von hinten anfangen zu zeichnen)
    for(unsigned i = 8; i; --i)
    {
        if(wares[i - 1])
            LOADER.GetMapImageN(2200 + wares[i - 1]->type)->Draw(x + WARES_POS[i - 1][0], y + WARES_POS[i - 1][1], 0, 0, 0, 0, 0, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung"
 *  für den Fog of War.
 *
 *  @author OLiver
 */
FOWObject* noFlag::CreateFOWObject() const
{
    return new fowFlag(player, flagtype);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Legt eine Ware an der Flagge ab.
 *
 *  @author OLiver
 */
void noFlag::AddWare(Ware* ware)
{
    for(unsigned char i = 0; i < 8; ++i)
    {
        if(!wares[i])
        {
            wares[i] = ware;

            // Träger Bescheid sagen
            if(ware->GetNextDir() != 0xFF)
                routes[ware->GetNextDir()]->AddWareJob(this);

            return;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Gibt die Anzahl der Waren zurück, die an der Flagge liegen.
 *
 *  @author OLiver
 */
unsigned noFlag::GetWareCount() const
{
    unsigned count = 0;
    for(unsigned char i = 0; i < 8; ++i)
        if(wares[i])
            ++count;

    return count;
}

///////////////////////////////////////////////////////////////////////////////
/**
 * Wählt eine Ware von einer Flagge aus (anhand der Transportreihenfolge),
 * entfernt sie von der Flagge und gibt sie zurück.
 *
 * wenn swap_wares true ist, bedeutet dies, dass Waren nur ausgetauscht werden
 * und somit nicht die Träger benachrichtigt werden müssen.
 *
 *  @author OLiver
 */
Ware* noFlag::SelectWare(const unsigned char dir, const bool swap_wares, const noFigure* const carrier)
{
    Ware* best_ware = NULL;

    // Index merken, damit wir die enstprechende Ware dann entfernen können
    unsigned best_ware_index = 0xFF;

    // Die mit der niedrigsten, d.h. höchsten Priorität wird als erstes transportiert
    for(unsigned char i = 0; i < 8; ++i)
    {
        if(wares[i])
        {
            if(wares[i]->GetNextDir() == dir)
            {
                if(best_ware)
                {
                    if(gwg->GetPlayer(player)->transport[wares[i]->type] < gwg->GetPlayer(player)->transport[best_ware->type])
                    {
                        best_ware = wares[i];
                        best_ware_index = i;
                    }
                }
                else
                {
                    best_ware = wares[i];
                    best_ware_index = i;
                }
            }
        }
    }

    // Ware von der Flagge entfernen
    if(best_ware)
        wares[best_ware_index] = NULL;

    // ggf. anderen Trägern Bescheid sagen, aber nicht dem, der die Ware aufgehoben hat!
    routes[dir]->WareJobRemoved(carrier);

    if(!swap_wares)
    {
        // Wenn nun wieder ein Platz frei ist, allen Wegen rundrum sowie evtl Warenhäusern
        // Bescheid sagen, die evtl waren, dass sie wieder was ablegen können
        for(unsigned char i = 0; i < 6; ++i)
        {
            if(routes[i])
            {
                if(routes[i]->GetLength() == 1)
                {
                    // Gebäude?

                    if(gwg->GetSpecObj<noBase>(gwg->GetXA(x, y, 1), gwg->GetYA(x, y, 1))->GetType() == NOP_BUILDING)
                    {
                        if(gwg->GetSpecObj<noBuilding>(gwg->GetXA(x, y, 1), gwg->GetYA(x, y, 1))->FreePlaceAtFlag())
                            break;
                    }
                }
                else
                {
                    // Richtiger Weg --> Träger Bescheid sagen
                    for(unsigned char c = 0; c < 2; ++c)
                    {
                        if(routes[i]->hasCarrier(c))
                        {
                            if(routes[i]->getCarrier(c)->SpaceAtFlag(this == routes[i]->GetF2()))
                                break;
                        }
                    }
                }
            }
        }
    }

    /*  assert(best_ware);
        if(!best_ware)
            LOG.lprintf("Achtung: Bug im Spiel: noFlag::SelectWare: best_ware = 0!\n");
    */
    return best_ware;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Gibt Wegstrafpunkte für das Pathfinden für Waren, die in eine bestimmte
 *  Richtung noch transportiert werden müssen.
 *
 *  @author OLiver
 */
unsigned short noFlag::GetPunishmentPoints(const unsigned char dir) const
{
    // Waren zählen, die in diese Richtung transportiert werden müssen
    unsigned short points = GetWaresCountForRoad(dir) << 1;

    // Wenn kein Träger auf der Straße ist, gibts nochmal extra satte Strafpunkte
    if(!routes[dir]->isOccupied())
        points += 500;
	else if (routes[dir]->hasCarrier(0) && !routes[dir]->getCarrier(0)->GetCarrierState() && !routes[dir]->hasCarrier(1)) //no donkey and the normal carrier has been ordered from the warehouse but has not yet arrived
		points +=50;

    return points;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zerstört evtl. vorhandenes Gebäude bzw. Baustelle vor der Flagge.
 *
 *  @author OLiver
 */
void noFlag::DestroyAttachedBuilding()
{
    // Achtung es wird ein Feuer durch Destroy gesetzt, daher Objekt merken!
    noBase* no = gwg->GetNO(gwg->GetXA(x, y, 1), gwg->GetYA(x, y, 1));
    if(no->GetType() == NOP_BUILDINGSITE || no->GetType() == NOP_BUILDING)
    {
        no->Destroy();
        delete no;
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Baut normale Flaggen zu "gloriösen" aus bei Eselstraßen.
 *
 *  @author OLiver
 */
void noFlag::Upgrade()
{
    if(flagtype == FT_NORMAL)
        flagtype = FT_LARGE;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Feind übernimmt die Flagge.
 *
 *  @author OLiver
 */
void noFlag::Capture(const unsigned char new_owner)
{
    // Alle Straßen um mich herum zerstören bis auf die zum Gebäude (also Nr. 1)
    for(unsigned char i = 0; i < 6; ++i)
    {
        if(i != 1)
            DestroyRoad(i);
    }

    // Waren vernichten
    for(unsigned char i = 0; i < 8; ++i)
    {
        if(wares[i])
        {
            wares[i]->WareLost(player);
            delete wares[i];
            wares[i] = NULL;
        }
    }

    this->player = new_owner;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Ist diese Flagge für eine bestimmte Lagerhausflüchtlingsgruppe (BWU) nicht zugänglich?
 *
 *  @author OLiver
 */
bool noFlag::IsImpossibleForBWU(const unsigned bwu_id) const
{
    // Zeitintervall, in der die Zugänglichkeit der Flaggen von einer bestimmten BWU überprüft wird
    const unsigned int MAX_BWU_INTERVAL = 2000;

    // BWU-ID erstmal suchen
    for(unsigned char i = 0; i < MAX_BWU; ++i)
    {
        if(bwus[i].id == bwu_id)
        {
            // Wenn letzter TÜV noch nicht zu lange zurückliegt, können wir sie als unzugänglich zurückgeben
            if(GameClient::inst().GetGFNumber() - bwus[i].last_gf <= MAX_BWU_INTERVAL)
                return true;

            // ansonsten nicht, evtl ist sie ja jetzt mal wieder zugänglich, sollte also mal neu geprüft werden
            else
                return false;
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Hinzufügen, dass diese Flagge für eine bestimmte Lagerhausgruppe nicht zugänglich ist.
 *
 *  @author OLiver
 */
void noFlag::ImpossibleForBWU(const unsigned bwu_id)
{
    // Evtl gibts BWU schon --> Dann einfach GF-Zahl aktualisieren
    for(unsigned char i = 0; i < MAX_BWU; ++i)
    {
        if(bwus[i].id == bwu_id)
        {
            bwus[i].last_gf = GameClient::inst().GetGFNumber();
            return;
        }
    }

    // Gibts noch nicht, dann den ältesten BWU-Account raussuchen und den überschreiben
    unsigned oldest_gf = 0xFFFFFFFF;
    unsigned oldest_account_id = 0;

    for(unsigned char i = 0; i < MAX_BWU; ++i)
    {
        if(bwus[i].last_gf < oldest_gf)
        {
            // Neuer ältester
            oldest_gf = bwus[i].last_gf;
            oldest_account_id = i;
        }
    }

    // Den ältesten dann schließlich überschreiben
    bwus[oldest_account_id].id = bwu_id;
    bwus[oldest_account_id].last_gf = oldest_gf;
}
