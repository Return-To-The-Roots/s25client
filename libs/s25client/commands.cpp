// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "commands.h"
#include "RttrConfig.h"
#include "convertSounds.h"
#include "files.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/ArchivItem_Sound_Wave.h"
#include "libsiedler2/libsiedler2.h"
#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <string>

void convertAndSaveSounds(const RttrConfig& config, const boost::filesystem::path& targetFolder)
{
    libsiedler2::Archiv sounds;
    if(libsiedler2::Load(config.ExpandPath(s25::files::soundOrig), sounds) != 0)
        throw std::runtime_error("Could not load sounds");
    convertSounds(sounds, config.ExpandPath(s25::files::soundScript));
    boost::filesystem::create_directories(targetFolder);
    for(unsigned i = 0; i < sounds.size(); i++)
    {
        const auto* wav = dynamic_cast<const libsiedler2::ArchivItem_Sound_Wave*>(sounds[i]);
        if(wav)
        {
            boost::nowide::ofstream f(targetFolder / (std::to_string(i) + ".wav"));
            if(wav->write(f) != 0)
                throw std::runtime_error("Could not write sound" + std::to_string(i));
        }
    }
}
