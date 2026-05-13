#pragma once
#include "wLibs/ImGui/imgui.h"
#include <string>
#include <fstream>
#include <sstream>

// Темы оформления
enum MenuTheme {
    THEME_DARK = 0,
    THEME_MIDNIGHT,
    THEME_NEON_BLUE,
    THEME_NEON_PURPLE,
    THEME_BLOOD_RED,
    THEME_FOREST,
    THEME_SUNSET,
    THEME_CYBERPUNK,
    THEME_COUNT
};

const char* ThemeNames[] = {
        "Тёмная", "Полночь", "Неон Синий", "Неон Фиолетовый",
        "Кровавая", "Лесная", "Закат", "Киберпанк"
};

// Структура темы
struct ThemeColors {
    ImVec4 windowBg;
    ImVec4 accent;
    ImVec4 accentHover;
    ImVec4 accentActive;
    ImVec4 text;
    ImVec4 textDim;
    ImVec4 frameBg;
    ImVec4 border;
    ImVec4 headerGradient1;
    ImVec4 headerGradient2;
};

// Пути для изображений
#define REE_CONFIG_DIR "/data/user/0/com.br.top/files/"
#define REE_ICON_PATH REE_CONFIG_DIR "ree_icon.png"
#define REE_BG_PATH REE_CONFIG_DIR "ree_background.png"
#define REE_CONFIG_PATH REE_CONFIG_DIR "ree_config.cfg"

// Глобальные переменные настроек
struct MenuState {
    // === Состояние ===
    // Добавить в структуру MenuState:
    bool showJsonLog = false;
    bool isOpen = true;
    float animAlpha = 0.0f;
    int activeTab = 0;
    bool needsSave = false;

    // === Настройки интерфейса ===
    int currentTheme = THEME_NEON_BLUE;
    float menuScale = 2.0f;
    float menuAlpha = 0.95f;
    float menuColor[3] = {0.08f, 0.08f, 0.10f};
    bool enableAnimations = true;
    bool showFPS = false;
    bool compactMode = false;

    // === ШРИФТ ===
    float fontSize = 0.5f;          // Множитель размера шрифта (0.5 - 2.0)
    float fontBaseSize = 42.0f;     // Базовый размер шрифта

    // === КАСТОМНЫЕ ИЗОБРАЖЕНИЯ ===
    bool useCustomIcon = false;     // Использовать кастомную иконку
    bool useCustomBackground = false; // Использовать кастомный фон
    float iconScale = 1.0f;         // Масштаб иконки
    float bgOpacity = 0.5f;         // Прозрачность фона
    int bgFitMode = 0;              // 0=Растянуть, 1=Вписать, 2=Заполнить

    // === Позиционирование ===
    ImVec2 menuPos = ImVec2(100, 100);
    ImVec2 iconPos = ImVec2(50, 200);
    bool iconDragging = false;      // Флаг перетаскивания иконки

    // === Фон ===
    int bgAnimStyle = 0;
    int bgAnimLocation = 0;
    float bgAnimSpeed = 1.0f;
    float bgAnimDensity = 1.0f;

    // === Часы (ВКЛЮЧЕНЫ ПО УМОЛЧАНИЮ) ===
    bool showClock = true;
    int clockStyle = 1;
    float clockSize = 0.6f;
    ImVec2 clockPos = ImVec2(100, 50);

    // === ФУНКЦИИ: Боевые ===
    bool aimbot = false;
    bool aimbotSmooth = false;
    float aimbotFOV = 90.0f;
    float aimbotSmoothing = 5.0f;
    int aimbotBone = 0;

    bool triggerbot = false;
    float triggerbotDelay = 0.0f;

    bool noRecoil = false;
    bool noSpread = false;
    bool rapidFire = false;
    bool infiniteAmmo = false;
    bool autoReload = false;

    // === ФУНКЦИИ: Движение ===
    bool airbreakUp = false;
    bool airbreakDown = false;
    bool speedhack = false;
    float speedMultiplier = 2.0f;
    bool superJump = false;
    float jumpHeight = 2.0f;
    bool noFall = false;
    bool fly = false;
    float flySpeed = 5.0f;
    bool noclip = false;
    bool teleport = false;
    bool antiKick = false;
    bool antiAFK = false;

    // === ФУНКЦИИ: Защита ===
    bool godmode = false;
    bool antiHeadshot = false;
    bool fakelag = false;
    int fakelagAmount = 0;

