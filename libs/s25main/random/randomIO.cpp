// Copyright (c) 2017 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "randomIO.h"
#include "RttrConfig.h"
#include <boost/nowide/fstream.hpp>
#include <iomanip>

std::ostream& operator<<(std::ostream& os, const RandomEntry& entry)
{
    static const std::string rttrSrcBaseName = RttrConfig::GetSourceDir().generic_string() + "/";
    std::string strippedSrcFile = boost::filesystem::path(entry.src_name).generic_string();
    if(strippedSrcFile.find(rttrSrcBaseName) == 0)
        strippedSrcFile = strippedSrcFile.substr(rttrSrcBaseName.size());
    return os << entry.counter << ":R(" << entry.max << ")=" << entry.GetValue() << ",z=" << std::hex << std::setw(8)
              << entry.rngState << std::dec << std::setw(0) << "\t| " << strippedSrcFile << "#" << entry.src_line
              << "\t| id=" << entry.obj_id;
}

void saveRandomLog(const boost::filesystem::path& filepath, const std::vector<RandomEntry>& log)
{
    boost::nowide::ofstream file(filepath);

    for(const RandomEntry& curLog : log)
        file << curLog << std::endl;
}
