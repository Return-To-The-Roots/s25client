// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwAddonPresets.h"
#include "ListDir.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "WindowManager.h"
#include "commonDefines.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlTable.h"
#include "controls/ctrlText.h"
#include "files.h"
#include "helpers/format.hpp"
#include "iwMsgbox.h"
#include "gameData/const_gui_ids.h"
#include "libsiedler2/ArchivItem_Ini.h"
#include "libsiedler2/ArchivItem_Text.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/Log.h"
#include "s25util/StringConversion.h"
#include <boost/filesystem.hpp>
#include <optional>

namespace bfs = boost::filesystem;

namespace {
constexpr unsigned contentX = 20;
constexpr unsigned contentWidth = 400;
constexpr unsigned tableY = 30;
constexpr unsigned tableHeight = 200;
/// Height of a single-line control (text, edit, button)
constexpr unsigned ctrlHeight = 22;
/// Gaps between neighboring controls
constexpr unsigned ctrlSpacingX = 8;
constexpr unsigned ctrlSpacingY = 8;
constexpr unsigned bottomMargin = 24;
/// Subclasses add their own controls from here
constexpr unsigned contentStartY = tableY + tableHeight + ctrlSpacingY + ctrlHeight;
constexpr unsigned halfWidth = (contentWidth - ctrlSpacingX) / 2;
constexpr unsigned rightColumnX = contentX + halfWidth + ctrlSpacingX;
} // namespace

static bfs::path getPresetsDir()
{
    return RTTRCONFIG.expandPath(s25::folders::addonPresets);
}

static std::optional<std::map<unsigned, unsigned>> loadPresetsFromFile(const bfs::path& filePath)
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

iwAddonPresetsBase::iwAddonPresetsBase(const std::string& title, const std::string& actionLabel,
                                       const unsigned additionalHeight)
    : IngameWindow(CGI_ADDON_PRESETS, IngameWindow::posLastOrCenter,
                   Extent(440, contentStartY + additionalHeight + ctrlHeight + bottomMargin), title,
                   LOADER.getImageN("resource", 41), true)
{
    using SRT = ctrlTable::SortType;
    addTable(ID_tblPresets, DrawPoint(contentX, tableY), Extent(contentWidth, tableHeight), TextureColor::Green2,
             NormalFont, ctrlTable::Columns{{_("Preset Name"), contentWidth, SRT::String}, {}});

    addText(ID_txtFolder, DrawPoint(contentX, tableY + tableHeight + ctrlSpacingY), getPresetsDir().string(),
            COLOR_YELLOW, FontStyle::TOP, SmallFont)
      ->setMaxWidth(contentWidth);

    const unsigned buttonY = contentStartY + additionalHeight;
    addTextButton(ID_btAction, DrawPoint(rightColumnX, buttonY), Extent(halfWidth, ctrlHeight), TextureColor::Green2,
                  actionLabel, NormalFont)
      ->setEnabled(false);
    addTextButton(ID_btDelete, DrawPoint(contentX, buttonY), Extent(halfWidth, ctrlHeight), TextureColor::Red1,
                  _("Delete"), NormalFont)
      ->setEnabled(false);

    refreshTable();
}

void iwAddonPresetsBase::refreshTable()
{
    auto& table = assertNonNull(getCtrl<ctrlTable>(ID_tblPresets));
    table.deleteAllItems();

    for(const auto& file : listDir(getPresetsDir(), "ini"))
        table.addRow({file.stem().string(), file.string()});

    table.sortRows(0, TableSortDir::Ascending);
}

bfs::path iwAddonPresetsBase::getSelectedFilePath() const
{
    const auto& table = assertNonNull(getCtrl<ctrlTable>(ID_tblPresets));
    const auto& selection = table.getSelection();
    if(!selection)
        return {};
    return table.getItemText(*selection, 1);
}

bool iwAddonPresetsBase::isPresetSelected() const
{
    return assertNonNull(getCtrl<ctrlTable>(ID_tblPresets)).getSelection().has_value();
}

void iwAddonPresetsBase::updateButtons()
{
    getCtrl<ctrlButton>(ID_btDelete)->setEnabled(isPresetSelected());
    getCtrl<ctrlButton>(ID_btAction)->setEnabled(canDoAction());
}

bool iwAddonPresetsBase::canDoAction() const
{
    return isPresetSelected();
}

void iwAddonPresetsBase::msgButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id == ID_btDelete)
        confirmDelete();
    else
    {
        RTTR_Assert(ctrl_id == ID_btAction);
        doAction();
    }
}

void iwAddonPresetsBase::msgTableSelectItem(const unsigned /*ctrl_id*/, const std::optional<unsigned>& /*selection*/)
{
    updateButtons();
}

void iwAddonPresetsBase::msgTableChooseItem(const unsigned /*ctrl_id*/, const unsigned /*selection*/)
{
    doAction();
}

void iwAddonPresetsBase::confirmDelete()
{
    const bfs::path filePath = getSelectedFilePath();
    if(filePath.empty())
        return;
    WINDOWMANAGER.show(std::make_unique<iwMsgbox>(
      _("Delete Preset"), helpers::format(_("Are you sure you want to delete preset '%1%'?"), filePath.stem().string()),
      this, MsgboxButton::YesNo, MsgboxIcon::QuestionRed, ID_mbDelete));
}

