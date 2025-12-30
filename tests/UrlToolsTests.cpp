\
/* tests/UrlToolsTests.cpp */
#include <catch2/catch_all.hpp>
#include "core/net/UrlTools.h"

TEST_CASE("stripTracking removes utm params") {
  QUrl url("https://example.com/path?utm_source=a&x=1&gclid=2");
  QUrl out = core::net::stripTracking(url);
  REQUIRE(out.toString().toStdString().find("utm_source") == std::string::npos);
  REQUIRE(out.toString().toStdString().find("gclid") == std::string::npos);
  REQUIRE(out.toString().toStdString().find("x=1") != std::string::npos);
}
