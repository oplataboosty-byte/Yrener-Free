//
// Created by n on 29.08.2025.
//

#ifndef REE_V3_HOOK_H
#define REE_V3_HOOK_H

#endif //REE_V3_HOOK_H
#include "main.h"



extern JNIEnv* thread_env;
#define qword long long



typedef void (*RakClient_Disconnect_t)(qword this_, unsigned int blockDuration, unsigned char orderingChannel);
static RakClient_Disconnect_t old_RakClient_Disconnect = nullptr;
static qword rakClientInstance = 0;

typedef bool (*InflictDamage_t)(qword this_, qword damagedBy, int method, float damage, int pedPiece, char direction);
static InflictDamage_t old_CPedInflictDamage = nullptr;
typedef void (*CVehicleInflictDamage_t)(qword this_, qword damagedBy, int weaponType, float damage, qword pos);
static CVehicleInflictDamage_t old_CVehicleInflictDamage = nullptr;
typedef void (*CBike_VehicleDamage_t)(qword this_);
static CBike_VehicleDamage_t old_CBike_VehicleDamage = nullptr;

void (*old_RakClient_Connect)(long long a1, char *a2, unsigned int a3, unsigned int a4, long long a5, unsigned int a6);
float (*FindGroundZ)(float, float);
bool CPedInfliDctamage(qword this_, qword damagedBy, int method, float damage, int pedPiece, char direction);
void (*old_SendOnFootSyncData)(qword a1);
void (*old_CNetGame_Process)();
void (*old_ChatWindowInputHandler)(const char* input);
char* (*SendChatMessage)(char* msg);
void (*old_OnJsonDataIncoming)(long long* a1, unsigned int guild, const char* json);
void (*AddChatMessage)(char* msg, ...);
void (*old_DialogBoxRPC)(RPCParameters* rpcParams);
void (*old_KeyBoard)();
void (*SendJsonData)(JNIEnv*, jclass, jint, jbyteArray);
void (*old_StopCamera)();
void (*old_WorldPlayerAdd)(RPCParameters* rpcParams);
void (*old_WorldVichleAdd)(RPCParameters* rpcParams);
void (*old_Collision)();
void (*old_Collision2)();
void (*old_Disconnect)(RPCParameters* rpcParams);
void (*Rpc_PlayerPoss_old)(RPCParameters* rpcParams);
void (*ping)(RPCParameters* rpcParams);
qword (*CPlayerPool_Delete)(long long a1, unsigned int);
qword (*CVehiclPool_Delete)(long long a1, unsigned int);
qword (*FindPlayerPed)();
void (*old_Java_com_blackhub_bronline_game_core_JNILib_multiTouchEvent)(JNIEnv* env, jclass cls, int actionMasked, int pointerId, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
