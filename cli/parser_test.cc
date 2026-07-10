#include "cli/parser.h"

#include <gtest/gtest.h>

#include <expected>
#include <initializer_list>
#include <string>
#include <vector>

namespace {

class Argv {
 public:
  Argv(std::initializer_list<std::string> arguments) : storage_(arguments) {
    values_.reserve(storage_.size());
    for (std::string& argument : storage_) {
      values_.push_back(argument.data());
    }
  }

  std::expected<cli::Arguments, std::string> Parse() {
    return cli::Parse(static_cast<int>(values_.size()), values_.data());
  }

 private:
  std::vector<std::string> storage_;
  std::vector<char*> values_;
};

TEST(ParserTest, ParsesProgramAndPositionalArguments) {
  Argv argv{"rados-drip", "config.conf", "pool", "objects.txt"};

  const auto result = argv.Parse();

  ASSERT_TRUE(result) << result.error();
  EXPECT_EQ(result->program, "rados-drip");
  EXPECT_EQ(result->positional,
            (std::vector<std::string>{"config.conf", "pool", "objects.txt"}));
  EXPECT_TRUE(result->flags.empty());
}

TEST(ParserTest, ParsesSeparateAndInlineFlagValues) {
  Argv argv{"rados-drip", "--name", "client.test", "--output=result=file",
            "--empty="};

  const auto result = argv.Parse();

  ASSERT_TRUE(result) << result.error();
  EXPECT_EQ(result->flags, (std::vector<cli::Flag>{{"--name", "client.test"},
                                                   {"--output", "result=file"},
                                                   {"--empty", ""}}));
}

TEST(ParserTest, PreservesRepeatedFlagsInOrder) {
  Argv argv{"rados-drip", "--name=first", "--name", "second"};

  const auto result = argv.Parse();

  ASSERT_TRUE(result) << result.error();
  EXPECT_EQ(result->flags, (std::vector<cli::Flag>{{"--name", "first"},
                                                   {"--name", "second"}}));
}

TEST(ParserTest, ConsumesFlagLikeValueAfterSeparateFlag) {
  Argv argv{"rados-drip", "--cursor", "--not-a-flag"};

  const auto result = argv.Parse();

  ASSERT_TRUE(result) << result.error();
  EXPECT_EQ(result->flags,
            (std::vector<cli::Flag>{{"--cursor", "--not-a-flag"}}));
}

TEST(ParserTest, ReturnsErrorWhenFlagValueIsMissing) {
  Argv argv{"rados-drip", "--name"};

  const auto result = argv.Parse();

  ASSERT_FALSE(result);
  EXPECT_EQ(result.error(), "--name requires a value");
}

TEST(ParserTest, ReturnsErrorWhenProgramNameIsMissing) {
  const auto result = cli::Parse(0, nullptr);

  ASSERT_FALSE(result);
  EXPECT_EQ(result.error(), "program name is required");
}

}  // namespace
