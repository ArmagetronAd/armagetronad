#include "doctest.h"
#include "eAxis.h"

// Tests for eAxis class
// Purpose: Verify axis and winding operations

TEST_SUITE("eAxis")
{
    TEST_CASE("eAxis default construction")
    {
        GIVEN("a default eAxis")
        {
            eAxis axis;

            // Default axis should have 4 windings
            THEN("it has 4 windings")
            {
                CHECK(axis.WindingNumber() == 4);
            }
        }
    }

    TEST_CASE("eAxis with a winding number")
    {
        GIVEN("an eAxis constructed with a winding number")
        {
            eAxis axis8(8);
            THEN("the winding number is 8")
            {
                CHECK(axis8.WindingNumber() == 8);
            }

            eAxis axis16(16);
            THEN("the winding number is 16")
            {
                CHECK(axis16.WindingNumber() == 16);
            }
        }
    }

    TEST_CASE("eAxis GetDirection method")
    {
        GIVEN("an eAxis with 4 windings")
        {
            eAxis axis(4);

            // Get direction for winding 0 (270 degrees = down)
            eCoord dir0 = axis.GetDirection(0);
            THEN("winding 0 points down")
            {
                CHECK(dir0.x == doctest::Approx(0.0f));
                CHECK(dir0.y == doctest::Approx(-1.0f));
            }

            // Get direction for winding 1 (180 degrees = left)
            eCoord dir1 = axis.GetDirection(1);
            THEN("winding 1 points left")
            {
                CHECK(dir1.x == doctest::Approx(-1.0f));
                CHECK(dir1.y == doctest::Approx(0.0f));
            }

            // Get direction for winding 2 (90 degrees = up)
            eCoord dir2 = axis.GetDirection(2);
            THEN("winding 2 points up")
            {
                CHECK(dir2.x == doctest::Approx(0.0f));
                CHECK(dir2.y == doctest::Approx(1.0f));
            }

            // Get direction for winding 3 (0 degrees = right)
            eCoord dir3 = axis.GetDirection(3);
            THEN("winding 3 points right")
            {
                CHECK(dir3.x == doctest::Approx(1.0f));
                CHECK(dir3.y == doctest::Approx(0.0f));
            }
        }
    }

    TEST_CASE("eAxis NearestWinding method")
    {
        GIVEN("an eAxis with 4 windings")
        {
            eAxis axis(4);

            // With 4 windings: 0=down(0,-1), 1=left(-1,0), 2=up(0,1), 3=right(1,0)

            // Test point along positive x axis (right) -> winding 3
            eCoord posX(1.0f, 0.0f);
            int nearest0 = axis.NearestWinding(posX);
            THEN("positive x axis maps to winding 3")
            {
                CHECK(nearest0 == 3);
            }

            // Test point along positive y axis (up) -> winding 2
            eCoord posY(0.0f, 1.0f);
            int nearest1 = axis.NearestWinding(posY);
            THEN("positive y axis maps to winding 2")
            {
                CHECK(nearest1 == 2);
            }

            // Test point along negative x axis (left) -> winding 1
            eCoord negX(-1.0f, 0.0f);
            int nearest2 = axis.NearestWinding(negX);
            THEN("negative x axis maps to winding 1")
            {
                CHECK(nearest2 == 1);
            }

            // Test point along negative y axis (down) -> winding 0
            eCoord negY(0.0f, -1.0f);
            int nearest3 = axis.NearestWinding(negY);
            THEN("negative y axis maps to winding 0")
            {
                CHECK(nearest3 == 0);
            }
        }
    }

    TEST_CASE("eAxis Turn method")
    {
        GIVEN("an eAxis with 4 windings")
        {
            eAxis axis(4);

            // Test turning from winding 0
            int current = 0;
            axis.Turn(current, 1); // Turn right
            THEN("turning right from 0 gives 1")
            {
                CHECK(current == 1);
            }

            axis.Turn(current, 1); // Turn right again
            THEN("turning right from 1 gives 2")
            {
                CHECK(current == 2);
            }

            axis.Turn(current, -1); // Turn left
            THEN("turning left from 2 gives 1")
            {
                CHECK(current == 1);
            }
        }
    }

    TEST_CASE("eAxis TurnRight method")
    {
        GIVEN("an eAxis with 4 windings")
        {
            eAxis axis(4);

            int direction = 0;
            axis.TurnRight(direction);
            // TurnRight decrements, wraps around: (0-1+4)%4 = 3
            THEN("TurnRight from 0 gives 3")
            {
                CHECK(direction == 3);
            }

            axis.TurnRight(direction);
            // (3-1+4)%4 = 2
            THEN("TurnRight from 3 gives 2")
            {
                CHECK(direction == 2);
            }
        }
    }

    TEST_CASE("eAxis TurnLeft method")
    {
        GIVEN("an eAxis with 4 windings")
        {
            eAxis axis(4);

            int direction = 2;
            axis.TurnLeft(direction);
            // TurnLeft increments, wraps: (2+1)%4 = 3
            THEN("TurnLeft from 2 gives 3")
            {
                CHECK(direction == 3);
            }

            axis.TurnLeft(direction);
            // (3+1)%4 = 0
            THEN("TurnLeft from 3 gives 0")
            {
                CHECK(direction == 0);
            }
        }
    }

    TEST_CASE("eAxis with different winding numbers")
    {
        GIVEN("eAxis instances with different winding numbers")
        {
            eAxis axis3(3);
            THEN("axis with 3 windings has winding number 3")
            {
                CHECK(axis3.WindingNumber() == 3);
            }

            eAxis axis6(6);
            THEN("axis with 6 windings has winding number 6")
            {
                CHECK(axis6.WindingNumber() == 6);
            }

            // Test GetDirection with different winding numbers
            // With 6 windings, the directions are evenly spaced
            eCoord dir0 = axis6.GetDirection(0);
            eCoord dir3 = axis6.GetDirection(3);

            // Verify they are valid directions (unit vectors)
            REAL len0 = dir0.NormSquared();
            REAL len3 = dir3.NormSquared();
            THEN("GetDirection returns unit vectors")
            {
                CHECK(len0 == doctest::Approx(1.0f));
                CHECK(len3 == doctest::Approx(1.0f));
            }
        }
    }
}
