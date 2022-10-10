enable_testing()

file(GLOB ALL_TEST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(tests main_test.cpp ${ALL_TEST_FILES})
target_link_libraries(tests PUBLIC Boost::context)
target_include_directories(tests PUBLIC ../include)

add_test(NAME unit_tests
         COMMAND $<TARGET_FILE:tests>)