// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/aijh/debug/StatsConfig.h"
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

class EventLogBatchWriter
{
public:
    explicit EventLogBatchWriter(std::string fileName, std::string header = {})
        : fileName_(std::move(fileName)), header_(std::move(header))
    {}

    ~EventLogBatchWriter() { Flush(); }

    void Append(unsigned gf, std::string line)
    {
        if(STATS_CONFIG.disableEventLogging || STATS_CONFIG.statsPath.empty())
            return;

        pendingLines_.push_back(std::move(line));

        if(!hasNextFlushGF_)
        {
            nextFlushGF_ = kFlushPeriodGF;
            hasNextFlushGF_ = true;
        }

        if(gf >= nextFlushGF_)
        {
            Flush();
            while(gf >= nextFlushGF_)
                nextFlushGF_ += kFlushPeriodGF;
        }
    }

    void Flush()
    {
        if(pendingLines_.empty())
            return;

        std::ofstream log = Open();
        if(!log)
            return;

        if(log.tellp() == 0 && !header_.empty())
            log << header_ << std::endl;

        for(const std::string& line : pendingLines_)
            log << line << std::endl;

        pendingLines_.clear();
    }

private:
    std::ofstream Open() const
    {
        if(STATS_CONFIG.disableEventLogging || STATS_CONFIG.statsPath.empty())
            return {};

        const boost::filesystem::path path = boost::filesystem::path(STATS_CONFIG.statsPath) / fileName_;
        return std::ofstream(path.string(), std::ios::app);
    }

    static constexpr unsigned kFlushPeriodGF = 500;

    std::string fileName_;
    std::string header_;
    std::vector<std::string> pendingLines_;
    unsigned nextFlushGF_ = 0;
    bool hasNextFlushGF_ = false;
};
