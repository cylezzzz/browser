add_executable(NovaBrowseTests
  UrlToolsTests.cpp
  EntityDetectorTests.cpp
)

target_include_directories(NovaBrowseTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(NovaBrowseTests PRIVATE Catch2::Catch2WithMain Qt6::Core)
