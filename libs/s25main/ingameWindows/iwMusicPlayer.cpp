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

#include "iwMusicPlayer.h"
#include "ListDir.h"
#include "Loader.h"
#include "MusicPlayer.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlList.h"
#include "controls/ctrlTextDeepening.h"
#include "files.h"
#include "helpers/toString.h"
#include "iwMsgbox.h"
#include "gameData/const_gui_ids.h"
#include "s25util/Log.h"
#include "s25util/StringConversion.h"
#include "s25util/colors.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

iwMusicPlayer::InputWindow::InputWindow(iwMusicPlayer& playerWnd, const unsigned win_id, const std::string& title)
    : IngameWindow(CGI_INPUTWINDOW, IngameWindow::posAtMouse, Extent(300, 100), title, LOADER.GetImageN("resource", 41), true),
      win_id(win_id), playerWnd_(playerWnd)
{
    AddEdit(0, DrawPoint(20, 30), Extent(GetSize().x - 40, 22), TC_GREEN2, NormalFont);
    AddTextButton(1, DrawPoint(20, 60), Extent(100, 22), TC_GREEN1, _("OK"), NormalFont);
    AddTextButton(2, DrawPoint(130, 60), Extent(100, 22), TC_RED1, _("Abort"), NormalFont);
}

void iwMusicPlayer::InputWindow::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id == 1)
        playerWnd_.Msg_Input(win_id, GetCtrl<ctrlEdit>(0)->GetText());

    Close();
}

void iwMusicPlayer::InputWindow::Msg_EditEnter(const unsigned /*ctrl_id*/)
{
    Msg_ButtonClick(1);
}

