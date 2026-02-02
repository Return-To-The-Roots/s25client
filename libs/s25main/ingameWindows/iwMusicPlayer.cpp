// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
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
    : IngameWindow(CGI_INPUTWINDOW, IngameWindow::posCenter, Extent(300, 100), title, LOADER.GetImageN("resource", 41),
                   true),
      win_id(win_id), playerWnd_(playerWnd)
{
    AddEdit(ID_edtName, DrawPoint(20, 30), Extent(GetSize().x - 40, 22), TextureColor::Green2, NormalFont)->SetFocus();
    AddTextButton(ID_btOk, DrawPoint(20, 60), Extent(100, 22), TextureColor::Green1, _("OK"), NormalFont);
    AddTextButton(ID_btAbort, DrawPoint(130, 60), Extent(100, 22), TextureColor::Red1, _("Abort"), NormalFont);
}

void iwMusicPlayer::InputWindow::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id == ID_btOk)
        playerWnd_.Msg_Input(win_id, GetCtrl<ctrlEdit>(ID_edtName)->GetText());

    Close();
}

void iwMusicPlayer::InputWindow::Msg_EditEnter(const unsigned /*ctrl_id*/)
{
    Msg_ButtonClick(ID_btOk);
}

iwMusicPlayer::iwMusicPlayer()
    : IngameWindow(CGI_MUSICPLAYER, IngameWindow::posLastOrCenter, Extent(440, 330), _("Music player"),
                   LOADER.GetImageN("resource", 41)),
      changed(false)
{
    AddList(ID_lstSongs, DrawPoint(20, 30), Extent(330, 200), TextureColor::Green1, NormalFont);
    AddText(ID_txtPlaylist, DrawPoint(20, 240), _("Playlist:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddComboBox(ID_cbPlaylist, DrawPoint(20, 260), Extent(330, 22), TextureColor::Green1, NormalFont, 200);

    // Playlist buttons
    const unsigned short button_distance = 10;
    const Extent buttonSize((330 - button_distance) / 2, 22);
    ctrlButton* b1 =
      AddTextButton(ID_btAddPlaylist, DrawPoint(20, 288), buttonSize, TextureColor::Green2, _("Add"), NormalFont);
    AddTextButton(ID_btRemovePlaylist, b1->GetPos() + DrawPoint(buttonSize.x + button_distance, 0), buttonSize,
                  TextureColor::Green2, _("Remove"), NormalFont);

    // Track buttons
    AddImageButton(ID_btAddTrack, DrawPoint(360, 30), Extent(30, 40), TextureColor::Grey, LOADER.GetImageN("io", 138),
                   _("Add track"));
    AddImageButton(ID_btAddTrackDir, DrawPoint(390, 30), Extent(30, 40), TextureColor::Grey,
                   LOADER.GetImageN("io_new", 2), _("Add directory of tracks"));
    AddImageButton(ID_btRemoveTrack, DrawPoint(370, 80), Extent(40, 40), TextureColor::Red1,
                   LOADER.GetImageN("io", 220), _("Remove track"));
    AddImageButton(ID_btUp, DrawPoint(370, 130), Extent(40, 15), TextureColor::Grey, LOADER.GetImageN("io", 33),
                   _("Upwards"));
    AddImageButton(ID_btDown, DrawPoint(370, 145), Extent(40, 15), TextureColor::Grey, LOADER.GetImageN("io", 34),
                   _("Downwards"));
    AddTextDeepening(ID_txtRepeat, DrawPoint(370, 170), Extent(40, 20), TextureColor::Grey, "1", NormalFont,
                     COLOR_YELLOW);
    AddImageButton(ID_btDecRepeat, DrawPoint(370, 190), Extent(20, 20), TextureColor::Red1, LOADER.GetImageN("io", 139),
                   _("Less repeats"));
    AddImageButton(ID_btIncRepeat, DrawPoint(390, 190), Extent(20, 20), TextureColor::Grey, LOADER.GetImageN("io", 138),
                   _("More repeats"));
    AddImageButton(ID_btRandom, DrawPoint(370, 220), Extent(40, 40), TextureColor::Grey, LOADER.GetImageN("io", 107),
                   _("Playback in this order"));
    AddImageButton(ID_btSave, DrawPoint(370, 270), Extent(40, 40), TextureColor::Grey, LOADER.GetImageN("io", 37),
                   _("Save playlist"));

    // Fill values for current playlist
    UpdateFromPlaylist(MUSICPLAYER.GetPlaylist());
    UpdatePlaylistCombo(SETTINGS.sound.playlist);
    auto* cbPlayList = GetCtrl<ctrlComboBox>(ID_cbPlaylist);
    if(!cbPlayList->GetSelection() && cbPlayList->GetNumItems() > 0u)
    {
        cbPlayList->SetSelection(0u);
        Msg_ComboSelectItem(ID_cbPlaylist, 0u);
    }
}

static bool isReadonlyPlaylist(const std::string& name)
{
    // RTTR-Playlists shall not be changed/removed
    return name == boost::filesystem::path(s25::files::defaultPlaylist).stem();
}

void iwMusicPlayer::Close()
{
    IngameWindow::Close();

    if(SaveCurrentPlaylist())
    {
        const auto& selection = GetCtrl<ctrlComboBox>(ID_cbPlaylist)->GetSelection();
        if(selection)
        {
            SETTINGS.sound.playlist =
              GetFullPlaylistPath(GetCtrl<ctrlComboBox>(ID_cbPlaylist)->GetText(*selection)).string();
        }
    }

    // Update the player if anything has changed
    if(changed)
    {
        MUSICPLAYER.SetPlaylist(MakePlaylist());
        MUSICPLAYER.Play();
        changed = false;
    }
}

void iwMusicPlayer::Msg_ComboSelectItem(const unsigned ctrl_id, const unsigned selection)
{
    RTTR_Assert(ctrl_id == ID_cbPlaylist);
    Playlist pl;
    const std::string playlistName = GetCtrl<ctrlComboBox>(ID_cbPlaylist)->GetText(selection);
    if(pl.Load(LOG, GetFullPlaylistPath(playlistName)))
    {
        UpdateFromPlaylist(pl);
        changed = true;
    } else
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), _("The specified file couldn't be loaded!"), this,
                                                      MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
    }
    UpdateSaveChangeButtonsState();
}

