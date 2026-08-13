cmake_minimum_required(VERSION 3.15)

project(embedded-devops-demo
    VERSION 1.0.0
    DESCRIPTION "T-Box Diagnostic Simulation"
    LANGUAGES C
)

# Use C11 standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable useful compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Define the executable and its sources
add_executable(tbox_diagnostic
    main.c
    vehicle.c
)

# Include the project root so headers are found without path prefixes
target_include_directories(tbox_diagnostic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ---------------------------------------------------------------
# Testing — CTest smoke test
# Verifies the binary executes successfully (exit code 0).
# Run with: ctest --test-dir build --output-on-failure
# ---------------------------------------------------------------
enable_testing()

add_test(
    NAME smoke_test
    COMMAND tbox_diagnostic
)

set_tests_properties(smoke_test PROPERTIES
    PASS_REGULAR_EXPRESSION "T-Box Diagnostic Report"
)

# Print a summary during configuration
message(STATUS "Project   : ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler  : ${CMAKE_C_COMPILER_ID}")
