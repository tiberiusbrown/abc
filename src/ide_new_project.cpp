#include "ide_common.hpp"

#include<algorithm>

#include <imgui.h>
#include <imgui_internal.h>

static std::string const main_name = "main.abc";
static std::string const main_prog = R"(int x;
bool dir;

void setup()
{
	//dir = true;
}

void loop()
{
    while(!$next_frame())
        ;
    $draw_filled_rect(x, 0, 16, 16, 1);
    if(dir)
    {
    	x = x + 1;
    	if(x >= 112)
    		dir = false;
    }
    else
    {
    	x = x - 1;
    	if(x <= 0)
    		dir = true;
    }
    $display();
}

void main()
{
    setup();
    while(true)
        loop();
})";

static std::string const info_json = R"({
    "name": "My Game",
    "author": "Unknown",
    "desc": "A totally awesome game!"
})";

void new_project()
{
    project = {};

    {
        auto f = project.files[INFO_FILENAME] = std::make_shared<project_file_t>();
        f->filename = INFO_FILENAME;
        f->set_content(info_json);
    }

    {
        auto f = project.files[main_name] = std::make_shared<project_file_t>();
        f->filename = main_name;
        f->set_content(main_prog);
    }

    {
        auto f = create_project_info_file(INFO_FILENAME);
        f->dirty = true;
        f->save();
    }

    open_files[main_name] = create_code_file(main_name);

    // set up docking
    {
        using namespace ImGui;
        ImGuiID t0, t1;
        DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f, &dockid_project, &t0);
        ImGuiDockNode* p = ImGui::DockBuilderGetNode(dockid_project);
        p->LocalFlags |=
            ImGuiDockNodeFlags_NoTabBar |
            ImGuiDockNodeFlags_NoDockingOverMe |
            ImGuiDockNodeFlags_NoDockingSplitMe;
        ImGuiDockNode* root = ImGui::DockBuilderGetNode(dockspace_id);
        root->LocalFlags |= ImGuiDockNodeFlags_NoDockingSplitMe;

        DockBuilderSplitNode(t0, ImGuiDir_Left, 0.50f, &t0, &t1);
        DockBuilderDockWindow(open_files[main_name]->window_id().c_str(), t0);
    }
}
