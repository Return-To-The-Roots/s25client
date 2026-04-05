// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CountryPlotEventLogger.h"

#include "RttrForeachPt.h"
#include "ai/aijh/debug/StatsConfig.h"
#include "country_plots_log.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "world/GameWorldBase.h"
#include "world/World.h"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace {
namespace bfs = boost::filesystem;
namespace pb = ru::pkopachevsky::proto;

struct HeaderInfo
{
    uint32_t mapWidth = 0;
    uint32_t mapHeight = 0;
    uint32_t numPlayers = 0;
    bool isValid = false;
};

HeaderInfo gHeaderInfo;
bool gHeaderWritten = false;
std::vector<pb::CountryPlotsLogRecord> gPendingRecords;
unsigned gNextFlushGF = 0;
bool gHasNextFlushGF = false;
constexpr unsigned kFlushPeriodGF = 500;

bfs::path GetCountryPlotsLogPath()
{
    return bfs::path(STATS_CONFIG.statsPath) / "country_plots_log.pb";
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
    gHeaderInfo.numPlayers = world.GetNumPlayers();
    gHeaderInfo.isValid = true;
}

bool EnsureHeaderWritten(std::ofstream& log)
{
    if(gHeaderWritten)
        return true;
    if(!gHeaderInfo.isValid)
        return false;

    const bfs::path path = GetCountryPlotsLogPath();
    const bool needsHeader = !bfs::exists(path) || bfs::file_size(path) == 0;
    if(needsHeader)
    {
        pb::CountryPlotsLogHeader header;
        header.set_map_width(gHeaderInfo.mapWidth);
        header.set_map_height(gHeaderInfo.mapHeight);
        header.set_num_players(gHeaderInfo.numPlayers);
        if(!WriteDelimitedMessage(log, header))
            return false;
    }

    gHeaderWritten = true;
    return true;
}

std::ofstream OpenCountryPlotsLog()
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::CountryPlot))
        return {};
    return std::ofstream(GetCountryPlotsLogPath().string(), std::ios::binary | std::ios::app);
}

void FlushPendingRecords()
{
    if(gPendingRecords.empty())
        return;

    std::ofstream log = OpenCountryPlotsLog();
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

pb::CountryPlotsLogRecord CreateRecord(unsigned gf, MapPoint origin, MapExtent size)
{
    pb::CountryPlotsLogRecord record;
    record.set_gameframe(gf);
    record.set_origin_x(origin.x);
    record.set_origin_y(origin.y);
    record.set_width(size.x);
    record.set_height(size.y);
    return record;
}

void EnqueueRecord(unsigned gf, pb::CountryPlotsLogRecord&& record)
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

namespace CountryPlotEventLogger {

void LogInitialCountryPlots(unsigned gf, const GameWorldBase& world)
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::CountryPlot))
        return;

    RememberHeaderInfo(world);

    std::vector<OwnershipChange> changes;
    changes.reserve(static_cast<std::size_t>(world.GetWidth()) * world.GetHeight());
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        const uint8_t owner = world.GetNode(pt).owner;
        if(owner == 0)
            continue;
        changes.push_back(OwnershipChange{Position(pt), 0, owner});
    }

    LogCountryPlotChanges(gf, world, MapPoint(0, 0), world.GetSize(), changes);
}

void LogCountryPlotChanges(unsigned gf, const GameWorldBase& world, MapPoint origin, MapExtent size,
                           const std::vector<OwnershipChange>& changes)
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::CountryPlot) || changes.empty())
        return;

    RememberHeaderInfo(world);

    using TransitionKey = std::pair<uint8_t, uint8_t>;
    std::map<TransitionKey, std::vector<uint32_t>> groupedIndices;
    for(const OwnershipChange& change : changes)
    {
        const uint32_t localIndex = static_cast<uint32_t>(change.localPos.y) * size.x + change.localPos.x;
        groupedIndices[{change.oldOwner, change.newOwner}].push_back(localIndex);
    }

    pb::CountryPlotsLogRecord record = CreateRecord(gf, origin, size);
    for(const auto& [owners, indices] : groupedIndices)
    {
        auto* transition = record.add_transitions();
        transition->set_old_owner_id(owners.first);
        transition->set_new_owner_id(owners.second);

        uint32_t previousIndex = 0;
        bool first = true;
        for(const uint32_t index : indices)
        {
            transition->add_cell_index_deltas(first ? index : index - previousIndex);
            previousIndex = index;
            first = false;
        }
    }

    EnqueueRecord(gf, std::move(record));
}

} // namespace CountryPlotEventLogger
