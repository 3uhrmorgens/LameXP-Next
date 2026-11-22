cmake_minimum_required(VERSION 3.24)
project(LameXPNext VERSION 0.8.0 LANGUAGES CXX)
option(LAMEXP_CORE_ONLY "Build portable C++ core/tests without Qt GUI" OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(LameXPCore STATIC src/core/LameAdapter.cpp src/core/LameAdapter.h
  src/core/PipelinePlan.cpp src/core/PipelinePlan.h
  src/core/OutputNaming.cpp src/core/OutputNaming.h
  src/core/ResourcePolicy.cpp src/core/ResourcePolicy.h)
target_include_directories(LameXPCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(NOT LAMEXP_CORE_ONLY)
  find_package(Qt6 6.8 REQUIRED COMPONENTS Core Widgets)
  qt_standard_project_setup()
  qt_add_executable(LameXPNext WIN32
  src/main.cpp
  src/MainWindow.cpp src/MainWindow.h
  src/JobManager.cpp src/JobManager.h
  src/MetadataScanner.cpp src/MetadataScanner.h
)
  target_link_libraries(LameXPNext PRIVATE Qt6::Core Qt6::Widgets LameXPCore)
  if(MSVC)
    target_compile_options(LameXPNext PRIVATE /W4 /permissive- /utf-8)
  else()
    target_compile_options(LameXPNext PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  install(TARGETS LameXPNext RUNTIME DESTINATION bin)
endif()

include(CTest)
if(BUILD_TESTING)
  add_executable(ResourcePolicyTests tests/ResourcePolicyTests.cpp)
  target_link_libraries(ResourcePolicyTests PRIVATE LameXPCore)
  add_test(NAME ResourcePolicyTests COMMAND ResourcePolicyTests)
  add_executable(OutputNamingTests tests/OutputNamingTests.cpp)
  target_link_libraries(OutputNamingTests PRIVATE LameXPCore)
  add_test(NAME OutputNamingTests COMMAND OutputNamingTests)
  add_executable(LameAdapterTests tests/LameAdapterTests.cpp)
  add_executable(PipelinePlanTests tests/PipelinePlanTests.cpp)
  target_link_libraries(PipelinePlanTests PRIVATE LameXPCore)
  add_test(NAME PipelinePlanTests COMMAND PipelinePlanTests)
  target_link_libraries(LameAdapterTests PRIVATE LameXPCore)
  add_test(NAME LameAdapterTests COMMAND LameAdapterTests)
endif()
