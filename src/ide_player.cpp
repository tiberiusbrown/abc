#include "common.hpp"

#include <imgui.h>

#include "font_icons.hpp"

void player_window_contents()
{
    using namespace ImGui;

    Button("Debug " ICON_FA_PLAY_CIRCLE);
    Button("Pause " ICON_FA_PAUSE_CIRCLE);
    Button("Stop " ICON_FA_STOP_CIRCLE);

}