    // === ФУНКЦИИ: ESP ===
    bool espEnabled = false;
    bool espBox = false;
    int espBoxStyle = 0;
    bool espLine = false;
    bool espName = false;
    bool espHealth = false;
    bool espDistance = false;
    bool espSkeleton = false;
    bool espWeapon = false;
    bool espSnapline = false;
    float espMaxDistance = 500.0f;
    float espColor[3] = {1.0f, 0.0f, 0.0f};
    float espEnemyColor[3] = {1.0f, 0.0f, 0.0f};
    float espFriendlyColor[3] = {0.0f, 1.0f, 0.0f};

    // === ФУНКЦИИ: Мир ===
    bool nightVision = false;
    bool thermalVision = false;
    bool noFog = false;
    bool noWater = false;
    bool brightWorld = false;
    float worldBrightness = 1.0f;

    // === ФУНКЦИИ: Ловля ===
    bool autoFish = false;
    bool instantCatch = false;
    bool autoSell = false;
    bool fishESP = false;
    bool rareFishOnly = false;

    // === ФУНКЦИИ: Транспорт ===
    bool vehicleGodmode = false;
    bool vehicleSpeedhack = false;
    float vehicleSpeedMult = 2.0f;
    bool vehicleFly = false;
    bool vehicleNoCollision = false;
    bool vehicleBoost = false;

    // === ФУНКЦИИ: Разное ===
    bool unlimitedStamina = false;
    bool fastInteract = false;
    bool autoHeal = false;
    bool noReload = false;
    bool silentAim = false;
    bool magicBullet = false;
    bool crashProtection = false;

    // Тестовые функции
    bool testFunc1 = false;
    bool testFunc2 = false;
    bool testFunc3 = false;
    bool testFunc4 = false;
    bool testFunc5 = false;
    float testFloat1 = 0.0f;
    float testFloat2 = 1.0f;
    int testInt1 = 0;
};

extern MenuState g_Menu;

