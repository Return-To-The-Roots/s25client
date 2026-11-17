// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
#include "iwMsgbox.h"
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
    : IngameWindow(CGI_SAVE, IngameWindow::posLastOrCenter, Extent(700, 400 + addHeight), window_title,
                   LOADER.getImageN("resource", 41))
{
    using SRT = ctrlTable::SortType;
    addTable(ID_tblSaveGames, DrawPoint(20, 30), Extent(660, 300), TextureColor::Green2, NormalFont,
             ctrlTable::Columns{{_("Filename"), 320, SRT::String},
                                {_("Map"), 250, SRT::String},
                                {_("Time"), 160, SRT::Date},
                                {_("Game Time"), 115, SRT::Time},
                                {}});

    addText(ID_txtSaveFolder, DrawPoint(20, 333), RTTRCONFIG.expandPath(s25::folders::save).string(), COLOR_YELLOW,
            FontStyle::TOP, SmallFont)
      ->setMaxWidth(510);
    // maxLength 251 = 255 filename limit - 4 chars for ".sav"; just discourages absurdly long
    // input, isValidFileName() may still reject it since it counts bytes, not codepoints.
    addEdit(ID_edtFilename, DrawPoint(20, 350), Extent(510, 22), TextureColor::Green2, NormalFont, 251);
    addImageButton(ID_btSaveOrLoad, DrawPoint(540, 341), Extent(40, 40), TextureColor::Green2, btImg);
    // Initially fill the table
    refreshTable();
}

void iwSaveLoad::msgEditEnter(const unsigned /*ctrl_id*/)
{
    saveLoad();
}

void iwSaveLoad::msgButtonClick(const unsigned ctrl_id)
{
    RTTR_Assert(ctrl_id == ID_btSaveOrLoad);
    saveLoad();
}

void iwSaveLoad::msgTableSelectItem(const unsigned /*ctrl_id*/, const std::optional<unsigned>& selection)
{
    // On selecting a table entry put the filename into the edit control
    getCtrl<ctrlEdit>(ID_edtFilename)
      ->setText(selection ? getCtrl<ctrlTable>(ID_tblSaveGames)->getItemText(*selection, 0) : "");
}

void iwSaveLoad::refreshTable()
{
    static bool loadedOnce = false;

    auto* table = getCtrl<ctrlTable>(ID_tblSaveGames);

    table->deleteAllItems();

    std::vector<boost::filesystem::path> saveFiles = listDir(RTTRCONFIG.expandPath(s25::folders::save), "sav");
    for(const auto& saveFile : saveFiles)
    {
        Savegame save;
        if(!save.load(saveFile, SaveGameDataToLoad::Header))
        {
            // Show errors only first time this is loaded
            if(!loadedOnce)
            {
                LOG.write(_("Invalid Savegame %1%! Reason: %2%\n")) % saveFile
                  % (save.getLastErrorMsg().empty() ? _("Unknown") : save.getLastErrorMsg());
            }
            continue;
        }

        const auto fileName = saveFile.stem().string();
        const std::string dateStr = s25util::Time::FormatTime("%d.%m.%Y - %H:%i", save.getSaveTime());
        const std::string gameTime = GAMECLIENT.formatGFTime(save.start_gf);

        table->addRow({fileName, save.getMapName(), dateStr, gameTime, saveFile.string()});
    }

    // Sort by time
    table->sortRows(2, TableSortDir::Descending);
    loadedOnce = true;
}

void iwSave::saveLoad()
{
    const auto fileNameResult = getCtrl<ctrlEdit>(ID_edtFilename)->getFileName(".sav");
    switch(fileNameResult.status)
    {
        case FileNameStatus::Empty:
            WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Invalid Filename"), _("Please enter a filename."), this,
                                                          MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
            return;
        case FileNameStatus::Invalid:
            WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Invalid Filename"), _("Please enter a valid filename."),
                                                          this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
            return;
        case FileNameStatus::Valid: break;
    }
    GAMECLIENT.saveToFile(RTTRCONFIG.expandPath(s25::folders::save) / fileNameResult.name);

    refreshTable();
    getCtrl<ctrlEdit>(ID_edtFilename)->setText("");
}

