// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "s25util/MessageInterface.h"
#include "s25util/warningSuppression.h"
#include <boost/preprocessor.hpp>

// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define __GENERATE_FWD_DECL_SINGLE(s, data, expression) class expression;
#define __GENERATE_FWD_DECL(...) \
    BOOST_PP_SEQ_FOR_EACH(__GENERATE_FWD_DECL_SINGLE, 0, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define __GENERATE_CALLBACK_SINGLE(s, data, expression) \
    virtual bool OnGameMessage(const expression& /*msg*/) { return false; }
#define __GENERATE_CALLBACKS(...) \
    BOOST_PP_SEQ_FOR_EACH(__GENERATE_CALLBACK_SINGLE, 0, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

/// Generates the class "GameMessageInterface" with one virtual method "OnGameMessage" for each parameter
/// Also handles the required forward declaration of the classes
/// Example: GENERATE_GAME_MESSAGE_INTERFACE(GM_Foo, GM_Bar) -->
///     class GM_Foo; class GM_Bar;
///     class GameMessageInterface (...)
///         virtual bool OnGameMessage(const GM_Foo& /*msg*/){}
///         virtual bool OnGameMessage(const GM_Bar& /*msg*/){}
///     };
///
#define GENERATE_GAME_MESSAGE_INTERFACE(...)             \
    __GENERATE_FWD_DECL(__VA_ARGS__)                     \
    class GameMessageInterface : public MessageInterface \
    {                                                    \
    protected:                                           \
        ~GameMessageInterface() override {}              \
                                                         \
    public:                                              \
        __GENERATE_CALLBACKS(__VA_ARGS__)                \
    };

RTTR_IGNORE_OVERLOADED_VIRTUAL
GENERATE_GAME_MESSAGE_INTERFACE(GameMessage_Ping, GameMessage_Pong,

                                GameMessage_Server_Type, GameMessage_Server_TypeOK, GameMessage_Server_Password,
                                GameMessage_Server_Name, GameMessage_Server_Start, GameMessage_Chat,
                                GameMessage_Server_Async, GameMessage_Countdown, GameMessage_CancelCountdown,

                                GameMessage_Player_Id, GameMessage_Player_Name, GameMessage_Player_List,
                                GameMessage_Player_State, GameMessage_Player_Nation, GameMessage_Player_Team,
                                GameMessage_Player_Color, GameMessage_Player_Kicked, GameMessage_Player_Ping,
                                GameMessage_Player_New, GameMessage_Player_Ready, GameMessage_Player_Swap,
                                GameMessage_Player_SwapConfirm,

                                GameMessage_Map_Info, GameMessage_MapRequest, GameMessage_Map_Data,
                                GameMessage_Map_Checksum, GameMessage_Map_ChecksumOK, GameMessage_GGSChange,
                                GameMessage_RemoveLua, GameMessage_Pause, GameMessage_SkipToGF,
                                GameMessage_Server_NWFDone, GameMessage_GameCommand, GameMessage_Speed,

                                GameMessage_GetAsyncLog, GameMessage_AsyncLog)
RTTR_POP_DIAGNOSTIC
