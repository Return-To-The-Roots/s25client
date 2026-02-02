// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwSave.h"
#include "ListDir.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "RttrLobbyClient.hpp"
#include "Savegame.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlTable.h"
#include "controls/ctrlText.h"
#include "desktops/dskLobby.h"
#include "files.h"
#include "helpers/make_array.h"
#include "helpers/toString.h"
#include "iwConnecting.h"
#include "network/GameClient.h"
#include "gameData/GameConsts.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/LobbyClient.h"
#include "s25util/Log.h"
#include <boost/range/adaptors.hpp>
#include <utility>

namespace {
enum
{
    ID_tblSaveGames,
    ID_edtFilename,
    ID_btSaveOrLoad,
    ID_txtAutoSave,
    ID_cbAutoSaveInterval,
    ID_txtSaveFolder,
};

using namespace std::chrono_literals;
constexpr std::array AUTO_SAVE_INTERVALS{1min, 5min, 10min, 15min, 30min, 60min, 90min};
} // namespace

iwSaveLoad::iwSaveLoad(const std::string& window_title, ITexture* btImg, const unsigned addHeight)
    : IngameWindow(CGI_SAVE, IngameWindow::posLastOrCenter, Extent(600, 400 + addHeight), window_title,
                   LOADER.GetImageN("resource", 41))
{
    using SRT = ctrlTable::SortType;
    AddTable(ID_tblSaveGames, DrawPoint(20, 30), Extent(560, 300), TextureColor::Green2, NormalFont,
             ctrlTable::Columns{{_("Filename"), 270, SRT::String},
                                {_("Map"), 250, SRT::String},
                                {_("Time"), 250, SRT::Date},
                                {_("Game Time"), 320, SRT::Time},
                                {}});

    AddText(ID_txtSaveFolder, DrawPoint(20, 333), RTTRCONFIG.ExpandPath(s25::folders::save).string(), COLOR_YELLOW,
            FontStyle::TOP, SmallFont)
      ->setMaxWidth(510);
    AddEdit(ID_edtFilename, DrawPoint(20, 350), Extent(510, 22), TextureColor::Green2, NormalFont);
    AddImageButton(ID_btSaveOrLoad, DrawPoint(540, 341), Extent(40, 40), TextureColor::Green2, btImg);
    // Initially fill the table
    RefreshTable();
}

void iwSaveLoad::Msg_EditEnter(const unsigned /*ctrl_id*/)
{
    SaveLoad();
}

void iwSaveLoad::Msg_ButtonClick(const unsigned ctrl_id)
{
    RTTR_Assert(ctrl_id == ID_btSaveOrLoad);
    SaveLoad();
}

void iwSaveLoad::Msg_TableSelectItem(const unsigned /*ctrl_id*/, const boost::optional<unsigned>& selection)
{
    // On selecting a table entry put the filename into the edit control
    GetCtrl<ctrlEdit>(ID_edtFilename)
      ->SetText(selection ? GetCtrl<ctrlTable>(ID_tblSaveGames)->GetItemText(*selection, 0) : "");
}

void iwSaveLoad::RefreshTable()
{
    static bool loadedOnce = false;

    auto* table = GetCtrl<ctrlTable>(ID_tblSaveGames);

    table->DeleteAllItems();

    std::vector<boost::filesystem::path> saveFiles = ListDir(RTTRCONFIG.ExpandPath(s25::folders::save), "sav");
    for(const auto& saveFile : saveFiles)
    {
        Savegame save;
        if(!save.Load(saveFile, SaveGameDataToLoad::Header))
        {
            // Show errors only first time this is loaded
            if(!loadedOnce)
            {
                LOG.write(_("Invalid Savegame %1%! Reason: %2%\n")) % saveFile
                  % (save.GetLastErrorMsg().empty() ? _("Unknown") : save.GetLastErrorMsg());
            }
            continue;
        }

        const auto fileName = saveFile.stem().string();
        const std::string dateStr = s25util::Time::FormatTime("%d.%m.%Y - %H:%i", save.GetSaveTime());
        const std::string gameTime = GAMECLIENT.FormatGFTime(save.start_gf);

        table->AddRow({fileName, save.GetMapName(), dateStr, gameTime, saveFile.string()});
    }

    // Sort by time
    table->SortRows(2, TableSortDir::Descending);
    loadedOnce = true;
}

