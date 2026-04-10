#pragma once

#ifndef STATSCONFIG_H
#define STATSCONFIG_H
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <set>
#include <string>

enum class EventLoggerType
{
    Building,
    Combat,
    Country,
    CountryPlot,
    Military,
    Road,
    ToolPriority,
    TroopsLimit,
    Ware,
};

inline const char* GetEventLoggerCliName(const EventLoggerType loggerType)
{
    switch(loggerType)
    {
        case EventLoggerType::Building: return "building";
        case EventLoggerType::Combat: return "combat";
        case EventLoggerType::Country: return "country";
        case EventLoggerType::CountryPlot: return "country-plot";
        case EventLoggerType::Military: return "military";
        case EventLoggerType::Road: return "road";
        case EventLoggerType::ToolPriority: return "tool-priority";
        case EventLoggerType::TroopsLimit: return "troops-limit";
        case EventLoggerType::Ware: return "ware";
    }

    return "unknown";
}

inline std::string GetSupportedEventLoggerNames()
{
    return "building, combat, country, country-plot, military, road, tool-priority, troops-limit, ware";
}

inline bool TryParseEventLoggerType(std::string loggerName, EventLoggerType& out)
{
    std::transform(loggerName.begin(), loggerName.end(), loggerName.begin(),
                   [](const unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    loggerName.erase(
      std::remove_if(loggerName.begin(), loggerName.end(),
                     [](const unsigned char ch) { return ch == '-' || ch == '_' || std::isspace(ch) != 0; }),
      loggerName.end());

    constexpr std::size_t kEventLoggerSuffixLength = 11;
    if(loggerName.size() > kEventLoggerSuffixLength
       && loggerName.compare(loggerName.size() - kEventLoggerSuffixLength, kEventLoggerSuffixLength,
                             "eventlogger")
            == 0)
    {
        loggerName.erase(loggerName.size() - kEventLoggerSuffixLength);
    }

    if(loggerName == "building")
        out = EventLoggerType::Building;
    else if(loggerName == "combat")
        out = EventLoggerType::Combat;
    else if(loggerName == "country")
        out = EventLoggerType::Country;
    else if(loggerName == "countryplot" || loggerName == "countryplots")
        out = EventLoggerType::CountryPlot;
    else if(loggerName == "military")
        out = EventLoggerType::Military;
    else if(loggerName == "road" || loggerName == "roads")
        out = EventLoggerType::Road;
    else if(loggerName == "toolpriority")
        out = EventLoggerType::ToolPriority;
    else if(loggerName == "troopslimit")
        out = EventLoggerType::TroopsLimit;
    else if(loggerName == "ware" || loggerName == "wares")
        out = EventLoggerType::Ware;
    else
        return false;

    return true;
}

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
    std::string screensPath = "/home/pavel/s2/manual/minimaps/";
    bool disableEventLogging = false;
    std::set<EventLoggerType> enabledEventLoggers;

    std::string weightsPath = []() -> std::string {
        const char* env = std::getenv("RTTR_WEIGHTS_PATH");
        return env ? env : "";
    }();

    bool IsEventLoggerEnabled(const EventLoggerType loggerType) const
    {
        return !disableEventLogging && !statsPath.empty()
               && (enabledEventLoggers.empty() || enabledEventLoggers.count(loggerType) != 0);
    }
};

extern StatsConfig STATS_CONFIG;

inline bool IsStatsPeriodHit(unsigned currentGF, unsigned period)
{
    return period > 0 && (currentGF % period) == 0u;
}
#endif
