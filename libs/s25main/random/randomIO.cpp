// Copyright (c) 2017 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "randomIO.h"
#include "RttrConfig.h"
#include <boost/nowide/fstream.hpp>
#include <iomanip>

std::ostream& operator<<(std::ostream& os, const RandomEntry& entry)
{
    static const std::string rttrSrcBaseName = RttrConfig::GetSourceDir().generic_string() + "/";
    std::string strippedSrcFile = boost::filesystem::path(entry.srcName).generic_string();
    if(strippedSrcFile.find(rttrSrcBaseName) == 0)
        strippedSrcFile = strippedSrcFile.substr(rttrSrcBaseName.size());
    return os << entry.counter << ":R(" << entry.maxExcl << ")=" << entry.GetValue() << ",z=" << std::hex
              << std::setw(8) << entry.rngState << std::dec << std::setw(0) << "\t| " << strippedSrcFile << "#"
              << entry.srcLine << "\t| id=" << entry.objId;
}

void saveRandomLog(const boost::filesystem::path& filepath, const std::vector<RandomEntry>& log)
{
    boost::nowide::ofstream file(filepath);

    for(const RandomEntry& curLog : log)
        file << curLog << std::endl;
}
