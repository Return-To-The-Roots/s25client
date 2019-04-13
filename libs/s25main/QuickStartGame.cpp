// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "QuickStartGame.h"
#include "Settings.h"
#include "WindowManager.h"
#include "desktops/dskGameLoader.h"
#include "desktops/dskSelectMap.h"
#include "ingameWindows/iwPleaseWait.h"
#include "network/ClientInterface.h"
#include "network/CreateServerInfo.h"
#include "network/GameClient.h"
#include <boost/filesystem/path.hpp>
#include <algorithm>
#include <iostream>

class SwitchOnStart : public ClientInterface
{
public:
    SwitchOnStart() { GAMECLIENT.SetInterface(this); }
    ~SwitchOnStart() override { GAMECLIENT.RemoveInterface(this); }

    void CI_GameLoading(const std::shared_ptr<Game>& game) override { WINDOWMANAGER.Switch(std::make_unique<dskGameLoader>(game)); }
};

bool QuickStartGame(const std::string& filePath, bool singlePlayer)
{
    CreateServerInfo csi(singlePlayer ? ServerType::LOCAL : ServerType::DIRECT, SETTINGS.server.localPort, _("Unlimited Play"));

    std::cout << "Loading game!" << std::endl;
    std::string extension = bfs::path(filePath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    WINDOWMANAGER.Switch(std::make_unique<dskSelectMap>(csi));

    if((extension == ".sav" && GAMECLIENT.HostGame(csi, filePath, MAPTYPE_SAVEGAME))
       || ((extension == ".swd" || extension == ".wld") && GAMECLIENT.HostGame(csi, filePath, MAPTYPE_OLDMAP)))
    {
        WINDOWMANAGER.ShowAfterSwitch(std::make_unique<iwPleaseWait>());
        return true;
    } else
    {
        SwitchOnStart switchOnStart;
        return GAMECLIENT.StartReplay(filePath);
    }
}
