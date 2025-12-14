cmake_minimum_required(VERSION 3.20)
project(blackbox-tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================
# 1. External Dependencies (GoogleTest)
# =========================================================
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Prevent GTest from overriding parent compiler settings (Windows/Visual Studio specific mostly)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# =========================================================
# 2. Paths to Real Source Code
# =========================================================
set(CORE_ROOT "${CMAKE_SOURCE_DIR}/../../blackbox-core")
include_directories(${CORE_ROOT}/include)

# Dependencies required by the Core (Boost, etc.)
find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
# We assume CURL/Hiredis are installed on the system via apt-get
find_package(CURL REQUIRED)

# =========================================================
# 3. Define the Test Sources
# =========================================================
# We compile the TEST files + the ACTUAL IMPLEMENTATION files they need.
# We do NOT link the 'flight-recorder' binary; we re-compile the objects.

set(TEST_SOURCES
    # --- Test Entry Point ---
    main.cpp

    # --- Unit Tests ---
    ingest/test_ring_buffer.cpp
    ingest/test_rate_limiter.cpp
    parser/test_string_utils.cpp

    # --- Actual Implementation Files (From Core) ---
    # We explicitly list only logic files (no main.cpp)
    ${CORE_ROOT}/src/ingest/ring_buffer.cpp
    ${CORE_ROOT}/src/ingest/rate_limiter.cpp
    ${CORE_ROOT}/src/common/string_utils.cpp
    ${CORE_ROOT}/src/common/logger.cpp
    ${CORE_ROOT}/src/common/time_utils.cpp
)

# =========================================================
# 4. Build Test Executable
# =========================================================
add_executable(run_core_tests ${TEST_SOURCES})

# Link Libraries
target_link_libraries(run_core_tests PRIVATE
    GTest::gtest_main
    Boost::system
    Threads::Threads
    ${CURL_LIBRARIES}
)

# Enable CTest
include(GoogleTest)
gtest_discover_tests(run_core_tests)