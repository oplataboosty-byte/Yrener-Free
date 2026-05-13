//
// Created by n on 01.05.2025.
//
#pragma once
using namespace std;



//
// Created by n on 12.07.2025.
//
#include "bools.h"

//======hthtvtyyst============


FlooderSettings flooder = {
        .state = false,
        .fl = false,
        .garage = false,
        .delay = 1,
        .message = nullptr,
        .lastSendTime = 0,
        .waitInputDelay = false,
        .quantity = 0
};

CpedSetting CPed = {
        .slap = false,
        .posX = 0x38,
        .posY = 0x3C,
        .posZ = 0x40,
        .bias = 0
};

FastCommandSettings fastCommand = {
        .state = false,
        .processing = false,
        .lastCommand = nullptr,
        .bh = false,
        .cgp = false,
        .smh = false,
        .fs = false,
        .hm = false
};

GlobalSetStruct global = {
        .main_time = 0
};

FunctionFlags func = {
        .opt = false,
        .dell = false,
        .collision = false,
        .afk = false,
        .camfreeze = false,
        .ldm = false,
        .flist = false,
        .time = false,
        .ree = false,
        .autoArmor = false,
        .timer = false,
        .menuOpen = false,
        .notloadvichle = false,
        .antiafk = false,
        .notloadplayer = false,
        .openchat = true,
        .bypassDamage = false,
        .underwaterPed = false,
        .underwaterVch = false,
        .bypassDamagePed = false,
        .bypassDamageVch = false,
        .reconnect = false,
        .autoSendGevent = false,
        .spawnMessageMoment = 0,
        .blockDisconnect = false
};

JsonHandling json = {
        .ab = false,
        .besk = false,
        .state = false,
        .fo = false,
        .ap = false,
        .fo2 = false,
        .wo = false
};

AutosendSettings autosend = {
        .state = false,
        .connected = false,
        .spawnMoment = 0,
        .awaitingMoment = false,
        .waitInputDelay = false,
        .delay = 0,
        .task = 0,
        .fastspawn = false,
        .fsatspawensend = false,
        .dely = 250,
        .fsatspawensendTime =0,
        .send = 5

};
DebugSettings debug{
        .stateGloverment = false,
        .stateCommandTest  = false,
        .spawnMoment  = false,
        .awaitingMoment  = false,
        .waitInputDelay  = false,
        .startmessage  = false,
        .hostcheck  = false,
        .hostcheckResult  = false,
        .delay  = false,
        .cb  = false
};
DeliterSetting deliter {
        .state = false,
        .numdel = 0
};
SpeedSetting speed {
        .waitInput = false,
        .value = 1
};


