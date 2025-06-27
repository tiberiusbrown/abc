#include "ide_common.hpp"

#include <imgui.h>

#include <imgui_markdown/imgui_markdown.h>

static std::string const WELCOME_STR =
std::string("# ABC IDE ") + abc_version + R"(
Copyright (c) )" ABC_YEAR R"( Peter Brown (MIT License).

ABC is a C-like scripting language with an associated interpreter designed for the Arduboy FX. It includes runtime error checking and support for various FX assets, such as sprites, fonts, strings, and arbitrary data.
)";

void project_welcome()
{
    using namespace ImGui;

    MarkdownConfig cfg{};

    cfg.headingFormats[0] = { font_h1, true };
    cfg.headingFormats[1] = { font_h2, true };
    cfg.headingFormats[2] = { font_h3, false };

    Markdown(WELCOME_STR.c_str(), WELCOME_STR.size(), cfg);

    NewLine();

    if(Button("Open System Reference"))
    {
        SetWindowFocus("System Reference");
        show_sys = true;
    }
}
