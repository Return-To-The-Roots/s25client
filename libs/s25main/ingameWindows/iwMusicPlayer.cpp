// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMusicPlayer.h"
#include "ListDir.h"
#include "Loader.h"
#include "MusicPlayer.h"
#include "RTTR_Assert.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlList.h"
#include "controls/ctrlTextDeepening.h"
#include "files.h"
#include "helpers/Range.h"
#include "helpers/toString.h"
#include "iwMsgbox.h"
#include "gameData/const_gui_ids.h"
#include "s25util/Log.h"
#include "s25util/StringConversion.h"
#include "s25util/colors.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

namespace {
enum
{
    ID_lstSongs,
    ID_txtPlaylist,
    ID_cbPlaylist,
    ID_btAddPlaylist,
    ID_btRemovePlaylist,
    ID_btAddTrack,
    ID_btAddTrackDir,
    ID_btRemoveTrack,
    ID_btUp,
    ID_btDown,
    ID_txtRepeat,
    ID_btIncRepeat,
    ID_btDecRepeat,
    ID_btRandom,
    ID_btSave,

    ID_edtName,
    ID_btOk,
    ID_btAbort,

    ID_wndAddTrack,
    ID_wndAddPlaylist,
    ID_wndAddTrackDir
};
}

iwMusicPlayer::InputWindow::InputWindow(iwMusicPlayer& playerWnd, const unsigned win_id, const std::string& title)
    : IngameWindow(CGI_INPUTWINDOW, IngameWindow::posCenter, Extent(300, 100), title, LOADER.getImageN("resource", 41),
                   true),
      win_id(win_id), playerWnd_(playerWnd)
{
    addEdit(ID_edtName, DrawPoint(20, 30), Extent(getSize().x - 40, 22), TextureColor::Green2, NormalFont)->setFocus();
    addTextButton(ID_btOk, DrawPoint(20, 60), Extent(100, 22), TextureColor::Green1, _("OK"), NormalFont);
    addTextButton(ID_btAbort, DrawPoint(130, 60), Extent(100, 22), TextureColor::Red1, _("Abort"), NormalFont);
}

void iwMusicPlayer::InputWindow::msgButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id == ID_btOk)
        playerWnd_.msgInput(win_id, getCtrl<ctrlEdit>(ID_edtName)->getText());

    close();
}

void iwMusicPlayer::InputWindow::msgEditEnter(const unsigned /*ctrl_id*/)
{
    msgButtonClick(ID_btOk);
}

