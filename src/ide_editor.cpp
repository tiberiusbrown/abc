#include "ide_common.hpp"

#include <fstream>

#include "abc_compiler.hpp"
#include "abc_assembler.hpp"

struct open_code_file_t : public open_file_t
{
    TextEditor editor;
    open_code_file_t(std::string const& filename);
    void save_impl() override;
    void window_contents() override;
};

std::unique_ptr<open_file_t> create_code_file(std::string const& filename)
{
    return std::make_unique<open_code_file_t>(filename);
}

static TextEditor::LanguageDefinition const& ABC()
{
    static bool inited = false;
    static TextEditor::LanguageDefinition abc{};
    if(inited) return abc;

    abc = TextEditor::LanguageDefinition::C();

    abc.mKeywords.clear();
    for(auto const& k : abc::keywords)
        abc.mKeywords.insert(k);
    for(auto const& [k, v] : abc::sys_names)
        abc.mKeywords.insert(k);

    abc.mIdentifiers.clear();
    //for(auto const& [k, v] : abc::sys_names)
    //{
    //    TextEditor::Identifier id{};
    //    //id.mDeclaration = "System function";
    //    abc.mIdentifiers.insert(std::make_pair(k, id));
    //}

    abc.mName = "ABC";

    return abc;
}

open_code_file_t::open_code_file_t(std::string const& filename)
    : open_file_t(filename)
    , editor()
{
    editor.SetColorizerEnable(true);
    editor.SetPalette(editor.GetDarkPalette());
    editor.SetShowWhitespaces(false);
    editor.SetLanguageDefinition(ABC());
    editor.SetText(read_as_string());
}

void open_code_file_t::window_contents()
{
    bool was_changed = editor.IsTextChanged();

    TextEditor::ErrorMarkers errors{};
    if(auto it = project.errors.find(filename()); it != project.errors.end())
    {
        for(auto const& e : it->second)
            errors[(int)e.line_info.first] = e.msg;
    }
    editor.SetErrorMarkers(errors);

    editor.Render("###file");

    if(editor.IsTextChanged() && !was_changed)
        dirty = true;
}

void open_code_file_t::save_impl()
{
    std::ofstream f(path, std::ios::out | std::ios::binary);
    if(f.fail()) return;
    f << editor.GetText();
}
