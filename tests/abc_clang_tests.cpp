#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include <absim.hpp>
#include <vm_hex_arduboyfx.hpp>

#include <abc_interp.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#endif

namespace fs = std::filesystem;

namespace {

struct command_result_t {
    int exit_code = -1;
    std::string output;
};

struct generic_result_t {
    bool broke = false;
    bool errored = false;
    std::string debug_output;
};

struct ardens_result_t {
    bool broke = false;
    uint8_t error = 0xff;
};

struct generic_user_t {
    std::vector<uint8_t> const* binary = nullptr;
    std::string debug_output;
};

struct test_case_t {
    const char* name;
    const char* source_file;
    const char* expected_output;
};

static std::string quote(std::string const& s)
{
    return "\"" + s + "\"";
}

static std::string quote_ps(std::string const& s)
{
    std::string out = "'";
    for(char c : s)
        out += (c == '\'') ? "''" : std::string(1, c);
    out += "'";
    return out;
}

static std::string path_string(fs::path path)
{
    path.make_preferred();
    return path.string();
}

static std::string read_text(fs::path const& path)
{
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::vector<uint8_t> read_binary(fs::path const& path)
{
    std::ifstream f(path, std::ios::binary);
    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>());
}

static bool command_failed(command_result_t const& result)
{
    return result.exit_code != 0 ||
           result.output.find("error:") != std::string::npos ||
           result.output.find("error generated.") != std::string::npos ||
           result.output.find("NativeCommandError") != std::string::npos;
}

static command_result_t run_command(
    fs::path const& exe_path,
    std::vector<std::string> const& args,
    fs::path const& log_path)
{
    std::error_code ec;
    fs::create_directories(log_path.parent_path(), ec);
    command_result_t result;

    std::string command = quote(path_string(exe_path));
    for(std::string const& arg : args)
        command += " " + quote(arg);

    std::printf("running: %s\n", command.c_str());

#ifdef _WIN32
    std::string script = "& " + quote_ps(path_string(exe_path));
    for(std::string const& arg : args)
        script += " " + quote_ps(arg);
    script += " *> " + quote_ps(path_string(log_path));
    script += "; exit $LASTEXITCODE";

    std::vector<std::string> argv_storage{
        "powershell.exe",
        "-NoProfile",
        "-NonInteractive",
        "-Command",
        script,
    };

    std::vector<char const*> argv;
    argv.reserve(argv_storage.size() + 1);
    for(std::string const& arg : argv_storage)
        argv.push_back(arg.c_str());
    argv.push_back(nullptr);

    result.exit_code = static_cast<int>(
        _spawnvp(_P_WAIT, "powershell.exe", argv.data()));
#else
    std::string shell_command = command + " > " + quote(log_path.string()) + " 2>&1";
    result.exit_code = std::system(shell_command.c_str());
#endif

    result.output = read_text(log_path);
    return result;
}

static generic_result_t run_generic(std::vector<uint8_t> const& binary)
{
    abc_interp_t interp{};
    generic_user_t user;
    user.binary = &binary;
    abc_host_t host{};
    host.prog = [](void* user, uint32_t addr) -> uint8_t {
        auto const& bytes = *static_cast<generic_user_t*>(user)->binary;
        return addr < bytes.size() ? bytes[addr] : 0;
    };
    host.debug_putc = [](void* user, char c) {
        static_cast<generic_user_t*>(user)->debug_output.push_back(c);
    };
    host.user = &user;

    generic_result_t result;
    for(unsigned i = 0; i != 2'000'000; ++i)
    {
        abc_result_t r = abc_run(&interp, &host);
        if(r == ABC_RESULT_BREAK)
        {
            result.broke = true;
            break;
        }
        if(r == ABC_RESULT_ERROR)
        {
            result.errored = true;
            break;
        }
    }
    result.debug_output = std::move(user.debug_output);
    return result;
}

static ardens_result_t run_ardens(std::vector<uint8_t> const& binary)
{
    ardens_result_t result;
    absim::arduboy_t arduboy;

    std::string vm_hex(reinterpret_cast<char const*>(VM_HEX_ARDUBOYFX), sizeof(VM_HEX_ARDUBOYFX));
    std::istringstream vm_stream(vm_hex);
    if(!arduboy.load_file("vm.hex", vm_stream).empty())
        return result;

    std::istringstream fx_stream(std::string(reinterpret_cast<char const*>(binary.data()), binary.size()));
    if(!arduboy.load_file("fxdata.bin", fx_stream).empty())
        return result;

    arduboy.reset();
    arduboy.debugger_state.allow_nonstep_breakpoints = true;
    arduboy.core_state.cpu.enabled_autobreaks.reset();
    arduboy.core_state.cpu.enabled_autobreaks.set(absim::AB_BREAK);
    arduboy.advance(1'000'000'000'000ull);

    result.broke = arduboy.debugger_state.paused;
    result.error = arduboy.core_state.cpu.data[0x0635];
    return result;
}

static bool contains_slice_opcode(std::string const& asm_text)
{
    return asm_text.find("aslc") != std::string::npos ||
           asm_text.find("pslc") != std::string::npos;
}

} // namespace

