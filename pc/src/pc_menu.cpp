/* pc_menu.cpp - start menu (Dear ImGui) shown in the game window before the game boots.
 *
 * Log in to a town server, then pick one of your towns, create one, or join one with
 * an invite code; or play offline. All server calls go through pc_cloud (blocking,
 * they are short). */
#include "pc_menu.h"

#include <glad/gl.h>
#include <SDL.h>
#include <stdio.h>
#include <string.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

extern "C" {
#include "pc_cloud.h"
extern SDL_Window* g_pc_window;
extern SDL_GLContext g_pc_gl_context;
}

namespace {

enum Screen { SCREEN_LOGIN, SCREEN_TOWNS };

struct Menu {
    Screen screen = SCREEN_LOGIN;
    int result = -1;

    char server[256] = "";
    char user[64] = "";
    char pass[128] = "";
    char status[256] = ""; /* last error or notice */
    bool status_is_error = false;

    PcCloudTown towns[PC_CLOUD_MAX_TOWNS];
    int town_count = 0;

    char new_town_name[32] = "";
    char join_code[16] = "";
    char shared_code[16] = ""; /* code to show after creating a town */
};

const ImVec4 kGreen(0.36f, 0.62f, 0.30f, 1.0f);
const ImVec4 kError(0.85f, 0.25f, 0.20f, 1.0f);
const ImVec4 kMuted(0.45f, 0.42f, 0.36f, 1.0f);

void set_status(Menu& m, const char* msg, bool error) {
    snprintf(m.status, sizeof(m.status), "%s", msg);
    m.status_is_error = error;
}

void refresh_towns(Menu& m) {
    char err[160];
    int n = pc_cloud_list_towns(m.towns, PC_CLOUD_MAX_TOWNS, err, sizeof(err));

    if (n < 0) {
        m.town_count = 0;
        set_status(m, err, true);
        return;
    }
    m.town_count = n;
}

void login(Menu& m, bool create_account) {
    char err[160];

    if (m.server[0] == '\0') {
        set_status(m, "Escribe la dirección del servidor.", true);
        return;
    }
    pc_cloud_set_server(m.server);
    snprintf(m.server, sizeof(m.server), "%s", pc_cloud_get_server());
    if (!pc_cloud_auth(m.user, m.pass, create_account, err, sizeof(err))) {
        set_status(m, err, true);
        return;
    }
    memset(m.pass, 0, sizeof(m.pass));
    m.status[0] = '\0';
    m.screen = SCREEN_TOWNS;
    refresh_towns(m);
}

void style_setup() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImGui::StyleColorsLight();
    s.WindowRounding = 12.0f;
    s.FrameRounding = 8.0f;
    s.GrabRounding = 8.0f;
    s.FramePadding = ImVec2(12, 8);
    s.ItemSpacing = ImVec2(10, 10);
    s.WindowPadding = ImVec2(24, 20);
    s.Colors[ImGuiCol_WindowBg] = ImVec4(0.98f, 0.96f, 0.88f, 1.0f);
    s.Colors[ImGuiCol_Button] = ImVec4(0.55f, 0.76f, 0.44f, 1.0f);
    s.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.62f, 0.83f, 0.50f, 1.0f);
    s.Colors[ImGuiCol_ButtonActive] = ImVec4(0.44f, 0.66f, 0.35f, 1.0f);
    s.Colors[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    s.Colors[ImGuiCol_Text] = ImVec4(0.25f, 0.20f, 0.14f, 1.0f);
    s.Colors[ImGuiCol_PopupBg] = ImVec4(0.99f, 0.97f, 0.90f, 1.0f);
    s.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.93f, 0.90f, 0.80f, 0.6f);
    s.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.84f, 0.90f, 0.70f, 1.0f);
    s.Colors[ImGuiCol_TitleBg] = ImVec4(0.55f, 0.76f, 0.44f, 1.0f);
    s.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.55f, 0.76f, 0.44f, 1.0f);
    s.Colors[ImGuiCol_Header] = ImVec4(0.84f, 0.90f, 0.70f, 1.0f);
    s.Colors[ImGuiCol_NavCursor] = ImVec4(0.90f, 0.55f, 0.20f, 1.0f);
}

