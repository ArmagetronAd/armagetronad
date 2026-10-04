#include "doctest.h"
#include "tMath.h"
#include <cmath>
#include <limits>

// Tests for tMath utilities
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("tMath")
{
    TEST_CASE("good function with finite values")
    {
        GIVEN("finite real numbers")
        {
            THEN("good returns true for regular numbers")
            {
                CHECK(good(0.0f) == true);
                CHECK(good(1.0f) == true);
                CHECK(good(-1.0f) == true);
                CHECK(good(3.14159f) == true);
                CHECK(good(-2.71828f) == true);
                CHECK(good(1000.0f) == true);
                CHECK(good(-1000.0f) == true);
            }
        }
    }

    TEST_CASE("good function with special floating point values")
    {
        GIVEN("special floating point values")
        {
            THEN("good returns false for non-finite values")
            {
                // Test with infinity
                float inf = std::numeric_limits<float>::infinity();
                CHECK(good(inf) == false);
                CHECK(good(-inf) == false);

                // Test with NaN
                float nan = std::numeric_limits<float>::quiet_NaN();
                CHECK(good(nan) == false);
            }
        }
    }

    TEST_CASE("good function with double precision")
    {
        GIVEN("double precision values")
        {
            THEN("good works with double precision")
            {
                CHECK(good(0.0) == true);
                CHECK(good(1.0) == true);

                double inf = std::numeric_limits<double>::infinity();
                CHECK(good(inf) == false);

                double nan = std::numeric_limits<double>::quiet_NaN();
                CHECK(good(nan) == false);
            }
        }
    }

    TEST_CASE("good function edge cases")
    {
        GIVEN("edge case values")
        {
            THEN("good handles edge cases correctly")
            {
                // Very small and very large finite values (REAL is float)
                CHECK(good(std::numeric_limits<float>::min()) == true);
                CHECK(good(std::numeric_limits<float>::max()) == true);

                // Zero and negative zero
                CHECK(good(0.0f) == true);
                CHECK(good(-0.0f) == true);
            }
        }
    }
}
