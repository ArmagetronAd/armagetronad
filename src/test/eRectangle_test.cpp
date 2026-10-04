#include "doctest.h"
#include "eRectangle.h"

// Tests for eRectangle class
// Purpose: Verify 2D rectangle operations

TEST_SUITE("eRectangle")
{
    TEST_CASE("eRectangle default construction")
    {
        GIVEN("a default eRectangle")
        {
            eRectangle rect;

            // Default rectangle is invalid (low > high)
            eCoord low = rect.GetLow();
            eCoord high = rect.GetHigh();

            // Check that coordinates are accessible and represent an empty/invalid rectangle
            THEN("GetLow returns the maximum coordinate")
            {
                CHECK(low.x == doctest::Approx(1E+30f));
                CHECK(low.y == doctest::Approx(1E+30f));
            }
            THEN("GetHigh returns the minimum coordinate")
            {
                CHECK(high.x == doctest::Approx(-1E+30f));
                CHECK(high.y == doctest::Approx(-1E+30f));
            }
        }
    }

    TEST_CASE("eRectangle construction with corners")
    {
        GIVEN("a rectangle with specified corners")
        {
            eCoord low(0.0f, 0.0f);
            eCoord high(10.0f, 20.0f);
            eRectangle rect(low, high);

            eCoord rectLow = rect.GetLow();
            eCoord rectHigh = rect.GetHigh();

            THEN("GetLow returns the low corner")
            {
                CHECK(rectLow.x == doctest::Approx(0.0f));
                CHECK(rectLow.y == doctest::Approx(0.0f));
            }
            THEN("GetHigh returns the high corner")
            {
                CHECK(rectHigh.x == doctest::Approx(10.0f));
                CHECK(rectHigh.y == doctest::Approx(20.0f));
            }
        }
    }

    TEST_CASE("eRectangle Clear method")
    {
        GIVEN("a rectangle to clear")
        {
            eCoord low(5.0f, 5.0f);
            eCoord high(15.0f, 15.0f);
            eRectangle rect(low, high);

            rect.Clear();

            eCoord clearedLow = rect.GetLow();
            eCoord clearedHigh = rect.GetHigh();

            // After clear, rectangle is invalid (low > high)
            // Clear sets to: low_(1E+30, 1E+30), high_(-1E+30, -1E+30)
            THEN("Clear sets GetLow to maximum coordinate")
            {
                CHECK(clearedLow.x == doctest::Approx(1E+30f));
                CHECK(clearedLow.y == doctest::Approx(1E+30f));
            }
            THEN("Clear sets GetHigh to minimum coordinate")
            {
                CHECK(clearedHigh.x == doctest::Approx(-1E+30f));
                CHECK(clearedHigh.y == doctest::Approx(-1E+30f));
            }
        }
    }

    TEST_CASE("eRectangle Include method")
    {
        GIVEN("a rectangle and a point to include")
        {
            eCoord low(0.0f, 0.0f);
            eCoord high(10.0f, 10.0f);
            eRectangle rect(low, high);

            // Include a point outside the current rectangle
            eCoord newPoint(15.0f, 15.0f);
            rect.Include(newPoint);

            eCoord rectLow = rect.GetLow();
            eCoord rectHigh = rect.GetHigh();

            // Rectangle should now include the new point
            THEN("the low corner remains at or below the original")
            {
                CHECK(rectLow.x <= 0.0f);
                CHECK(rectLow.y <= 0.0f);
            }
            THEN("the high corner expands to include the new point")
            {
                CHECK(rectHigh.x >= 15.0f);
                CHECK(rectHigh.y >= 15.0f);
            }
        }
    }

    TEST_CASE("eRectangle Contains method")
    {
        GIVEN("a rectangle and points to test")
        {
            eCoord low(0.0f, 0.0f);
            eCoord high(10.0f, 10.0f);
            eRectangle rect(low, high);

            // Point inside
            eCoord insidePoint(5.0f, 5.0f);
            THEN("Contains returns true for points inside")
            {
                CHECK(rect.Contains(insidePoint) == true);
            }

            // Point on edge
            eCoord edgePoint(10.0f, 5.0f);
            THEN("Contains returns true for points on the edge")
            {
                CHECK(rect.Contains(edgePoint) == true);
            }

            // Point outside
            eCoord outsidePoint(15.0f, 15.0f);
            THEN("Contains returns false for points outside")
            {
                CHECK(rect.Contains(outsidePoint) == false);
            }
        }
    }

    TEST_CASE("eRectangle Clamp method")
    {
        GIVEN("a rectangle and points to clamp")
        {
            eCoord low(0.0f, 0.0f);
            eCoord high(10.0f, 10.0f);
            eRectangle rect(low, high);

            // Clamp a point inside - should remain unchanged
            eCoord insidePoint(5.0f, 5.0f);
            rect.Clamp(insidePoint);
            THEN("Clamp leaves points inside the rectangle unchanged")
            {
                CHECK(insidePoint.x == doctest::Approx(5.0f));
                CHECK(insidePoint.y == doctest::Approx(5.0f));
            }
            // Clamp returns the maximum movement; for inside points this can be negative
            // Just verify the point wasn't moved

            // Clamp a point outside - should be moved to edge
            eCoord outsidePoint(15.0f, 15.0f);
            REAL dist2 = rect.Clamp(outsidePoint);
            THEN("Clamp moves points outside to the boundary")
            {
                CHECK(outsidePoint.x <= 10.0f);
                CHECK(outsidePoint.y <= 10.0f);
                CHECK(dist2 > 0.0f);
            }
        }
    }

    TEST_CASE("eRectangle GetPoint method")
    {
        GIVEN("a rectangle and normalized coordinates")
        {
            eCoord low(0.0f, 0.0f);
            eCoord high(10.0f, 10.0f);
            eRectangle rect(low, high);

            // Get a point in the interior using normalized coordinates [0,1]
            eCoord interiorPoint = rect.GetPoint(eCoord(0.5f, 0.5f));

            // Should be inside the rectangle (at center)
            THEN("GetPoint returns a point inside the rectangle")
            {
                CHECK(rect.Contains(interiorPoint) == true);
            }
            THEN("GetPoint returns the correct point")
            {
                CHECK(interiorPoint.x == doctest::Approx(5.0f));
                CHECK(interiorPoint.y == doctest::Approx(5.0f));
            }
        }
    }

    TEST_CASE("eRectangle Clip method")
    {
        GIVEN("a rectangle and a line segment to clip")
        {
            eCoord low(0.0f, 0.0f);
            eCoord high(10.0f, 10.0f);
            eRectangle rect(low, high);

            eCoord start(0.0f, 0.0f);
            eCoord stop(20.0f, 20.0f);

            // Clip stop to lie inside the rectangle
            rect.Clip(start, stop);

            // Stop should be clamped to the rectangle boundary
            THEN("Clip clamps the stop point to the rectangle boundary")
            {
                CHECK(stop.x <= 10.0f);
                CHECK(stop.y <= 10.0f);
            }
        }
    }

    TEST_CASE("eRectangle with negative coordinates")
    {
        GIVEN("a rectangle with negative coordinates")
        {
            eCoord low(-10.0f, -10.0f);
            eCoord high(10.0f, 10.0f);
            eRectangle rect(low, high);

            eCoord origin(0.0f, 0.0f);
            THEN("Contains returns true for the origin")
            {
                CHECK(rect.Contains(origin) == true);
            }
        }
    }
}