iwMusicPlayer::iwMusicPlayer()
    : IngameWindow(CGI_MUSICPLAYER, IngameWindow::posLastOrCenter, Extent(430, 330), _("Music player"), LOADER.GetImageN("resource", 41)),
      changed(false)
{
    AddList(0, DrawPoint(20, 30), Extent(330, 200), TC_GREEN1, NormalFont);
    AddText(1, DrawPoint(20, 240), _("Playlist:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddComboBox(2, DrawPoint(20, 260), Extent(330, 22), TC_GREEN1, NormalFont, 200);

    // Playlistbuttons
    const unsigned short button_distance = 10;
    const Extent buttonSize((330 - button_distance) / 2, 22);
    ctrlButton* b1 = AddTextButton(3, DrawPoint(20, 290), buttonSize, TC_GREEN2, _("Add"), NormalFont);
    AddTextButton(4, b1->GetPos() + DrawPoint(buttonSize.x + button_distance, 0), buttonSize, TC_GREEN2, _("Remove"), NormalFont);
    // AddTextButton(5,b1->GetPos().x,320,button_width,22,TC_GREEN2,_("Save"),NormalFont);
    // AddTextButton(6,b2->GetPos().x,320,button_width,22,TC_GREEN2,_("Load"),NormalFont);

    // Buttons für die Musikstücke
    AddImageButton(7, DrawPoint(370, 30), Extent(40, 40), TC_GREY, LOADER.GetImageN("io", 138), _("Add track"));
    AddImageButton(8, DrawPoint(370, 80), Extent(40, 40), TC_GREY, LOADER.GetImageN("io_new", 2), _("Add directory of tracks"));
    AddImageButton(9, DrawPoint(370, 130), Extent(40, 40), TC_RED1, LOADER.GetImageN("io", 220), _("Remove track"));
    AddImageButton(10, DrawPoint(370, 180), Extent(40, 15), TC_GREY, LOADER.GetImageN("io", 33), _("Upwards"));
    AddImageButton(11, DrawPoint(370, 195), Extent(40, 15), TC_GREY, LOADER.GetImageN("io", 34), _("Downwards"));
    AddTextDeepening(12, DrawPoint(370, 220), Extent(40, 20), TC_GREY, "1", NormalFont, COLOR_YELLOW);
    AddImageButton(13, DrawPoint(370, 240), Extent(20, 20), TC_RED1, LOADER.GetImageN("io", 139), _("Less repeats"));
    AddImageButton(14, DrawPoint(390, 240), Extent(20, 20), TC_GREY, LOADER.GetImageN("io", 138), _("More repeats"));
    AddImageButton(15, DrawPoint(370, 270), Extent(40, 40), TC_GREY, LOADER.GetImageN("io", 107), _("Playback in this order")); // 225

    // Mit Werten füllen
    MUSICPLAYER.GetPlaylist().FillMusicPlayer(this);
    UpdatePlaylistCombo(SETTINGS.sound.playlist);
}

iwMusicPlayer::~iwMusicPlayer()
{
    // Playlist ggf. speichern, die ausgewählt ist, falls eine ausgewählt ist
    unsigned short selection = GetCtrl<ctrlComboBox>(2)->GetSelection();

    // Entsprechende Datei speichern
    if(selection != 0xFFFF)
    {
        Playlist pl;
        pl.ReadMusicPlayer(this);

        std::string str(GetCtrl<ctrlComboBox>(2)->GetText(selection));

        // RTTR-Playlisten dürfen nicht gelöscht werden
        if(str == "S2_Standard")
            return;

        try
        {
            if(!pl.SaveAs(GetFullPlaylistPath(str), true))
                // Fehler, konnte nicht gespeichert werden
                WINDOWMANAGER.Show(
                  std::make_unique<iwMsgbox>(_("Error"), _("The specified file couldn't be saved!"), nullptr, MSB_OK, MSB_EXCLAMATIONRED));
        } catch(std::exception&)
        {
        }

        // Entsprechenden Dateipfad speichern
        SETTINGS.sound.playlist = GetCtrl<ctrlComboBox>(2)->GetText(selection);
    }

    // Werte in Musikplayer bringen
    if(changed)
    {
        MUSICPLAYER.GetPlaylist().ReadMusicPlayer(this);
        MUSICPLAYER.Play();
    }
}

void iwMusicPlayer::Msg_ComboSelectItem(const unsigned /*ctrl_id*/, const int selection)
{
    // Entsprechende Datei geladen
    if(selection != 0xFFFF)
    {
        Playlist pl;
        if(pl.Load(LOG, GetFullPlaylistPath(GetCtrl<ctrlComboBox>(2)->GetText(selection))))
        {
            // Das Fenster entsprechend mit den geladenen Werten füllen
            pl.FillMusicPlayer(this);
            changed = true;
        } else
            // Fehler, konnte nicht geladen werden
            WINDOWMANAGER.Show(
              std::make_unique<iwMsgbox>(_("Error"), _("The specified file couldn't be loaded!"), this, MSB_OK, MSB_EXCLAMATIONRED));
    }
}

void iwMusicPlayer::Msg_ListChooseItem(const unsigned /*ctrl_id*/, const unsigned selection)
{
    // Werte in Musikplayer bringen
    MUSICPLAYER.GetPlaylist().ReadMusicPlayer(this);
    MUSICPLAYER.GetPlaylist().SetStartSong(selection);
    MUSICPLAYER.Play();

    // Wir haben ab jetzt quasi keine Veränderungen mehr --> damit Musik nicht neugestartet werden muss
    changed = false;
}

std::string iwMusicPlayer::GetFullPlaylistPath(const std::string& name)
{
    return RTTRCONFIG.ExpandPath(s25::folders::music) + "/" + name + ".pll";
}

void iwMusicPlayer::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        // Add Playlist
        case 3: { WINDOWMANAGER.Show(std::make_unique<InputWindow>(*this, 1, _("Specify the playlist name")));
        }
        break;
        // Remove Playlist
        case 4:
        {
            unsigned short selection = GetCtrl<ctrlComboBox>(2)->GetSelection();

            // Entsprechende Datei löschen
            if(selection != 0xFFFF)
            {
                std::string str(GetCtrl<ctrlComboBox>(2)->GetText(selection));

                // RTTR-Playlisten dürfen nicht gelöscht werden
                if(str == "S2_Standard")
                {
                    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), _("You are not allowed to delete the standard playlist!"),
                                                                  this, MSB_OK, MSB_EXCLAMATIONRED));
                    return;
                }

                boost::system::error_code ec;
                boost::filesystem::remove(GetFullPlaylistPath(str), ec);
                this->UpdatePlaylistCombo(SETTINGS.sound.playlist);
            }
        }
        break;
        // Add Track
        case 7:
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<InputWindow>(*this, 0, _("Add track")));
            changed = true;
        }
        break;
        // Add Directory of tracks
        case 8:
        {
            WINDOWMANAGER.ToggleWindow(std::make_unique<InputWindow>(*this, 2, _("Add directory of tracks")));
            changed = true;
        }
        break;
        // Remove Track
        case 9:
        {
            unsigned short selection = GetCtrl<ctrlList>(0)->GetSelection();

            if(selection != 0xFFFF)
            {
                GetCtrl<ctrlList>(0)->Remove(selection);
                changed = true;
            }
        }
        break;
        // Upwards
        case 10:
        {
            unsigned short selection = GetCtrl<ctrlList>(0)->GetSelection();

            if(selection > 0 && selection != 0xFFFF)
                GetCtrl<ctrlList>(0)->Swap(selection - 1, selection);
        }
        break;
        // Downwards
        case 11:
        {
            unsigned short selection = GetCtrl<ctrlList>(0)->GetSelection();

            if(selection < GetCtrl<ctrlList>(0)->GetNumLines() - 1 && selection != 0xFFFF)
                GetCtrl<ctrlList>(0)->Swap(selection + 1, selection);
        }
        break;
        // Less Repeats
        case 13:
        {
            unsigned repeats = GetRepeats();

            if(repeats)
            {
                --repeats;
                SetRepeats(repeats);
                changed = true;
            }
        }
        break;
        // More Repeats
        case 14:
        {
            unsigned repeats = GetRepeats();
            ++repeats;
            SetRepeats(repeats);
            changed = true;
        }
        break;
        // Play Order
        case 15:
        {
            GetCtrl<ctrlImageButton>(15)->SetImage(GetCtrl<ctrlImageButton>(15)->GetImage() == LOADER.GetTextureN("io", 107) ?
                                                     LOADER.GetTextureN("io", 225) :
                                                     LOADER.GetTextureN("io", 107));
            GetCtrl<ctrlImageButton>(15)->SetTooltip(GetCtrl<ctrlImageButton>(15)->GetImage() == LOADER.GetTextureN("io", 107) ?
                                                       _("Playback in this order") :
                                                       _("Random playback"));
            changed = true;
        }
        break;
    }
}

