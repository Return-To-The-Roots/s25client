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

#include "rttrDefines.h" // IWYU pragma: keep
#include "noFlag.h"

#include "EventManager.h"
#include "FOWObjects.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "buildings/noBuilding.h"
#include "figures/nofCarrier.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorldGame.h"
#include "gameData/TerrainDesc.h"
#include <boost/bind.hpp>

noFlag::noFlag(const MapPoint pos, const unsigned char player, const unsigned char dis_dir)
    : noRoadNode(NOP_FLAG, pos, player), ani_offset(rand() % 20000)
{
    for(unsigned i = 0; i < wares.size(); ++i)
        wares[i] = NULL;

    // BWUs nullen
    for(unsigned i = 0; i < bwus.size(); ++i)
    {
        bwus[i].id = 0xFFFFFFFF;
        bwus[i].last_gf = 0;
    }

    // Gucken, ob die Flagge auf einen bereits bestehenden Weg gesetzt wurde
    Direction dir;
    noFlag* flag = gwg->GetRoadFlag(pos, dir, dis_dir);

    if(flag && flag->GetRoute(dir))
        flag->GetRoute(dir)->SplitRoad(this);

    // auf Wasseranteile prüfen
    if(gwg->HasTerrain(pos, boost::bind(&TerrainDesc::kind, _1) == TerrainKind::WATER))
        flagtype = FT_WATER;
    else
        flagtype = FT_NORMAL;
}

noFlag::noFlag(SerializedGameData& sgd, const unsigned obj_id)
    : noRoadNode(sgd, obj_id), ani_offset(rand() % 20000), flagtype(FlagType(sgd.PopUnsignedChar()))
{
    for(unsigned i = 0; i < wares.size(); ++i)
        wares[i] = sgd.PopObject<Ware>(GOT_WARE);

    // BWUs laden
    for(unsigned i = 0; i < bwus.size(); ++i)
    {
        bwus[i].id = sgd.PopUnsignedInt();
        bwus[i].last_gf = sgd.PopUnsignedInt();
    }
}

noFlag::~noFlag()
{
    // Waren vernichten
    for(unsigned i = 0; i < wares.size(); ++i)
        delete wares[i];
}

void noFlag::Destroy_noFlag()
{
    /// Da ist dann nichts
    gwg->SetNO(pos, NULL);

    // Waren vernichten
    for(unsigned i = 0; i < wares.size(); ++i)
    {
        if(wares[i])
        {
            // Inventur entsprechend verringern
            wares[i]->WareLost(player);
            wares[i]->Destroy();
            deletePtr(wares[i]);
        }
    }

    // Den Flag-Workern Bescheid sagen, die hier ggf. arbeiten
    gwg->GetPlayer(player).FlagDestroyed(this);

    Destroy_noRoadNode();
}

void noFlag::Serialize_noFlag(SerializedGameData& sgd) const
{
    Serialize_noRoadNode(sgd);

    sgd.PushUnsignedChar(static_cast<unsigned char>(flagtype));
    for(unsigned i = 0; i < wares.size(); ++i)
        sgd.PushObject(wares[i], true);

    // BWUs speichern
    for(unsigned i = 0; i < bwus.size(); ++i)
    {
        sgd.PushUnsignedInt(bwus[i].id);
        sgd.PushUnsignedInt(bwus[i].last_gf);
    }
}

void noFlag::Draw(DrawPoint drawPt)
{
    // Positionen der Waren an der Flagge relativ zur Flagge
    static const DrawPointInit WARES_POS[8] = {{0, 0}, {-4, 0}, {3, -1}, {-7, -1}, {6, -2}, {-10, -2}, {9, -5}, {-13, -5}};

    unsigned ani_step = GAMECLIENT.GetGlobalAnimation(8, 2, 1, ani_offset);

    LOADER.flag_cache[gwg->GetPlayer(player).nation][flagtype][ani_step].draw(drawPt, 0xFFFFFFFF, gwg->GetPlayer(player).color);

    // Waren (von hinten anfangen zu zeichnen)
    for(unsigned i = 8; i; --i)
    {
        if(wares[i - 1])
            LOADER.GetMapImageN(2200 + wares[i - 1]->type)->DrawFull(drawPt + WARES_POS[i - 1]);
    }
}