void draw_status(Menu& m) {
    if (m.status[0] != '\0') {
        ImGui::PushStyleColor(ImGuiCol_Text, m.status_is_error ? kError : kGreen);
        ImGui::TextWrapped("%s", m.status);
        ImGui::PopStyleColor();
    }
}

void draw_login(Menu& m) {
    ImGui::TextColored(kMuted, "Juega en el mismo pueblo con tus amigos y hermanos.");
    ImGui::Spacing();

    ImGui::Text("Servidor");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##server", "tu-servidor.up.railway.app", m.server, sizeof(m.server));
    ImGui::Text("Usuario");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##user", m.user, sizeof(m.user), ImGuiInputTextFlags_CharsNoBlank);
    ImGui::Text("Contraseña");
    ImGui::SetNextItemWidth(-1);
    bool enter = ImGui::InputText("##pass", m.pass, sizeof(m.pass),
                                  ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::Spacing();

    float w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2;
    if (ImGui::Button("Entrar", ImVec2(w, 0)) || enter) {
        login(m, false);
    }
    ImGui::SameLine();
    if (ImGui::Button("Crear cuenta", ImVec2(w, 0))) {
        login(m, true);
    }
    draw_status(m);

    ImGui::Separator();
    if (ImGui::Button("Jugar sin conexión", ImVec2(-1, 0))) {
        m.result = PC_MENU_OFFLINE;
    }
}

void draw_town_list(Menu& m) {
    const char* me = pc_cloud_get_user();

    if (m.town_count == 0) {
        ImGui::TextColored(kMuted, "Todavía no vives en ningún pueblo: crea uno o únete con un código.");
        return;
    }
    if (!ImGui::BeginTable("towns", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        return;
    }
    ImGui::TableSetupColumn("Pueblo", ImGuiTableColumnFlags_WidthStretch, 3.0f);
    ImGui::TableSetupColumn("Residentes", ImGuiTableColumnFlags_WidthStretch, 1.6f);
    ImGui::TableSetupColumn("Ahora", ImGuiTableColumnFlags_WidthStretch, 2.2f);
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 110.0f);
    ImGui::TableHeadersRow();

    for (int i = 0; i < m.town_count; i++) {
        PcCloudTown& t = m.towns[i];
        bool someone_else = t.host[0] != '\0' && strcmp(t.host, me) != 0;

        ImGui::PushID(i);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", t.name);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Código para invitar: %s", t.code);
        }
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%d de 4", t.residents);
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        if (someone_else) {
            ImGui::TextColored(kGreen, "%s está jugando", t.host);
        } else if (t.save_version == 0) {
            ImGui::TextColored(kMuted, "pueblo nuevo");
        } else {
            ImGui::TextColored(kMuted, "libre");
        }
        ImGui::TableNextColumn();
        ImGui::BeginDisabled(someone_else);
        if (ImGui::Button(t.save_version == 0 ? "Fundar" : "Jugar", ImVec2(-1, 0))) {
            pc_cloud_select_town(t.id);
            m.result = PC_MENU_CLOUD_TOWN;
        }
        ImGui::EndDisabled();
        if (someone_else && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Unirse a su partida en vivo llega pronto.\nPor ahora: --join con su IP.");
        }
        ImGui::PopID();
    }
    ImGui::EndTable();
}

