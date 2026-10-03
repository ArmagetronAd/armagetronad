#include "doctest.h"
#include "ePath.h"

// Tests for ePath system
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("ePath")
{
    TEST_CASE("ePath basic construction")
    {
        GIVEN("ePath system")
        {
            // we can only properly fill ePath with data from an eGrid

            THEN("ePath can be constructed")
            {
                ePath path;
            }

            THEN("ePath can be destroyed")
            {
                ePath* path = new ePath();
                delete path;
            }
        }
    }

    TEST_CASE("ePath simple methods")
    {
        GIVEN("an ePath instance")
        {
            ePath path;

            THEN("Valid returns false for empty path")
            {
                CHECK(path.Valid() == false);
            }

            THEN("Clear can be called")
            {
                path.Clear();
            }
        }
    }

// Note: RenderLast requires DEBUG to be defined
#ifdef DEBUG
    TEST_CASE("ePath static methods")
    {
        GIVEN("ePath static methods")
        {
            THEN("static methods exist")
            {
                (void)&ePath::RenderLast;
            }
        }
    }
#endif // DEBUG
}

// Z-Man: TODO add full grid, make path, walk through it