void iwMusicPlayer::Msg_ListChooseItem(const unsigned /*ctrl_id*/, const unsigned selection)
{
    // (Re)create playlist with selected song as start song set in musicplayer
    Playlist pl = MakePlaylist();
    pl.SetStartSong(selection);
    MUSICPLAYER.SetPlaylist(std::move(pl));
    MUSICPLAYER.Play();

    // No changes anymore
    changed = false;
}

boost::filesystem::path iwMusicPlayer::GetFullPlaylistPath(const std::string& name)
{
    const boost::filesystem::path folder =
      RTTRCONFIG.ExpandPath(isReadonlyPlaylist(name) ? s25::folders::music : s25::folders::playlists);
    return folder / (name + ".pll");
}

bool iwMusicPlayer::SaveCurrentPlaylist()
{
    const auto playlistName = GetCtrl<ctrlComboBox>(ID_cbPlaylist)->GetSelectedText();
    if(!playlistName)
        return false;

    const Playlist pl = MakePlaylist();

    const auto fullPlaylistPath = GetFullPlaylistPath(*playlistName);

    if(isReadonlyPlaylist(*playlistName))
        return false;

    return pl.SaveAs(fullPlaylistPath);
}

void iwMusicPlayer::UpdateFromPlaylist(const Playlist& playlist)
{
    auto* lstSongs = GetCtrl<ctrlList>(ID_lstSongs);
    lstSongs->DeleteAllItems();

    for(const auto& song : playlist.getSongs())
        lstSongs->AddString(song);

    const auto& currentSong = playlist.getCurrentSong();
    if(!currentSong.empty())
    {
        for(const auto i : helpers::range<unsigned>(lstSongs->GetNumLines()))
        {
            if(currentSong == lstSongs->GetItemText(i))
            {
                lstSongs->SetSelection(i);
                break;
            }
        }
    }

    SetRepeats(playlist.getNumRepeats());
    SetRandomPlayback(playlist.isRandomized());
}

