cmake_minimum_required(VERSION 3.24)
project(NovaBrowse VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC: faster builds and warnings
if (MSVC)
  add_compile_options(/W4 /permissive- /Zc:__cplusplus)
  add_compile_options(/utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# vcpkg deps
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-gumbo CONFIG REQUIRED)
find_package(SQLite3 CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)

# Qt
find_package(Qt6 6.6 REQUIRED COMPONENTS Widgets WebEngineWidgets WebEngineCore Network Gui)

# App target
add_executable(NovaBrowse
  src/main.cpp
  src/app/NovaApp.cpp
  src/app/NovaApp.h
  src/ui/MainWindow.cpp
  src/ui/MainWindow.h
  src/ui/TabWidget.cpp
  src/ui/TabWidget.h
  src/ui/BrowserTab.cpp
  src/ui/BrowserTab.h
  src/ui/SidePanel.cpp
  src/ui/SidePanel.h
  src/ui/ScrapeDialog.cpp
  src/ui/ScrapeDialog.h
  src/ui/EntityGraphWidget.cpp
  src/ui/EntityGraphWidget.h
  src/ui/InternalSchemeHandler.cpp
  src/ui/InternalSchemeHandler.h
  src/core/storage/SqliteDb.cpp
  src/core/storage/SqliteDb.h
  src/core/storage/Migrations.cpp
  src/core/storage/Migrations.h
  src/core/net/RateLimiter.cpp
  src/core/net/RateLimiter.h
  src/core/net/UrlTools.cpp
  src/core/net/UrlTools.h
  src/core/net/FetchService.cpp
  src/core/net/FetchService.h
  src/core/extract/HtmlExtractor.cpp
  src/core/extract/HtmlExtractor.h
  src/core/entities/EntityDetector.cpp
  src/core/entities/EntityDetector.h
  src/services/search/DdgHtmlSearch.cpp
  src/services/search/DdgHtmlSearch.h
  src/services/ai/OllamaClient.cpp
  src/services/ai/OllamaClient.h
  src/services/ai/RagComposer.cpp
  src/services/ai/RagComposer.h
  src/services/deepsearch/DeepSearchService.cpp
  src/services/deepsearch/DeepSearchService.h
  src/services/scraper/ScraperService.cpp
  src/services/scraper/ScraperService.h
  src/util/Config.cpp
  src/util/Config.h
  src/util/Log.cpp
  src/util/Log.h
  src/util/Time.cpp
  src/util/Time.h
)

target_include_directories(NovaBrowse PRIVATE src)

target_link_libraries(NovaBrowse PRIVATE
  Qt6::Widgets
  Qt6::WebEngineWidgets
  Qt6::WebEngineCore
  Qt6::Network
  Qt6::Gui
  nlohmann_json::nlohmann_json
  unofficial::gumbo::gumbo
  SQLite::SQLite3
  tinyxml2::tinyxml2
)

# Windows subsystem for GUI app
if (WIN32)
  set_target_properties(NovaBrowse PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Copy default config on build
add_custom_command(TARGET NovaBrowse POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:NovaBrowse>/data
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/config/config.json
          $<TARGET_FILE_DIR:NovaBrowse>/data/config.json
)



option(BUILD_TESTS "Build NovaBrowse unit tests" ON)
if (BUILD_TESTS)
  enable_testing()
  find_package(Catch2 CONFIG REQUIRED)
  add_subdirectory(tests)
  include(CTest)
endif()
