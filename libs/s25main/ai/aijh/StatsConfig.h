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

    std::string outputPath = "/home/pavel/Documents/settlers_ai/runsets/";
    std::string statsPath = "/home/pavel/Documents/settlers_ai/stats/";
    std::string savesPath = "/home/pavel/Documents/settlers_ai/saves/";
};

extern StatsConfig STATS_CONFIG;
#endif