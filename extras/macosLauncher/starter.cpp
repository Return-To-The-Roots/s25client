// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RttrConfig.h"
#include <s25util/System.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <cstdio>
#include <iostream>
#include <stdexcept>

namespace bfs = boost::filesystem;

void copyDirectory(const bfs::path& sourceDir, const bfs::path& destinationDir)
{
    if(!bfs::is_directory(sourceDir))
        throw std::runtime_error("Directory " + sourceDir.string() + " does not exist");
    bfs::create_directories(destinationDir);

    for(const auto& it : bfs::recursive_directory_iterator(sourceDir))
    {
        const bfs::path& curPath = it.path();
        std::string relativePathStr = curPath.string();
        boost::replace_first(relativePathStr, sourceDir.string(), "");
        bfs::copy(curPath, destinationDir / relativePathStr);
    }
}

bool isS2Installed()
{
    const bfs::path fileA = RTTRCONFIG.ExpandPath("<RTTR_GAME>/DATA/CREDITS.LST");
    const bfs::path fileB = RTTRCONFIG.ExpandPath("<RTTR_GAME>/GFX/PALETTE/PAL5.BBM");
    return bfs::exists(fileA) && bfs::exists(fileB);
}

int main(int argc, char* argv[])
{
    if(!RTTRCONFIG.Init())
    {
        std::cerr << "Failed to init program!" << std::endl;
        return 1;
    }
    if(argc <= 1 || argv[1] != std::string("interminal"))
    {
        try
        {
            const bfs::path tmpPath = "/tmp/rttr.command";
            boost::nowide::ofstream file(tmpPath);
            file << "\"" << RTTRCONFIG.ExpandPath("<RTTR_BIN>/starter") << "\" interminal" << std::endl;
            file << "rm " << tmpPath << std::endl;
            file.close();
            bfs::permissions(tmpPath, bfs::owner_all);
            bfs::current_path(RTTRCONFIG.ExpandPath("<RTTR_BIN>"));
            system("open rttr.terminal");
        } catch(const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        return 0;
    }

    if(!isS2Installed())
    {
        const bfs::path S2CD = "/Volumes/S2_GOLD";
        while(!bfs::is_directory(S2CD) && !isS2Installed())
        {
            std::cerr << "Couldn't find data files for Settlers II" << std::endl;
            std::cerr << "Please copy the folders \"DATA\" and \"GFX\" from your Settlers II install to" << std::endl;
            std::cerr << "\"" << RTTRCONFIG.ExpandPath("<RTTR_GAME>") << "\" or" << std::endl;
            std::cerr << "insert the Settlers II Gold CD in your drive and hit \"Enter\"." << std::endl;
            std::cin.ignore();
        }
        if(!isS2Installed())
        {
            std::cout << "Copying files..." << std::endl;
            try
            {
                copyDirectory(S2CD / "DATA", RTTRCONFIG.ExpandPath("<RTTR_GAME>/DATA"));
            } catch(const std::runtime_error& e)
            {
                std::cerr << "Error copying DATA dir: " << e.what() << std::endl;
                return 1;
            }
            try
            {
                copyDirectory(S2CD / "GFX", RTTRCONFIG.ExpandPath("<RTTR_GAME>/GFX"));
            } catch(const std::runtime_error& e)
            {
                std::cerr << "Error copying GFX dir: " << e.what() << std::endl;
                return 1;
            }
            std::cout << "Done" << std::endl;
        }
    }
    if(argc <= 2 || argv[2] != std::string("noupdate"))
    {
        bfs::path updaterPath = RTTRCONFIG.ExpandPath("<RTTR_EXTRA_BIN>/s25update");
        if(bfs::exists(updaterPath))
            System::execute(updaterPath, "--dir \"../../../\" -v");
        else
            std::cerr << "Updater not found at " << updaterPath << std::endl;
    }
    System::execute(RTTRCONFIG.ExpandPath("<RTTR_BIN>/s25client"));
}