iwMusicPlayer::iwMusicPlayer()
    : IngameWindow(CGI_MUSICPLAYER, IngameWindow::posLastOrCenter, Extent(440, 330), _("Music player"),
                   LOADER.getImageN("resource", 41)),
      changed(false)
{
    addList(ID_lstSongs, DrawPoint(20, 30), Extent(330, 200), TextureColor::Green1, NormalFont);
    addText(ID_txtPlaylist, DrawPoint(20, 240), _("Playlist:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addComboBox(ID_cbPlaylist, DrawPoint(20, 260), Extent(330, 22), TextureColor::Green1, NormalFont, 200);

    // Playlist buttons
    const unsigned short buttonDistance = 10;
    const Extent buttonSize((330 - buttonDistance) / 2, 22);
    ctrlButton* b1 =
      addTextButton(ID_btAddPlaylist, DrawPoint(20, 288), buttonSize, TextureColor::Green2, _("Add"), NormalFont);
    addTextButton(ID_btRemovePlaylist, b1->getPos() + DrawPoint(buttonSize.x + buttonDistance, 0), buttonSize,
                  TextureColor::Green2, _("Remove"), NormalFont);

    // Track buttons
    addImageButton(ID_btAddTrack, DrawPoint(360, 30), Extent(30, 40), TextureColor::Grey, LOADER.getImageN("io", 138),
                   _("Add track"));
    addImageButton(ID_btAddTrackDir, DrawPoint(390, 30), Extent(30, 40), TextureColor::Grey,
                   LOADER.getImageN("io_new", 2), _("Add directory of tracks"));
    addImageButton(ID_btRemoveTrack, DrawPoint(370, 80), Extent(40, 40), TextureColor::Red1,
                   LOADER.getImageN("io", 220), _("Remove track"));
    addImageButton(ID_btUp, DrawPoint(370, 130), Extent(40, 15), TextureColor::Grey, LOADER.getImageN("io", 33),
                   _("Upwards"));
    addImageButton(ID_btDown, DrawPoint(370, 145), Extent(40, 15), TextureColor::Grey, LOADER.getImageN("io", 34),
                   _("Downwards"));
    addTextDeepening(ID_txtRepeat, DrawPoint(370, 170), Extent(40, 20), TextureColor::Grey, "1", NormalFont,
                     COLOR_YELLOW);
    addImageButton(ID_btDecRepeat, DrawPoint(370, 190), Extent(20, 20), TextureColor::Red1, LOADER.getImageN("io", 139),
                   _("Less repeats"));
    addImageButton(ID_btIncRepeat, DrawPoint(390, 190), Extent(20, 20), TextureColor::Grey, LOADER.getImageN("io", 138),
                   _("More repeats"));
    addImageButton(ID_btRandom, DrawPoint(370, 220), Extent(40, 40), TextureColor::Grey, LOADER.getImageN("io", 107),
                   _("Playback in this order"));
    addImageButton(ID_btSave, DrawPoint(370, 270), Extent(40, 40), TextureColor::Grey, LOADER.getImageN("io", 37),
                   _("Save playlist"));

    // Fill values for current playlist
    updateFromPlaylist(MUSICPLAYER.getPlaylist());
    updatePlaylistCombo(SETTINGS.sound.playlist);
    auto* cbPlayList = getCtrl<ctrlComboBox>(ID_cbPlaylist);
    if(!cbPlayList->getSelection() && cbPlayList->getNumItems() > 0u)
    {
        cbPlayList->setSelection(0u);
        msgComboSelectItem(ID_cbPlaylist, 0u);
    }
}

static bool isReadonlyPlaylist(const std::string& name)
{
    // RTTR-Playlists shall not be changed/removed
    return name == boost::filesystem::path(s25::files::defaultPlaylist).stem();
}

void iwMusicPlayer::close()
{
    IngameWindow::close();

    if(saveCurrentPlaylist())
    {
        const auto& selection = getCtrl<ctrlComboBox>(ID_cbPlaylist)->getSelection();
        if(selection)
        {
            SETTINGS.sound.playlist =
              getFullPlaylistPath(getCtrl<ctrlComboBox>(ID_cbPlaylist)->getText(*selection)).string();
        }
    }

    // Update the player if anything has changed
    if(changed)
    {
        MUSICPLAYER.setPlaylist(makePlaylist());
        MUSICPLAYER.play();
        changed = false;
    }
}

void iwMusicPlayer::msgComboSelectItem(const unsigned ctrl_id, const unsigned selection)
{
    RTTR_Assert(ctrl_id == ID_cbPlaylist);
    Playlist pl;
    const std::string playlistName = getCtrl<ctrlComboBox>(ID_cbPlaylist)->getText(selection);
    if(pl.load(LOG, getFullPlaylistPath(playlistName)))
    {
        updateFromPlaylist(pl);
        changed = true;
    } else
    {
        WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"), _("The specified file couldn't be loaded!"), this,
                                                      MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
    }
    updateSaveChangeButtonsState();
}

void iwMusicPlayer::msgListChooseItem(const unsigned /*ctrl_id*/, const unsigned selection)
{
    // (Re)create playlist with selected song as start song set in musicplayer
    Playlist pl = makePlaylist();
    pl.setStartSong(selection);
    MUSICPLAYER.setPlaylist(std::move(pl));
    MUSICPLAYER.play();

    // No changes anymore
    changed = false;
}

boost::filesystem::path iwMusicPlayer::getFullPlaylistPath(const std::string& name)
{
    const boost::filesystem::path folder =
      RTTRCONFIG.expandPath(isReadonlyPlaylist(name) ? s25::folders::music : s25::folders::playlists);
    return folder / (name + ".pll");
}

bool iwMusicPlayer::saveCurrentPlaylist()
{
    const auto playlistName = getCtrl<ctrlComboBox>(ID_cbPlaylist)->getSelectedText();
    if(!playlistName)
        return false;

    const Playlist pl = makePlaylist();

    const auto fullPlaylistPath = getFullPlaylistPath(*playlistName);

    if(isReadonlyPlaylist(*playlistName))
        return false;

    return pl.saveAs(fullPlaylistPath);
}

void iwMusicPlayer::updateFromPlaylist(const Playlist& playlist)
{
    auto* lstSongs = getCtrl<ctrlList>(ID_lstSongs);
    lstSongs->deleteAllItems();

    for(const auto& song : playlist.getSongs())
        lstSongs->addItem(song);

    const auto& currentSong = playlist.getCurrentSong();
    if(!currentSong.empty())
    {
        for(const auto i : helpers::range<unsigned>(lstSongs->getNumLines()))
        {
            if(currentSong == lstSongs->getItemText(i))
            {
                lstSongs->setSelection(i);
                break;
            }
        }
    }

    setRepeats(playlist.getNumRepeats());
    setRandomPlayback(playlist.isRandomized());
}

void iwMusicPlayer::updateSaveChangeButtonsState()
{
    const auto playlistName = getCtrl<ctrlComboBox>(ID_cbPlaylist)->getSelectedText();
    const bool isReadOnly = !playlistName || isReadonlyPlaylist(*playlistName);
    for(const auto id : {ID_btRemovePlaylist, ID_btSave})
        getCtrl<ctrlButton>(id)->setEnabled(!isReadOnly);
}

Playlist iwMusicPlayer::makePlaylist()
{
    const auto* lstSongs = getCtrl<ctrlList>(ID_lstSongs);
    std::vector<std::string> songs;
    for(const auto i : helpers::range<unsigned>(lstSongs->getNumLines()))
        songs.push_back(lstSongs->getItemText(i));

    return Playlist(std::move(songs), getRepeats(), getRandomPlayback());
}

void iwMusicPlayer::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btAddPlaylist:
            WINDOWMANAGER.show(std::make_unique<InputWindow>(*this, ID_wndAddPlaylist, _("Specify the playlist name")));
            break;
        case ID_btRemovePlaylist:
        {
            const auto& playlistName = getCtrl<ctrlComboBox>(ID_cbPlaylist)->getSelectedText();
            if(playlistName)
            {
                // RTTR-playlists must not be deleted
                if(isReadonlyPlaylist(*playlistName))
                {
                    WINDOWMANAGER.show(
                      std::make_unique<iwMsgbox>(_("Error"), _("You are not allowed to delete the standard playlist!"),
                                                 this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
                    return;
                }

                boost::system::error_code ec;
                boost::filesystem::remove(getFullPlaylistPath(*playlistName), ec);
                SETTINGS.sound.playlist = RTTRCONFIG.expandPath(s25::files::defaultPlaylist).string();
                updatePlaylistCombo(SETTINGS.sound.playlist);
            }
        }
        break;
        case ID_btAddTrack:
            WINDOWMANAGER.toggleWindow(std::make_unique<InputWindow>(*this, ID_wndAddTrack, _("Add track")));
            changed = true;
            break;
        case ID_btAddTrackDir:
            WINDOWMANAGER.toggleWindow(
              std::make_unique<InputWindow>(*this, ID_wndAddTrackDir, _("Add directory of tracks")));
            changed = true;
            break;
        case ID_btRemoveTrack:
        {
            const auto& selection = getCtrl<ctrlList>(ID_lstSongs)->getSelection();

            if(selection)
            {
                getCtrl<ctrlList>(ID_lstSongs)->remove(*selection);
                changed = true;
            }
        }
        break;
        case ID_btUp:
        {
            const auto& selection = getCtrl<ctrlList>(ID_lstSongs)->getSelection();

            if(selection && *selection > 0u)
                getCtrl<ctrlList>(ID_lstSongs)->swap(*selection - 1u, *selection);
        }
        break;
        case ID_btDown:
        {
            const auto& selection = getCtrl<ctrlList>(ID_lstSongs)->getSelection();

            if(selection && *selection < getCtrl<ctrlList>(ID_lstSongs)->getNumLines() - 1u)
                getCtrl<ctrlList>(ID_lstSongs)->swap(*selection + 1u, *selection);
        }
        break;
        case ID_btDecRepeat:
        {
            unsigned repeats = getRepeats();
            if(repeats > 0u)
            {
                --repeats;
                setRepeats(repeats);
                changed = true;
            }
        }
        break;
        case ID_btIncRepeat:
            setRepeats(getRepeats() + 1);
            changed = true;
            break;
        case ID_btRandom:
            setRandomPlayback(!getRandomPlayback());
            changed = true;
            break;
        case ID_btSave:
            if(saveCurrentPlaylist())
            {
                WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Ok"), _("The playlist was saved!"), nullptr,
                                                              MsgboxButton::Ok, MsgboxIcon::ExclamationGreen));
            } else
            {
                WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"), _("The specified file couldn't be saved!"),
                                                              nullptr, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
            }
            break;
    }
}