void iwSave::SaveLoad()
{
    const boost::filesystem::path savePath =
      RTTRCONFIG.ExpandPath(s25::folders::save) / (GetCtrl<ctrlEdit>(ID_edtFilename)->GetText() + ".sav");
    GAMECLIENT.SaveToFile(savePath);

    RefreshTable();
    GetCtrl<ctrlEdit>(ID_edtFilename)->SetText("");
}

iwSave::iwSave() : iwSaveLoad(_("Save game!"), LOADER.GetTextureN("io", 47), 30)
{
    const auto* fileNameEdit = GetCtrl<ctrlEdit>(ID_edtFilename);
    DrawPoint pos(GetSize().x / 2, fileNameEdit->GetPos().y + fileNameEdit->GetSize().y + 10);

    ctrlComboBox* combo =
      AddComboBox(ID_cbAutoSaveInterval, pos, Extent(130, 22), TextureColor::Green2, NormalFont, 100);
    pos += DrawPoint(-5, 11);
    AddText(ID_txtAutoSave, pos, _("Auto-Save every:"), 0xFFFFFF00, FontStyle::RIGHT | FontStyle::VCENTER, NormalFont);

    // Add intervals
    combo->AddString(_("Disabled"));
    for(const std::chrono::minutes interval : AUTO_SAVE_INTERVALS)
        combo->AddString((boost::format(_("%1% min")) % interval.count()).str());
    // Last entry is only for debugging
    if(SETTINGS.global.debugMode)
        combo->AddString(_("Every GF"));

    // Select interval
    combo->SetSelection(0); // Use disabled by default and change if possible
    if(SETTINGS.interface.autosave_interval == 1)
        combo->SetSelection(AUTO_SAVE_INTERVALS.size() + 1);
    else
    {
        // Start selection index at 1, 0 is "disabled"
        for(const auto& i : AUTO_SAVE_INTERVALS | boost::adaptors::indexed(1))
        {
            if(SETTINGS.interface.autosave_interval == duration_to_gfs(i.value()))
            {
                combo->SetSelection(static_cast<unsigned>(i.index()));
                break;
            }
        }
    }
}

void iwSave::Msg_ComboSelectItem(const unsigned /*ctrl_id*/, const unsigned selection)
{
    if(selection == 0) // First entry is "disabled"
        SETTINGS.interface.autosave_interval = 0;
    else if(selection > AUTO_SAVE_INTERVALS.size()) // Last entry is "every GF" (in debug mode)
        SETTINGS.interface.autosave_interval = 1;
    else
    {
        // selection is the index into the array ignoring the first ("disabled") entry
        RTTR_Assert(selection >= 1 && selection <= AUTO_SAVE_INTERVALS.size());
        SETTINGS.interface.autosave_interval = duration_to_gfs(AUTO_SAVE_INTERVALS[selection - 1]);
    }
}

iwLoad::iwLoad(CreateServerInfo csi) : iwSaveLoad(_("Load game!"), LOADER.GetTextureN("io", 48)), csi(std::move(csi)) {}

void iwLoad::SaveLoad()
{
    const auto* table = GetCtrl<ctrlTable>(ID_tblSaveGames);
    if(!table->GetSelection())
        return;

    if(GAMECLIENT.HostGame(csi, {table->GetItemText(*table->GetSelection(), 4), MapType::Savegame}))
    {
        std::unique_ptr<ILobbyClient> lobbyClient;
        if(csi.type == ServerType::Lobby)
            lobbyClient = std::make_unique<RttrLobbyClient>(LOBBYCLIENT);
        WINDOWMANAGER.Show(std::make_unique<iwConnecting>(csi.type, std::move(lobbyClient)));
    } else
    {
        if(LOBBYCLIENT.IsLoggedIn())
            WINDOWMANAGER.Switch(std::make_unique<dskLobby>());
        else
            Close();
    }
}

/// Handle double click on the table
void iwLoad::Msg_TableChooseItem(const unsigned /*ctrl_id*/, const unsigned /*selection*/)
{
    SaveLoad();
}
