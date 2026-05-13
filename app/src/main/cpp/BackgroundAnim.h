
#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>
#include "wLibs/ImGui/imgui.h"
#pragma once
#include "wLibs/ImGui/imgui.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include "MenuConfig.h"

namespace BgAnim {

    struct Particle {
        float x, y;
        float vx, vy;
        float size;
        float alpha;
        char ch;
    };

    static std::vector<Particle> particles;
    static bool initialized = false;
    static float animTime = 0.0f;
    static int particleCount = 0;

    void Init(int count = 100) {
        if (count < 8) count = 8;
        particleCount = count;
        particles.resize(count);
        for (auto& p : particles) {
            p.x = (float)(rand() % 2000);
            p.y = (float)(rand() % 2000);
            p.vx = (float)(rand() % 100 - 50) / 50.0f;
            p.vy = (float)(rand() % 100) / 30.0f + 1.0f;
            p.size = (float)(rand() % 4 + 2);
            p.alpha = (float)(rand() % 50 + 50) / 100.0f;
            p.ch = '0' + (rand() % 10);
        }
        initialized = true;
    }

    void EnsureParticleCount(int count) {
        if (count < 8) count = 8;
        if (!initialized || count != particleCount) {
            Init(count);
        }
    }

    void Render(ImDrawList* dl, int style, float w, float h, float dt, ImVec2 offset = ImVec2(0,0)) {
        if (!initialized) Init();
        if (style == 0) return;

        animTime += dt;
        ThemeColors tc = GetThemeColors(g_Menu.currentTheme);

        for (auto& p : particles) {
            switch (style) {
                case 1: // Снег
                    p.y += p.vy * dt * 60.0f;
                    p.x += sinf(animTime + p.y * 0.01f) * 0.5f;
                    if (p.y > h) { p.y = -10; p.x = (float)(rand() % (int)w); }

                    dl->AddCircleFilled(
                            ImVec2(offset.x + p.x, offset.y + p.y),
                            p.size,
                            ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, p.alpha * 0.6f))
                    );
                    break;

                case 2: // Дождь
                    p.y += p.vy * dt * 120.0f;
                    p.x += p.vx * dt * 30.0f;
                    if (p.y > h) { p.y = -10; p.x = (float)(rand() % (int)w); }

                    dl->AddLine(
                            ImVec2(offset.x + p.x, offset.y + p.y),
                            ImVec2(offset.x + p.x + p.vx * 3, offset.y + p.y + 15),
                            ImGui::ColorConvertFloat4ToU32(ImVec4(0.6f, 0.7f, 1.0f, p.alpha * 0.4f)),
                            1.5f
                    );
                    break;

                case 3: // Матрица
                    p.y += p.vy * dt * 80.0f;
                    if (p.y > h) {
                        p.y = -20;
                        p.x = (float)(rand() % (int)w);
                        p.ch = '0' + (rand() % 10);
                    }

                    {
                        char buf[2] = {p.ch, 0};
                        dl->AddText(
                                ImVec2(offset.x + p.x, offset.y + p.y),
                                ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.3f, p.alpha * 0.7f)),
                                buf
                        );
                    }
                    break;

                case 4: // Связи (Connected dots)
                    p.x += p.vx * dt * 20.0f;
                    p.y += p.vy * dt * 15.0f;

                    if (p.x < 0) p.vx = fabsf(p.vx);
                    if (p.x > w) p.vx = -fabsf(p.vx);
                    if (p.y < 0) p.vy = fabsf(p.vy);
                    if (p.y > h) p.vy = -fabsf(p.vy);

                    dl->AddCircleFilled(
                            ImVec2(offset.x + p.x, offset.y + p.y),
                            3.0f,
                            ImGui::ColorConvertFloat4ToU32(ImVec4(tc.accent.x, tc.accent.y, tc.accent.z, 0.8f))
                    );
                    break;
            }
        }

        if (style == 4) {
            for (size_t i = 0; i < particles.size(); i++) {
                for (size_t j = i + 1; j < particles.size(); j++) {
                    float dx = particles[i].x - particles[j].x;
                    float dy = particles[i].y - particles[j].y;
                    float dist = sqrtf(dx*dx + dy*dy);

                    if (dist < 120.0f) {
                        float alpha = (1.0f - dist / 120.0f) * 0.3f;
                        dl->AddLine(
                                ImVec2(offset.x + particles[i].x, offset.y + particles[i].y),
                                ImVec2(offset.x + particles[j].x, offset.y + particles[j].y),
                                ImGui::ColorConvertFloat4ToU32(ImVec4(tc.accent.x, tc.accent.y, tc.accent.z, alpha)),
                                1.0f
                        );
                    }
                }
            }
        }
    }

}