void iwMusicPlayer::msgInput(const unsigned win_id, const std::string& msg)
{
    switch(win_id)
    {
        case ID_wndAddTrack:
        {
            bool valid = false;
            if(boost::filesystem::exists(msg))
                valid = true;
            else
            {
                // Could be an original The settlers song ("sNN")?
                if(msg.length() == 3 && msg[0] == 's')
                {
                    if(s25util::fromStringClassicDef(msg.substr(1), 999u) <= 14u)
                        valid = true;
                }
            }

            if(valid)
            {
                getCtrl<ctrlList>(ID_lstSongs)->addItem(msg);
                changed = true;
            } else
                WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"), _("The specified file couldn't be opened!"),
                                                              this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
        }
        break;
        case ID_wndAddPlaylist:
        {
            // Playlist name must start with a letter
            const auto isLetter = [](const char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); };
            const boost::filesystem::path fullPlaylistPath = getFullPlaylistPath(msg);
            if(!msg.empty() && isLetter(msg.front()) && Playlist().saveAs(fullPlaylistPath))
            {
                updatePlaylistCombo(fullPlaylistPath.string());
                changed = true;
            } else
            {
                WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"), _("The specified file couldn't be saved!"),
                                                              this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
            }
        }
        break;
        case ID_wndAddTrackDir:
        {
            std::vector<boost::filesystem::path> oggFiles = listDir(msg, "ogg");

            for(const auto& oggFile : oggFiles)
                getCtrl<ctrlList>(ID_lstSongs)->addItem(oggFile.string());

            changed = true;
        }
        break;
    }
}

