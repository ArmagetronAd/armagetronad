#include "doctest.h"
#include "tMemStack.h"

#include "tDefer.h"

// Tests for tMemStack class
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("tMemStack")
{
    TEST_CASE("tMemStack construction and destruction")
    {
        GIVEN("a tMemStack for testing")
        {
            tMemStack stack;
            THEN("GetMem returns non-null pointer")
            {
                CHECK(stack.GetMem() != nullptr);
            }
            THEN("GetSize returns a reasonable size")
            {
                CHECK(stack.GetSize() >= 10);
            }
        }
    }

    TEST_CASE("tMemStack IncreaseMem")
    {
        GIVEN("a tMemStack for memory increase testing")
        {
            tMemStack stack;
            int originalSize = stack.GetSize();

            stack.IncreaseMem();
            THEN("GetSize returns a larger size after IncreaseMem")
            {
                CHECK(stack.GetSize() > originalSize);
            }
            THEN("GetMem still returns non-null pointer")
            {
                CHECK(stack.GetMem() != nullptr);
            }
        }
    }

    TEST_CASE("tMemStack Push/Pop")
    {
        GIVEN("an outer tMemStack")
        {
            tMemStack stack(1); // we just need one byte
            CHECK(stack.GetSize() > 0);
            auto outerPtr = static_cast<char*>(stack.GetMem());
            *outerPtr = 42;

            auto const defer = tDefer(
                [outerPtr]()
                { CHECK(42 == *outerPtr); });

            WHEN("An innter tMemStack is created and eleted")
            {
                tMemStack innerStack;
                CHECK(42 == *outerPtr);

                THEN("The outer stack is not affected") {}
            }
        }
    }
}
