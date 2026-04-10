// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RoadEventLogger.h"

#include "RoadSegment.h"
#include "ai/aijh/debug/StatsConfig.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "road_log.pb.h"
#include "world/GameWorldBase.h"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <string>
#include <vector>

namespace {
namespace bfs = boost::filesystem;
namespace pb = ru::pkopachevsky::proto;

struct HeaderInfo
{
    uint32_t mapWidth = 0;
    uint32_t mapHeight = 0;
    bool isValid = false;
};

struct RoadDemolitionContext
{
    RoadEventLogger::RoadDemolitionReason reason = RoadEventLogger::RoadDemolitionReason::Unspecified;
    unsigned initiatorPlayerId = 0;
};

struct FlagDemolitionContext
{
    RoadEventLogger::FlagDemolitionReason reason = RoadEventLogger::FlagDemolitionReason::Unspecified;
    unsigned initiatorPlayerId = 0;
};

HeaderInfo gHeaderInfo;
bool gHeaderWritten = false;
std::vector<pb::RoadLogRecord> gPendingRecords;
unsigned gNextFlushGF = 0;
bool gHasNextFlushGF = false;
constexpr unsigned kFlushPeriodGF = 500;

RoadDemolitionContext gRoadDemolitionContext;
FlagDemolitionContext gFlagDemolitionContext;

bfs::path GetRoadLogPath()
{
    return bfs::path(STATS_CONFIG.statsPath) / "road_log.pb";
}

template<typename T>
bool WriteDelimitedMessage(std::ostream& os, const T& message)
{
    std::string payload;
    if(!message.SerializeToString(&payload))
        return false;

    google::protobuf::io::OstreamOutputStream zeroCopyOut(&os);
    google::protobuf::io::CodedOutputStream codedOut(&zeroCopyOut);
    codedOut.WriteVarint32(static_cast<uint32_t>(payload.size()));
    codedOut.WriteRaw(payload.data(), static_cast<int>(payload.size()));
    return !codedOut.HadError() && os.good();
}

void RememberHeaderInfo(const GameWorldBase& world)
{
    const MapExtent size = world.GetSize();
    gHeaderInfo.mapWidth = size.x;
    gHeaderInfo.mapHeight = size.y;
    gHeaderInfo.isValid = true;
}

bool EnsureHeaderWritten(std::ofstream& log)
{
    if(gHeaderWritten)
        return true;
    if(!gHeaderInfo.isValid)
        return false;

    const bfs::path path = GetRoadLogPath();
    const bool needsHeader = !bfs::exists(path) || bfs::file_size(path) == 0;
    if(needsHeader)
    {
        pb::RoadLogHeader header;
        header.set_map_width(gHeaderInfo.mapWidth);
        header.set_map_height(gHeaderInfo.mapHeight);
        if(!WriteDelimitedMessage(log, header))
            return false;
    }

    gHeaderWritten = true;
    return true;
}

std::ofstream OpenRoadLog()
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Road))
        return {};
    return std::ofstream(GetRoadLogPath().string(), std::ios::binary | std::ios::app);
}

void FlushPendingRecords()
{
    if(gPendingRecords.empty())
        return;

    std::ofstream log = OpenRoadLog();
    if(!log)
        return;
    if(!EnsureHeaderWritten(log))
        return;

    for(const auto& record : gPendingRecords)
    {
        if(!WriteDelimitedMessage(log, record))
            return;
    }

    gPendingRecords.clear();
}

struct PendingFlushAtExit
{
    ~PendingFlushAtExit() { FlushPendingRecords(); }
};

PendingFlushAtExit gPendingFlushAtExit;