// === Получение цветов темы ===
inline ThemeColors GetThemeColors(int theme) {
    ThemeColors c;
    switch (theme) {
        case THEME_DARK:
            c.windowBg = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
            c.accent = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
            c.accentHover = ImVec4(0.35f, 0.35f, 0.40f, 1.0f);
            c.accentActive = ImVec4(0.45f, 0.45f, 0.50f, 1.0f);
            c.text = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            c.textDim = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
            c.frameBg = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
            c.border = ImVec4(0.3f, 0.3f, 0.35f, 1.0f);
            c.headerGradient1 = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
            c.headerGradient2 = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
            break;

        case THEME_MIDNIGHT:
            c.windowBg = ImVec4(0.05f, 0.05f, 0.12f, 1.0f);
            c.accent = ImVec4(0.15f, 0.20f, 0.45f, 1.0f);
            c.accentHover = ImVec4(0.20f, 0.30f, 0.60f, 1.0f);
            c.accentActive = ImVec4(0.25f, 0.40f, 0.75f, 1.0f);
            c.text = ImVec4(0.9f, 0.9f, 1.0f, 1.0f);
            c.textDim = ImVec4(0.5f, 0.5f, 0.7f, 1.0f);
            c.frameBg = ImVec4(0.08f, 0.08f, 0.18f, 1.0f);
            c.border = ImVec4(0.2f, 0.25f, 0.5f, 1.0f);
            c.headerGradient1 = ImVec4(0.10f, 0.12f, 0.30f, 1.0f);
            c.headerGradient2 = ImVec4(0.05f, 0.05f, 0.15f, 1.0f);
            break;

        case THEME_NEON_BLUE:
            c.windowBg = ImVec4(0.06f, 0.06f, 0.10f, 1.0f);
            c.accent = ImVec4(0.0f, 0.45f, 0.90f, 1.0f);
            c.accentHover = ImVec4(0.0f, 0.55f, 1.0f, 1.0f);
            c.accentActive = ImVec4(0.2f, 0.65f, 1.0f, 1.0f);
            c.text = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            c.textDim = ImVec4(0.5f, 0.7f, 0.9f, 1.0f);
            c.frameBg = ImVec4(0.08f, 0.10f, 0.15f, 1.0f);
            c.border = ImVec4(0.0f, 0.5f, 1.0f, 0.5f);
            c.headerGradient1 = ImVec4(0.0f, 0.3f, 0.6f, 1.0f);
            c.headerGradient2 = ImVec4(0.0f, 0.15f, 0.35f, 1.0f);
            break;

        case THEME_NEON_PURPLE:
            c.windowBg = ImVec4(0.08f, 0.05f, 0.12f, 1.0f);
            c.accent = ImVec4(0.55f, 0.20f, 0.85f, 1.0f);
            c.accentHover = ImVec4(0.65f, 0.30f, 0.95f, 1.0f);
            c.accentActive = ImVec4(0.75f, 0.40f, 1.0f, 1.0f);
            c.text = ImVec4(1.0f, 0.95f, 1.0f, 1.0f);
            c.textDim = ImVec4(0.7f, 0.5f, 0.8f, 1.0f);
            c.frameBg = ImVec4(0.12f, 0.08f, 0.18f, 1.0f);
            c.border = ImVec4(0.6f, 0.2f, 0.9f, 0.5f);
            c.headerGradient1 = ImVec4(0.4f, 0.1f, 0.6f, 1.0f);
            c.headerGradient2 = ImVec4(0.2f, 0.05f, 0.35f, 1.0f);
            break;

        case THEME_BLOOD_RED:
            c.windowBg = ImVec4(0.10f, 0.05f, 0.05f, 1.0f);
            c.accent = ImVec4(0.80f, 0.15f, 0.15f, 1.0f);
            c.accentHover = ImVec4(0.90f, 0.25f, 0.25f, 1.0f);
            c.accentActive = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
            c.text = ImVec4(1.0f, 0.95f, 0.95f, 1.0f);
            c.textDim = ImVec4(0.8f, 0.5f, 0.5f, 1.0f);
            c.frameBg = ImVec4(0.15f, 0.08f, 0.08f, 1.0f);
            c.border = ImVec4(0.9f, 0.2f, 0.2f, 0.5f);
            c.headerGradient1 = ImVec4(0.6f, 0.1f, 0.1f, 1.0f);
            c.headerGradient2 = ImVec4(0.3f, 0.05f, 0.05f, 1.0f);
            break;

        case THEME_FOREST:
            c.windowBg = ImVec4(0.05f, 0.10f, 0.06f, 1.0f);
            c.accent = ImVec4(0.20f, 0.70f, 0.30f, 1.0f);
            c.accentHover = ImVec4(0.30f, 0.80f, 0.40f, 1.0f);
            c.accentActive = ImVec4(0.40f, 0.90f, 0.50f, 1.0f);
            c.text = ImVec4(0.95f, 1.0f, 0.95f, 1.0f);
            c.textDim = ImVec4(0.5f, 0.7f, 0.5f, 1.0f);
            c.frameBg = ImVec4(0.08f, 0.15f, 0.10f, 1.0f);
            c.border = ImVec4(0.2f, 0.7f, 0.3f, 0.5f);
            c.headerGradient1 = ImVec4(0.15f, 0.5f, 0.2f, 1.0f);
            c.headerGradient2 = ImVec4(0.08f, 0.25f, 0.1f, 1.0f);
            break;

        case THEME_SUNSET:
            c.windowBg = ImVec4(0.12f, 0.06f, 0.08f, 1.0f);
            c.accent = ImVec4(0.95f, 0.45f, 0.20f, 1.0f);
            c.accentHover = ImVec4(1.0f, 0.55f, 0.30f, 1.0f);
            c.accentActive = ImVec4(1.0f, 0.65f, 0.40f, 1.0f);
            c.text = ImVec4(1.0f, 0.95f, 0.9f, 1.0f);
            c.textDim = ImVec4(0.9f, 0.6f, 0.5f, 1.0f);
            c.frameBg = ImVec4(0.18f, 0.10f, 0.12f, 1.0f);
            c.border = ImVec4(1.0f, 0.5f, 0.2f, 0.5f);
            c.headerGradient1 = ImVec4(0.9f, 0.35f, 0.15f, 1.0f);
            c.headerGradient2 = ImVec4(0.5f, 0.15f, 0.1f, 1.0f);
            break;

        case THEME_CYBERPUNK:
            c.windowBg = ImVec4(0.02f, 0.02f, 0.05f, 1.0f);
            c.accent = ImVec4(1.0f, 0.0f, 0.5f, 1.0f);
            c.accentHover = ImVec4(1.0f, 0.2f, 0.6f, 1.0f);
            c.accentActive = ImVec4(0.0f, 1.0f, 0.8f, 1.0f);
            c.text = ImVec4(0.0f, 1.0f, 0.9f, 1.0f);
            c.textDim = ImVec4(0.0f, 0.7f, 0.6f, 1.0f);
            c.frameBg = ImVec4(0.05f, 0.05f, 0.12f, 1.0f);
            c.border = ImVec4(1.0f, 0.0f, 0.5f, 0.7f);
            c.headerGradient1 = ImVec4(0.8f, 0.0f, 0.4f, 1.0f);
            c.headerGradient2 = ImVec4(0.0f, 0.3f, 0.4f, 1.0f);
            break;

        default:
            return GetThemeColors(THEME_NEON_BLUE);
    }
    return c;
}




inline bool FileExists(const char* path) {
    std::ifstream f(path);
    return f.good();
}