void iwMusicPlayer::UpdateSaveChangeButtonsState()
{
    const auto playlistName = GetCtrl<ctrlComboBox>(ID_cbPlaylist)->GetSelectedText();
    const bool isReadOnly = !playlistName || isReadonlyPlaylist(*playlistName);
    for(const auto id : {ID_btRemovePlaylist, ID_btSave})
        GetCtrl<ctrlButton>(id)->SetEnabled(!isReadOnly);
}

Playlist iwMusicPlayer::MakePlaylist()
{
    const auto* lstSongs = GetCtrl<ctrlList>(ID_lstSongs);
    std::vector<std::string> songs;
    for(const auto i : helpers::range<unsigned>(lstSongs->GetNumLines()))
        songs.push_back(lstSongs->GetItemText(i));

    return Playlist(std::move(songs), GetRepeats(), GetRandomPlayback());
}

void iwMusicPlayer::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btAddPlaylist:
            WINDOWMANAGER.Show(std::make_unique<InputWindow>(*this, ID_wndAddPlaylist, _("Specify the playlist name")));
            break;
        case ID_btRemovePlaylist:
        {
            const auto& playlistName = GetCtrl<ctrlComboBox>(ID_cbPlaylist)->GetSelectedText();
            if(playlistName)
            {
                // RTTR-playlists must not be deleted
                if(isReadonlyPlaylist(*playlistName))
                {
                    WINDOWMANAGER.Show(
                      std::make_unique<iwMsgbox>(_("Error"), _("You are not allowed to delete the standard playlist!"),
                                                 this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
                    return;
                }

                boost::system::error_code ec;
                boost::filesystem::remove(GetFullPlaylistPath(*playlistName), ec);
                SETTINGS.sound.playlist = RTTRCONFIG.ExpandPath(s25::files::defaultPlaylist).string();
                UpdatePlaylistCombo(SETTINGS.sound.playlist);
            }
        }
        break;
        case ID_btAddTrack:
            WINDOWMANAGER.ToggleWindow(std::make_unique<InputWindow>(*this, ID_wndAddTrack, _("Add track")));
            changed = true;
            break;
        case ID_btAddTrackDir:
            WINDOWMANAGER.ToggleWindow(
              std::make_unique<InputWindow>(*this, ID_wndAddTrackDir, _("Add directory of tracks")));
            changed = true;
            break;
        case ID_btRemoveTrack:
        {
            const auto& selection = GetCtrl<ctrlList>(ID_lstSongs)->GetSelection();

            if(selection)
            {
                GetCtrl<ctrlList>(ID_lstSongs)->Remove(*selection);
                changed = true;
            }
        }
        break;
        case ID_btUp:
        {
            const auto& selection = GetCtrl<ctrlList>(ID_lstSongs)->GetSelection();

            if(selection && *selection > 0u)
                GetCtrl<ctrlList>(ID_lstSongs)->Swap(*selection - 1u, *selection);
        }
        break;
        case ID_btDown:
        {
            const auto& selection = GetCtrl<ctrlList>(ID_lstSongs)->GetSelection();

            if(selection && *selection < GetCtrl<ctrlList>(ID_lstSongs)->GetNumLines() - 1u)
                GetCtrl<ctrlList>(ID_lstSongs)->Swap(*selection + 1u, *selection);
        }
        break;
        case ID_btDecRepeat:
        {
            unsigned repeats = GetRepeats();
            if(repeats > 0u)
            {
                --repeats;
                SetRepeats(repeats);
                changed = true;
            }
        }
        break;
        case ID_btIncRepeat:
            SetRepeats(GetRepeats() + 1);
            changed = true;
            break;
        case ID_btRandom:
            SetRandomPlayback(!GetRandomPlayback());
            changed = true;
            break;
        case ID_btSave:
            if(SaveCurrentPlaylist())
            {
                WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Ok"), _("The playlist was saved!"), nullptr,
                                                              MsgboxButton::Ok, MsgboxIcon::ExclamationGreen));
            } else
            {
                WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), _("The specified file couldn't be saved!"),
                                                              nullptr, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
            }
            break;
    }
}

