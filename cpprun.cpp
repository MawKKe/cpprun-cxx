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

static const std::vector<std::string> DEFAULT_CXXFLAGS = {
    "-Wall",
    "-Wextra",
    "-pedantic",
    "-g",
};

static const std::string DEFAULT_CXX_STANDARD = "-std=c++23";

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

auto split_args(const std::vector<std::string> & args) {
    auto it = std::find(args.begin(), args.end(), "--");
    if (it == args.end()) {
        return std::make_pair(args, std::vector<std::string>{});
    }
    auto idx = std::distance(args.begin(), it);
    return std::make_pair(std::vector<std::string>(args.begin(), args.begin() + idx),
                          std::vector<std::string>(it + 1, args.end()));
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

int main(int argc, const char ** argv_raw) {
    std::vector<std::string> argv(argv_raw + 1, argv_raw + argc);
    auto [cpprun_args, run_args] = split_args(argv);

    CpprunArgs args = parse_cpprun_args(cpprun_args);

    if (args.show_compiler_info) {
        run_cmd(args.cxx, {"--version"}, true);
        return 0;
    }

    auto tmpdir = fs::temp_directory_path();

    std::string suffix = args.build_only ? ".o" : ".exe";

    auto subdir = "cpprun-" + std::to_string(random_value()) + "-" + std::to_string(getpid());

    fs::path output_path = fs::absolute(args.output_path.value_or(tmpdir / subdir / ("artifact" + suffix)));

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

    rc = run_cmd(output_path.string(), run_args, args.verbose);

    cleanup();

    return rc;
}
