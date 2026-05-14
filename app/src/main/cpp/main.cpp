#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <ctime>
#include <android/log.h>
#include <dlfcn.h>
#include <jni.h>
#include <pthread.h>
#include <cstring>
#include "main.h"
#include "MenuConfig.h"
#include "GuiUtils.h"
#include "BackgroundAnim.h"
#include "wLibs/Dobby/dobby.h"
#include "wLibs/res/alpha_icon_image.h"
#include "wLibs/ImGui/imgui.h"
#include "wLibs/ImGui/imgui_internal.h"
#include "wLibs/ImGui/backends/imgui_impl_opengl3.h"
#include "func/bools.h"
#include "setting.h"
#include "wLibs/Utils.h"

#ifndef qword
#define qword unsigned long long
#endif

extern qword g_PlayerPoolPtr;

JavaVM* g_JavaVM = nullptr;
JNIEnv* g_MainEnv = nullptr;
int glWidth = 0;
int glHeight = 0;
bool is_setup = false;
MenuState g_Menu;
float g_SaveTimer = 0.0f;
float g_AdTimer = 0.0f;  // NEW: timer for chat ads
const float AD_INTERVAL = 120.0f;  // 2 minutes in seconds
qword libaddr = 0;
bool debugmsg = false;
bool logSendJsonData = true;
bool logIncomingJson = true;
struct JsonLogEntry {
    int guiId;
    std::string data;
    bool isOutgoing;
    int jsonType;
    time_t timestamp;
};
std::vector<JsonLogEntry> g_JsonLog;
pthread_mutex_t g_JsonLogMutex = PTHREAD_MUTEX_INITIALIZER;
const int MAX_JSON_LOG = 100;

bool g_IconTouchActive = false;
ImVec2 g_IconTouchStart;
ImVec2 g_IconPosStart;

// ==================== PLAYER ACCESS HELPERS ====================
// CPlayer structure offsets
constexpr qword OFFSET_HEALTH = 0xB4;
constexpr qword OFFSET_ARMOR = 0xBC;
constexpr qword OFFSET_POS_X = 0x18C;
constexpr qword OFFSET_POS_Y = 0x190;
constexpr qword OFFSET_POS_Z = 0x194;
constexpr qword OFFSET_VEL_Z = 0x1AC;
constexpr qword OFFSET_VEHICLE_PTR = 0x3B4;
constexpr qword OFFSET_WEAPON_SLOT = 0x5A0;
constexpr qword OFFSET_WEAPON_ARRAY = 0x5A8;

// PlayerPool offsets
constexpr qword OFFSET_POOL_LOCAL_PLAYER = 0x610;
constexpr qword OFFSET_POOL_ALL_PLAYERS = 0x640;

// ViewMatrix offset (pointer to pointer)
constexpr qword OFFSET_VIEWMATRIX_PTR = 0x264F700;

// Get local player pointer from pool
qword GetLocalPlayer() {
    if (g_PlayerPoolPtr == 0) return 0;
    qword pool = *(qword*)g_PlayerPoolPtr;
    if (pool == 0) return 0;
    qword localPlayer = *(qword*)(pool + OFFSET_POOL_LOCAL_PLAYER);
    return localPlayer;
}

// Get player health
float GetPlayerHealth(qword playerPtr) {
    if (playerPtr == 0) return 0.0f;
    return *(float*)(playerPtr + OFFSET_HEALTH);
}

// Set player health
void SetPlayerHealth(qword playerPtr, float health) {
    if (playerPtr == 0) return;
    *(float*)(playerPtr + OFFSET_HEALTH) = health;
}

// Get player armor
float GetPlayerArmor(qword playerPtr) {
    if (playerPtr == 0) return 0.0f;
    return *(float*)(playerPtr + OFFSET_ARMOR);
}

// Set player armor
void SetPlayerArmor(qword playerPtr, float armor) {
    if (playerPtr == 0) return;
    *(float*)(playerPtr + OFFSET_ARMOR) = armor;
}

// Get player position
void GetPlayerPos(qword playerPtr, float& x, float& y, float& z) {
    if (playerPtr == 0) {
        x = y = z = 0.0f;
        return;
    }
    x = *(float*)(playerPtr + OFFSET_POS_X);
    y = *(float*)(playerPtr + OFFSET_POS_Y);
    z = *(float*)(playerPtr + OFFSET_POS_Z);
}

// Set player position
void SetPlayerPos(qword playerPtr, float x, float y, float z) {
    if (playerPtr == 0) return;
    *(float*)(playerPtr + OFFSET_POS_X) = x;
    *(float*)(playerPtr + OFFSET_POS_Y) = y;
    *(float*)(playerPtr + OFFSET_POS_Z) = z;
}

// Check if player in vehicle
bool IsPlayerInVehicle(qword playerPtr) {
    if (playerPtr == 0) return false;
    qword vehPtr = *(qword*)(playerPtr + OFFSET_VEHICLE_PTR);
    return vehPtr != 0;
}

// Get all players array
qword GetPlayerPool() {
    if (g_PlayerPoolPtr == 0) return 0;
    qword pool = *(qword*)g_PlayerPoolPtr;
    return pool;
}

// Get player by ID from pool
qword GetPlayerByID(int playerID) {
    qword pool = GetPlayerPool();
    if (pool == 0) return 0;
    if (playerID < 0 || playerID >= 1000) return 0;
    
    qword allPlayersArray = *(qword*)(pool + OFFSET_POOL_ALL_PLAYERS);
    if (allPlayersArray == 0) return 0;
    
    qword playerPtr = *(qword*)(allPlayersArray + playerID * 8);
    return playerPtr;
}

// Check if player is valid/spawned
bool IsPlayerValid(qword playerPtr) {
    if (playerPtr == 0) return false;
    float health = GetPlayerHealth(playerPtr);
    return health > 0.0f;
}

// WorldToScreen - convert 3D world coordinates to 2D screen coordinates
bool WorldToScreen(float worldX, float worldY, float worldZ, float& screenX, float& screenY) {
    if (libaddr == 0) return false;
    
    // Get ViewMatrix pointer
    qword viewMatrixPtr = *(qword*)(libaddr + OFFSET_VIEWMATRIX_PTR);
    if (viewMatrixPtr == 0) return false;
    
    float* viewMatrix = (float*)viewMatrixPtr;
    
    // Matrix multiply: screenPos = viewMatrix * worldPos
    float w = viewMatrix[3] * worldX + viewMatrix[7] * worldY + viewMatrix[11] * worldZ + viewMatrix[15];
    
    if (w < 0.01f) return false; // Behind camera
    
    float x = viewMatrix[0] * worldX + viewMatrix[4] * worldY + viewMatrix[8] * worldZ + viewMatrix[12];
    float y = viewMatrix[1] * worldX + viewMatrix[5] * worldY + viewMatrix[9] * worldZ + viewMatrix[13];
    
    // Perspective divide
    x /= w;
    y /= w;
    
    // NDC to screen space
    int screenWidth = g_Menu.screenWidth > 0 ? g_Menu.screenWidth : 1920;
    int screenHeight = g_Menu.screenHeight > 0 ? g_Menu.screenHeight : 1080;
    
    screenX = (screenWidth / 2.0f) + (x * screenWidth / 2.0f);
    screenY = (screenHeight / 2.0f) - (y * screenHeight / 2.0f);
    
    return (screenX >= 0 && screenX <= screenWidth && screenY >= 0 && screenY <= screenHeight);
}
// ==================== END PLAYER HELPERS ====================


