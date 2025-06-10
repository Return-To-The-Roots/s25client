//
// Created by pavel on 09.06.25.
//

#ifndef WEIGHTPARSER_H
#define WEIGHTPARSER_H
#include "WeightParams.h"
#include <yaml-cpp/yaml.h>

namespace Weights {
    ProximityParams parseProximityParams(const YAML::Node& node, const ProximityParams& defaults);
    LocationParams parseLocationParams(const YAML::Node& node, const LocationParams& defaults);
    BuildParams parseBuildParams(const YAML::Node& node, const BuildParams& defaults);
    WantedParams parseWantedParams(const YAML::Node& node, WantedParams params);
}
#endif // WEIGHTPARSER_H
