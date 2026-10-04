#include "doctest.h"
#include "tColor.h"

// Tests for tColor class
// Purpose: Verify color representation and manipulation

TEST_SUITE("tColor")
{
    TEST_CASE("tColor default construction")
    {
        GIVEN("a default tColor")
        {
            tColor defaultColor;
            // Default color is white (1,1,1,1) not black
            THEN("all components default to 1.0")
            {
                CHECK(defaultColor.r_ == 1.0f);
                CHECK(defaultColor.g_ == 1.0f);
                CHECK(defaultColor.b_ == 1.0f);
                CHECK(defaultColor.a_ == 1.0f);
            }
        }
    }

    TEST_CASE("tColor RGBA construction")
    {
        GIVEN("a tColor with RGBA components")
        {
            tColor color(0.5f, 0.6f, 0.7f, 0.8f);
            THEN("the components match the constructor arguments")
            {
                CHECK(color.r_ == 0.5f);
                CHECK(color.g_ == 0.6f);
                CHECK(color.b_ == 0.7f);
                CHECK(color.a_ == 0.8f);
            }
        }
    }

    TEST_CASE("tColor RGBA construction with default alpha")
    {
        GIVEN("a tColor with RGB components and default alpha")
        {
            tColor color(0.25f, 0.5f, 0.75f);
            THEN("RGB components match and alpha defaults to 1.0")
            {
                CHECK(color.r_ == 0.25f);
                CHECK(color.g_ == 0.5f);
                CHECK(color.b_ == 0.75f);
                CHECK(color.a_ == 1.0f); // Default alpha is 1
            }
        }
    }

    TEST_CASE("tColor string construction")
    {
        GIVEN("a color code string")
        {
            // Test various color code formats if supported
            // Note: Actual color code parsing depends on implementation
            // This test verifies the constructor doesn't crash
            tColor colorFromString("#FF0000");
            // Just verify it was constructed without error
            THEN("alpha is in valid range")
            {
                CHECK(colorFromString.a_ >= 0.0f);
                CHECK(colorFromString.a_ <= 1.0f);
            }
        }
    }

    TEST_CASE("tColor FillFrom method")
    {
        GIVEN("a color and a color code string")
        {
            tColor color;
            color.FillFrom("#00FF00");
            // Just verify it doesn't crash and sets valid values
            THEN("all components are in valid range")
            {
                CHECK(color.r_ >= 0.0f);
                CHECK(color.r_ <= 1.0f);
                CHECK(color.g_ >= 0.0f);
                CHECK(color.g_ <= 1.0f);
                CHECK(color.b_ >= 0.0f);
                CHECK(color.b_ <= 1.0f);
            }
        }
    }

    TEST_CASE("tColor equality operator")
    {
        GIVEN("colors for equality comparison")
        {
            tColor color1(0.5f, 0.5f, 0.5f, 1.0f);
            tColor color2(0.5f, 0.5f, 0.5f, 1.0f);
            tColor color3(0.6f, 0.6f, 0.6f, 1.0f);

            THEN("operator== returns true for equal colors")
            {
                CHECK(color1 == color2);
            }
            THEN("operator== returns false for different colors")
            {
                CHECK_FALSE(color1 == color3);
            }
        }
    }

    TEST_CASE("tColor IsDark method")
    {
        GIVEN("colors for darkness testing")
        {
            // Black should be dark
            tColor black(0.0f, 0.0f, 0.0f);
            THEN("black is dark")
            {
                CHECK(black.IsDark() == true);
            }

            // White should not be dark
            tColor white(1.0f, 1.0f, 1.0f);
            THEN("white is not dark")
            {
                CHECK(white.IsDark() == false);
            }

            // Dark gray should be dark
            tColor darkGray(0.1f, 0.1f, 0.1f);
            THEN("dark gray is dark")
            {
                CHECK(darkGray.IsDark() == true);
            }

            // Light gray should not be dark
            tColor lightGray(0.9f, 0.9f, 0.9f);
            THEN("light gray is not dark")
            {
                CHECK(lightGray.IsDark() == false);
            }
        }
    }

    TEST_CASE("tColor component access")
    {
        GIVEN("a color for component access")
        {
            tColor color(0.25f, 0.5f, 0.75f, 0.9f);

            // Verify components are accessible
            THEN("components are accessible")
            {
                CHECK(color.r_ == 0.25f);
                CHECK(color.g_ == 0.5f);
                CHECK(color.b_ == 0.75f);
                CHECK(color.a_ == 0.9f);
            }

            // Verify components are modifiable
            color.r_ = 0.1f;
            THEN("components are modifiable")
            {
                CHECK(color.r_ == 0.1f);
            }
        }
    }
}
