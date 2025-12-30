\
/* tests/EntityDetectorTests.cpp */
#include <catch2/catch_all.hpp>
#include "core/entities/EntityDetector.h"

TEST_CASE("EntityDetector finds capitalized sequences") {
  core::entities::EntityDetector d;
  auto ents = d.detect("Alice Wonderland met Bob Builder at Berlin City Hall. Example GmbH announced news.");
  REQUIRE(!ents.empty());
}