unsigned iwMusicPlayer::getRepeats() const
{
    return boost::lexical_cast<unsigned>(getCtrl<ctrlTextDeepening>(ID_txtRepeat)->getText());
}

void iwMusicPlayer::setRepeats(unsigned repeats)
{
    getCtrl<ctrlTextDeepening>(ID_txtRepeat)->setText(helpers::toString(repeats));
}

bool iwMusicPlayer::getRandomPlayback() const
{
    return getCtrl<ctrlImageButton>(ID_btRandom)->getImage() != LOADER.getTextureN("io", 107);
}

void iwMusicPlayer::setRandomPlayback(const bool random_playback)
{
    getCtrl<ctrlImageButton>(ID_btRandom)
      ->setImage(random_playback ? LOADER.getTextureN("io", 225) : LOADER.getTextureN("io", 107));
    getCtrl<ctrlImageButton>(ID_btRandom)
      ->setTooltip(random_playback ? _("Playback in this order") : _("Random playback"));
}

void iwMusicPlayer::updatePlaylistCombo(const std::string& highlight_entry)
{
    auto* cbPlaylist = getCtrl<ctrlComboBox>(ID_cbPlaylist);
    cbPlaylist->deleteAllItems();

    std::vector<boost::filesystem::path> playlists = listDir(RTTRCONFIG.expandPath(s25::folders::playlists), "pll");
    playlists.insert(playlists.begin(), RTTRCONFIG.expandPath(s25::files::defaultPlaylist));
    const auto currentPath = RTTRCONFIG.expandPath(highlight_entry);

    unsigned i = 0;
    for(const auto& playlistPath : playlists)
    {
        // Reduce to pure filename
        cbPlaylist->addItem(playlistPath.stem().string());
        if(playlistPath == currentPath)
            cbPlaylist->setSelection(i);
        ++i;
    }
    updateSaveChangeButtonsState();
}