void EnqueueRecord(unsigned gf, const GameWorldBase& world, pb::RoadLogRecord&& record)
{
    RememberHeaderInfo(world);
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

pb::RoadLogPoint ToProtoPoint(MapPoint pt)
{
    pb::RoadLogPoint out;
    out.set_x(pt.x);
    out.set_y(pt.y);
    return out;
}

pb::RoadDirection ToProtoDirection(Direction dir)
{
    switch(dir)
    {
        case Direction::West: return pb::ROAD_DIRECTION_WEST;
        case Direction::NorthWest: return pb::ROAD_DIRECTION_NORTH_WEST;
        case Direction::NorthEast: return pb::ROAD_DIRECTION_NORTH_EAST;
        case Direction::East: return pb::ROAD_DIRECTION_EAST;
        case Direction::SouthEast: return pb::ROAD_DIRECTION_SOUTH_EAST;
        case Direction::SouthWest: return pb::ROAD_DIRECTION_SOUTH_WEST;
    }
    return pb::ROAD_DIRECTION_UNSPECIFIED;
}

pb::RoadLogType ToProtoRoadType(RoadType roadType)
{
    switch(roadType)
    {
        case RoadType::Normal: return pb::ROAD_LOG_TYPE_NORMAL;
        case RoadType::Donkey: return pb::ROAD_LOG_TYPE_DONKEY;
        case RoadType::Water: return pb::ROAD_LOG_TYPE_WATER;
    }
    return pb::ROAD_LOG_TYPE_UNSPECIFIED;
}

pb::RoadConstructionFailureReason ToProtoFailureReason(RoadEventLogger::RoadConstructionFailureReason reason)
{
    using In = RoadEventLogger::RoadConstructionFailureReason;
    switch(reason)
    {
        case In::RouteTooShort: return pb::ROAD_CONSTRUCTION_FAILURE_REASON_ROUTE_TOO_SHORT;
        case In::InvalidStartFlag: return pb::ROAD_CONSTRUCTION_FAILURE_REASON_INVALID_START_FLAG;
        case In::BlockedRoute: return pb::ROAD_CONSTRUCTION_FAILURE_REASON_BLOCKED_ROUTE;
        case In::EndFlagWrongOwner: return pb::ROAD_CONSTRUCTION_FAILURE_REASON_END_FLAG_WRONG_OWNER;
        case In::EndFlagCannotBePlaced: return pb::ROAD_CONSTRUCTION_FAILURE_REASON_END_FLAG_CANNOT_BE_PLACED;
        case In::Unspecified: break;
    }
    return pb::ROAD_CONSTRUCTION_FAILURE_REASON_UNSPECIFIED;
}

pb::RoadDemolitionReason ToProtoDemolitionReason(RoadEventLogger::RoadDemolitionReason reason)
{
    using In = RoadEventLogger::RoadDemolitionReason;
    switch(reason)
    {
        case In::Manual: return pb::ROAD_DEMOLITION_REASON_MANUAL;
        case In::FlagDestroyed: return pb::ROAD_DEMOLITION_REASON_FLAG_DESTROYED;
        case In::BuildingDestroyed: return pb::ROAD_DEMOLITION_REASON_BUILDING_DESTROYED;
        case In::Capture: return pb::ROAD_DEMOLITION_REASON_CAPTURE;
        case In::TerritoryRecalc: return pb::ROAD_DEMOLITION_REASON_TERRITORY_RECALC;
        case In::Split: return pb::ROAD_DEMOLITION_REASON_SPLIT;
        case In::Unspecified: break;
    }
    return pb::ROAD_DEMOLITION_REASON_UNSPECIFIED;
}

pb::FlagBuildReason ToProtoFlagBuildReason(RoadEventLogger::FlagBuildReason reason)
{
    using In = RoadEventLogger::FlagBuildReason;
    switch(reason)
    {
        case In::Manual: return pb::FLAG_BUILD_REASON_MANUAL;
        case In::RoadEndpoint: return pb::FLAG_BUILD_REASON_ROAD_ENDPOINT;
        case In::AutoFlags: return pb::FLAG_BUILD_REASON_AUTOFLAGS;
        case In::BuildingFront: return pb::FLAG_BUILD_REASON_BUILDING_FRONT;
        case In::Split: return pb::FLAG_BUILD_REASON_SPLIT;
        case In::Unspecified: break;
    }
    return pb::FLAG_BUILD_REASON_UNSPECIFIED;
}

pb::FlagDemolitionReason ToProtoFlagDemolitionReason(RoadEventLogger::FlagDemolitionReason reason)
{
    using In = RoadEventLogger::FlagDemolitionReason;
    switch(reason)
    {
        case In::Manual: return pb::FLAG_DEMOLITION_REASON_MANUAL;
        case In::TerritoryRecalc: return pb::FLAG_DEMOLITION_REASON_TERRITORY_RECALC;
        case In::Unspecified: break;
    }
    return pb::FLAG_DEMOLITION_REASON_UNSPECIFIED;
}

void AddRoute(pb::RoadConstructed* event, const std::vector<Direction>& route)
{
    for(const Direction dir : route)
        event->add_route(ToProtoDirection(dir));
}

void AddRoute(pb::RoadConstructionFailed* event, const std::vector<Direction>& route)
{
    for(const Direction dir : route)
        event->add_route(ToProtoDirection(dir));
}

void AddRoute(pb::RoadDemolished* event, const std::vector<Direction>& route)
{
    for(const Direction dir : route)
        event->add_route(ToProtoDirection(dir));
}

MapPoint CalculateEndPoint(const GameWorldBase& world, MapPoint start, const std::vector<Direction>& route)
{
    for(const Direction dir : route)
        start = world.GetNeighbour(start, dir);
    return start;
}

} // namespace

