// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noFlag.h"
#include "EventManager.h"
#include "FOWObjects.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "buildings/noBuilding.h"
#include "figures/nofCarrier.h"
#include "helpers/EnumRange.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorld.h"
#include "gameData/TerrainDesc.h"
#include "gameTypes/GoodTypes.h"
#include <algorithm>

noFlag::noFlag(const MapPoint pos, const unsigned char player)
    : noRoadNode(NodalObjectType::Flag, pos, player), ani_offset(rand() % 20000)
{
    // BWUs nullen
    for(auto& bwu : bwus)
    {
        bwu.id = 0xFFFFFFFF;
        bwu.last_gf = 0;
    }

    // Gucken, ob die Flagge auf einen bereits bestehenden Weg gesetzt wurde
    Direction dir;
    noFlag* flag = world->GetRoadFlag(pos, dir);

    if(flag && flag->GetRoute(dir))
        flag->GetRoute(dir)->SplitRoad(this);

    // auf Wasseranteile prüfen
    if(world->HasTerrain(pos, [](const auto& desc) { return desc.kind == TerrainKind::Water; }))
        flagtype = FlagType::Water;
    else
        flagtype = FlagType::Normal;
}

noFlag::noFlag(SerializedGameData& sgd, const unsigned obj_id)
    : noRoadNode(sgd, obj_id), ani_offset(rand() % 20000), flagtype(sgd.Pop<FlagType>())
{
    if(sgd.GetGameDataVersion() < 8)
    {
        for(unsigned i = 0; i < wares.max_size(); i++)
        {
            auto* ware = sgd.PopObject<Ware>(GO_Type::Ware);
            if(ware)
                wares.emplace_back(ware);
        }
    } else
        sgd.PopObjectContainer(wares, GO_Type::Ware);

    for(const auto i : helpers::enumRange<GoodType>())
    {
        inventory.visual[i] = sgd.PopUnsignedInt();
        inventory.real[i] = sgd.PopUnsignedInt();
    }
    // BWUs laden
    for(auto& bwu : bwus)
    {
        bwu.id = sgd.PopUnsignedInt();
        bwu.last_gf = sgd.PopUnsignedInt();
    }
}

noFlag::~noFlag() = default;

void noFlag::Destroy()
{
    /// Da ist dann nichts
    world->SetNO(pos, nullptr);

    // Waren vernichten
    for(auto& ware : wares)
    {
        // Inventur entsprechend verringern
        ware->WareLost(player);
        ware->Destroy();
    }
    wares.clear();

    // Den Flag-Workern Bescheid sagen, die hier ggf. arbeiten
    world->GetPlayer(player).FlagDestroyed(this);

    noRoadNode::Destroy();
}

void noFlag::Serialize(SerializedGameData& sgd) const
{
    noRoadNode::Serialize(sgd);

    sgd.PushEnum<uint8_t>(flagtype);
    sgd.PushObjectContainer(wares, true);
    for(const auto i : helpers::enumRange<GoodType>())
    {
        sgd.PushUnsignedInt(inventory.visual[i]);
        sgd.PushUnsignedInt(inventory.real[i]);
    }
    // BWUs speichern
    for(const auto& bwu : bwus)
    {
        sgd.PushUnsignedInt(bwu.id);
        sgd.PushUnsignedInt(bwu.last_gf);
    }
}

void noFlag::Draw(DrawPoint drawPt)
{
    // Positionen der Waren an der Flagge relativ zur Flagge
    static constexpr std::array<DrawPoint, 8> WARES_POS = {
      {{0, 0}, {-4, 0}, {3, -1}, {-7, -1}, {6, -2}, {-10, -2}, {9, -5}, {-13, -5}}};

    unsigned ani_step = GAMECLIENT.GetGlobalAnimation(8, 2, 1, ani_offset);

    LOADER.flag_cache[world->GetPlayer(player).nation][flagtype][ani_step].draw(drawPt, 0xFFFFFFFF,
                                                                                world->GetPlayer(player).color);

    // Waren (von hinten anfangen zu zeichnen)
    for(unsigned i = wares.size(); i > 0; --i)
    {
        LOADER.GetWareStackTex(wares[i - 1]->type)->DrawFull(drawPt + WARES_POS[i - 1]);
    }
}

/**
 *  Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung"
 *  für den Fog of War.
 */
std::unique_ptr<FOWObject> noFlag::CreateFOWObject() const
{
    const GamePlayer& owner = world->GetPlayer(player);
    return std::make_unique<fowFlag>(owner.color, owner.nation, flagtype);
}

/**
 *  Legt eine Ware an der Flagge ab.
 */
void noFlag::AddWare(std::unique_ptr<Ware> ware)
{
    const GoodType good = ware->type;

    inventory.Add(good, 1);
    // First add ware, then tell carrier. So get the info from the ware first
    const RoadPathDirection nextDir = ware->GetNextDir();
    wares.push_back(std::move(ware));

    if(nextDir != RoadPathDirection::None)
        GetRoute(toDirection(nextDir))->AddWareJob(this);
}

/**
 * Wählt eine Ware von einer Flagge aus (anhand der Transportreihenfolge),
 * entfernt sie von der Flagge und gibt sie zurück.
 *
 * wenn swap_wares true ist, bedeutet dies, dass Waren nur ausgetauscht werden
 * und somit nicht die Träger benachrichtigt werden müssen.
 */
