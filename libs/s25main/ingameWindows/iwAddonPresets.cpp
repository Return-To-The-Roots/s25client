// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwAddonPresets.h"
#include "ListDir.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "WindowManager.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlTable.h"
#include "controls/ctrlText.h"
#include "files.h"
#include "helpers/format.hpp"
#include "iwMsgbox.h"
#include "gameData/const_gui_ids.h"
#include "libsiedler2/ArchivItem_Ini.h"
#include "libsiedler2/ArchivItem_Text.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/Log.h"
#include "s25util/StringConversion.h"
#include <boost/filesystem.hpp>
#include <optional>

namespace bfs = boost::filesystem;

static bfs::path GetPresetsDir()
{
    return RTTRCONFIG.ExpandPath(s25::folders::addonPresets);
}

static std::optional<std::map<unsigned, unsigned>> LoadPresetsFromFile(const bfs::path& filePath)
{
    libsiedler2::Archiv archive;
    if(libsiedler2::Load(filePath, archive) != 0)
    {
        LOG.write("Failed to load addon preset from %1%\n") % filePath;
        return std::nullopt;
    }

    const auto* ini = dynamic_cast<const libsiedler2::ArchivItem_Ini*>(archive.find("addons"));
    if(!ini)
        return std::nullopt;

    std::map<unsigned, unsigned> states;
    for(unsigned i = 0; i < ini->size(); ++i)
    {
        const auto* item = dynamic_cast<const libsiedler2::ArchivItem_Text*>(ini->get(i));
        if(!item)
        {
            LOG.write("Skipping addon preset %1%: entry #%2% is not a text entry\n") % filePath % i;
            return std::nullopt;
        }
        unsigned id, status;
        if(!s25util::tryFromStringClassic(item->getName(), id)
           || !s25util::tryFromStringClassic(item->getText(), status))
        {
            LOG.write("Failed to parse addon option #%1% ('%2%' = '%3%') in %4%\n") % i % item->getName()
              % item->getText() % filePath;
            return std::nullopt;
        }
        states[id] = status;
    }
    return states;
}

// iwAddonPresetsBase
iwAddonPresetsBase::iwAddonPresetsBase(const std::string& title, const std::string& actionLabel)
    : IngameWindow(CGI_ADDON_PRESETS, IngameWindow::posLastOrCenter, Extent(440, 330), title,
                   LOADER.GetImageN("resource", 41))
{
    const bfs::path presetsDir = GetPresetsDir();
    boost::system::error_code ec;
    bfs::create_directories(presetsDir, ec);
    if(ec)
    {
        LOG.write("Failed to create addon preset folder %1%: %2%\n") % presetsDir % ec.message();
        // Without the folder, saving/loading/deleting presets can't work.
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
          _("Addon Presets Unavailable"),
          _("The addon presets folder could not be created. Saving and loading addon presets is unavailable."), nullptr,
          MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
        Close();
        return;
    }

    using SRT = ctrlTable::SortType;
    AddTable(ID_tblPresets, DrawPoint(20, 30), Extent(400, 200), TextureColor::Green2, NormalFont,
             ctrlTable::Columns{{_("Preset Name"), 400, SRT::String}, {}});

    AddText(ID_txtFolder, DrawPoint(20, 236), presetsDir.string(), COLOR_YELLOW, FontStyle::TOP, SmallFont)
      ->setMaxWidth(400);

    // maxLength 251 = 255 filename limit - 4 chars for ".ini"; just discourages absurdly long
    // input, isValidFileName() may still reject it since it counts bytes, not codepoints.
    AddEdit(ID_edtName, DrawPoint(20, 254), Extent(400, 22), TextureColor::Green2, NormalFont, 251);
    GetCtrl<ctrlEdit>(ID_edtName)->SetType(EditType::Filename);

    AddTextButton(ID_btAction, DrawPoint(20, 284), Extent(185, 22), TextureColor::Green2, actionLabel, NormalFont);
    AddTextButton(ID_btDelete, DrawPoint(235, 284), Extent(185, 22), TextureColor::Red1, _("Delete"), NormalFont);

    RefreshTable();
}

void iwAddonPresetsBase::RefreshTable()
{
    auto& table = *GetCtrl<ctrlTable>(ID_tblPresets);
    table.DeleteAllItems();

    for(const auto& file : ListDir(GetPresetsDir(), "ini"))
        table.AddRow({file.stem().string(), file.string()});

    table.SortRows(0, TableSortDir::Ascending);
}

bfs::path iwAddonPresetsBase::GetTargetFilePath() const
{
    const auto res = GetCtrl<ctrlEdit>(ID_edtName)->GetFileName(".ini");
    if(res.status != FileNameStatus::Valid)
        return {};
    bfs::path path = GetPresetsDir() / res.name;
    boost::system::error_code ec;
    if(!bfs::exists(path, ec) || ec)
        return {};
    return path;
}

bfs::path iwAddonPresetsBase::GetTargetFileOrNotify()
{
    bfs::path path = GetTargetFilePath();
    if(!path.empty())
        return path;

    const auto& edit = *GetCtrl<ctrlEdit>(ID_edtName);
    const auto res = edit.GetFileName(".ini");
    if(res.status == FileNameStatus::Empty)
        return {};

    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Preset Not Found"),
                                                  helpers::format(_("Preset '%1%' was not found."), edit.GetText()),
                                                  nullptr, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
    return {};
}