namespace RoadEventLogger {

ScopedRoadDemolitionContext::ScopedRoadDemolitionContext(const RoadDemolitionReason reason, const unsigned initiatorPlayerId)
    : previousReason_(gRoadDemolitionContext.reason), previousInitiatorPlayerId_(gRoadDemolitionContext.initiatorPlayerId)
{
    gRoadDemolitionContext.reason = reason;
    gRoadDemolitionContext.initiatorPlayerId = initiatorPlayerId;
}

ScopedRoadDemolitionContext::~ScopedRoadDemolitionContext()
{
    gRoadDemolitionContext.reason = previousReason_;
    gRoadDemolitionContext.initiatorPlayerId = previousInitiatorPlayerId_;
}

ScopedFlagDemolitionContext::ScopedFlagDemolitionContext(const FlagDemolitionReason reason, const unsigned initiatorPlayerId)
    : previousReason_(gFlagDemolitionContext.reason), previousInitiatorPlayerId_(gFlagDemolitionContext.initiatorPlayerId)
{
    gFlagDemolitionContext.reason = reason;
    gFlagDemolitionContext.initiatorPlayerId = initiatorPlayerId;
}

ScopedFlagDemolitionContext::~ScopedFlagDemolitionContext()
{
    gFlagDemolitionContext.reason = previousReason_;
    gFlagDemolitionContext.initiatorPlayerId = previousInitiatorPlayerId_;
}

RoadDemolitionReason GetCurrentRoadDemolitionReason()
{
    return gRoadDemolitionContext.reason;
}

unsigned GetCurrentRoadDemolitionInitiatorPlayerId()
{
    return gRoadDemolitionContext.initiatorPlayerId;
}

FlagDemolitionReason GetCurrentFlagDemolitionReason()
{
    return gFlagDemolitionContext.reason;
}

unsigned GetCurrentFlagDemolitionInitiatorPlayerId()
{
    return gFlagDemolitionContext.initiatorPlayerId;
}

void LogRoadConstructed(const unsigned gf, const GameWorldBase& world, const unsigned char playerId, const MapPoint start,
                        const std::vector<Direction>& route, const RoadType roadType, const bool createdEndFlag)
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Road))
        return;

    pb::RoadLogRecord record;
    record.set_gameframe(gf);

    auto* event = record.mutable_constructed();
    event->set_player_id(static_cast<uint32_t>(playerId + 1));
    *event->mutable_start() = ToProtoPoint(start);
    *event->mutable_end() = ToProtoPoint(CalculateEndPoint(world, start, route));
    AddRoute(event, route);
    event->set_road_type(ToProtoRoadType(roadType));
    event->set_created_end_flag(createdEndFlag);

    EnqueueRecord(gf, world, std::move(record));
}

void LogRoadConstructionFailed(unsigned gf, const GameWorldBase& world, const unsigned char playerId, const MapPoint start,
                               const std::vector<Direction>& route, const RoadType roadType,
                               const RoadConstructionFailureReason reason)
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Road))
        return;

    pb::RoadLogRecord record;
    record.set_gameframe(gf);

    auto* event = record.mutable_construction_failed();
    event->set_player_id(static_cast<uint32_t>(playerId + 1));
    *event->mutable_start() = ToProtoPoint(start);
    *event->mutable_end() = ToProtoPoint(CalculateEndPoint(world, start, route));
    AddRoute(event, route);
    event->set_road_type(ToProtoRoadType(roadType));
    event->set_reason(ToProtoFailureReason(reason));

    EnqueueRecord(gf, world, std::move(record));
}

void LogRoadDemolished(unsigned gf, const GameWorldBase& world, const unsigned char playerId, const MapPoint start,
                       const MapPoint end, const std::vector<Direction>& route, const RoadType roadType)
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Road))
        return;

    pb::RoadLogRecord record;
    record.set_gameframe(gf);

    auto* event = record.mutable_demolished();
    event->set_player_id(static_cast<uint32_t>(playerId + 1));
    *event->mutable_start() = ToProtoPoint(start);
    *event->mutable_end() = ToProtoPoint(end);
    AddRoute(event, route);
    event->set_road_type(ToProtoRoadType(roadType));
    event->set_reason(ToProtoDemolitionReason(gRoadDemolitionContext.reason));
    event->set_initiator_player_id(gRoadDemolitionContext.initiatorPlayerId);

    EnqueueRecord(gf, world, std::move(record));
}

void LogFlagBuilt(unsigned gf, const GameWorldBase& world, const unsigned char playerId, const MapPoint pt,
                  const FlagBuildReason reason)
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Road))
        return;

    pb::RoadLogRecord record;
    record.set_gameframe(gf);

    auto* event = record.mutable_flag_built();
    event->set_player_id(static_cast<uint32_t>(playerId + 1));
    *event->mutable_point() = ToProtoPoint(pt);
    event->set_reason(ToProtoFlagBuildReason(reason));

    EnqueueRecord(gf, world, std::move(record));
}

void LogFlagDemolished(unsigned gf, const GameWorldBase& world, const unsigned char playerId, const MapPoint pt)
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Road))
        return;

    pb::RoadLogRecord record;
    record.set_gameframe(gf);

    auto* event = record.mutable_flag_demolished();
    event->set_player_id(static_cast<uint32_t>(playerId + 1));
    *event->mutable_point() = ToProtoPoint(pt);
    event->set_reason(ToProtoFlagDemolitionReason(gFlagDemolitionContext.reason));
    event->set_initiator_player_id(gFlagDemolitionContext.initiatorPlayerId);

    EnqueueRecord(gf, world, std::move(record));
}

} // namespace RoadEventLogger
