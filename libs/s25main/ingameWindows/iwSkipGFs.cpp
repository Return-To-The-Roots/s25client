// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwSkipGFs.h"
#include "Loader.h"
#include "controls/ctrlEdit.h"
#include "helpers/Range.h"
#include "network/GameClient.h"
#include "gameData/const_gui_ids.h"
#include "s25util/StringConversion.h"
#include "s25util/colors.h"

namespace {
enum
{
    ID_lblToGf,
    ID_edtToGf,
    ID_btToGf,
    ID_lblByGf,
    ID_edtByGf,
    ID_btByGf,
    ID_btJumpPresetStart, // Must be last, one per jump preset
};
constexpr std::array jumpPresets = {
  100,
  1000,
  5000,
  10000,
};
} // namespace

iwSkipGFs::iwSkipGFs(GameWorldView& gwv)
    : IngameWindow(CGI_SKIPGFS, IngameWindow::posLastOrCenter, Extent(300, 120), _("Skip GameFrames"),
                   LOADER.GetImageN("resource", 41)),
      gwv(gwv)
{
    constexpr auto lblWidth = 135;
    const auto edtOffset = contentOffset.x + lblWidth;
    constexpr auto spacing = 5;
    constexpr Extent edtSize(100, 20);
    Extent btSize(GetIwSize().x - edtOffset - edtSize.x - spacing, edtSize.y);

    DrawPoint curPos(edtOffset, 28);
    ctrlEdit* edit = AddEdit(ID_edtToGf, curPos, edtSize, TextureColor::Grey, NormalFont);
    edit->SetFocus();
    curPos.x -= spacing;
    AddText(ID_lblToGf, curPos + DrawPoint(0, 4), _("to GameFrame:"), COLOR_YELLOW, FontStyle::RIGHT, NormalFont);
    curPos.x += edtSize.x + spacing * 2;
    AddTextButton(ID_btToGf, curPos, btSize, TextureColor::Green2, "->|", NormalFont);

    curPos.x = edtOffset;
    curPos.y += edtSize.y + 5;

    AddEdit(ID_edtByGf, curPos, edtSize, TextureColor::Grey, NormalFont);
    curPos.x -= spacing;
    AddText(ID_lblByGf, curPos + DrawPoint(0, 4), _("by GameFrames:"), COLOR_YELLOW, FontStyle::RIGHT, NormalFont);
    curPos.x += edtSize.x + spacing * 2;
    AddTextButton(ID_btByGf, curPos, btSize, TextureColor::Green2, "-->", NormalFont);

    curPos.x = contentOffset.x + spacing;
    curPos.y += edtSize.y + 5;
    const auto availWidth = GetIwSize().x - spacing * 2;
    btSize.x = (availWidth - spacing * (jumpPresets.size() - 1)) / jumpPresets.size();
    for(auto i : helpers::range(jumpPresets.size()))
    {
        AddTextButton(ID_btJumpPresetStart + i, curPos, btSize, TextureColor::Green1,
                      "+" + std::to_string(jumpPresets[i]), NormalFont);
        curPos.x += btSize.x + spacing;
    }
}

void iwSkipGFs::SkipGFs(const unsigned edtCtrlId)
{
    int targetGF = s25util::fromStringClassicDef(GetCtrl<ctrlEdit>(edtCtrlId)->GetText(), 0);
    if(edtCtrlId == ID_edtByGf)
        targetGF += GAMECLIENT.GetGFNumber();
    GAMECLIENT.SkipGF(targetGF, gwv);
}

void iwSkipGFs::Msg_ButtonClick(const unsigned ctrlId)
{
    if(ctrlId < ID_btJumpPresetStart)
        SkipGFs((ctrlId == ID_btByGf) ? ID_edtByGf : ID_edtToGf);
    else
    {
        const unsigned targetGF = GAMECLIENT.GetGFNumber() + jumpPresets[ctrlId - ID_btJumpPresetStart];
        GAMECLIENT.SkipGF(targetGF, gwv);
    }
}

void iwSkipGFs::Msg_EditEnter(const unsigned ctrlId)
{
    SkipGFs(ctrlId);
}
