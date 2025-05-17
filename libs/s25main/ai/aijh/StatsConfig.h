#pragma once

#ifndef STATSCONFIG_H
#define STATSCONFIG_H
#include <string>

struct StatsConfig
{
    std::string runId = "000";
    std::string runSetId = "000";
    unsigned stats_period = 500;
    unsigned save_period = 500;
    unsigned debug_stats_period = 500;

    std::string outputPath = "/home/pavel/s2/manual/runsets/";
    std::string statsPath = "/home/pavel/s2/manual/stats/";
    std::string savesPath = "/home/pavel/s2/manual/saves/";
};

extern StatsConfig STATS_CONFIG;
#endif