iwSave::iwSave() : iwSaveLoad(_("Save game!"), LOADER.getTextureN("io", 47), 30)
{
    auto* fileNameEdit = getCtrl<ctrlEdit>(ID_edtFilename);
    fileNameEdit->setType(EditType::Filename);
    DrawPoint pos(getSize().x / 2, fileNameEdit->getPos().y + fileNameEdit->getSize().y + 10);

    ctrlComboBox* combo =
      addComboBox(ID_cbAutoSaveInterval, pos, Extent(130, 22), TextureColor::Green2, NormalFont, 100);
    pos += DrawPoint(-5, 11);
    addText(ID_txtAutoSave, pos, _("Auto-Save every:"), 0xFFFFFF00, FontStyle::RIGHT | FontStyle::VCENTER, NormalFont);

    // Add intervals
    combo->addItem(_("Disabled"));
    for(const std::chrono::minutes interval : AUTO_SAVE_INTERVALS)
        combo->addItem((boost::format(_("%1% min")) % interval.count()).str());
    // Last entry is only for debugging
    if(SETTINGS.global.debugMode)
        combo->addItem(_("Every GF"));

    // Select interval
    combo->setSelection(0); // Use disabled by default and change if possible
    if(SETTINGS.interface.autosaveInterval == 1)
        combo->setSelection(AUTO_SAVE_INTERVALS.size() + 1);
    else
    {
        // Start selection index at 1, 0 is "disabled"
        for(const auto& i : AUTO_SAVE_INTERVALS | boost::adaptors::indexed(1))
        {
            if(SETTINGS.interface.autosaveInterval == durationToGfs(i.value()))
            {
                combo->setSelection(static_cast<unsigned>(i.index()));
                break;
            }
        }
    }
}

void iwSave::msgComboSelectItem(const unsigned /*ctrl_id*/, const unsigned selection)
{
    if(selection == 0) // First entry is "disabled"
        SETTINGS.interface.autosaveInterval = 0;
    else if(selection > AUTO_SAVE_INTERVALS.size()) // Last entry is "every GF" (in debug mode)
        SETTINGS.interface.autosaveInterval = 1;
    else
    {
        // selection is the index into the array ignoring the first ("disabled") entry
        RTTR_Assert(selection >= 1 && selection <= AUTO_SAVE_INTERVALS.size());
        SETTINGS.interface.autosaveInterval = durationToGfs(AUTO_SAVE_INTERVALS[selection - 1]);
    }
}

iwLoad::iwLoad(CreateServerInfo csi) : iwSaveLoad(_("Load game!"), LOADER.getTextureN("io", 48)), csi(std::move(csi)) {}

void iwLoad::saveLoad()
{
    const auto* table = getCtrl<ctrlTable>(ID_tblSaveGames);
    if(!table->getSelection())
        return;

    if(GAMECLIENT.hostGame(csi, {table->getItemText(*table->getSelection(), 4), MapType::Savegame}))
    {
        std::unique_ptr<ILobbyClient> lobbyClient;
        if(csi.type == ServerType::Lobby)
            lobbyClient = std::make_unique<RttrLobbyClient>(LOBBYCLIENT);
        WINDOWMANAGER.show(std::make_unique<iwConnecting>(csi.type, std::move(lobbyClient)));
    } else
    {
        if(LOBBYCLIENT.IsLoggedIn())
            WINDOWMANAGER.switchTo(std::make_unique<dskLobby>());
        else
            close();
    }
}

/// Handle double click on the table
void iwLoad::msgTableChooseItem(const unsigned /*ctrl_id*/, const unsigned /*selection*/)
{
    saveLoad();
}
