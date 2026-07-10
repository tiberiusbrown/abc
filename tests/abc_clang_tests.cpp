#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include <absim.hpp>
#include <vm_hex_arduboyfx.hpp>

#include <abc_interp.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

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
    const char* expected_output;
};

static std::string path_string(fs::path path)
{
    path.make_preferred();
    return path.string();
}

static std::vector<uint8_t> read_binary(fs::path const& path)
{
    std::ifstream f(path, std::ios::binary);
    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>());
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
    std::unique_ptr<absim::arduboy_t> arduboy = std::make_unique<absim::arduboy_t>();

    std::string vm_hex(reinterpret_cast<char const*>(VM_HEX_ARDUBOYFX), sizeof(VM_HEX_ARDUBOYFX));
    std::istringstream vm_stream(vm_hex);
    if(!arduboy->load_file("vm.hex", vm_stream).empty())
        return result;

    std::istringstream fx_stream(std::string(reinterpret_cast<char const*>(binary.data()), binary.size()));
    if(!arduboy->load_file("fxdata.bin", fx_stream).empty())
        return result;

    arduboy->reset();
    arduboy->debugger_state.allow_nonstep_breakpoints = true;
    arduboy->core_state.cpu.enabled_autobreaks.reset();
    arduboy->core_state.cpu.enabled_autobreaks.set(absim::AB_BREAK);
    arduboy->advance(1'000'000'000'000ull);

    result.broke = arduboy->debugger_state.paused;
    result.error = arduboy->core_state.cpu.data[0x0635];
    return result;
}

} // namespace

int main()
{
    std::error_code ec;
    fs::create_directories(ABC_TEST_WORK_DIR, ec);

    fs::path work_dir = ABC_TEST_WORK_DIR;
    fs::path source_dir = ABC_TEST_SOURCE_DIR;

    std::array<test_case_t, 7> tests{{
        {"printf_static", "static"},
        {"printf_char", "char:C"},
        {"printf_ram_string", "ram:ram"},
        {"printf_prog_string", "prog:prog"},
        {"printf_mixed_strings", "strings:ram|prog"},
        {"printf_integers", "ints:-42|42|abcd|0007|-123456|123456|12345678"},
        {"syscalls_buttons", "buttons:0|0|0|1|0|1"},
    }};

    int failures = 0;
    for(test_case_t const& test : tests)
    {
        fs::path asm_out = source_dir / "asm" / (std::string(test.name) + ".s");
        fs::path bin = work_dir / (std::string(test.name) + ".bin");

        if(!fs::exists(asm_out))
        {
            std::printf("%s asm output missing: %s\n", test.name, path_string(asm_out).c_str());
            ++failures;
            continue;
        }

        if(!fs::exists(bin))
        {
            std::printf("%s binary output missing: %s\n", test.name, path_string(bin).c_str());
            ++failures;
            continue;
        }

        std::vector<uint8_t> binary = read_binary(bin);
        generic_result_t generic = run_generic(binary);
        if(!generic.broke || generic.errored || generic.debug_output != test.expected_output)
        {
            std::printf("%s generic mismatch\n   expected: <%s>\n   actual: <%s>\n",
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

    return failures == 0 ? 0 : 1;
}