constexpr jint kDialogGuiId = 10;
constexpr jint kFilterGuiId = 50;
constexpr jint kServerSelectGuiId = 76;
constexpr float kFloatingIconSafePadding = 28.0f;
constexpr float kMenuWidth = 760.0f;
constexpr float kMenuHeight = 420.0f;
constexpr float kMenuHeaderHeight = 56.0f;
constexpr const char kDialogTypePromptJson[] =
        "{\"r\":1,\"i\":\"Тип:\",\"l\":0}";
constexpr const char kFoSpawnReplyJson[] = "{\"t\":2}";
constexpr const char kWorkOfferReplyJson[] = "{\"t\":2}";
constexpr const char kServerSelectReplyJson[] = "{\"g\":2}";

float GetMenuVisualScale() {
    const float requestedScale = fmaxf(g_Menu.menuScale, 0.5f);
    if (glWidth <= 0 || glHeight <= 0) {
        return requestedScale;
    }

    const float safeWidth = fmaxf(320.0f, glWidth - 24.0f);
    const float safeHeight = fmaxf(180.0f, glHeight - 24.0f);
    const float fitScale = fminf(safeWidth / kMenuWidth, safeHeight / kMenuHeight);
    return fmaxf(0.35f, fminf(requestedScale, fitScale));
}

float GetMenuWidthScaled() {
    return kMenuWidth * GetMenuVisualScale();
}

float GetMenuHeightScaled() {
    return kMenuHeight * GetMenuVisualScale();
}

float GetMenuHeaderHeightScaled() {
    return kMenuHeaderHeight * GetMenuVisualScale();
}

EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface) = nullptr;
void (*old_Java_multiTouchEvent)(JNIEnv*, jclass, int, int, int, int, int, int, int, int) = nullptr;
void (*old_SendOnFootSyncData)(qword a1) = nullptr;
void (*old_CNetGame_Process)();
void (*old_LocalPlayer_Tick)(qword a1) = nullptr;  // NEW: replaced CNetGame_Process
void (*old_PedFight_fn)(qword a1, qword a2) = nullptr;  // NEW: damage handler
void (*old_JNILib_sendJsonData)(JNIEnv*, jclass, jint, jbyteArray) = nullptr;
void (*Call_JNILib_sendJsonData)(JNIEnv*, jclass, jint, jbyteArray) = nullptr;
void (*old_OnJsonDataIncoming_v2)(qword a1, unsigned int guiId, const char* jsonStr) = nullptr;
char* (*SendChatMessage)(char* msg) = nullptr;
void (*AddChatMessage)(char* msg, ...) = nullptr;
qword (*CPlayerPool_Delete)(qword a1, unsigned int playerId) = nullptr;
qword (*FindPlayerPed)() = nullptr;
float (*FindGroundZ)(float x, float y) = nullptr;
void (*old_DialogBoxRPC)() = nullptr;

// NEW: PlayerPool globals
qword g_PlayerPoolPtr = 0;  // Will be: libaddr + 0x2660610

int ExtractJsonType(const std::string& data) {
    size_t pos = data.find("\"t\":");
    if (pos == std::string::npos) {
        return -1;
    }

    pos += 4;
    while (pos < data.size() && (data[pos] == ' ' || data[pos] == '\t')) {
        ++pos;
    }

    bool negative = false;
    if (pos < data.size() && data[pos] == '-') {
        negative = true;
        ++pos;
    }

    int value = 0;
    bool hasDigits = false;
    while (pos < data.size() && data[pos] >= '0' && data[pos] <= '9') {
        hasDigits = true;
        value = (value * 10) + (data[pos] - '0');
        ++pos;
    }

    if (!hasDigits) {
        return -1;
    }
    return negative ? -value : value;
}

void AddJsonToLog(int guiId, const std::string& data, bool isOutgoing) {
    pthread_mutex_lock(&g_JsonLogMutex);

    JsonLogEntry entry;
    entry.guiId = guiId;
    entry.data = data;
    entry.isOutgoing = isOutgoing;
    entry.jsonType = ExtractJsonType(data);
    entry.timestamp = time(nullptr);

    g_JsonLog.push_back(entry);

    while (g_JsonLog.size() > MAX_JSON_LOG) {
        g_JsonLog.erase(g_JsonLog.begin());
    }

    pthread_mutex_unlock(&g_JsonLogMutex);
}

void DialogBoxRPC();
void OnJsonDataIncoming(long long* a1, unsigned int guild, const char* json_str);
bool SafeSendJsonData(jint guiId, const char* jsonData);
bool TryGetLocalPedPosition(float& x, float& y, float& z);
void Hook_JNILib_sendJsonData(JNIEnv* env, jclass clazz, jint guiId, jbyteArray data) {
    std::string jsonStr;
    if (env && data) {
        const jsize length = env->GetArrayLength(data);
        if (length > 0 && length < 100000) {
            jbyte* bytes = env->GetByteArrayElements(data, nullptr);
            if (bytes) {
                jsonStr.assign(reinterpret_cast<const char*>(bytes), static_cast<size_t>(length));
                env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
            }
        }
    }


    if (!jsonStr.empty() && (jsonStr.front() == '{' || jsonStr.front() == '[')) {
        if (logSendJsonData) {
            AddJsonToLog(guiId, jsonStr, true);
        }


        if (AddChatMessage && debugmsg) {
            if (jsonStr.length() < 120) {
                AddChatMessage((char*)"{FFD700}[OUT]{FFFFFF} G:%d t:%d | %s",
                               guiId, ExtractJsonType(jsonStr), jsonStr.c_str());
            } else {
                AddChatMessage((char*)"{FFD700}[OUT]{FFFFFF} G:%d t:%d len=%zu",
                               guiId, ExtractJsonType(jsonStr), jsonStr.length());
            }
        }
    }
    if (old_JNILib_sendJsonData) {
        old_JNILib_sendJsonData(env, clazz, guiId, data);
    }
}


void Hook_OnJsonDataIncoming_v2(qword a1, unsigned int guiId, const char* jsonStr) {
    OnJsonDataIncoming(reinterpret_cast<long long*>(a1), guiId, jsonStr);
}


void DialogBoxRPC() {
    if (json.ap || json.ab) {
        SafeSendJsonData(kDialogGuiId, kDialogTypePromptJson);
        return;
    }

    if (old_DialogBoxRPC) {
        old_DialogBoxRPC();
    }
}

