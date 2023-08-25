#include "ide_common.hpp"

#include <fstream>

#include "ards_compiler.hpp"
#include "ards_assembler.hpp"

std::unordered_map<std::string, editor_t> editors;

static TextEditor::LanguageDefinition const& ABC()
{
    static bool inited = false;
    static TextEditor::LanguageDefinition abc{};
    if(inited) return abc;

    abc = TextEditor::LanguageDefinition::C();

    static const char* const keywords[] =
    {
        "u8", "u16", "u24", "u32",
        "i8", "i16", "i24", "i32",
        "uchar", "uint", "ulong",
        "char", "int", "long",
        "void", "bool",
        "if", "else", "for", "while", "return",
        "true", "false",
    };
    abc.mKeywords.clear();
    for(auto k : keywords)
        abc.mKeywords.insert(k);

    abc.mIdentifiers.clear();
    for(auto const& [k, v] : ards::sys_names)
    {
        TextEditor::Identifier id{};
        id.mDeclaration = "System function";
        abc.mIdentifiers.insert(std::make_pair(k.c_str(), id));
    }

    abc.mName = "ABC";

    return abc;
}

editor_t::editor_t(std::string const& fname)
    : editor()
    , filename(fname)
    , dirty(false)
    , open(true)
{
    editor.SetColorizerEnable(true);
    editor.SetPalette(editor.GetDarkPalette());
    editor.SetShowWhitespaces(false);
    editor.SetLanguageDefinition(ABC());
    if(auto const* f = project.get_file(fname))
    {
        auto const& b = f->bytes;
        editor.SetText(std::string((char const*)b.data(), b.size()));
    }
}

std::string editor_t::window_name()
{
    std::string id = filename;
    if(dirty) id += "*";
    id += "###file_" + filename;
    return id;
}

void editor_t::update()
{
    bool was_changed = editor.IsTextChanged();
    ImGui::SetNextWindowSize(
        { 400 * pixel_ratio, 400 * pixel_ratio },
        ImGuiCond_FirstUseEver);
    if(ImGui::Begin(window_name().c_str(), &open))
    {
        if(ImGui::IsWindowFocused(ImGuiHoveredFlags_ChildWindows))
            selected_dockid = ImGui::GetWindowDockID();
        editor.Render("###file");
    }
    ImGui::End();

    if(editor.IsTextChanged() && !was_changed)
        dirty = true;
}
