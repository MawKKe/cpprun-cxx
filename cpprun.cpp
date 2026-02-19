// cpprun.cpp - a simple utility to compile and run C++ programs with a single command.

/*
compile with:
    $ c++ -std=c++17 -Wall -Wextra -pedantic -g cpprun.cpp -o cpprun
or:
    $ make out/cpprun

then place the 'cpprun' binary somewhere in your PATH and use it like this:
    $ cpprun [cpprun or build options] -- [program options]

cpprun options:
    --cpprun-compiler-info: show compiler version information and exit
    -c: build only, do not run the program
    -o <file>: specify output file (default is a temporary file in the system temp directory)
    -std=<version>: specify the C++ standard to use (overrides CPPRUN_CXX_STANDARD environment variable)
    (any other options are passed to the compiler as-is)

environment variables:
    CPPRUN_CXXFLAGS: additional flags to pass to the compiler (default is "-Wall -Wextra -pedantic -g")
    CPPRUN_CXX_STANDARD: specify the C++ standard to use (default is "-std=c++23",set to empty string to disable)
    CPPRUN_CXX: specify the C++ compiler to use (default is "c++")
    CPPRUN_VERBOSE: if set to a non-empty value, print the commands being executed
*/

#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace cpprun {

const std::vector<std::string> DEFAULT_CXXFLAGS = {
    "-Wall",
    "-Wextra",
    "-pedantic",
    "-g",
};

const std::string DEFAULT_CXX_STANDARD = "-std=c++23";

template <typename T>
void extend(std::vector<T> & dst, const std::vector<T> & src) {
    dst.insert(dst.end(), src.begin(), src.end());
}

template <typename T>
void append(std::vector<T> & dst, const T & value) {
    dst.push_back(value);
}

static std::string join_shell(const std::vector<std::string> & args) {
    std::string out;
    bool first = true;
    for (auto & a : args) {
        if (!first) {
            out += ' ';
        }
        first = false;
        if (a.find(' ') != std::string::npos) {
            out += '"';
            out += a;
            out += '"';
        } else {
            out += a;
        }
    }
    return out;
}