void OnJsonDataIncoming(long long* a1, unsigned int guild, const char* json_str) {
    if (json_str && logIncomingJson) {
        std::string jsonRaw(json_str);
        std::string jsonUtf8 = jsonRaw;
        AddJsonToLog((int)guild, jsonUtf8, false);
        if (AddChatMessage && debugmsg) {
            AddChatMessage((char*)"{66CDAA}[R]{FFFFFF} guild=%u | json=%s", guild, jsonUtf8.c_str());
        }
    }

    if (guild == kFilterGuiId && json.fo) {
        SafeSendJsonData(kFilterGuiId, kFoSpawnReplyJson);
        return;
    }

    if (json.wo && guild == 40) {
        SafeSendJsonData(kFilterGuiId, kWorkOfferReplyJson);
        return;
    }

    if (guild == static_cast<unsigned int>(-1)) {
        SafeSendJsonData(kServerSelectGuiId, kServerSelectReplyJson);
        return;
    }

    if (old_OnJsonDataIncoming_v2) {
        old_OnJsonDataIncoming_v2((qword)a1, guild, json_str);
    }
}


void LocalPlayer_Tick(qword a1) {
    // AD MESSAGE - every 2 minutes
    static bool adTimerInitialized = false;
    if (!adTimerInitialized) {
        g_AdTimer = 0.0f;
        adTimerInitialized = true;
    }
    
    // Increment timer (assuming ~60 ticks per second)
    g_AdTimer += 1.0f / 60.0f;
    
    if (g_AdTimer >= AD_INTERVAL) {
        if (AddChatMessage) {
            AddChatMessage((char*)"{00FF00}Тг Проекта: {FFFFFF}@Yrener_Soft");
        }
        g_AdTimer = 0.0f;  // Reset timer
    }
    
    // FLOODER - spam commands every tick
    if (flooder.fl) {
        SendChatMessage((char*)"/buyhouse");
    }

    if (flooder.garage) {
        SendChatMessage((char*)"/buygarage");
    }

    // GOD MODE - set health/armor to max
    if (func.collision) {
        qword player = GetLocalPlayer();
        if (player != 0) {
            SetPlayerHealth(player, 100.0f);
            SetPlayerArmor(player, 100.0f);
        }
    }

    // Call original function
    if (old_LocalPlayer_Tick) {
        old_LocalPlayer_Tick(a1);
    }
}

void PedFight_fn(qword a1, qword a2) {
    // BYPASS DAMAGE - block damage if godmode enabled
    if (func.collision) {
        return;  // Don't take damage
    }

    // Call original
    if (old_PedFight_fn) {
        old_PedFight_fn(a1, a2);
    }
}

void CNetGame_Process() {
    // LEGACY - keep for compatibility but unused now
    if (old_CNetGame_Process) {
        old_CNetGame_Process();
    }
}

void SendOnFootSyncData(qword a1) {
    if (func.afk) return;
    if (old_SendOnFootSyncData) {
        old_SendOnFootSyncData(a1);
    }
}

bool SafeSendJsonData(jint guiId, const char* jsonData) {
    void (*sendJsonFn)(JNIEnv*, jclass, jint, jbyteArray) =
    old_JNILib_sendJsonData ? old_JNILib_sendJsonData : Call_JNILib_sendJsonData;
    if (!sendJsonFn || !g_JavaVM || !jsonData || !*jsonData) {
        return false;
    }

    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (g_JavaVM->AttachCurrentThread(&env, nullptr) != JNI_OK || !env) {
            return false;
        }
        attached = true;
    }

    const jsize length = static_cast<jsize>(strlen(jsonData));
    jbyteArray byteArray = env->NewByteArray(length);
    if (!byteArray) {
        if (attached) {
            g_JavaVM->DetachCurrentThread();
        }
        return false;
    }

    env->SetByteArrayRegion(byteArray, 0, length, reinterpret_cast<const jbyte*>(jsonData));
    sendJsonFn(env, nullptr, guiId, byteArray);
    env->DeleteLocalRef(byteArray);

    if (logSendJsonData) {
        AddJsonToLog(guiId, jsonData, true);
    }

    if (attached) {
        g_JavaVM->DetachCurrentThread();
    }
    return true;
}
bool TryGetLocalPlayerHealth(float& health) {
    qword player = GetLocalPlayer();
    if (player == 0) {
        return false;
    }
    health = GetPlayerHealth(player);
    return true;
}

float x1, y2, z3;


bool TryGetLocalPedPosition(float& x, float& y, float& z) {
    qword player = GetLocalPlayer();
    if (player == 0) {
        return false;
    }

    GetPlayerPos(player, x, y, z);
    
    // Fast spawn logic - save/restore position
    if (func.autoArmor) {
        if (z >= 1500) {
            SetPlayerPos(player, x1, y2, z3);
            x = x1;
            y = y2;
            z = z3;
        } else {
            x1 = x;
            y2 = y;
            z3 = z;
        }
    }
    
    return true;
}

void ClampFloatingIconPosition(float iconSize) {
    const float windowSize = iconSize + 20.0f;
    const float minX = kFloatingIconSafePadding;
    const float minY = kFloatingIconSafePadding;
    const float maxX = fmaxf(minX, glWidth - windowSize - kFloatingIconSafePadding);
    const float maxY = fmaxf(minY, glHeight - windowSize - kFloatingIconSafePadding);

    g_Menu.iconPos.x = fmaxf(minX, fminf(g_Menu.iconPos.x, maxX));
    g_Menu.iconPos.y = fmaxf(minY, fminf(g_Menu.iconPos.y, maxY));
}


bool IsTouchInIconArea(float x, float y) {
    float iconSize = 70.0f * g_Menu.menuScale * g_Menu.iconScale;
    float dx = x - (g_Menu.iconPos.x + iconSize * 0.5f);
    float dy = y - (g_Menu.iconPos.y + iconSize * 0.5f);
    return sqrtf(dx*dx + dy*dy) < iconSize * 0.6f;
}

bool IsTouchInMenuArea(float x, float y) {
    if (!g_Menu.isOpen || g_Menu.animAlpha < 0.5f) {
        return IsTouchInIconArea(x, y);
    }
    return (x >= g_Menu.menuPos.x && x <= g_Menu.menuPos.x + GetMenuWidthScaled() &&
            y >= g_Menu.menuPos.y && y <= g_Menu.menuPos.y + GetMenuHeightScaled());
}

