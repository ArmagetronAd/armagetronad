#include "doctest.h"
#include "eGrid.h"

// Tests for eGrid system
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("eGrid")
{
    TEST_CASE("eGrid basic construction")
    {
        GIVEN("eGrid system")
        {
            auto grid = tRefPtr<eGrid>::Make();

            THEN("construction works")
            {
            }

            THEN("drawing works")
            {
                auto firstPoint = grid->Insert(eCoord{0, 0});
                grid->DrawLine(firstPoint, eCoord{1, 0});
            }
        }
    }
}

// TODO: More tests, collisions, game objects moving about