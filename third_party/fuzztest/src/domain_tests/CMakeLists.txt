get_filename_component(PARENT_DIR ../ ABSOLUTE)
include_directories(${PARENT_DIR})

include_directories(${CMAKE_BINARY_DIR})
add_library(
  fuzztest_domain_testing
  OBJECT
  domain_testing.h
)
set_target_properties(
  fuzztest_domain_testing
  PROPERTIES
  LINKER_LANGUAGE CXX
)
target_link_libraries(
  fuzztest_domain_testing
  PUBLIC
  absl::hash
  absl::status
  absl::strings
  absl::flat_hash_set
  absl::random_random
  fuzztest_meta
  fuzztest_serialization
  test_protobuf
  fuzztest_logging
  fuzztest_type_support
  protobuf::libprotobuf
  GTest::gmock
)

add_executable(
  fuzztest_aggregate_combinators_test
  aggregate_combinators_test.cc
)
target_link_libraries(
  fuzztest_aggregate_combinators_test
  PUBLIC
  fuzztest_domain_testing
  fuzztest_coverage
  fuzztest_type_support
  fuzztest_serialization
  fuzztest_status
  fuzztest_logging
  absl::flat_hash_set
  absl::optional
  absl::random_random
  absl::span
  absl::variant
  GTest::gmock_main
)
add_test(
  NAME
  fuzztest_aggregate_combinators_test
  COMMAND
  fuzztest_aggregate_combinators_test
)

add_executable(
  fuzztest_arbitrary_domains_test
  arbitrary_domains_test.cc
)
target_link_libraries(
  fuzztest_arbitrary_domains_test
  PUBLIC
  fuzztest_domain_testing
  fuzztest_coverage
  fuzztest_type_support
  fuzztest_serialization
  fuzztest_status
  fuzztest_logging
  protobuf::libprotobuf
  test_protobuf
  absl::flat_hash_map
  absl::flat_hash_set
  absl::random_random
  absl::random_bit_gen_ref
  absl::time
  absl::span
  absl::status
  GTest::gmock_main
)
add_test(
  NAME
  fuzztest_arbitrary_domains_test
  COMMAND
  fuzztest_arbitrary_domains_test
)

add_executable(
  fuzztest_container_combinators_test
  container_combinators_test.cc
)
target_link_libraries(
  fuzztest_container_combinators_test
  PUBLIC
  fuzztest_domain_testing
  fuzztest_coverage
  fuzztest_type_support
  fuzztest_serialization
  fuzztest_status
  fuzztest_logging
  absl::flat_hash_map
  absl::flat_hash_set
  absl::random_random
  absl::span
  absl::strings
  GTest::gmock_main
)
add_test(
  NAME
  fuzztest_container_combinators_test
  COMMAND
  fuzztest_container_combinators_test
)

add_executable(
  fuzztest_in_regexp_domain_test
  in_regexp_domain_test.cc
)
target_link_libraries(
  fuzztest_in_regexp_domain_test
  PUBLIC
  fuzztest_domain_testing
  fuzztest_coverage
  fuzztest_type_support
  fuzztest_serialization
  fuzztest_status
  fuzztest_logging
  absl::flat_hash_set
  absl::random_random
  absl::span
  re2
  fuzztest_regexp_dfa
  GTest::gmock_main
)
add_test(
  NAME
  fuzztest_in_regexp_domain_test
  COMMAND
  fuzztest_in_regexp_domain_test
)

add_executable(
  fuzztest_in_grammar_domain_test
  in_grammar_domain_test.cc
)
target_link_libraries(
  fuzztest_in_grammar_domain_test
  PUBLIC
  fuzztest_domain_testing
  fuzztest_domain
  fuzztest_coverage
  fuzztest_type_support
  fuzztest_serialization
  fuzztest_status
  fuzztest_logging
  absl::random_random
  absl::flat_hash_set
  nlohmann_json
  json_grammar
  re2
  fuzztest_regexp_dfa
  GTest::gmock_main
)
add_test(
  NAME
  fuzztest_in_grammar_domain_test
  COMMAND
  fuzztest_in_grammar_domain_test
)