void DrawFPS() {
    if (!g_Menu.showFPS) return;
    ImGuiIO& io = ImGui::GetIO();
    ThemeColors tc = GetThemeColors(g_Menu.currentTheme);
    
    // FPS текст
    char fpsText[64];
    snprintf(fpsText, sizeof(fpsText), "FPS: %.0f", io.Framerate);
    
    float fpsFontSize = ImGui::GetFontSize() * 1.6f;
    ImVec2 fpsTextSize = ImGui::GetFont()->CalcTextSizeA(fpsFontSize, FLT_MAX, 0.0f, fpsText);
    ImVec2 fpsPos = ImVec2(glWidth - fpsTextSize.x - 80, 55);
    
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    
    // Фон для FPS
    dl->AddRectFilled(ImVec2(fpsPos.x - 16, fpsPos.y - 10),
                      ImVec2(fpsPos.x + fpsTextSize.x + 16, fpsPos.y + fpsTextSize.y + 10),
                      ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.70f)), 10.0f);
    dl->AddText(ImGui::GetFont(), fpsFontSize, fpsPos,
                ImGui::ColorConvertFloat4ToU32(tc.text), fpsText);
    
    // PING с цветами (симуляция, можно заменить на реальный пинг)
    static int currentPing = 45; // TODO: получать реальный пинг из игры
    
    // Определяем цвет пинга
    ImVec4 pingColor;
    if (currentPing >= 150) {
        pingColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Красный
    } else if (currentPing >= 50) {
        pingColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Жёлтый
    } else {
        pingColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Зелёный
    }
    
    char pingText[64];
    snprintf(pingText, sizeof(pingText), "PING: %dms", currentPing);
    
    ImVec2 pingTextSize = ImGui::GetFont()->CalcTextSizeA(fpsFontSize, FLT_MAX, 0.0f, pingText);
    ImVec2 pingPos = ImVec2(fpsPos.x, fpsPos.y + fpsTextSize.y + 20);
    
    // Фон для PING
    dl->AddRectFilled(ImVec2(pingPos.x - 16, pingPos.y - 10),
                      ImVec2(pingPos.x + pingTextSize.x + 16, pingPos.y + pingTextSize.y + 10),
                      ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.70f)), 10.0f);
    
    // Рисуем PING цветным текстом
    dl->AddText(ImGui::GetFont(), fpsFontSize, pingPos,
                ImGui::ColorConvertFloat4ToU32(pingColor), pingText);
}

// FPS-ANALIZE: график истории FPS
static float fpsHistory[100] = {0};
static int fpsHistoryIndex = 0;

void DrawFPSGraph() {
    if (!g_Menu.showFPSGraph) return;
    
    ImGuiIO& io = ImGui::GetIO();
    ThemeColors tc = GetThemeColors(g_Menu.currentTheme);
    
    // Обновляем историю FPS
    fpsHistory[fpsHistoryIndex] = io.Framerate;
    fpsHistoryIndex = (fpsHistoryIndex + 1) % 100;
    
    // Позиция графика (справа снизу)
    float graphWidth = 300.0f;
    float graphHeight = 120.0f;
    ImVec2 graphPos = ImVec2(glWidth - graphWidth - 20, glHeight - graphHeight - 20);
    
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    
    // Фон графика
    dl->AddRectFilled(
        graphPos,
        ImVec2(graphPos.x + graphWidth, graphPos.y + graphHeight),
        ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.75f)),
        8.0f
    );
    
    // Рамка
    dl->AddRect(
        graphPos,
        ImVec2(graphPos.x + graphWidth, graphPos.y + graphHeight),
        ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.3f, 1.0f)),
        8.0f,
        0,
        1.5f
    );
    
    // Находим min/max FPS для масштабирования
    float minFPS = 999.0f, maxFPS = 0.0f;
    for (int i = 0; i < 100; i++) {
        if (fpsHistory[i] > 0) {
            if (fpsHistory[i] < minFPS) minFPS = fpsHistory[i];
            if (fpsHistory[i] > maxFPS) maxFPS = fpsHistory[i];
        }
    }
    
    if (maxFPS - minFPS < 10.0f) {
        minFPS = maxFPS - 10.0f;
        if (minFPS < 0) minFPS = 0;
    }
    
    // Рисуем линию графика
    ImVec2 prevPoint;
    bool firstPoint = true;
    
    for (int i = 0; i < 100; i++) {
        int idx = (fpsHistoryIndex + i) % 100;
        float fps = fpsHistory[idx];
        
        if (fps <= 0) continue;
        
        // Нормализуем FPS к высоте графика
        float normalized = (fps - minFPS) / (maxFPS - minFPS + 0.001f);
        normalized = 1.0f - normalized; // Инвертируем (высокий FPS = сверху)
        
        float x = graphPos.x + 10 + (i / 100.0f) * (graphWidth - 20);
        float y = graphPos.y + 30 + normalized * (graphHeight - 50);
        
        ImVec2 point = ImVec2(x, y);
        
        if (!firstPoint) {
            // Цвет линии в зависимости от FPS
            ImU32 lineColor;
            if (fps >= 55) {
                lineColor = IM_COL32(0, 255, 0, 255); // Зелёный = хороший FPS
            } else if (fps >= 30) {
                lineColor = IM_COL32(255, 255, 0, 255); // Жёлтый = средний
            } else {
                lineColor = IM_COL32(255, 0, 0, 255); // Красный = плохой
            }
            
            dl->AddLine(prevPoint, point, lineColor, 2.0f);
        }
        
        prevPoint = point;
        firstPoint = false;
    }
    
    // Текст с текущим FPS и min/max
    char infoText[128];
    snprintf(infoText, sizeof(infoText), "FPS: %.0f  Min: %.0f  Max: %.0f", 
             io.Framerate, minFPS, maxFPS);
    
    ImVec2 textPos = ImVec2(graphPos.x + 10, graphPos.y + 8);
    dl->AddText(textPos, ImGui::ColorConvertFloat4ToU32(tc.text), infoText);
}

