#include "doctest.h"
#include "tEventQueue.h"

// Tests for tEventQueue system
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("tEventQueue")
{
    TEST_CASE("tEventQueue default construction")
    {
        GIVEN("a default tEventQueue")
        {
            tEventQueue queue;

            THEN("it can be constructed without crashing")
            {
            }
        }
    }
}

// Z-Man: This class is unused and uses plain manual memory management, and based off the terible tHeap. I don't think we will use it in this form.