std::unique_ptr<Ware> noFlag::SelectWare(const Direction roadDir, const bool swap_wares, const noFigure* const carrier)
{
    // Index merken, damit wir die enstprechende Ware dann entfernen können
    int best_ware_index = -1;

    // Die mit der niedrigsten, d.h. höchsten Priorität wird als erstes transportiert
    for(unsigned i = 0; i < wares.size(); ++i)
    {
        if(wares[i]->GetNextDir() == toRoadPathDirection(roadDir))
        {
            if(best_ware_index >= 0)
            {
                if(world->GetPlayer(player).GetTransportPriority(wares[i]->type)
                   < world->GetPlayer(player).GetTransportPriority(wares[best_ware_index]->type))
                {
                    best_ware_index = i;
                }
            } else
                best_ware_index = i;
        }
    }

    // Ware von der Flagge entfernen
    std::unique_ptr<Ware> bestWare;
    if(best_ware_index >= 0)
    {
        bestWare = std::move(wares[best_ware_index]);
        wares.erase(wares.begin() + best_ware_index);
    }

    // ggf. anderen Trägern Bescheid sagen, aber nicht dem, der die Ware aufgehoben hat!
    GetRoute(roadDir)->WareJobRemoved(carrier);

    if(!swap_wares && bestWare)
    {
        // Wenn nun wieder ein Platz frei ist, allen Wegen rundrum sowie evtl Warenhäusern
        // Bescheid sagen, die evtl waren, dass sie wieder was ablegen können
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            const auto* route = GetRoute(dir);
            if(!route)
                continue;

            if(route->GetLength() == 1)
            {
                // Gebäude?

                if(world->GetSpecObj<noBase>(world->GetNeighbour(pos, Direction::NorthWest))->GetType()
                   == NodalObjectType::Building)
                {
                    if(world->GetSpecObj<noBuilding>(world->GetNeighbour(pos, Direction::NorthWest))->FreePlaceAtFlag())
                        break;
                }
            } else
            {
                // Richtiger Weg --> Träger Bescheid sagen
                for(unsigned char c = 0; c < 2; ++c)
                {
                    if(route->hasCarrier(c))
                    {
                        if(route->getCarrier(c)->SpaceAtFlag(this == route->GetF2()))
                            break;
                    }
                }
            }
        }
    }

    return bestWare;
}

unsigned noFlag::GetNumWaresForRoad(const Direction dir) const
{
    const auto roadDir = toRoadPathDirection(dir);
    return helpers::count_if(wares, [roadDir](const auto& ware) { return ware->GetNextDir() == roadDir; });
}

unsigned noFlag::GetPunishmentPoints(const Direction dir) const
{
    constexpr auto PATHFINDING_PENALTY_CARRIER_ARRIVING = 50;
    constexpr auto PATHFINDING_PENALTY_NO_CARRIER = 500;
    // This must be the same as "NO_CARRIER" for replay compatibility
    // TODO(Replay): Move to `way_costs` in nobHarborBuilding::GetShipConnections
    constexpr auto PATHFINDING_PENALTY_START_SHIPPING = 500;
    // 2 Points per ware as carrier has to walk to other point and back for each ware
    unsigned points = GetNumWaresForRoad(dir) * 2;

    const RoadSegment* routeInDir = GetRoute(dir);
    const nofCarrier* humanCarrier = routeInDir->getCarrier(0);
    if(humanCarrier)
    {
        // normal carrier has been ordered from the warehouse but has not yet arrived and no donkey
        if(humanCarrier->GetCarrierState() == CarrierState::FigureWork && !routeInDir->hasCarrier(1))
            points += PATHFINDING_PENALTY_CARRIER_ARRIVING;
    } else if(!routeInDir->hasCarrier(1))
    {
        // Routes are either between flags or from a flag to a building, see the ctor of noBaseBuilding
        const bool isBuildingEntry = (dir == Direction::NorthWest) && (routeInDir->GetF2()->GetGOT() != GO_Type::Flag);
        // For entering a building no carrier is required
        // Only roads to buildings considered by path finding are those to harbors
        if(isBuildingEntry)
        {
            RTTR_Assert(routeInDir->GetF2()->GetGOT() == GO_Type::NobHarborbuilding);
            points += PATHFINDING_PENALTY_START_SHIPPING;
        } else
            points += PATHFINDING_PENALTY_NO_CARRIER;
    }

    return points;
}

/**
 *  Zerstört evtl. vorhandenes Gebäude bzw. Baustelle vor der Flagge.
 */
void noFlag::DestroyAttachedBuilding()
{
    // Achtung es wird ein Feuer durch Destroy gesetzt, daher Objekt merken!
    noBase* no = world->GetNO(world->GetNeighbour(pos, Direction::NorthWest));
    if(no->GetType() == NodalObjectType::Buildingsite || no->GetType() == NodalObjectType::Building)
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
    if(flagtype == FlagType::Normal)
        flagtype = FlagType::Large;
}

/**
 *  Feind übernimmt die Flagge.
 */
void noFlag::Capture(const unsigned char new_owner)
{
    // Alle Straßen um mich herum zerstören bis auf die zum Gebäude
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(dir != Direction::NorthWest)
            DestroyRoad(dir);
    }

    // Waren vernichten
    for(auto& ware : wares)
    {
        ware->WareLost(player);
        ware->Destroy();
    }
    wares.clear();

    // Unregister this flag in the players flags
    world->GetPlayer(player).FlagDestroyed(this);
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
    for(auto bwu : bwus)
    {
        if(bwu.id == bwu_id)
        {
            // Wenn letzter TÜV noch nicht zu lange zurückliegt, können wir sie als unzugänglich zurückgeben
            return GetEvMgr().GetCurrentGF() - bwu.last_gf <= MAX_BWU_INTERVAL;
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
    for(auto& bwu : bwus)
    {
        if(bwu.id == bwu_id)
        {
            bwu.last_gf = GetEvMgr().GetCurrentGF();
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

const Inventory& noFlag::GetInventory() const
{
    return inventory.visual;
}