void iwAddonPresetsBase::msgMsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    if(msgbox_id != ID_mbDelete || mbr != MsgboxResult::Yes)
        return;

    const bfs::path filePath = getSelectedFilePath();
    if(filePath.empty())
        return;

    boost::system::error_code ec;
    bfs::remove(filePath, ec);
    if(ec)
    {
        LOG.write("Failed to delete addon preset %1%: %2%\n") % filePath % ec.message();
        WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Delete Failed"), _("Failed to delete the selected preset."),
                                                      this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
    }
    // Refresh in both cases so the list reflects the actual filesystem state
    // (e.g. the file became a directory or was removed out from under us).
    refreshTable();
}

iwSaveAddonPreset::iwSaveAddonPreset(std::map<unsigned, unsigned> states)
    : iwAddonPresetsBase(_("Save Addon Preset"), _("Save"), ctrlHeight + ctrlSpacingY), states_(std::move(states))
{
    // maxLength 251 = 255 filename limit - 4 chars for ".ini"; just discourages absurdly long
    // input, isValidFileName() may still reject it since it counts bytes, not codepoints.
    addEdit(ID_edtName, DrawPoint(contentX, contentStartY), Extent(contentWidth, ctrlHeight), TextureColor::Green2,
            NormalFont, 251, false, false, true)
      ->setType(EditType::Filename);
}

void iwSaveAddonPreset::msgEditEnter(const unsigned /*ctrl_id*/)
{
    doAction();
}

void iwSaveAddonPreset::msgEditChange(const unsigned /*ctrl_id*/)
{
    // Editing makes the selected preset ambiguous, so drop the selection instead of guessing which one is meant
    assertNonNull(getCtrl<ctrlTable>(ID_tblPresets)).setSelection(std::nullopt);
}

void iwSaveAddonPreset::msgTableSelectItem(const unsigned ctrl_id, const std::optional<unsigned>& selection)
{
    if(selection)
    {
        const auto& table = assertNonNull(getCtrl<ctrlTable>(ID_tblPresets));
        auto& edit = assertNonNull(getCtrl<ctrlEdit>(ID_edtName));
        // Suppress the change event so Msg_EditChange won't deselect the row just selected
        edit.setNotify(false);
        edit.setText(table.getItemText(*selection, 0));
        edit.setNotify(true);
    }
    // The name must be filled first, as it decides whether saving is possible
    iwAddonPresetsBase::msgTableSelectItem(ctrl_id, selection);
}

bool iwSaveAddonPreset::canDoAction() const
{
    // An invalid name still allows saving, which then tells the user what is wrong with it
    return getCtrl<ctrlEdit>(ID_edtName)->getFileName(".ini").status != FileNameStatus::Empty;
}

void iwSaveAddonPreset::doAction()
{
    const auto fileNameResult = getCtrl<ctrlEdit>(ID_edtName)->getFileName(".ini");
    switch(fileNameResult.status)
    {
        case FileNameStatus::Empty:
            WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Invalid Name"), _("Please enter a preset name."), this,
                                                          MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
            return;
        case FileNameStatus::Invalid:
            WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Invalid Name"), _("Please enter a valid preset name."),
                                                          this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
            return;
        case FileNameStatus::Valid: break;
    }
    const bfs::path filePath = getPresetsDir() / fileNameResult.name;

    if(bfs::exists(filePath))
    {
        WINDOWMANAGER.show(std::make_unique<iwMsgbox>(
          _("Overwrite Preset"),
          helpers::format(_("A preset named '%1%' already exists. Do you want to overwrite it?"),
                          filePath.stem().string()),
          this, MsgboxButton::YesNo, MsgboxIcon::QuestionRed, ID_mbOverwrite));
        return;
    }

    saveToPath(filePath);
}

void iwSaveAddonPreset::saveToPath(const bfs::path& filePath)
{
    auto iniItem = std::make_unique<libsiedler2::ArchivItem_Ini>("addons");
    for(const auto& [id, status] : states_)
        iniItem->setValue(s25util::toStringClassic(id), s25util::toStringClassic(status));

    libsiedler2::Archiv archive;
    archive.push(std::move(iniItem));

    const int ec = libsiedler2::Write(filePath, archive);
    if(ec == libsiedler2::ErrorCode::NONE)
    {
        close();
        return;
    }

    LOG.write("Failed to save addon preset to %1%: %2%\n") % filePath % libsiedler2::getErrorString(ec);
    WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Save Failed"),
                                                  _("Failed to save the preset. Check application logs for details."),
                                                  this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
    refreshTable();
}

void iwSaveAddonPreset::msgMsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    if(msgbox_id != ID_mbOverwrite)
    {
        iwAddonPresetsBase::msgMsgBoxResult(msgbox_id, mbr);
        return;
    }
    if(mbr != MsgboxResult::Yes)
        return;

    const auto fileNameResult = getCtrl<ctrlEdit>(ID_edtName)->getFileName(".ini");
    if(fileNameResult.status == FileNameStatus::Valid)
        saveToPath(getPresetsDir() / fileNameResult.name);
}

iwLoadAddonPreset::iwLoadAddonPreset(std::function<void(const std::map<unsigned, unsigned>&)> onLoad)
    : iwAddonPresetsBase(_("Load Addon Preset"), _("Load"), 0), onLoad_(std::move(onLoad))
{}

void iwLoadAddonPreset::doAction()
{
    const bfs::path filePath = getSelectedFilePath();
    if(filePath.empty())
        return;

    const auto states = loadPresetsFromFile(filePath);
    if(!states)
    {
        WINDOWMANAGER.show(std::make_unique<iwMsgbox>(
          _("Load Failed"),
          _("The selected preset could not be loaded. The file may be corrupted or have an invalid format."), this,
          MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
        return;
    }

    onLoad_(*states);
    close();
}
