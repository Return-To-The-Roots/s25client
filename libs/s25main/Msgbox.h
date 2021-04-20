// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

enum class MsgboxButton
{
    Ok,
    OkCancel,
    YesNo,
    YesNoCancel
};
constexpr auto maxEnumValue(MsgboxButton)
{
    return MsgboxButton::YesNoCancel;
}

enum class MsgboxIcon
{
    QuestionGreen = 72,
    ExclamationGreen,
    QuestionRed,
    ExclamationRed
};

enum class MsgboxResult
{
    Ok,
    Cancel,
    Yes,
    No
};
