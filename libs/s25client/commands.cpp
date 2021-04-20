// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
