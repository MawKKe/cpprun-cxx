#include <gtest/gtest.h>

#include "cpprun.cpp"

TEST(CppRun, RandomValue) {
    uint32_t v1 = cpprun::random_value();
    uint32_t v2 = cpprun::random_value();
    EXPECT_NE(v1, v2);  // very unlikely to be equal

    EXPECT_TRUE(v1 <= std::numeric_limits<uint32_t>::max());
    EXPECT_TRUE(v2 <= std::numeric_limits<uint32_t>::max());
}

TEST(CppRun, UnwrapOrElse) {
    EXPECT_EQ(cpprun::unwrap_or_else(std::optional<int>(0), []() { return -1; }), 0);
    EXPECT_EQ(cpprun::unwrap_or_else(std::optional<int>(42), []() { return -1; }), 42);
    EXPECT_EQ(cpprun::unwrap_or_else(std::optional<int>(std::nullopt), []() { return -1; }), -1);
}

TEST(CppRun, Contains) {
    std::vector<std::string> vec = {"foo", "bar", "baz"};
    EXPECT_TRUE(cpprun::contains(vec, "bar"));
    EXPECT_FALSE(cpprun::contains(vec, "qux"));

    std::vector<std::string> empty_vec;
    EXPECT_FALSE(cpprun::contains(empty_vec, "foo"));
}

TEST(CppRun, Append) {
    using V = std::vector<int>;

    V vec;
    EXPECT_EQ(vec, V{});

    cpprun::append(vec, 42);
    EXPECT_EQ(vec, V({42}));

    cpprun::append(vec, 7);
    EXPECT_EQ(vec, V({42, 7}));
}

TEST(CppRun, Extend) {
    std::vector<int> vec = {1, 2, 3};
    std::vector<int> to_add = {4, 5};
    cpprun::extend(vec, to_add);
    EXPECT_EQ(vec, std::vector<int>({1, 2, 3, 4, 5}));
    EXPECT_EQ(to_add, std::vector<int>({4, 5}));  // src should not be modified
}

TEST(CppRun, JoinShell) {
    EXPECT_EQ(cpprun::join_shell({"echo", "hello world"}), "echo \"hello world\"");
    EXPECT_EQ(cpprun::join_shell({"ls", "-l", "/path/with spaces"}), "ls -l \"/path/with spaces\"");
    EXPECT_EQ(cpprun::join_shell({"simple", "args"}), "simple args");
    EXPECT_EQ(cpprun::join_shell({}), "");
}

TEST(CppRun, SplitArgs) {
    struct SplitArgsCase {
        std::vector<std::string> input;
        std::vector<std::string> expected_cpprun;
        std::vector<std::string> expected_run;
    };
    std::vector<SplitArgsCase> cases{
        SplitArgsCase{{}, {}, {}},
        SplitArgsCase{{"--"}, {}, {}},
        SplitArgsCase{{"arg1", "arg2", "--", "run1", "run2"}, {"arg1", "arg2"}, {"run1", "run2"}},
        SplitArgsCase{{"arg1", "arg2"}, {"arg1", "arg2"}, {}},
        SplitArgsCase{{"--", "run1", "run2"}, {}, {"run1", "run2"}},
    };
    for (const auto & c : cases) {
        auto [cpprun_args, run_args] = cpprun::split_args(c.input);
        EXPECT_EQ(cpprun_args, c.expected_cpprun);
        EXPECT_EQ(run_args, c.expected_run);
    }
}

bool starts_with(const std::string & str, const std::string & prefix) {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

TEST(CppRun, MakeRandomTempPath) {
    auto path1 = cpprun::make_random_temp_path();
    auto path2 = cpprun::make_random_temp_path();
    EXPECT_NE(path1, path2);  // very unlikely to be equal
    EXPECT_TRUE(starts_with(path1.filename().string(), "cpprun-"));
    EXPECT_TRUE(starts_with(path2.filename().string(), "cpprun-"));
}

TEST(CppRun, ParseCpprunArgs) {
    EXPECT_THROW(cpprun::parse_cpprun_args({"-foo", "-std=c++17", "-o", "output", "--", "runarg"}), std::runtime_error);

    auto args = cpprun::parse_cpprun_args({"-foo", "-std=c++17", "-o", "output"});
    EXPECT_EQ(args.build_args, std::vector<std::string>({"-Wall", "-Wextra", "-pedantic", "-g", "-foo"}));
    EXPECT_EQ(args.cxx_standard, std::optional<std::string>("-std=c++17"));
    EXPECT_EQ(args.output_path, std::optional<std::string>("output"));
    EXPECT_FALSE(args.build_only);
    EXPECT_FALSE(args.show_compiler_info);
    EXPECT_FALSE(args.verbose);
}

TEST(CppRun, RunCmd) {
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    auto exit_code = cpprun::run_cmd("echo", {"hello world 1 2 3"}, false);

    auto output = testing::internal::GetCapturedStdout();
    auto error = testing::internal::GetCapturedStderr();

    EXPECT_EQ(exit_code, 0);
    EXPECT_EQ(output, "hello world 1 2 3\n");
    EXPECT_EQ(error, "");
}