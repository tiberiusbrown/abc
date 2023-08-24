#include "ide_common.hpp"

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
    std::error_code ec;
    project_path = std::filesystem::temp_directory_path(ec);
    if(ec)
        project_path.clear();
    else
    {
        project_path = project_path / "_abc_project";
        std::filesystem::create_directories(project_path);
    }

    auto& f_main = editors["main.abc"] = editor_t("main.abc");
    f_main.dirty = true;
    f_main.editor.SetText(main_prog);
}
