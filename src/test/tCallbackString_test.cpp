#include "doctest.h"
#include "tCallbackString.h"

// Tests for tCallbackString class
// Purpose: Verify string-returning callback registration and execution

// Helper functions for testing
tString stringFunc1() { return tString("test1"); }
tString stringFunc2() { return tString("test2"); }
tString emptyStringFunc() { return tString(""); }

TEST_SUITE("tCallbackString")
{
    TEST_CASE("tCallbackString construction and execution")
    {
        GIVEN("a tCallbackString for execution")
        {
            tCallbackString* anchor = NULL;

            // Create a callback with our test function
            tCallbackString cb(anchor, stringFunc1);

            // Execute all callbacks and get the result
            tString result = tCallbackString::Exec(anchor);

            // Should return the string from our function
            THEN("Exec returns the callback result")
            {
                CHECK(result == "test1");
            }
        }
    }

    TEST_CASE("tCallbackString multiple callbacks")
    {
        GIVEN("multiple tCallbackStrings")
        {
            tCallbackString* anchor = NULL;

            // Create multiple callbacks
            tCallbackString cb1(anchor, stringFunc1);
            tCallbackString cb2(anchor, stringFunc2);

            // Execute - should return the first one (or concatenate, depending on implementation)
            tString result = tCallbackString::Exec(anchor);

            // Check that we got a result
            THEN("Exec returns a non-empty result")
            {
                CHECK(result.Len() > 0);
            }
        }
    }

    TEST_CASE("tCallbackString empty string")
    {
        GIVEN("a tCallbackString with empty string function")
        {
            tCallbackString* anchor = NULL;

            tCallbackString cb(anchor, emptyStringFunc);

            tString result = tCallbackString::Exec(anchor);
            THEN("Exec returns empty string")
            {
                CHECK(result == "");
            }
        }
    }
}