void draw_towns(Menu& m) {
    ImGui::TextColored(kMuted, "Hola, %s  -  %s", pc_cloud_get_user(), pc_cloud_get_server());
    ImGui::Spacing();
    ImGui::Text("Mis pueblos");
    draw_town_list(m);
    ImGui::Spacing();

    if (m.shared_code[0] != '\0') {
        ImGui::PushStyleColor(ImGuiCol_Text, kGreen);
        ImGui::TextWrapped("Pueblo creado. Invita a tus amigos con el código:");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("%s", m.shared_code);
        ImGui::SameLine();
        if (ImGui::SmallButton("Copiar")) {
            SDL_SetClipboardText(m.shared_code);
        }
    }

    float w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2;
    if (ImGui::Button("Crear pueblo nuevo", ImVec2(w, 0))) {
        m.new_town_name[0] = '\0';
        ImGui::OpenPopup("create");
    }
    ImGui::SameLine();
    if (ImGui::Button("Unirme con código", ImVec2(w, 0))) {
        m.join_code[0] = '\0';
        ImGui::OpenPopup("join");
    }
    ImGui::BeginDisabled();
    ImGui::Button("Visitar un pueblo (pronto)", ImVec2(-1, 0));
    ImGui::EndDisabled();

    if (ImGui::BeginPopupModal("create", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        char err[160];
        PcCloudTown t;

        ImGui::Text("Nombre del pueblo");
        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }
        bool enter = ImGui::InputText("##name", m.new_town_name, sizeof(m.new_town_name),
                                      ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::TextColored(kMuted, "La primera vez que lo juegues empieza la intro del tren.");
        if ((ImGui::Button("Crear") || enter) && m.new_town_name[0] != '\0') {
            if (pc_cloud_create_town(m.new_town_name, &t, err, sizeof(err))) {
                snprintf(m.shared_code, sizeof(m.shared_code), "%s", t.code);
                m.status[0] = '\0';
                refresh_towns(m);
            } else {
                set_status(m, err, true);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancelar")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("join", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        char err[160];
        PcCloudTown t;

        ImGui::Text("Código del pueblo (ej. HOJA-7K2M)");
        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }
        bool enter = ImGui::InputText("##code", m.join_code, sizeof(m.join_code),
                                      ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::TextColored(kMuted, "Te mudas como residente nuevo (máximo 4 por pueblo).");
        if ((ImGui::Button("Unirme") || enter) && m.join_code[0] != '\0') {
            if (pc_cloud_join_town(m.join_code, &t, err, sizeof(err))) {
                char msg[160];
                snprintf(msg, sizeof(msg), "Ahora vives en %s. Al jugarlo, crea tu personaje.", t.name);
                set_status(m, msg, false);
                refresh_towns(m);
            } else {
                set_status(m, err, true);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancelar")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    draw_status(m);
    ImGui::Separator();

    float w3 = (ImGui::GetContentRegionAvail().x - 2 * ImGui::GetStyle().ItemSpacing.x) / 3;
    if (ImGui::Button("Actualizar", ImVec2(w3, 0))) {
        m.status[0] = '\0';
        refresh_towns(m);
    }
    ImGui::SameLine();
    if (ImGui::Button("Sin conexión", ImVec2(w3, 0))) {
        m.result = PC_MENU_OFFLINE;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cerrar sesión", ImVec2(w3, 0))) {
        pc_cloud_logout();
        m.screen = SCREEN_LOGIN;
        m.town_count = 0;
        m.status[0] = '\0';
    }
}

void draw(Menu& m) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 size(vp->WorkSize.x * 0.86f, vp->WorkSize.y * 0.86f);

    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x * 0.5f, vp->WorkPos.y + vp->WorkSize.y * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::Begin("ACGC Online", NULL,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings);
    if (m.screen == SCREEN_LOGIN) {
        draw_login(m);
    } else {
        draw_towns(m);
    }
    ImGui::End();
}

} // namespace

extern "C" int pc_menu_run(void) {
    Menu m;
    SDL_Event ev;

    if (pc_cloud_load_settings()) {
        m.screen = SCREEN_TOWNS;
    }
    snprintf(m.server, sizeof(m.server), "%s", pc_cloud_get_server());
    snprintf(m.user, sizeof(m.user), "%s", pc_cloud_get_user());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    style_setup();
    ImGui::GetStyle().FontSizeBase = 20.0f;
    io.Fonts->AddFontDefault();
    ImGui_ImplSDL2_InitForOpenGL(g_pc_window, g_pc_gl_context);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    if (m.screen == SCREEN_TOWNS) {
        refresh_towns(m);
    }

    while (m.result < 0) {
        int w;
        int h;

        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT ||
                (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_CLOSE)) {
                m.result = PC_MENU_QUIT;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        draw(m);
        ImGui::Render();

        SDL_GL_GetDrawableSize(g_pc_window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.47f, 0.70f, 0.40f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(g_pc_window);
        SDL_Delay(16);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    return m.result;
}
