/* pc_worldmap.cpp - the world map Porter shows (see pc_worldmap.h).
 *
 * Drawn with Dear ImGui's draw lists only (no windows): a sea, one island per town at the
 * spot the server gives it, name tags, and a card for the selected town. Moved with the
 * game's own controls: stick/D-pad between towns, A to choose, B to go back. */
#include "pc_worldmap.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <glad/gl.h>
#include <SDL.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

extern "C" {
#include "pc_msg.h"
extern SDL_Window* g_pc_window;
extern SDL_GLContext g_pc_gl_context;
}

namespace {

/* PAD_BUTTON_* (dolphin/pad.h) */
const unsigned kLeft = 1 << 0, kRight = 1 << 1, kDown = 1 << 2, kUp = 1 << 3;
const unsigned kA = 1 << 8, kB = 1 << 9, kStart = 1 << 12;

struct WorldMap {
    bool open = false;
    int result = PC_WORLDMAP_OPEN;
    PcWorldTown towns[PC_WORLD_MAX_TOWNS];
    char names[PC_WORLD_MAX_TOWNS][48]; /* as the game's font can show them */
    int count = 0;
    int selected = 0;
    bool wait_release = true; /* the A that opened the map must be let go first */
    unsigned prev_buttons = 0;
    int move_cooldown = 0;
    float time = 0.0f;
    ImGuiContext* ctx = nullptr;
    ImFont* font = nullptr;
};

WorldMap g_map;

ImU32 rgba(int r, int g, int b, int a = 255) {
    return IM_COL32(r, g, b, a);
}

void ensure_imgui() {
    if (g_map.ctx != nullptr) {
        return;
    }
    g_map.ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(g_map.ctx);
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    g_map.font = io.Fonts->AddFontFromFileTTF("fonts/VarelaRound-Regular.ttf", 22.0f);
    if (g_map.font == nullptr) {
        g_map.font = io.Fonts->AddFontDefault();
    }
    ImGui_ImplSDL2_InitForOpenGL(g_pc_window, g_pc_gl_context);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

void shutdown_imgui() {
    if (g_map.ctx == nullptr) {
        return;
    }
    ImGui::SetCurrentContext(g_map.ctx);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(g_map.ctx);
    g_map.ctx = nullptr;
    g_map.font = nullptr;
}

/* Nearest town in the pushed direction (within a 120 degree cone). */
void move_selection(float dx, float dy) {
    const PcWorldTown& cur = g_map.towns[g_map.selected];
    int best = -1;
    float best_score = 1e9f;

    for (int i = 0; i < g_map.count; i++) {
        if (i == g_map.selected) {
            continue;
        }
        float vx = g_map.towns[i].x - cur.x;
        float vy = g_map.towns[i].y - cur.y;
        float dist = sqrtf(vx * vx + vy * vy);
        if (dist < 1e-4f) {
            continue;
        }
        float along = (vx * dx + vy * dy) / dist;
        if (along < 0.5f) {
            continue;
        }
        float score = dist * (2.0f - along);
        if (score < best_score) {
            best_score = score;
            best = i;
        }
    }
    if (best >= 0) {
        g_map.selected = best;
    }
}

void text_centered(ImDrawList* dl, float size, ImVec2 center, ImU32 col, const char* text) {
    ImVec2 ts = g_map.font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    dl->AddText(g_map.font, size, ImVec2(center.x - ts.x * 0.5f, center.y - ts.y * 0.5f), col, text);
}

void draw_island(ImDrawList* dl, ImVec2 c, float r, const PcWorldTown& t, int index, bool selected) {
    float bob = sinf(g_map.time * 2.0f + index) * 1.5f;

    c.y += bob;
    dl->AddCircleFilled(ImVec2(c.x + 3, c.y + 5), r * 1.08f, rgba(20, 60, 110, 70), 32); /* shadow */
    dl->AddCircleFilled(c, r * 1.12f, rgba(150, 215, 230), 32);                      /* shallows */
    dl->AddCircleFilled(c, r, rgba(238, 222, 170), 32);                             /* sand */
    dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.08f), r * 0.78f, rgba(118, 190, 92), 32);
    /* a few trees and a house per resident */
    for (int k = 0; k < 3; k++) {
        float a = (float)(index * 37 + k * 120) * 0.0174533f;
        ImVec2 p(c.x + cosf(a) * r * 0.45f, c.y + sinf(a) * r * 0.38f - r * 0.1f);
        dl->AddCircleFilled(p, r * 0.16f, rgba(70, 140, 70), 12);
    }
    for (int k = 0; k < t.residents && k < 4; k++) {
        ImVec2 p(c.x - r * 0.33f + k * r * 0.22f, c.y + r * 0.18f);
        float s = r * 0.09f;
        dl->AddRectFilled(ImVec2(p.x - s, p.y - s * 0.4f), ImVec2(p.x + s, p.y + s), rgba(250, 245, 230));
        dl->AddTriangleFilled(ImVec2(p.x - s * 1.4f, p.y - s * 0.4f), ImVec2(p.x + s * 1.4f, p.y - s * 0.4f),
                              ImVec2(p.x, p.y - s * 1.6f), rgba(205, 80, 60));
    }
    if (t.playing || t.players > 0) {
        float pulse = 0.5f + 0.5f * sinf(g_map.time * 4.0f);
        dl->AddCircle(c, r * (1.25f + 0.08f * pulse), rgba(255, 245, 140, (int)(120 + 100 * pulse)), 32, 3.0f);
    }
    if (selected) {
        float jump = fabsf(sinf(g_map.time * 5.0f)) * 6.0f;
        ImVec2 tip(c.x, c.y - r * 1.35f - jump);
        dl->AddTriangleFilled(ImVec2(tip.x - 9, tip.y - 14), ImVec2(tip.x + 9, tip.y - 14), tip, rgba(240, 120, 40));
        dl->AddTriangle(ImVec2(tip.x - 9, tip.y - 14), ImVec2(tip.x + 9, tip.y - 14), tip, rgba(120, 50, 20), 2.0f);
    }