add_executable(
  fuzztest_map_filter_combinator_test
  map_filter_combinator_test.cc
)
target_link_libraries(
  fuzztest_map_filter_combinator_test
  PUBLIC
  fuzztest_coverage
  fuzztest_domain
  fuzztest_domain_testing
  fuzztest_logging
  fuzztest_serialization
  fuzztest_status
  fuzztest_type_support
  absl::algorithm_container
  absl::random_random
  GTest::gmock_main
)
add_test(
  NAME
  fuzztest_map_filter_combinator_test
  COMMAND
  fuzztest_map_filter_combinator_test
)

add_executable(
  fuzztest_misc_domains_test
  misc_domains_test.cc
)
target_link_libraries(
  fuzztest_misc_domains_test
  PUBLIC
  fuzztest_domain_testing
  fuzztest_coverage
  fuzztest_type_support
  fuzztest_meta
  fuzztest_serialization
  fuzztest_status
  fuzztest_logging
  absl::flat_hash_set
  absl::int128
  absl::random_random
  absl::span
  GTest::gmock_main
)
add_test(
  NAME
  fuzztest_misc_domains_test
  COMMAND
  fuzztest_misc_domains_test
)

add_executable(
  fuzztest_numeric_domains_test
  numeric_domains_test.cc
)
target_link_libraries(
  fuzztest_numeric_domains_test
  PUBLIC
  fuzztest_domain_testing
  fuzztest_coverage
  fuzztest_type_support
  fuzztest_serialization
  fuzztest_status
  fuzztest_logging
  absl::flat_hash_set
  absl::int128
  absl::random_random
  absl::span
  absl::status
  absl::strings
  GTest::gmock_main
)
add_test(
  NAME
  fuzztest_numeric_domains_test
  COMMAND
  fuzztest_numeric_domains_test
)

add_executable(
  fuzztest_pointer_domains_test
  pointer_domains_test.cc
)
target_link_libraries(
  fuzztest_pointer_domains_test
  PUBLIC
  fuzztest_domain_testing
  fuzztest_coverage
  fuzztest_type_support
  fuzztest_serialization
  fuzztest_status
  fuzztest_logging
  absl::random_random
  GTest::gmock_main
)
add_test(
  NAME
  fuzztest_pointer_domains_test
  COMMAND
  fuzztest_pointer_domains_test
)

add_executable(
  fuzztest_recursive_domains_test
  recursive_domains_test.cc
)
target_link_libraries(
  fuzztest_recursive_domains_test
  PUBLIC
  fuzztest_domain_testing
  fuzztest_coverage
  fuzztest_type_support
  fuzztest_serialization
  fuzztest_status
  fuzztest_logging
  absl::random_random
  GTest::gmock_main
)
add_test(
  NAME
  fuzztest_recursive_domains_test
  COMMAND
  fuzztest_recursive_domains_test
)

add_executable(
  fuzztest_string_domains_test
  string_domains_test.cc
)
target_link_libraries(
  fuzztest_string_domains_test
  PUBLIC
  fuzztest_domain_testing
  fuzztest_coverage
  fuzztest_type_support
  fuzztest_serialization
  fuzztest_status
  fuzztest_logging
  absl::flat_hash_set
  absl::random_random
  GTest::gmock_main
)
add_test(
  NAME
  fuzztest_string_domains_test
  COMMAND
  fuzztest_string_domains_test
)

add_executable(
  fuzztest_specific_value_domains_test
  specific_value_domains_test.cc
)
target_link_libraries(
  fuzztest_specific_value_domains_test
  PUBLIC
  fuzztest_domain_testing
  fuzztest_coverage
  fuzztest_type_support
  fuzztest_serialization
  fuzztest_status
  fuzztest_logging
  absl::flat_hash_set
  absl::random_random
  absl::span
  GTest::gmock_main
)
add_test(
  NAME
  fuzztest_specific_value_domains_test
  COMMAND
  fuzztest_specific_value_domains_test
)
