
find_package(GTest REQUIRED)

add_executable(urtsched_unittests test_sched.cpp)
target_include_directories(urtsched_unittests PUBLIC ${GTEST_INCLUDE_DIRS})
target_link_libraries(urtsched_unittests -lgtest -lgmock urtsched gtest_main)


add_test(NAME googletest
         COMMAND urtsched_unittests)