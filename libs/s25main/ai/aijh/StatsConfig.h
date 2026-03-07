#pragma once

#ifndef STATSCONFIG_H
#define STATSCONFIG_H
#include <string>

struct StatsConfig
{
    std::string runId = "000";
    std::string profileId = "000";
    std::string runSetId = "000";
    unsigned stats_period = 2500;
    unsigned save_period = 2500;
    unsigned debug_stats_period = 2500;
    unsigned minimap_period = 2500;

    std::string outputPath = "/home/pavel/s2/manual/runsets/";
    std::string statsPath = "/home/pavel/s2/manual/stats/";
    std::string savesPath = "/home/pavel/s2/manual/saves/";
    std::string screensPath = "/home/pavel/s2/manual/screens/";
    bool disableEventLogging = false;

    std::string weightsPath = "/home/pavel/s2/volume/config/Version_AAAAA.yaml";
};

extern StatsConfig STATS_CONFIG;

inline bool IsStatsPeriodHit(unsigned currentGF, unsigned period)
{
    return period > 0 && (currentGF % period) == 0u;
}
#endif
