// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CombatEventLogger.h"

#include "ai/aijh/debug/StatsConfig.h"
#include "combat_log.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include <boost/filesystem/path.hpp>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {
namespace pb = ru::pkopachevsky::proto;

struct DestroyedBuilding
{
    BuildingType type;
    unsigned buildingObjId;
};

unsigned gLastLoggedGF = 0;
bool gHasLastLoggedGF = false;
std::vector<pb::CombatLogRecord> gPendingRecords;
unsigned gNextFlushGF = 0;
bool gHasNextFlushGF = false;
constexpr unsigned kFlushPeriodGF = 500;

uint64_t gNextCombatId = 1;
std::unordered_map<unsigned, uint64_t> gActiveCombatIds;
std::unordered_map<unsigned, std::vector<DestroyedBuilding>> gCaptureDestroyed;

std::ofstream OpenCombatLog()
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Combat))
        return {};
    const boost::filesystem::path path = boost::filesystem::path(STATS_CONFIG.statsPath) / "combat_log.pb";
    return std::ofstream(path.string(), std::ios::binary | std::ios::app);
}

bool WriteDelimitedRecord(std::ostream& os, const pb::CombatLogRecord& record)
{
    std::string payload;
    if(!record.SerializeToString(&payload))
        return false;

    google::protobuf::io::OstreamOutputStream zeroCopyOut(&os);
    google::protobuf::io::CodedOutputStream codedOut(&zeroCopyOut);
    codedOut.WriteVarint32(static_cast<uint32_t>(payload.size()));
    codedOut.WriteRaw(payload.data(), static_cast<int>(payload.size()));
    return !codedOut.HadError() && os.good();
}

void FlushPendingRecords()
{
    if(gPendingRecords.empty())
        return;

    std::ofstream log = OpenCombatLog();
    if(!log)
        return;

    for(const auto& record : gPendingRecords)
    {
        if(!WriteDelimitedRecord(log, record))
            return;
    }

    gPendingRecords.clear();
}

struct PendingFlushAtExit
{
    ~PendingFlushAtExit() { FlushPendingRecords(); }
};

PendingFlushAtExit gPendingFlushAtExit;

pb::BuildingType ToProtoBuildingType(const BuildingType buildingType)
{
    const int raw = static_cast<int>(buildingType);
    if(raw >= static_cast<int>(BuildingType::Headquarters) && raw <= static_cast<int>(BuildingType::HarborBuilding))
        return static_cast<pb::BuildingType>(raw + 1);
    return pb::BuildingType::BUILDING_TYPE_UNSPECIFIED;
}

pb::Rank ToProtoRank(const unsigned rank)
{
    if(rank < NUM_SOLDIER_RANKS)
        return static_cast<pb::Rank>(rank + 1);
    return pb::Rank::RANK_UNSPECIFIED;
}

pb::CombatWinnerRole ToProtoWinnerRole(const char* winnerRole)
{
    const std::string_view role = winnerRole ? std::string_view(winnerRole) : std::string_view{};
    if(role == "Attacker")
        return pb::CombatWinnerRole::COMBAT_WINNER_ROLE_ATTACKER;
    if(role == "Defender")
        return pb::CombatWinnerRole::COMBAT_WINNER_ROLE_DEFENDER;
    return pb::CombatWinnerRole::COMBAT_WINNER_ROLE_UNSPECIFIED;
}

void FillTarget(pb::CombatLogTarget* target, const BuildingType buildingType, const unsigned buildingObjId)
{
    target->set_building_type(ToProtoBuildingType(buildingType));
    target->set_building_id(buildingObjId);
}

void AddRankCounts(google::protobuf::RepeatedPtrField<pb::CombatLogCountByRank>* out,
                   const std::array<unsigned, NUM_SOLDIER_RANKS>& counts)
{
    for(std::size_t rank = 0; rank < counts.size(); ++rank)
    {
        if(counts[rank] == 0)
            continue;
        auto* entry = out->Add();
        entry->set_rank(ToProtoRank(static_cast<unsigned>(rank)));
        entry->set_count(counts[rank]);
    }
}

void AddAttackSources(google::protobuf::RepeatedPtrField<pb::CombatLogAttackSource>* out,
                      const std::vector<CombatEventLogger::AttackSource>& sources)
{
    for(const CombatEventLogger::AttackSource& source : sources)
    {
        auto* entry = out->Add();
        FillTarget(entry->mutable_source(), source.buildingType, source.buildingObjId);
        entry->set_total_count(source.count);
        AddRankCounts(entry->mutable_by_rank(), source.byRank);
    }
}

void AddDestroyedBuildings(google::protobuf::RepeatedPtrField<pb::CombatDestroyedBuilding>* out,
                           const std::vector<DestroyedBuilding>& destroyed)
{
    for(const DestroyedBuilding& building : destroyed)
    {
        auto* entry = out->Add();
        FillTarget(entry->mutable_building(), building.type, building.buildingObjId);
    }
}

uint64_t AllocateCombatId()
{
    return gNextCombatId++;
}

uint64_t GetOrCreateCombatId(const unsigned targetObjId)
{
    if(targetObjId == 0)
        return AllocateCombatId();

    const auto it = gActiveCombatIds.find(targetObjId);
    if(it != gActiveCombatIds.end())
        return it->second;

    const uint64_t combatId = AllocateCombatId();
    gActiveCombatIds[targetObjId] = combatId;
    return combatId;
}