void iwAddonPresetsBase::Msg_EditEnter(const unsigned /*ctrl_id*/)
{
    DoAction();
}

void iwAddonPresetsBase::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btAction: DoAction(); break;
        case ID_btDelete: ConfirmDelete(); break;
        default: break;
    }
}

void iwAddonPresetsBase::Msg_TableSelectItem(const unsigned /*ctrl_id*/, const std::optional<unsigned>& selection)
{
    const auto& table = *GetCtrl<ctrlTable>(ID_tblPresets);
    GetCtrl<ctrlEdit>(ID_edtName)->SetText(selection ? table.GetItemText(*selection, 0) : "");
}

void iwAddonPresetsBase::Msg_TableChooseItem(const unsigned /*ctrl_id*/, const unsigned /*selection*/)
{
    DoAction();
}

void iwAddonPresetsBase::ConfirmDelete()
{
    const bfs::path filePath = GetTargetFileOrNotify();
    if(filePath.empty())
        return;
    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
      _("Delete Preset"), helpers::format(_("Are you sure you want to delete preset '%1%'?"), filePath.stem().string()),
      this, MsgboxButton::YesNo, MsgboxIcon::QuestionRed, ID_mbDelete));
}

void iwAddonPresetsBase::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    if(msgbox_id != ID_mbDelete || mbr != MsgboxResult::Yes)
        return;

    const bfs::path filePath = GetTargetFilePath();
    if(filePath.empty())
        return;

    boost::system::error_code ec;
    bfs::remove(filePath, ec);
    if(ec)
    {
        LOG.write("Failed to delete addon preset %1%: %2%\n") % filePath % ec.message();
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Delete Failed"), _("Failed to delete the selected preset."),
                                                      this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
    }
    // Refresh in both cases so the list reflects the actual filesystem state
    // (e.g. the file became a directory or was removed out from under us).
    RefreshTable();
    GetCtrl<ctrlEdit>(ID_edtName)->SetText("");
}

// iwSaveAddonPreset
iwSaveAddonPreset::iwSaveAddonPreset(std::map<unsigned, unsigned> states)
    : iwAddonPresetsBase(_("Save Addon Preset"), _("Save")), states_(std::move(states))
{}

void iwSaveAddonPreset::DoAction()
{
    const auto fileNameResult = GetCtrl<ctrlEdit>(ID_edtName)->GetFileName(".ini");
    switch(fileNameResult.status)
    {
        case FileNameStatus::Empty:
            WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Invalid Name"), _("Please enter a preset name."), this,
                                                          MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
            return;
        case FileNameStatus::Invalid:
            WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Invalid Name"), _("Please enter a valid preset name."),
                                                          this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
            return;
        case FileNameStatus::Valid: break;
    }
    const bfs::path filePath = GetPresetsDir() / fileNameResult.name;

    if(bfs::exists(filePath))
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
          _("Overwrite Preset"), _("A preset with this name already exists. Do you want to overwrite it?"), this,
          MsgboxButton::YesNo, MsgboxIcon::QuestionRed, ID_mbOverwrite));
        return;
    }

    SaveToPath(filePath);
}

void iwSaveAddonPreset::SaveToPath(const bfs::path& filePath)
{
    auto iniItem = std::make_unique<libsiedler2::ArchivItem_Ini>("addons");
    for(const auto& [id, status] : states_)
        iniItem->setValue(s25util::toStringClassic(id), s25util::toStringClassic(status));

    libsiedler2::Archiv archive;
    archive.push(std::move(iniItem));

    if(libsiedler2::Write(filePath, archive) == 0)
    {
        Close();
        return;
    }

    LOG.write("Failed to save addon preset to %1%\n") % filePath;
    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
      _("Save Failed"), _("Failed to save the preset. Please check the filename and try again."), this,
      MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
    RefreshTable();
}

void iwSaveAddonPreset::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    if(msgbox_id == ID_mbOverwrite)
    {
        if(mbr == MsgboxResult::Yes)
        {
            const auto fileNameResult = GetCtrl<ctrlEdit>(ID_edtName)->GetFileName(".ini");
            if(fileNameResult.status == FileNameStatus::Valid)
                SaveToPath(GetPresetsDir() / fileNameResult.name);
        }
    } else
        iwAddonPresetsBase::Msg_MsgBoxResult(msgbox_id, mbr);
}

// iwLoadAddonPreset
iwLoadAddonPreset::iwLoadAddonPreset(std::function<void(const std::map<unsigned, unsigned>&)> onLoad)
    : iwAddonPresetsBase(_("Load Addon Preset"), _("Load")), onLoad_(std::move(onLoad))
{}

void iwLoadAddonPreset::DoAction()
{
    const bfs::path filePath = GetTargetFileOrNotify();
    if(filePath.empty())
        return;

    const auto states = LoadPresetsFromFile(filePath);
    if(!states)
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(
          _("Load Failed"),
          _("The selected preset could not be loaded. The file may be corrupted or have an invalid format."), this,
          MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
        return;
    }

    onLoad_(*states);
    Close();
}
