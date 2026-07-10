add_library(cli-parser
    parser.cc
)
target_include_directories(cli-parser PUBLIC
    ${PROJECT_SOURCE_DIR}
)

if(BUILD_TESTING)
  find_package(GTest REQUIRED)

  add_executable(cli-parser-test
      parser_test.cc
  )
  target_link_libraries(cli-parser-test PRIVATE
      cli-parser
      GTest::gtest_main
  )

  include(GoogleTest)
  gtest_discover_tests(cli-parser-test)
endif()