void iwMusicPlayer::Msg_Input(const unsigned win_id, const std::string& msg)
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
                GetCtrl<ctrlList>(ID_lstSongs)->AddString(msg);
                changed = true;
            } else
                WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), _("The specified file couldn't be opened!"),
                                                              this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
        }
        break;
        case ID_wndAddPlaylist:
        {
            // Playlist name must start with a letter
            const auto isLetter = [](const char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); };
            const boost::filesystem::path fullPlaylistPath = GetFullPlaylistPath(msg);
            if(!msg.empty() && isLetter(msg.front()) && Playlist().SaveAs(fullPlaylistPath))
            {
                UpdatePlaylistCombo(fullPlaylistPath.string());
                changed = true;
            } else
            {
                WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), _("The specified file couldn't be saved!"),
                                                              this, MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
            }
        }
        break;
        case ID_wndAddTrackDir:
        {
            std::vector<boost::filesystem::path> oggFiles = ListDir(msg, "ogg");

            for(const auto& oggFile : oggFiles)
                GetCtrl<ctrlList>(ID_lstSongs)->AddString(oggFile.string());

            changed = true;
        }
        break;
    }
}

unsigned iwMusicPlayer::GetRepeats() const
{
    return boost::lexical_cast<unsigned>(GetCtrl<ctrlTextDeepening>(ID_txtRepeat)->GetText());
}

void iwMusicPlayer::SetRepeats(unsigned repeats)
{
    GetCtrl<ctrlTextDeepening>(ID_txtRepeat)->SetText(helpers::toString(repeats));
}

bool iwMusicPlayer::GetRandomPlayback() const
{
    return GetCtrl<ctrlImageButton>(ID_btRandom)->GetImage() != LOADER.GetTextureN("io", 107);
}

void iwMusicPlayer::SetRandomPlayback(const bool random_playback)
{
    GetCtrl<ctrlImageButton>(ID_btRandom)
      ->SetImage(random_playback ? LOADER.GetTextureN("io", 225) : LOADER.GetTextureN("io", 107));
    GetCtrl<ctrlImageButton>(ID_btRandom)
      ->SetTooltip(random_playback ? _("Playback in this order") : _("Random playback"));
}

void iwMusicPlayer::UpdatePlaylistCombo(const std::string& highlight_entry)
{
    auto* cbPlaylist = GetCtrl<ctrlComboBox>(ID_cbPlaylist);
    cbPlaylist->DeleteAllItems();

    std::vector<boost::filesystem::path> playlists = ListDir(RTTRCONFIG.ExpandPath(s25::folders::playlists), "pll");
    playlists.insert(playlists.begin(), RTTRCONFIG.ExpandPath(s25::files::defaultPlaylist));
    const auto currentPath = RTTRCONFIG.ExpandPath(highlight_entry);

    unsigned i = 0;
    for(const auto& playlistPath : playlists)
    {
        // Reduce to pure filename
        cbPlaylist->AddString(playlistPath.stem().string());
        if(playlistPath == currentPath)
            cbPlaylist->SetSelection(i);
        ++i;
    }
    UpdateSaveChangeButtonsState();
}