/**
 *  Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung"
 *  für den Fog of War.
 */
FOWObject* noFlag::CreateFOWObject() const
{
    const GamePlayer& owner = gwg->GetPlayer(player);
    return new fowFlag(owner.color, owner.nation, flagtype);
}

/**
 *  Legt eine Ware an der Flagge ab.
 */
void noFlag::AddWare(Ware*& ware)
{
    for(unsigned i = 0; i < wares.size(); ++i)
    {
        if(wares[i])
            continue;

        wares[i] = ware;
        // Träger Bescheid sagen
        if(ware->GetNextDir() != 0xFF)
            routes[ware->GetNextDir()]->AddWareJob(this);
        return;
    }
    RTTR_Assert(false); // No place found???
}

/**
 *  Gibt die Anzahl der Waren zurück, die an der Flagge liegen.
 */
unsigned noFlag::GetNumWares() const
{
    unsigned count = 0;
    for(unsigned i = 0; i < wares.size(); ++i)
        if(wares[i])
            ++count;

    return count;
}

/**
 * Wählt eine Ware von einer Flagge aus (anhand der Transportreihenfolge),
 * entfernt sie von der Flagge und gibt sie zurück.
 *
 * wenn swap_wares true ist, bedeutet dies, dass Waren nur ausgetauscht werden
 * und somit nicht die Träger benachrichtigt werden müssen.
 */
Ware* noFlag::SelectWare(const Direction roadDir, const bool swap_wares, const noFigure* const carrier)
{
    Ware* best_ware = NULL;

    // Index merken, damit wir die enstprechende Ware dann entfernen können
    unsigned best_ware_index = 0xFF;

    // Die mit der niedrigsten, d.h. höchsten Priorität wird als erstes transportiert
    for(unsigned i = 0; i < wares.size(); ++i)
    {
        if(!wares[i])
            continue;
        if(wares[i]->GetNextDir() == roadDir.toUInt())
        {
            if(best_ware)
            {
                if(gwg->GetPlayer(player).GetTransportPriority(wares[i]->type)
                   < gwg->GetPlayer(player).GetTransportPriority(best_ware->type))
                {
                    best_ware = wares[i];
                    best_ware_index = i;
                }
            } else
            {
                best_ware = wares[i];
                best_ware_index = i;
            }
        }
    }

    // Ware von der Flagge entfernen
    if(best_ware)
        wares[best_ware_index] = NULL;

    // ggf. anderen Trägern Bescheid sagen, aber nicht dem, der die Ware aufgehoben hat!
    GetRoute(roadDir)->WareJobRemoved(carrier);

    if(!swap_wares && best_ware)
    {
        // Wenn nun wieder ein Platz frei ist, allen Wegen rundrum sowie evtl Warenhäusern
        // Bescheid sagen, die evtl waren, dass sie wieder was ablegen können
        for(unsigned dir = 0; dir < Direction::COUNT; ++dir)
        {
            if(!routes[dir])
                continue;

            if(routes[dir]->GetLength() == 1)
            {
                // Gebäude?

                if(gwg->GetSpecObj<noBase>(gwg->GetNeighbour(pos, Direction::NORTHWEST))->GetType() == NOP_BUILDING)
                {
                    if(gwg->GetSpecObj<noBuilding>(gwg->GetNeighbour(pos, Direction::NORTHWEST))->FreePlaceAtFlag())
                        break;
                }
            } else
            {
                // Richtiger Weg --> Träger Bescheid sagen
                for(unsigned char c = 0; c < 2; ++c)
                {
                    if(routes[dir]->hasCarrier(c))
                    {
                        if(routes[dir]->getCarrier(c)->SpaceAtFlag(this == routes[dir]->GetF2()))
                            break;
                    }
                }
            }
        }
    }

    return best_ware;
}

unsigned noFlag::GetNumWaresForRoad(const Direction dir) const
{
    unsigned ret = 0;

    for(unsigned i = 0; i < wares.size(); i++)
    {
        if(wares[i] && (wares[i]->GetNextDir() == dir.toUInt()))
            ret++;
    }
    return ret;
}

