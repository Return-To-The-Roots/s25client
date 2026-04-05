// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "WareEventLogger.h"

#include "ai/aijh/debug/StatsConfig.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "wares_log.pb.h"
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <string>
#include <vector>

namespace {
namespace pb = ru::pkopachevsky::proto;

unsigned gLastLoggedGF = 0;
bool gHasLastLoggedGF = false;
std::vector<pb::WaresLogRecord> gPendingRecords;
unsigned gNextFlushGF = 0;
bool gHasNextFlushGF = false;
constexpr unsigned kFlushPeriodGF = 500;

std::ofstream OpenWaresLog()
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Ware))
        return {};
    const boost::filesystem::path path = boost::filesystem::path(STATS_CONFIG.statsPath) / "wares_log.pb";
    return std::ofstream(path.string(), std::ios::binary | std::ios::app);
}

pb::GoodType ToProtoGoodType(const ::GoodType good)
{
    const int raw = static_cast<int>(good);
    if(raw >= static_cast<int>(::GoodType::Beer) && raw <= static_cast<int>(::GoodType::Wine))
        return static_cast<pb::GoodType>(raw + 1);
    return pb::GoodType::GOOD_TYPE_UNSPECIFIED;
}

bool WriteDelimitedRecord(std::ostream& os, const pb::WaresLogRecord& record)
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

    std::ofstream log = OpenWaresLog();
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

} // namespace

namespace WareEventLogger {

void LogInventoryChange(unsigned gf, unsigned char playerId, GoodType good, int count)
{
    if(!STATS_CONFIG.IsEventLoggerEnabled(EventLoggerType::Ware))
        return;

    pb::WaresLogRecord record;
    if(gHasLastLoggedGF && gf >= gLastLoggedGF)
        record.set_delta_gf(gf - gLastLoggedGF);
    else if(!gHasLastLoggedGF)
        record.set_delta_gf(gf);
    else
        record.set_delta_gf(0);
    gLastLoggedGF = gf;
    gHasLastLoggedGF = true;

    record.set_player_id(static_cast<uint32_t>(playerId + 1));
    record.set_good_type(ToProtoGoodType(good));
    record.set_delta(count);
    gPendingRecords.push_back(record);

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

} // namespace WareEventLogger