void DrawClock() {
    if (!g_Menu.showClock) return;
    ThemeColors tc = GetThemeColors(g_Menu.currentTheme);
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(g_Menu.clockPos, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.75f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 10));

    ImGui::Begin("ClockOverlay", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(0)) {
        g_Menu.clockPos.x += io.MouseDelta.x;
        g_Menu.clockPos.y += io.MouseDelta.y;
    }

    time_t now = time(0);
    struct tm* t = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", t);

    ImGui::SetWindowFontScale(g_Menu.clockSize * 1.6f);
    ImGui::TextColored(tc.text, "%s", buf);
    ImGui::SetWindowFontScale(1.0f);

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void DrawCoordsWindow() {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float health = 0.0f;
    const bool hasHealth = TryGetLocalPlayerHealth(health);
    if (!TryGetLocalPedPosition(x, y, z)) {
        return;
    }

    ThemeColors tc = GetThemeColors(g_Menu.currentTheme);
    ImVec2 pos = g_Menu.showClock
                 ? ImVec2(g_Menu.clockPos.x, g_Menu.clockPos.y + 58.0f)
                 : ImVec2(glWidth - 240.0f, 90.0f);

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.72f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));

    ImGui::Begin("CoordsOverlay", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextColored(tc.textDim, "Coords");
    ImGui::Separator();
    ImGui::TextColored(tc.text, "X: %.2f", x);
    ImGui::TextColored(tc.text, "Y: %.2f", y);
    ImGui::TextColored(tc.text, "Z: %.2f", z);
    if (hasHealth) {
        ImGui::TextColored(tc.text, "HP: %.1f", health);
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void DrawFloatingIcon() {
    if (g_Menu.animAlpha >= 0.99f) return;

    ThemeColors tc = GetThemeColors(g_Menu.currentTheme);
    ImGuiIO& io = ImGui::GetIO();
    float iconSize = 70.0f * g_Menu.menuScale * g_Menu.iconScale;
    ClampFloatingIconPosition(iconSize);

    ImGui::SetNextWindowPos(g_Menu.iconPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(iconSize + 20, iconSize + 20));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0,0,0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGui::Begin("FloatingIcon", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float r = iconSize * 0.5f;
    ImVec2 baseCenter = ImVec2(p.x + r + 10, p.y + r + 10);

    static float pulse = 0.0f;
    pulse += io.DeltaTime * 3.0f;
    float pulseScale = 1.0f + sinf(pulse) * 0.06f;
    float floatOffset = sinf((float)ImGui::GetTime() * 1.8f) * (5.0f * g_Menu.iconScale);
    ImVec2 center = ImVec2(baseCenter.x, baseCenter.y + floatOffset);

    ImVec2 mousePos = io.MousePos;

    if (IsTouchInIconArea(mousePos.x, mousePos.y) && io.MouseDown[0] && !g_IconTouchActive) {
        g_IconTouchActive = true;
        g_IconTouchStart = mousePos;
        g_IconPosStart = g_Menu.iconPos;
    }

    if (g_IconTouchActive) {
        if (io.MouseDown[0]) {
            g_Menu.iconPos.x = g_IconPosStart.x + (mousePos.x - g_IconTouchStart.x);
            g_Menu.iconPos.y = g_IconPosStart.y + (mousePos.y - g_IconTouchStart.y);
            ClampFloatingIconPosition(iconSize);
        } else {
            float dragDist = sqrtf(
                    (mousePos.x - g_IconTouchStart.x) * (mousePos.x - g_IconTouchStart.x) +
                    (mousePos.y - g_IconTouchStart.y) * (mousePos.y - g_IconTouchStart.y)
            );
            if (dragDist < 15.0f) g_Menu.isOpen = true;
            g_IconTouchActive = false;
        }
    }

    for (int i = 4; i > 0; i--) {
        float glowR = r * pulseScale + i * 6;
        ImU32 glowCol = ImGui::ColorConvertFloat4ToU32(ImVec4(tc.accent.x, tc.accent.y, tc.accent.z, 0.12f / i));
        dl->AddCircleFilled(center, glowR, glowCol, 36);
    }


    ImVec4 iconColor;

    iconColor = tc.accentHover;


    dl->AddCircleFilled(center, r * pulseScale, ImGui::ColorConvertFloat4ToU32(iconColor), 36);


    ImFont* font = ImGui::GetFont();
    const float iconFontSize = r * 0.8f;
    const char* iconLabel = "R";
    ImVec2 textSize = font->CalcTextSizeA(iconFontSize, 1000.0f, 0.0f, iconLabel);
    dl->AddText(font, iconFontSize,
                ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f),
                IM_COL32_WHITE, iconLabel);


    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void DrawJsonLogWindow() {
    if (!g_Menu.showJsonLog) return;

    ImGui::SetNextWindowSize(ImVec2(900, 720), ImGuiCond_Appearing);
    ImGui::Begin("JSON Monitor", &g_Menu.showJsonLog);


    ImGui::Separator();

    ImGui::Checkbox("Log Outgoing", &logSendJsonData);
    ImGui::SameLine();
    ImGui::Checkbox("Log Incoming", &logIncomingJson);
    ImGui::SameLine();
    ImGui::Checkbox("Debug Chat", &debugmsg);

    if (ImGui::Button("Clear Log", ImVec2(120, 30))) {
        pthread_mutex_lock(&g_JsonLogMutex);
        g_JsonLog.clear();
        pthread_mutex_unlock(&g_JsonLogMutex);
    }
    ImGui::Separator();
    ImGui::BeginChild("LogScroll", ImVec2(0, -150), true);
    pthread_mutex_lock(&g_JsonLogMutex);
    for (int i = (int)g_JsonLog.size() - 1; i >= 0; i--) {
        const JsonLogEntry& e = g_JsonLog[i];


        char timeStr[32];
        struct tm* tm = localtime(&e.timestamp);
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm);
        ImVec4 color = e.isOutgoing ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(0.2f, 1.0f, 0.4f, 1.0f);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "[%s]", timeStr);
        ImGui::SameLine();
        ImGui::TextColored(color, "%s GUI:%d t:%d", e.isOutgoing ? "[OUT]" : "[IN]", e.guiId, e.jsonType);
        std::string displayData = e.data;
        if (displayData.length() > 200) {
            displayData = displayData.substr(0, 200) + "...";
        }
        ImGui::TextWrapped("%s", displayData.c_str());
        ImGui::Separator();
    }
    pthread_mutex_unlock(&g_JsonLogMutex);

    ImGui::EndChild();

    ImGui::Separator();
    ImGui::Text("Quick Actions:");

    if (ImGui::Button("Back to Servers", ImVec2(180, 40))) {

    }

    ImGui::End();
}

// ==================== ESP RENDERING ====================
void DrawESP() {
    if (!g_Menu.espEnabled) return;
    if (libaddr == 0) return;
    
    qword localPlayer = GetLocalPlayer();
    if (localPlayer == 0) return;
    
    float localX, localY, localZ;
    GetPlayerPos(localPlayer, localX, localY, localZ);
    
    // Update screen size
    ImGuiIO& io = ImGui::GetIO();
    g_Menu.screenWidth = (int)io.DisplaySize.x;
    g_Menu.screenHeight = (int)io.DisplaySize.y;
    
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    
    // Loop through all players
    for (int i = 0; i < 1000; i++) {
        qword player = GetPlayerByID(i);
        if (!IsPlayerValid(player)) continue;
        if (player == localPlayer) continue; // Skip self
        
        float px, py, pz;
        GetPlayerPos(player, px, py, pz);
        
        // Calculate distance
        float dx = px - localX;
        float dy = py - localY;
        float dz = pz - localZ;
        float distance = sqrtf(dx*dx + dy*dy + dz*dz);
        
        if (distance > g_Menu.espMaxDistance) continue;
        
        // World to screen
        float screenX, screenY;
        if (!WorldToScreen(px, py, pz, screenX, screenY)) continue;
        
        // Get ESP color
        ImU32 color = IM_COL32(
            (int)(g_Menu.espColor[0] * 255),
            (int)(g_Menu.espColor[1] * 255),
            (int)(g_Menu.espColor[2] * 255),
            255
        );
        
        // Draw ESP Box
        if (g_Menu.espBox) {
            float boxHeight = 2000.0f / distance; // Approximate height based on distance
            float boxWidth = boxHeight * 0.5f;
            
            draw->AddRect(
                ImVec2(screenX - boxWidth/2, screenY - boxHeight),
                ImVec2(screenX + boxWidth/2, screenY),
                color,
                0.0f,
                0,
                2.0f
            );
        }
        
        // Draw Snapline
        if (g_Menu.espSnapline) {
            draw->AddLine(
                ImVec2(g_Menu.screenWidth / 2.0f, g_Menu.screenHeight),
                ImVec2(screenX, screenY),
                color,
                1.5f
            );
        }
        
        // Draw Distance
        if (g_Menu.espDistance) {
            char distText[32];
            snprintf(distText, sizeof(distText), "%.0fm", distance);
            draw->AddText(ImVec2(screenX + 5, screenY), color, distText);
        }
        
        // Draw Health
        if (g_Menu.espHealth) {
            float health = GetPlayerHealth(player);
            float healthPercent = health / 1000.0f;
            
            ImU32 healthColor = IM_COL32(
                (int)((1.0f - healthPercent) * 255),
                (int)(healthPercent * 255),
                0,
                255
            );
            
            char healthText[32];
            snprintf(healthText, sizeof(healthText), "HP: %.0f", health);
            draw->AddText(ImVec2(screenX + 5, screenY + 15), healthColor, healthText);
        }
    }
}
// ==================== END ESP RENDERING ====================

void DrawMainMenu() {
    if (g_Menu.animAlpha < 0.01f) return;

    const float visualScale = GetMenuVisualScale();
    const float menuWidth = GetMenuWidthScaled();
    const float menuHeight = GetMenuHeightScaled();
    const float headerHeight = GetMenuHeaderHeightScaled();
    const auto S = [visualScale](float value) {
        return value * visualScale;
    };

    {
        ImGuiIO& ui = ImGui::GetIO();
        ThemeColors tc = GetThemeColors(g_Menu.currentTheme);

        ApplyTheme(g_Menu.currentTheme);

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_Menu.animAlpha);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, S(10.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(S(16.0f), S(16.0f)));
        ImGui::SetNextWindowPos(g_Menu.menuPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(menuWidth, menuHeight), ImGuiCond_Always);

        ImGui::Begin("YRENER MENU", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();

        dl->AddRectFilled(wp, ImVec2(wp.x + menuWidth, wp.y + menuHeight), IM_COL32(8, 8, 8, 245), S(10.0f));
        dl->AddRect(ImVec2(wp.x + 1.0f, wp.y + 1.0f),
                    ImVec2(wp.x + menuWidth - 1.0f, wp.y + menuHeight - 1.0f),
                    ImGui::ColorConvertFloat4ToU32(ImVec4(tc.accent.x, tc.accent.y, tc.accent.z, 0.25f)),
                    S(10.0f), 0, fmaxf(1.0f, S(1.5f)));

        ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
        ImGui::InvisibleButton("##drag_menu", ImVec2(menuWidth - S(120.0f), headerHeight));
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
            g_Menu.menuPos.x += ui.MouseDelta.x;
            g_Menu.menuPos.y += ui.MouseDelta.y;
        }

        ImGui::SetCursorPos(ImVec2(S(18.0f), S(16.0f)));
        ImGui::TextColored(ImVec4(0.88f, 0.93f, 1.0f, 1.0f), "@Yrener_Soft");

        ImGui::SetCursorPos(ImVec2(menuWidth - S(112.0f), S(13.0f)));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.68f, 0.82f, 1.0f, 1.0f));
        if (ImGui::Button("ЗАКРЫТЬ")) {
            g_Menu.isOpen = false;
        }
        ImGui::PopStyleColor(4);

        auto drawSmallButton = [&](const char* id, const char* label, bool active) -> bool {
            ImGui::PushID(id);
            ImVec4 bg = active ? ImVec4(0.68f, 0.84f, 1.0f, 1.0f) : ImVec4(0.98f, 0.98f, 0.99f, 1.0f);
            ImVec4 hover = active ? ImVec4(0.76f, 0.88f, 1.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, hover);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.07f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, S(8.0f));
            bool pressed = ImGui::Button(label, ImVec2(S(48.0f), S(34.0f)));
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);
            ImGui::PopID();
            return pressed;
        };

        auto drawNavButton = [&](const char* id, const char* label, int tabIndex) {
            bool active = g_Menu.activeTab == tabIndex;
            ImGui::PushID(id);
            ImVec4 bg = active ? ImVec4(0.62f, 0.80f, 1.0f, 1.0f) : ImVec4(0.96f, 0.96f, 0.97f, 1.0f);
            ImVec4 hover = active ? ImVec4(0.70f, 0.85f, 1.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, hover);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.07f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, S(9.0f));
            if (ImGui::Button(label, ImVec2(-1.0f, S(42.0f)))) {
                g_Menu.activeTab = tabIndex;
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);
            ImGui::PopID();
        };

        auto drawTileToggle = [&](const char* id, const char* label, bool* value, float width) {
            ImGui::PushID(id);
            ImVec4 bg = *value ? ImVec4(0.68f, 0.84f, 1.0f, 1.0f) : ImVec4(0.97f, 0.97f, 0.98f, 1.0f);
            ImVec4 hover = *value ? ImVec4(0.76f, 0.88f, 1.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, hover);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.07f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, S(9.0f));
            if (ImGui::Button(label, ImVec2(width, S(42.0f)))) {
                *value = !(*value);
                g_Menu.needsSave = true;
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);
            ImGui::PopID();
        };

        auto drawTileAction = [&](const char* id, const char* label, float width) -> bool {
            ImGui::PushID(id);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.97f, 0.97f, 0.98f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.90f, 0.94f, 1.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.07f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, S(9.0f));
            bool pressed = ImGui::Button(label, ImVec2(width, S(42.0f)));
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);
            ImGui::PopID();
            return pressed;
        };

        auto drawSectionTitle = [&](const char* label) {
            ImGui::Dummy(ImVec2(0.0f, S(6.0f)));
            float textWidth = ImGui::CalcTextSize(label).x;
            float avail = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - textWidth) * 0.5f);
            ImGui::TextColored(ImVec4(0.68f, 0.82f, 1.0f, 1.0f), "%s", label);
            ImGui::Dummy(ImVec2(0.0f, S(4.0f)));
        };

        if (json.ab && !json.ap) {
            json.ap = true;
        }

        const float sidebarW = S(212.0f);
        const float contentX = S(16.0f) + sidebarW + S(14.0f);
        const float contentW = menuWidth - contentX - S(16.0f);
        const float bodyY = headerHeight + S(8.0f);
        const float bodyH = menuHeight - bodyY - S(16.0f);

        ImGui::SetCursorPos(ImVec2(S(16.0f), bodyY));
        ImGui::BeginChild("##SidebarModern", ImVec2(sidebarW, bodyH), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::TextColored(ImVec4(0.76f, 0.82f, 0.92f, 1.0f), "Yrener v1.0");
        ImGui::TextColored(ImVec4(0.56f, 0.62f, 0.72f, 1.0f), "@Yrener_Soft");
        ImGui::Dummy(ImVec2(0.0f, S(8.0f)));

        if (drawSmallButton("btn_log", "LOG", g_Menu.showJsonLog)) {
            g_Menu.showJsonLog = !g_Menu.showJsonLog;
        }
        ImGui::SameLine();
        if (drawSmallButton("btn_fps", "FPS", g_Menu.showFPS)) {
            g_Menu.showFPS = !g_Menu.showFPS;
            g_Menu.needsSave = true;
        }
        ImGui::SameLine();
        if (drawSmallButton("btn_cfg", "CFG", false)) {

        }

        ImGui::Dummy(ImVec2(0.0f, S(14.0f)));
        drawNavButton("tab_game", "Игра", 0);
        drawNavButton("tab_lv", "Ловля", 1);
        drawNavButton("tab_visual", "Визуальное", 2);
        drawNavButton("tab_misc", "Разное", 3);
        drawNavButton("tab_settings", "Настройки", 4);
        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(contentX, bodyY));
        ImGui::BeginChild("##ContentModern", ImVec2(contentW, bodyH), true, ImGuiWindowFlags_NoScrollbar);

        ImGui::Dummy(ImVec2(0.0f, S(10.0f)));

        const float gap = S(14.0f);
        const float tileWidth = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;

        switch (g_Menu.activeTab) {
            case 0:
                drawTileToggle("game_collision", "collision", &func.collision, tileWidth);
                ImGui::SameLine(0.0f, gap);
                drawTileToggle("game_afk", "Ходить под водой", &func.afk, tileWidth);
                drawTileToggle("game_camera", "Заморозка камеры", &func.camfreeze, tileWidth);
                ImGui::SameLine(0.0f, gap);
                drawTileToggle("game_spawn", "Быстрый спавн", &func.autoArmor, tileWidth);
                drawTileToggle("game_fps", "Показывать FPS", &g_Menu.showFPS, tileWidth);
                ImGui::SameLine(0.0f, gap);
                drawTileToggle("game_clock", "Показывать часы", &g_Menu.showClock, tileWidth);
                break;

            case 1:
                drawTileToggle("lv_house", "Флудер /buyhouse", &flooder.fl, tileWidth);
                ImGui::SameLine(0.0f, gap);
                drawTileToggle("lv_garage", "Флудер /buygarage", &flooder.garage, tileWidth);
                drawTileToggle("lv_send", "AutoSend", &autosend.state, tileWidth);
                ImGui::SameLine(0.0f, gap);
                drawTileToggle("lv_timer", "Timer", &func.timer, tileWidth);
                drawTileToggle("auto_fo", "FO", &json.fo, tileWidth);
                ImGui::SameLine(0.0f, gap);
                if (drawTileAction("auto_ap", json.ap ? "AP: ON" : "AP: OFF", tileWidth)) {
                    json.ap = !json.ap;
                    json.ab = json.ap;
                    g_Menu.needsSave = true;
                }
                drawTileToggle("auto_wo", "WO", &json.wo, tileWidth);
                ImGui::SameLine(0.0f, gap);
                drawTileToggle("auto_timer", "Timer", &func.timer, tileWidth);
                drawTileToggle("lv_afk", "АФК призрак", &func.afk, tileWidth);
                break;

            case 2:
                drawSectionTitle("Визуальное");
                drawTileToggle("visual_in", "Log Incoming", &logIncomingJson, tileWidth);
                ImGui::SameLine(0.0f, gap);
                drawTileToggle("visual_out", "Log Outgoing", &logSendJsonData, tileWidth);
                drawTileToggle("visual_debug", "Debug Chat", &debugmsg, tileWidth);
                ImGui::SameLine(0.0f, gap);
                if (drawTileAction("visual_monitor", "Открыть JSON Monitor", tileWidth)) {
                    g_Menu.showJsonLog = true;
                }
                break;

            case 3:
                drawSectionTitle("ESP / WallHack");
                drawTileToggle("esp_enabled", "ESP Включен", &g_Menu.espEnabled, tileWidth);
                ImGui::SameLine(0.0f, gap);
                drawTileToggle("esp_box", "ESP Box", &g_Menu.espBox, tileWidth);
                drawTileToggle("esp_name", "ESP Имя", &g_Menu.espName, tileWidth);
                ImGui::SameLine(0.0f, gap);
                drawTileToggle("esp_health", "ESP Здоровье", &g_Menu.espHealth, tileWidth);
                drawTileToggle("esp_distance", "ESP Дистанция", &g_Menu.espDistance, tileWidth);
                ImGui::SameLine(0.0f, gap);
                drawTileToggle("esp_skeleton", "ESP Скелет", &g_Menu.espSkeleton, tileWidth);
                drawTileToggle("esp_weapon", "ESP Оружие", &g_Menu.espWeapon, tileWidth);
                ImGui::SameLine(0.0f, gap);
                drawTileToggle("esp_snapline", "ESP Линии", &g_Menu.espSnapline, tileWidth);
                
                // Color picker для ESP (только шестерёнка)
                ImGui::Dummy(ImVec2(0.0f, S(8.0f)));
                ImGui::ColorEdit3("Цвет ESP##esp_color", g_Menu.espColor, 
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB);
                
                drawSectionTitle("Chams");
                drawTileToggle("chams_enabled", "Chams", &g_Menu.chamsEnabled, tileWidth);
                ImGui::SameLine(0.0f, gap);
                drawTileToggle("chams_visibility", "Авто цвет (стена)", &g_Menu.chamsVisibilityCheck, tileWidth);
                
                // Color pickers для Chams (только шестерёнки)
                ImGui::Dummy(ImVec2(0.0f, S(8.0f)));
                ImGui::ColorEdit3("За стеной##chams_occluded", g_Menu.chamsOccludedColor, 
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB);
                ImGui::SameLine();
                ImGui::ColorEdit3("Видимый##chams_visible", g_Menu.chamsVisibleColor, 
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB);
                break;

            case 4:
                drawSectionTitle("Настройки");
                if (drawTileAction("settings_save", "Сохранить", tileWidth)) {

                }
                ImGui::SameLine(0.0f, gap);
                if (drawTileAction("settings_load", "Загрузить", tileWidth)) {

                }
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::SliderFloat("##menu_scale", &g_Menu.menuScale, 0.75f, 3.0f, "Размер меню %.2f")) {
                    g_Menu.needsSave = true;
                }
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::SliderFloat("##font_size", &g_Menu.fontSize, 0.5f, 2.0f, "Размер текста %.2f")) {
                    g_Menu.needsSave = true;
                }
                if (drawTileAction("settings_theme_prev", "Тема -", tileWidth)) {
                    g_Menu.currentTheme = (g_Menu.currentTheme + THEME_COUNT - 1) % THEME_COUNT;
                    g_Menu.needsSave = true;
                }
                ImGui::SameLine(0.0f, gap);
                if (drawTileAction("settings_theme_next", "Тема +", tileWidth)) {
                    g_Menu.currentTheme = (g_Menu.currentTheme + 1) % THEME_COUNT;
                    g_Menu.needsSave = true;
                }
                
                drawSectionTitle("FPS");
                drawTileToggle("fps_analyze", "FPS-ANALIZE", &g_Menu.showFPSGraph, tileWidth);
                break;
        }

        ImGui::EndChild();
        ImGui::End();
        ImGui::PopStyleVar(3);

        DrawJsonLogWindow();
        return;
    }
}

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!surface || !dpy) {
        return old_eglSwapBuffers ? old_eglSwapBuffers(dpy, surface) : EGL_FALSE;
    }

    if (!is_setup) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;

        eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);
        io.DisplaySize = ImVec2((float)glWidth, (float)glHeight);

        ImFontConfig cfg;
        cfg.SizePixels = 22.0f;
        io.Fonts->AddFontFromFileTTF("/system/fonts/Roboto-Regular.ttf", 22.0f, &cfg, io.Fonts->GetGlyphRangesCyrillic());

        g_Menu.menuPos = ImVec2(glWidth * 0.5f - GetMenuWidthScaled() * 0.5f,
                                glHeight * 0.5f - GetMenuHeightScaled() * 0.5f);
        g_Menu.iconPos = ImVec2(50, glHeight/2);
        g_Menu.clockPos = ImVec2(glWidth - 350, 60);

        ApplyTheme(g_Menu.currentTheme);
        ImGui_ImplOpenGL3_Init("#version 100");

        is_setup = true;

    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)glWidth, (float)glHeight);
    io.FontGlobalScale = g_Menu.menuScale * g_Menu.fontSize;

    ImGui::NewFrame();

    float targetAlpha = g_Menu.isOpen ? 1.0f : 0.0f;
    g_Menu.animAlpha += (targetAlpha - g_Menu.animAlpha) * io.DeltaTime * 10.0f;

    DrawClock();
    DrawCoordsWindow();
    DrawFPS();
    DrawFPSGraph();  // FPS-ANALIZE график
    DrawFloatingIcon();
    DrawMainMenu();
    DrawESP();  // Рисуем ESP поверх всего


    if (g_Menu.needsSave) {
        g_SaveTimer += io.DeltaTime;
        if (g_SaveTimer > 3.0f) {

            g_SaveTimer = 0.0f;
        }
    }

    ImGui::EndFrame();
    ImGui::Render();
    glViewport(0, 0, glWidth, glHeight);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return old_eglSwapBuffers(dpy, surface);
}

