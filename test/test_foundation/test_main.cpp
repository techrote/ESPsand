#include <unity.h>

#include <espsand/core/foundation.hpp>

namespace {

void test_project_identity() {
  TEST_ASSERT_EQUAL_STRING("ESPsand", espsand::core::project_name());
  TEST_ASSERT_EQUAL_UINT32(1U, espsand::core::kFoundationSchemaVersion);
}

} // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_project_identity);
  return UNITY_END();
}