static int run_cmd(const std::string & prog, const std::vector<std::string> & args, bool verbose) {
    if (verbose) {
        std::cout << ">>> " << prog << " " << join_shell(args) << std::endl;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 127;
    }
    if (pid == 0) {
        // child
        std::vector<char *> argv;
        argv.reserve(args.size() + 1);
        argv.push_back(const_cast<char *>(prog.c_str()));
        for (auto & s : args) {
            argv.push_back(const_cast<char *>(s.c_str()));
        }
        argv.push_back(nullptr);
        execvp(prog.c_str(), argv.data());
        perror("execvp");
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 127;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return status;
}

auto split_args(const std::vector<std::string> & args, const std::string & sep = "--") {
    auto it = std::find(std::begin(args), std::end(args), sep);

    if (it == std::end(args)) {
        return std::pair{
            args,
            std::vector<std::string>{},
        };
    }

    return std::pair{
        std::vector<std::string>{
            std::begin(args),
            it,
        },
        std::vector<std::string>{
            std::next(it),
            std::end(args),
        },
    };
}

struct CpprunArgs {
    bool show_compiler_info = false;
    bool build_only = false;
    bool verbose = false;
    std::string cxx = "c++";
    std::optional<std::string> cxx_standard = DEFAULT_CXX_STANDARD;
    std::optional<fs::path> output_path = std::nullopt;
    std::vector<std::string> build_args;
};

CpprunArgs parse_cpprun_args(const std::vector<std::string> & cpprun_args) {
    CpprunArgs args;

    if (const char * cxxflags = std::getenv("CPPRUN_CXXFLAGS")) {
        std::istringstream iss(cxxflags);
        std::string flag;
        while (iss >> flag) {
            args.build_args.push_back(flag);
        }
    } else {
        extend(args.build_args, DEFAULT_CXXFLAGS);
    }

    if (const char * verbose = std::getenv("CPPRUN_VERBOSE")) {
        args.verbose = std::atoi(verbose);
    }

    if (const char * _cxx_standard = std::getenv("CPPRUN_CXX_STANDARD")) {
        auto cxx_standard = std::string(_cxx_standard);
        args.cxx_standard = cxx_standard == "" ? std::nullopt : std::make_optional(cxx_standard);
    }

    if (const char * cxx = std::getenv("CPPRUN_CXX")) {
        // overrides CXX if set
        args.cxx = cxx;
    }

    for (size_t i = 0; i < cpprun_args.size(); ++i) {
        const std::string & a = cpprun_args[i];

        if (a == "--cpprun-compiler-info") {
            args.show_compiler_info = true;
        } else if (a == "-c") {
            args.build_only = true;
        } else if (a == "-o") {
            if (i + 1 >= cpprun_args.size()) {
                throw std::runtime_error("-o requires an argument");
            }
            args.output_path = fs::path(cpprun_args[++i]);
        } else if (a.substr(0, 5) == "-std=") {
            args.cxx_standard = a;
        } else {
            args.build_args.push_back(a);
        }
    }

    return args;
}

auto collect_build_args(const CpprunArgs & args, const fs::path & output_file) {
    std::vector<std::string> cmd;
    if (args.cxx_standard.has_value()) {
        append(cmd, args.cxx_standard.value());
    }
    extend(cmd, args.build_args);
    if (args.build_only) {
        append(cmd, std::string("-c"));
    }
    append(cmd, std::string("-o"));
    append(cmd, output_file.string());
    return cmd;
}

uint32_t random_value() {
    static_assert(sizeof(uint32_t) <= sizeof(std::mt19937::result_type), "rng result type is too small");
    std::mt19937 rng(std::random_device{}());
    return rng() & 0xFFFFFFFF;
}

fs::path make_random_temp_path() {
    auto tmpdir = fs::temp_directory_path();
    auto subdir = "cpprun-" + std::to_string(random_value()) + "-" + std::to_string(getpid());
    return tmpdir / subdir;
}

bool contains(const std::vector<std::string> & haystack, const std::string & needle) {
    return std::find(haystack.begin(), haystack.end(), needle) != haystack.end();
}

template <typename T, typename F>
auto unwrap_or_else(const std::optional<T> & opt, F && fallback) {
    if (opt.has_value()) {
        return opt.value();
    }
    return fallback();
}

int inner_main(int argc, const char ** argv_raw) {
    std::vector<std::string> argv(argv_raw + 1, argv_raw + argc);
    auto [cpprun_args, run_args] = split_args(argv);

    CpprunArgs args = parse_cpprun_args(cpprun_args);

    if (args.show_compiler_info or contains(cpprun_args, "--version") or contains(cpprun_args, "-v")) {
        run_cmd(args.cxx, {"--version"}, true);
        return 0;
    }

    auto make_path = [&args]() -> fs::path {
        return make_random_temp_path() / (args.build_only ? "artifact.o" : "artifact.exe");
    };

    fs::path output_path = fs::absolute(unwrap_or_else(args.output_path, make_path));

    fs::create_directories(output_path.parent_path());

    auto cleanup = [&]() {
        try {
            if (!args.output_path) {
                if (args.verbose) {
                    std::cerr << ">>> Cleaning up temporary directory: " << output_path.parent_path() << std::endl;
                }
                // only cleanup if we created the output file in a temporary directory
                fs::remove_all(output_path.parent_path());
            }
        } catch (...) {
        }
    };

    auto build_args = collect_build_args(args, output_path);

    int rc = run_cmd(args.cxx, build_args, args.verbose);

    if (rc != 0 || args.build_only) {
        cleanup();
        return rc;
    }

    if (not fs::exists(output_path)) {
        std::cerr << "ERROR: expected output file at " << output_path << " was not created, unable to continue!"
                  << std::endl;
        cleanup();
        return 127;
    }

    rc = run_cmd(output_path.string(), run_args, args.verbose);

    cleanup();

    return rc;
}
}  // namespace cpprun

int main(int argc, const char ** argv_raw) {
    return cpprun::inner_main(argc, argv_raw);
}
