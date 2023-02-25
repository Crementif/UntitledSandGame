#pragma once
#include "../common/types.h"

enum NET_ACTION : u8
{
    // client to server
    NET_ACTION_C_START_GAME = 0x01,

    // server to client
    NET_ACTION_S_START_GAME = 0x40,


};