extern "C"
JNIEXPORT void JNICALL Java_com_blackhub_bronline_game_core_JNILib_multiTouchEvent(
        JNIEnv* env, jclass cls, int action, int id, int x, int y,
        int x2, int y2, int x3, int y3) {

    if (!is_setup) {
        if (old_Java_multiTouchEvent) {
            old_Java_multiTouchEvent(env, cls, action, id, x, y, x2, y2, x3, y3);
        }
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);

    bool handled = false;

    switch (action) {
        case 0: case 5:
            io.MouseDown[0] = true;
            handled = IsTouchInMenuArea((float)x, (float)y);
            break;
        case 2:
            handled = IsTouchInMenuArea((float)x, (float)y) && io.MouseDown[0];
            break;
        case 1: case 6:
            io.MouseDown[0] = false;
            break;
    }

    if (!handled || !io.WantCaptureMouse) {
        if (old_Java_multiTouchEvent) {
            old_Java_multiTouchEvent(env, cls, action, id, x, y, x2, y2, x3, y3);
        }
    }
}

void hook_game_functions() {

    void* lib = dlopen("libblackrussia-client.so", RTLD_LAZY);
    if (!lib) {
        return;
    }

    void* touchPtr = dlsym(lib, "Java_com_blackhub_bronline_game_core_JNILib_multiTouchEvent");
    if (!touchPtr) {
        dlclose(lib);
        return;
    }
    qword touchOffset = 0xC84C84;  // UPDATED
    libaddr = (qword)touchPtr - touchOffset;

    // ==================== UPDATED OFFSETS ====================
    AddChatMessage = (void (*)(char*, ...))(libaddr + 0x7D2F28);      // ChatVM_AddMessage
    SendChatMessage = (char* (*)(char*))(libaddr + 0xC8529C);         // SendChatMessage
    
    // NEW: PlayerPool global pointer
    g_PlayerPoolPtr = libaddr + 0x2660610;  // BSS segment
    
    // Hook LocalPlayer_Tick instead of CNetGame_Process
    DobbyHook((void*)(libaddr + 0xB10D30), (void*)LocalPlayer_Tick, (void**)&old_LocalPlayer_Tick);
    
    // Hook PedFight for damage bypass
    DobbyHook((void*)(libaddr + 0xBD9708), (void*)PedFight_fn, (void**)&old_PedFight_fn);
    
    // OLD HOOKS - keeping for now but may not work
    // TODO: find new offsets for these
    /*
    FindPlayerPed = (qword (*)())(libaddr + 0x67BF40);
    FindGroundZ = (float (*)(float, float))(libaddr + 0x69A9A0);
    
    qword dialogBoxRpcOffset = 0xAC73A8;
    void* dialogBoxRpcAddr = (void*)(libaddr + dialogBoxRpcOffset);
    DobbyHook(dialogBoxRpcAddr, (void*)DialogBoxRPC, (void**)&old_DialogBoxRPC);
    
    DobbyHook((void*)(libaddr + string2Offset("0xA654A0")), (void *)SendOnFootSyncData, (void **)&old_SendOnFootSyncData);
    DobbyHook((void*)(libaddr + string2Offset("0x659684")), (void *)CNetGame_Process, (void **)&old_CNetGame_Process);
    */
    // ==================== END OFFSETS ====================

    const char* sendJsonSymbols[] = {
            "Java_com_blackhub_bronline_game_core_JNILib_sendJsonData",
            "Java_com_blackhub_bronline_game_JNILib_sendJsonData",
            "_Z12sendJsonDataiP11_jbyteArray",
            "sendJsonData",
            nullptr
    };

    void* sendJsonPtr = nullptr;

    for (int i = 0; sendJsonSymbols[i] != nullptr; i++) {
        sendJsonPtr = dlsym(lib, sendJsonSymbols[i]);
        if (sendJsonPtr) {
            break;
        }
    }

    Call_JNILib_sendJsonData = (void (*)(JNIEnv*, jclass, jint, jbyteArray))sendJsonPtr;

    qword onJsonIncomingOffset = 0xBBE9AC;  // needs verification for new lib
    void* onJsonAddr = (void*)(libaddr + onJsonIncomingOffset);

    if (onJsonAddr) {
        DobbyHook(onJsonAddr, (void*)Hook_OnJsonDataIncoming_v2, (void**)&old_OnJsonDataIncoming_v2);
    }
}
void hook_entry() {
    // EGL hook
    void* eglAddr = dlsym(RTLD_NEXT, "eglSwapBuffers");
    if (eglAddr) {
        DobbyHook(eglAddr, (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);

    }
    // MultiTouch hook
    void* lib = dlopen("libblackrussia-client.so", RTLD_LAZY);
    if (lib) {
        void* touchPtr = dlsym(lib, "Java_com_blackhub_bronline_game_core_JNILib_multiTouchEvent");
        if (touchPtr) {
            DobbyHook(touchPtr, (void*)Java_com_blackhub_bronline_game_core_JNILib_multiTouchEvent,
                      (void**)&old_Java_multiTouchEvent);
        }
        dlclose(lib);
    }
    hook_game_functions();
}
extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    vm->GetEnv((void**)&g_MainEnv, JNI_VERSION_1_6);
    g_JavaVM = vm;
    usleep(100000); // 100ms
    hook_entry();
    return JNI_VERSION_1_6;
}

}
