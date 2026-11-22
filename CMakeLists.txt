cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 REQUIRED COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

include(FetchContent)
FetchContent_Declare(
  libebur128
  URL https://github.com/jiixyj/libebur128/archive/refs/tags/v1.2.6.tar.gz
  URL_HASH SHA256=baa7fc293a3d4651e244d8022ad03ab797ca3c2ad8442c43199afe8059faa613
  SOURCE_SUBDIR do-not-add
)
FetchContent_MakeAvailable(libebur128)

add_library(ebur128 STATIC ${libebur128_SOURCE_DIR}/ebur128/ebur128.c)
target_include_directories(ebur128 PUBLIC ${libebur128_SOURCE_DIR}/ebur128)
target_include_directories(ebur128 SYSTEM PRIVATE $<$<PLATFORM_ID:Windows>:${libebur128_SOURCE_DIR}/ebur128/queue>)
target_compile_definitions(ebur128 PRIVATE $<$<C_COMPILER_ID:MSVC>:_USE_MATH_DEFINES>)
target_compile_options(ebur128 PRIVATE $<IF:$<C_COMPILER_ID:MSVC>,/w,-w>)
target_link_libraries(ebur128 PRIVATE $<$<PLATFORM_ID:Linux>:m>)
set_target_properties(ebur128 PROPERTIES POSITION_INDEPENDENT_CODE ON COMPILE_WARNING_AS_ERROR OFF)

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE src/plugin-main.c src/loudness.c src/meter.c src/presets.c src/ui.cpp src/ui.hpp src/history.hpp
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ebur128)

add_executable(airmeter-tests tests/airmeter-tests.cpp src/loudness.c src/presets.c)
target_include_directories(airmeter-tests PRIVATE src)
target_compile_definitions(airmeter-tests PRIVATE $<$<CXX_COMPILER_ID:MSVC>:_USE_MATH_DEFINES>)
target_compile_features(airmeter-tests PRIVATE cxx_std_17)
target_link_libraries(airmeter-tests PRIVATE ebur128)
enable_testing()
add_test(NAME measurement COMMAND airmeter-tests)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
