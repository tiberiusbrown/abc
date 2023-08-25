#include "ide_common.hpp"

#include<algorithm>

#include <imgui.h>
#include <imgui_internal.h>

static std::string const main_prog = R"(u8 x;

void setup()
{
}

void loop()
{
    draw_filled_rect(x, 0, 16, 16, 1);
    x = x + 1;
    if(x == 112)
        x = 0;
    display();
}

void main()
{
    setup();
    while(true)
        loop();
})";

void new_project()
{
    project = {};
    editors.clear();

    auto& f = project.code_files["main.abc"];
    f.filename = "main.abc";
    f.bytes = std::vector<uint8_t>(main_prog.begin(), main_prog.end());

    auto& e = editors["main.abc"] = editor_t("main.abc");

    // set up docking
    {
        using namespace ImGui;
        ImGuiID t0, t1;
        DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f, &dockid_project, &t0);
        ImGuiDockNode* project = ImGui::DockBuilderGetNode(dockid_project);
        project->LocalFlags |=
            ImGuiDockNodeFlags_NoTabBar |
            ImGuiDockNodeFlags_NoDockingOverMe |
            ImGuiDockNodeFlags_NoDockingSplitMe;
        ImGuiDockNode* root = ImGui::DockBuilderGetNode(dockspace_id);
        root->LocalFlags |= ImGuiDockNodeFlags_NoDockingSplitMe;

        DockBuilderSplitNode(t0, ImGuiDir_Left, 0.50f, &t0, &t1);
        DockBuilderDockWindow(e.window_name().c_str(), t0);
    }
}
