//
// Created by n on 12.07.2025.
//

#ifndef REE_V3_BOOLS_H
#define REE_V3_BOOLS_H

#endif //REE_V3_BOOLS_H

#pragma once



// ==================== Основные структуры ====================
#include <cstddef>
#include "bools.h"
#include "../qword.h"



struct FlooderSettings {
    bool state;
    bool fl;
    bool garage;
    int delay;
    char* message;
    long lastSendTime;
    bool waitInputDelay;
    bool quantity;
};

struct FastCommandSettings {
    bool state;
    bool processing;
    char* lastCommand;
    bool bh;
    bool cgp;
    bool smh;
    bool fs;
    bool hm;
};

struct FunctionFlags {
    bool opt;
    bool dell;
    bool collision;
    bool afk;
    bool camfreeze;
    bool ldm;
    bool flist;
    bool time;
    bool ree;
    bool autoArmor;
    bool timer;
    bool menuOpen;
    bool notloadvichle;
    bool antiafk;
    bool notloadplayer;
    bool openchat;
    bool bypassDamage;
    bool underwaterPed;
    bool underwaterVch;
    bool bypassDamagePed;
    bool bypassDamageVch;
    bool reconnect;
    bool autoSendGevent;
    int spawnMessageMoment;
    bool blockDisconnect;
};

struct JsonHandling {
    bool ab;
    bool besk;
    bool state;
    bool fo;
    bool ap;
    bool fo2;
    bool wo;
};

struct CpedSetting{
    bool slap;
    qword posX;
    qword posY;
    qword posZ;
    qword bias;
};

struct AutosendSettings {
    bool state;
    bool connected;
    long spawnMoment;
    bool awaitingMoment;
    bool waitInputDelay;
    int delay;
    int task;
    bool fastspawn;
    bool fsatspawensend;
    int dely;
    int fsatspawensendTime;
    int send;
};
struct DeliterSetting {
    bool state;
    int numdel;
};
struct DebugSettings {
    bool stateGloverment;
    bool stateCommandTest;
    long spawnMoment;
    bool awaitingMoment;
    bool waitInputDelay;
    bool startmessage;
    bool hostcheck;
    bool hostcheckResult;
    int delay;
    bool cb;
};
struct GlobalSetStruct{
    int main_time;
};

struct SpeedSetting {
    bool waitInput;
    int value;
} ;

extern CpedSetting CPed;
extern FlooderSettings flooder;
extern FastCommandSettings fastCommand;
extern FunctionFlags func;
extern JsonHandling json;
extern AutosendSettings autosend;
extern DebugSettings debug;
extern GlobalSetStruct global;
extern DeliterSetting deliter;
extern SpeedSetting speed;

struct {
    bool waitinputmsg;
    bool waitinputdelay;
    char* msg;
    int delay;
} Flooder;



//jnltkmyst gthtvtyyst