    /* name tag */
    float size = selected ? 19.0f : 16.0f;
    ImVec2 ts = g_map.font->CalcTextSizeA(size, FLT_MAX, 0.0f, g_map.names[index]);
    ImVec2 tag(c.x, c.y + r * 1.35f + 4);
    ImVec2 a(tag.x - ts.x * 0.5f - 8, tag.y - 2), b(tag.x + ts.x * 0.5f + 8, tag.y + ts.y + 2);
    dl->AddRectFilled(a, b, selected ? rgba(255, 236, 160) : rgba(250, 244, 222, 235), 10.0f);
    if (t.mine) {
        dl->AddRect(a, b, rgba(110, 170, 80), 10.0f, 0, 2.5f);
    }
    dl->AddText(g_map.font, size, ImVec2(tag.x - ts.x * 0.5f, tag.y), rgba(90, 64, 40), g_map.names[index]);
}

void draw_card(ImDrawList* dl, ImVec2 a, ImVec2 b) {
    char line[128];
    const PcWorldTown& t = g_map.towns[g_map.selected];
    ImVec2 c((a.x + b.x) * 0.5f, 0);

    dl->AddRectFilled(ImVec2(a.x + 4, a.y + 5), ImVec2(b.x + 4, b.y + 5), rgba(0, 0, 0, 60), 16.0f);
    dl->AddRectFilled(a, b, rgba(252, 246, 226), 16.0f);
    dl->AddRect(a, b, rgba(196, 160, 100), 16.0f, 0, 3.0f);

    c.y = a.y + 20;
    text_centered(dl, 24.0f, c, rgba(90, 64, 40), g_map.names[g_map.selected]);
    if (t.free_slots > 0) {
        snprintf(line, sizeof(line), "%d of 4 residents  -  %d house%s free", t.residents, t.free_slots,
                 t.free_slots == 1 ? "" : "s");
    } else {
        snprintf(line, sizeof(line), "%d of 4 residents  -  no houses free", t.residents);
    }
    c.y += 28;
    text_centered(dl, 17.0f, c, rgba(120, 96, 70), line);
    c.y += 22;
    if (t.players > 0) {
        snprintf(line, sizeof(line), "%d playing now%s", t.players, t.mine ? "  -  you live here" : "");
    } else if (t.playing) {
        snprintf(line, sizeof(line), "Someone is playing now%s", t.mine ? "  -  you live here" : "");
    } else {
        snprintf(line, sizeof(line), "%s", t.mine ? "You live here  -  nobody's around" : "Nobody's around right now");
    }
    text_centered(dl, 17.0f, c, (t.players > 0 || t.playing) ? rgba(70, 140, 60) : rgba(150, 130, 110), line);
}

} // namespace

extern "C" void pc_worldmap_open(const PcWorldTown* towns, int count) {
    if (count > PC_WORLD_MAX_TOWNS) {
        count = PC_WORLD_MAX_TOWNS;
    }
    memcpy(g_map.towns, towns, count * sizeof(PcWorldTown));
    for (int i = 0; i < count; i++) {
        pc_msg_sanitize(towns[i].name, g_map.names[i], sizeof(g_map.names[i]));
    }
    g_map.count = count;
    g_map.selected = 0;
    g_map.result = count > 0 ? PC_WORLDMAP_OPEN : PC_WORLDMAP_CANCEL;
    g_map.open = count > 0;
    g_map.wait_release = true;
    g_map.prev_buttons = 0;
    g_map.move_cooldown = 0;
    g_map.time = 0.0f;
}

extern "C" int pc_worldmap_is_open(void) {
    return g_map.open;
}

extern "C" int pc_worldmap_result(void) {
    return g_map.result;
}

