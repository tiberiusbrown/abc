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
}

void editor_t::update()
{
    if(editor.IsTextChanged())
        dirty = true;
    std::string id = filename;
    if(dirty) id += "*";
    id += "###file_" + filename;
    ImGui::SetNextWindowSize(
        { 400 * pixel_ratio, 400 * pixel_ratio },
        ImGuiCond_FirstUseEver);
    if(ImGui::Begin(id.c_str(), &open))
    {
        editor.Render("###file");
        ImGui::End();
    }
}

void editor_t::save()
{
    if(!dirty) return;
    if(project_path.empty()) return;
    std::ofstream fout(project_path / filename, std::ios::out);
    if(!fout) return;

    auto t = editor.GetText();
    fout.write(t.c_str(), t.size());
    dirty = false;
}