void iwMusicPlayer::Msg_Input(const unsigned win_id, const std::string& msg)
{
    switch(win_id)
    {
        // Add Track - Window
        case 0:
        {
            bool valid = false;
            if(boost::filesystem::exists(msg))
                valid = true;
            else
            {
                // Evtl ein Siedlerstück ("sNN")?
                if(msg.length() == 3 && msg[0] == 's')
                {
                    if(s25util::fromStringClassicDef(msg.substr(1), 999u) <= 14u)
                        valid = true;
                }
            }

            // Gültiges Siedlerstück?
            if(valid)
            {
                // Hinzufügen
                GetCtrl<ctrlList>(0)->AddString(msg);
                changed = true;
            } else
                WINDOWMANAGER.Show(
                  std::make_unique<iwMsgbox>(_("Error"), _("The specified file couldn't be opened!"), this, MSB_OK, MSB_EXCLAMATIONRED));
        }
        break;
        // Add Playlist
        case 1:
        {
            // Ungültige Namen ausschließen
            const auto isLetter = [](const char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); };
            if(!msg.empty() && isLetter(msg.front()) && Playlist().SaveAs(msg, true))
            {
                // Combobox updaten
                UpdatePlaylistCombo(msg);
                changed = true;
            } else
            {
                // Fehler, konnte nicht gespeichert werden
                WINDOWMANAGER.Show(
                  std::make_unique<iwMsgbox>(_("Error"), _("The specified file couldn't be saved!"), this, MSB_OK, MSB_EXCLAMATIONRED));
            }
        }
        break;
        // Add Track directory of tracks - Window
        case 2:
        {
            std::vector<boost::filesystem::path> oggFiles = ListDir(msg, "ogg");

            for(const auto& oggFile : oggFiles)
                GetCtrl<ctrlList>(0)->AddString(oggFile.string());

            changed = true;
        }
        break;
    }
}

void iwMusicPlayer::SetSegments(const std::vector<std::string>& segments)
{
    GetCtrl<ctrlList>(0)->DeleteAllItems();

    for(const auto& segment : segments)
        GetCtrl<ctrlList>(0)->AddString(segment);
}
void iwMusicPlayer::SetRepeats(unsigned repeats)
{
    GetCtrl<ctrlTextDeepening>(12)->SetText(helpers::toString(repeats));
}

void iwMusicPlayer::SetRandomPlayback(const bool random_playback)
{
    GetCtrl<ctrlImageButton>(15)->SetImage(random_playback ? LOADER.GetTextureN("io", 225) : LOADER.GetTextureN("io", 107));
}

void iwMusicPlayer::SetCurrentSong(const unsigned selection)
{
    GetCtrl<ctrlList>(0)->SetSelection(selection);
}

std::vector<std::string> iwMusicPlayer::GetSegments() const
{
    std::vector<std::string> segments;
    for(unsigned i = 0; i < GetCtrl<ctrlList>(0)->GetNumLines(); ++i)
        segments.push_back(GetCtrl<ctrlList>(0)->GetItemText(i));
    return segments;
}

unsigned iwMusicPlayer::GetRepeats() const
{
    return boost::lexical_cast<unsigned>(GetCtrl<ctrlTextDeepening>(12)->GetText());
}

bool iwMusicPlayer::GetRandomPlayback() const
{
    return !(GetCtrl<ctrlImageButton>(15)->GetImage() == LOADER.GetTextureN("io", 107));
}

/// Updatet die Playlist - Combo
void iwMusicPlayer::UpdatePlaylistCombo(const std::string& highlight_entry)
{
    GetCtrl<ctrlComboBox>(2)->DeleteAllItems();

    std::vector<boost::filesystem::path> playlists = ListDir(RTTRCONFIG.ExpandPath(s25::folders::music), "pll");

    unsigned i = 0;
    for(const auto& playlistPath : playlists)
    {
        // Reduce to pure filename
        const auto name = playlistPath.stem().string();
        GetCtrl<ctrlComboBox>(2)->AddString(name);
        if(name == highlight_entry)
            GetCtrl<ctrlComboBox>(2)->SetSelection(i);
        ++i;
    }
}