extern "C" void pc_worldmap_input(unsigned buttons, int stick_x, int stick_y) {
    unsigned pressed = buttons & ~g_map.prev_buttons;

    g_map.prev_buttons = buttons;
    if (!g_map.open) {
        return;
    }
    if (g_map.wait_release) {
        if ((buttons & (kA | kB | kStart)) == 0) {
            g_map.wait_release = false;
        }
        return;
    }

    if (pressed & kA) {
        g_map.result = g_map.selected;
        g_map.open = false;
        return;
    }
    if (pressed & (kB | kStart)) {
        g_map.result = PC_WORLDMAP_CANCEL;
        g_map.open = false;
        return;
    }

    float dx = 0.0f, dy = 0.0f;
    if ((buttons & kLeft) || stick_x < -40) dx = -1.0f;
    if ((buttons & kRight) || stick_x > 40) dx = 1.0f;
    if ((buttons & kUp) || stick_y > 40) dy = -1.0f; /* map y grows downwards */
    if ((buttons & kDown) || stick_y < -40) dy = 1.0f;

    if (dx == 0.0f && dy == 0.0f) {
        g_map.move_cooldown = 0;
    } else if (g_map.move_cooldown > 0) {
        g_map.move_cooldown--;
    } else {
        float len = sqrtf(dx * dx + dy * dy);
        move_selection(dx / len, dy / len);
        g_map.move_cooldown = 12;
    }
}

extern "C" void pc_worldmap_draw(void) {
    if (!g_map.open) {
        if (g_map.ctx != nullptr) {
            shutdown_imgui(); /* closed since last frame */
        }
        return;
    }
    ensure_imgui();
    ImGui::SetCurrentContext(g_map.ctx);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    g_map.time += ImGui::GetIO().DeltaTime;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 screen = ImGui::GetIO().DisplaySize;

    /* the board: 4:3, centered */
    float h = screen.y * 0.92f;
    float w = h * 4.0f / 3.0f;
    if (w > screen.x * 0.94f) {
        w = screen.x * 0.94f;
        h = w * 3.0f / 4.0f;
    }
    ImVec2 a((screen.x - w) * 0.5f, (screen.y - h) * 0.5f), b(a.x + w, a.y + h);

    dl->AddRectFilled(ImVec2(0, 0), screen, rgba(10, 20, 40, 150));
    dl->AddRectFilled(ImVec2(a.x + 6, a.y + 8), ImVec2(b.x + 6, b.y + 8), rgba(0, 0, 0, 80), 24.0f);
    dl->AddRectFilledMultiColor(a, b, rgba(120, 200, 230), rgba(100, 185, 225), rgba(60, 140, 200),
                                rgba(70, 150, 210));
    /* waves */
    for (int i = 0; i < 26; i++) {
        float wx = a.x + fmodf(i * 97.3f + g_map.time * 6.0f, w);
        float wy = a.y + fmodf(i * 61.7f, h);
        dl->AddBezierQuadratic(ImVec2(wx - 10, wy), ImVec2(wx, wy - 6), ImVec2(wx + 10, wy), rgba(255, 255, 255, 90),
                               2.0f);
    }
    dl->AddRect(a, b, rgba(250, 238, 200), 24.0f, 0, 8.0f);

    /* title */
    ImVec2 title(screen.x * 0.5f, a.y + 30);
    ImVec2 ts = g_map.font->CalcTextSizeA(28.0f, FLT_MAX, 0.0f, "World Map");
    dl->AddRectFilled(ImVec2(title.x - ts.x * 0.5f - 22, title.y - 20), ImVec2(title.x + ts.x * 0.5f + 22, title.y + 20),
                      rgba(240, 160, 70), 20.0f);
    text_centered(dl, 28.0f, title, rgba(255, 250, 235), "World Map");

    /* towns (the map area leaves room for the title and the card) */
    ImVec2 ma(a.x + w * 0.04f, a.y + h * 0.14f), mb(b.x - w * 0.04f, b.y - h * 0.26f);
    float r = h * 0.045f;
    for (int i = 0; i < g_map.count; i++) {
        if (i == g_map.selected) {
            continue; /* drawn last, on top */
        }
        ImVec2 c(ma.x + (mb.x - ma.x) * g_map.towns[i].x, ma.y + (mb.y - ma.y) * g_map.towns[i].y);
        draw_island(dl, c, r, g_map.towns[i], i, false);
    }
    {
        int i = g_map.selected;
        ImVec2 c(ma.x + (mb.x - ma.x) * g_map.towns[i].x, ma.y + (mb.y - ma.y) * g_map.towns[i].y);
        draw_island(dl, c, r * 1.15f, g_map.towns[i], i, true);
    }

    draw_card(dl, ImVec2(a.x + w * 0.18f, b.y - h * 0.21f), ImVec2(b.x - w * 0.18f, b.y - h * 0.035f));
    text_centered(dl, 15.0f, ImVec2(screen.x * 0.5f, b.y - h * 0.012f), rgba(255, 255, 255, 220),
                  "Stick: move    A: choose    B: back");

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