/**
 *  Gibt Wegstrafpunkte für das Pathfinden für Waren, die in eine bestimmte
 *  Richtung noch transportiert werden müssen.
 */
unsigned noFlag::GetPunishmentPoints(const Direction dir) const
{
    // Waren zählen, die in diese Richtung transportiert werden müssen
    unsigned points = GetNumWaresForRoad(dir) * 2;

    // Wenn kein Träger auf der Straße ist, gibts nochmal extra satte Strafpunkte
    const RoadSegment* routeInDir = routes[dir.toUInt()];
    if(!routeInDir->isOccupied())
        points += 500;
    else if(routes[dir.toUInt()]->hasCarrier(0) && routes[dir.toUInt()]->getCarrier(0)->GetCarrierState() == CARRS_FIGUREWORK
            && !routes[dir.toUInt()]->hasCarrier(
                 1)) // no donkey and the normal carrier has been ordered from the warehouse but has not yet arrived
        points += 50;

    return points;
}

/**
 *  Zerstört evtl. vorhandenes Gebäude bzw. Baustelle vor der Flagge.
 */
void noFlag::DestroyAttachedBuilding()
{
    // Achtung es wird ein Feuer durch Destroy gesetzt, daher Objekt merken!
    noBase* no = gwg->GetNO(gwg->GetNeighbour(pos, Direction::NORTHWEST));
    if(no->GetType() == NOP_BUILDINGSITE || no->GetType() == NOP_BUILDING)
    {
        no->Destroy();
        delete no;
    }
}

/**
 *  Baut normale Flaggen zu "gloriösen" aus bei Eselstraßen.
 */
void noFlag::Upgrade()
{
    if(flagtype == FT_NORMAL)
        flagtype = FT_LARGE;
}

/**
 *  Feind übernimmt die Flagge.
 */
void noFlag::Capture(const unsigned char new_owner)
{
    // Alle Straßen um mich herum zerstören bis auf die zum Gebäude (also Nr. 1)
    for(unsigned dir = 0; dir < Direction::COUNT; ++dir)
    {
        if(dir != 1)
            DestroyRoad(Direction::fromInt(dir));
    }

    // Waren vernichten
    for(unsigned i = 0; i < wares.size(); ++i)
    {
        if(wares[i])
        {
            wares[i]->WareLost(player);
            wares[i]->Destroy();
            deletePtr(wares[i]);
        }
    }

    this->player = new_owner;
}

/**
 *  Ist diese Flagge für eine bestimmte Lagerhausflüchtlingsgruppe (BWU) nicht zugänglich?
 */
bool noFlag::IsImpossibleForBWU(const unsigned bwu_id) const
{
    // Zeitintervall, in der die Zugänglichkeit der Flaggen von einer bestimmten BWU überprüft wird
    const unsigned MAX_BWU_INTERVAL = 2000;

    // BWU-ID erstmal suchen
    for(unsigned i = 0; i < bwus.size(); ++i)
    {
        if(bwus[i].id == bwu_id)
        {
            // Wenn letzter TÜV noch nicht zu lange zurückliegt, können wir sie als unzugänglich zurückgeben
            if(GetEvMgr().GetCurrentGF() - bwus[i].last_gf <= MAX_BWU_INTERVAL)
                return true;

            // ansonsten nicht, evtl ist sie ja jetzt mal wieder zugänglich, sollte also mal neu geprüft werden
            else
                return false;
        }
    }

    return false;
}

/**
 *  Hinzufügen, dass diese Flagge für eine bestimmte Lagerhausgruppe nicht zugänglich ist.
 */
void noFlag::ImpossibleForBWU(const unsigned bwu_id)
{
    // Evtl gibts BWU schon --> Dann einfach GF-Zahl aktualisieren
    for(unsigned i = 0; i < bwus.size(); ++i)
    {
        if(bwus[i].id == bwu_id)
        {
            bwus[i].last_gf = GetEvMgr().GetCurrentGF();
            return;
        }
    }

    // Gibts noch nicht, dann den ältesten BWU-Account raussuchen und den überschreiben
    unsigned oldest_gf = 0xFFFFFFFF;
    unsigned oldest_account_id = 0;

    for(unsigned i = 0; i < bwus.size(); ++i)
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