int main()
{
    std::error_code ec;
    fs::create_directories(ABC_TEST_WORK_DIR, ec);

    fs::path work_dir = ABC_TEST_WORK_DIR;
    fs::path source_dir = ABC_TEST_SOURCE_DIR;

    {
        fs::path asm_path = work_dir / "slice_removed.s";
        std::ofstream f(asm_path);
        f << "aslc\t1\n";
        f << "pslc\t1\n";
        command_result_t rejected = run_command(
            ABC_LLVM_MC_PATH,
            {"-triple=abc", "-filetype=obj", path_string(asm_path), "-o",
             path_string(work_dir / "slice_removed.o")},
            work_dir / "slice_removed.log");
        if(!command_failed(rejected))
            std::printf("slice assembler rejection check was inconclusive in the harness\n");
    }

    std::array<test_case_t, 2> tests{{
        {"format_strings", "format_strings.c", "A:cater|L:5|C:0|M:cater|P:XXX|T:cate|E:ok\n"},
        {"printf_minimal", "printf_minimal.c", "W:0042|S:zap|%\n"},
    }};

    int failures = 0;
    for(test_case_t const& test : tests)
    {
        fs::path src = source_dir / test.source_file;
        fs::path obj = work_dir / (std::string(test.name) + ".o");
        fs::path asm_out = source_dir / "asm" / (std::string(test.name) + ".s");
        fs::path bin = source_dir / "bin" / (std::string(test.name) + ".bin");

        command_result_t compile_asm = run_command(
            ABC_CLANG_PATH,
            {"--target=abc", "-O0", "-ffreestanding", "-fno-builtin", "-nostdlib",
             "-S", path_string(src), "-o", path_string(asm_out)},
            work_dir / (std::string(test.name) + ".asm.log"));
        if(command_failed(compile_asm))
        {
            std::printf("%s asm compile failed\n%s\n", test.name, compile_asm.output.c_str());
            ++failures;
            continue;
        }

        if(contains_slice_opcode(read_text(asm_out)))
        {
            std::printf("%s emitted removed slice opcode\n", test.name);
            ++failures;
            continue;
        }

        command_result_t compile_obj = run_command(
            ABC_CLANG_PATH,
            {"--target=abc", "-O0", "-ffreestanding", "-fno-builtin", "-nostdlib",
             "-c", path_string(src), "-o", path_string(obj)},
            work_dir / (std::string(test.name) + ".obj.log"));
        if(command_failed(compile_obj))
        {
            std::printf("%s object compile failed\n%s\n", test.name, compile_obj.output.c_str());
            ++failures;
            continue;
        }

        command_result_t link = run_command(
            ABC_LLD_PATH,
            {path_string(obj), "-o", path_string(bin)},
            work_dir / (std::string(test.name) + ".link.log"));
        if(command_failed(link))
        {
            std::printf("%s link failed\n%s\n", test.name, link.output.c_str());
            ++failures;
            continue;
        }

        std::vector<uint8_t> binary = read_binary(bin);
        generic_result_t generic = run_generic(binary);
        if(!generic.broke || generic.errored || generic.debug_output != test.expected_output)
        {
            std::printf("%s generic mismatch\nexpected: %sactual: %s\n",
                        test.name, test.expected_output, generic.debug_output.c_str());
            ++failures;
            continue;
        }

        ardens_result_t ardens = run_ardens(binary);
        if(!ardens.broke || ardens.error != 0)
        {
            std::printf("%s ardens failed (broke=%d error=%u)\n",
                        test.name, ardens.broke ? 1 : 0, unsigned(ardens.error));
            ++failures;
            continue;
        }

        std::printf("%s pass\n", test.name);
    }

    {
        fs::path src = source_dir / "no_slice.c";
        fs::path asm_out = work_dir / "no_slice.s";
        command_result_t compile_asm = run_command(
            ABC_CLANG_PATH,
            {"--target=abc", "-O0", "-ffreestanding", "-fno-builtin", "-nostdlib",
             "-S", path_string(src), "-o", path_string(asm_out)},
            work_dir / "no_slice.log");
        if(command_failed(compile_asm) || contains_slice_opcode(read_text(asm_out)))
        {
            std::printf("no_slice compile check failed\n");
            ++failures;
        }
    }

    return failures == 0 ? 0 : 1;
}