pb::CombatLogRecord CreateRecord(const unsigned gf, const uint64_t combatId)
{
    pb::CombatLogRecord record;
    if(gHasLastLoggedGF && gf >= gLastLoggedGF)
        record.set_delta_gf(gf - gLastLoggedGF);
    else if(!gHasLastLoggedGF)
        record.set_delta_gf(gf);
    else
        record.set_delta_gf(0);

    gLastLoggedGF = gf;
    gHasLastLoggedGF = true;
    record.set_combat_id(combatId);
    return record;
}

void EnqueueRecord(const unsigned gf, pb::CombatLogRecord&& record)
{
    gPendingRecords.push_back(std::move(record));

    if(!gHasNextFlushGF)
    {
        gNextFlushGF = kFlushPeriodGF;
        gHasNextFlushGF = true;
    }

    if(gf >= gNextFlushGF)
    {
        FlushPendingRecords();
        while(gf >= gNextFlushGF)
            gNextFlushGF += kFlushPeriodGF;
    }
}

} // namespace

namespace CombatEventLogger {

void LogAttackOrder(unsigned gf, unsigned char attackerPlayer, unsigned char defenderPlayer, BuildingType targetType,
                    unsigned targetObjId, bool strongSoldiers, unsigned desiredCount, unsigned actualCount,
                    const std::array<unsigned, NUM_SOLDIER_RANKS>& actualByRank,
                    const std::vector<AttackSource>& sources)
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Combat))
        return;

    pb::CombatLogRecord record = CreateRecord(gf, GetOrCreateCombatId(targetObjId));
    auto* event = record.mutable_attack_order();
    event->set_attacker_player_id(static_cast<uint32_t>(attackerPlayer + 1));
    event->set_defender_player_id(static_cast<uint32_t>(defenderPlayer + 1));
    FillTarget(event->mutable_target(), targetType, targetObjId);
    event->set_strong_soldiers(strongSoldiers);
    event->set_desired_count(desiredCount);
    event->set_actual_count(actualCount);
    AddRankCounts(event->mutable_actual_by_rank(), actualByRank);
    AddAttackSources(event->mutable_sources(), sources);
    EnqueueRecord(gf, std::move(record));
}

void LogAggressiveDefenderOrder(unsigned gf, unsigned char attackerPlayer, BuildingType targetType,
                                unsigned targetObjId, unsigned char defenderPlayer,
                                const std::vector<AttackSource>& sources, unsigned char defenderRank)
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Combat))
        return;

    pb::CombatLogRecord record = CreateRecord(gf, GetOrCreateCombatId(targetObjId));
    auto* event = record.mutable_aggressive_defender_order();
    event->set_attacker_player_id(static_cast<uint32_t>(attackerPlayer + 1));
    event->set_defender_player_id(static_cast<uint32_t>(defenderPlayer + 1));
    FillTarget(event->mutable_target(), targetType, targetObjId);
    event->set_defender_rank(ToProtoRank(defenderRank));
    AddAttackSources(event->mutable_sources(), sources);
    EnqueueRecord(gf, std::move(record));
}

void LogFightResult(unsigned gf, unsigned char attackerPlayer, BuildingType targetType, unsigned targetObjId,
                    unsigned char defenderPlayer, unsigned attackerRank, unsigned attackerStartHp, unsigned defenderRank,
                    unsigned defenderStartHp, const char* winnerRole, unsigned winnerRemainingHp, unsigned x,
                    unsigned y)
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Combat))
        return;

    pb::CombatLogRecord record = CreateRecord(gf, GetOrCreateCombatId(targetObjId));
    auto* event = record.mutable_fight();
    event->set_attacker_player_id(static_cast<uint32_t>(attackerPlayer + 1));
    event->set_defender_player_id(static_cast<uint32_t>(defenderPlayer + 1));
    FillTarget(event->mutable_target(), targetType, targetObjId);
    event->set_attacker_rank(ToProtoRank(attackerRank));
    event->set_attacker_hp(attackerStartHp);
    event->set_defender_rank(ToProtoRank(defenderRank));
    event->set_defender_hp(defenderStartHp);
    event->set_winner(ToProtoWinnerRole(winnerRole));
    event->set_winner_hp(winnerRemainingHp);
    event->set_x(x);
    event->set_y(y);
    EnqueueRecord(gf, std::move(record));
}

void RecordCaptureDestroyed(const unsigned capturingObjId, const BuildingType type, const unsigned destroyedObjId)
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Combat) || !capturingObjId)
        return;
    gCaptureDestroyed[capturingObjId].push_back(DestroyedBuilding{type, destroyedObjId});
}

void LogCapture(unsigned gf, unsigned char attackerPlayer, unsigned char defenderPlayer, BuildingType buildingType,
                unsigned buildingObjId)
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Combat))
        return;

    const uint64_t combatId = GetOrCreateCombatId(buildingObjId);

    std::vector<DestroyedBuilding> destroyed;
    const auto it = gCaptureDestroyed.find(buildingObjId);
    if(it != gCaptureDestroyed.end())
    {
        destroyed = std::move(it->second);
        gCaptureDestroyed.erase(it);
    }

    pb::CombatLogRecord record = CreateRecord(gf, combatId);
    auto* event = record.mutable_capture();
    event->set_attacker_player_id(static_cast<uint32_t>(attackerPlayer + 1));
    event->set_defender_player_id(static_cast<uint32_t>(defenderPlayer + 1));
    FillTarget(event->mutable_target(), buildingType, buildingObjId);
    AddDestroyedBuildings(event->mutable_destroyed(), destroyed);
    EnqueueRecord(gf, std::move(record));

    FinishCombat(buildingObjId);
}

void FinishCombat(const unsigned targetObjId)
{
    if(targetObjId == 0)
        return;

    gActiveCombatIds.erase(targetObjId);
    gCaptureDestroyed.erase(targetObjId);
}

} // namespace CombatEventLogger
