#include "doctest.h"
#include "tCallback.h"

// Tests for tCallback, tCallbackAnd, tCallbackOr classes
// Purpose: Verify callback registration and execution

// Helper functions for testing
bool testFlag = false;
void testVoidFunc()
{
    testFlag = true;
}

bool boolFunc1() { return true; }
bool boolFunc2() { return false; }
bool boolFunc3() { return true; }

TEST_SUITE("tCallback")
{
    TEST_CASE("tCallback construction and execution")
    {
        GIVEN("a callback and a function to call")
        {
            tCallback* anchor = NULL;
            testFlag = false;

            // Create a callback with our test function
            tCallback cb(anchor, testVoidFunc);

            // Execute all callbacks
            tCallback::Exec(anchor);

            // Verify our function was called
            THEN("Exec calls the registered function")
            {
                CHECK(testFlag == true);
            }

            // Clean up - callbacks should be deleted when anchor is reset
            // Note: The actual cleanup depends on tLinkedList implementation
        }
    }

    TEST_CASE("tCallbackAnd construction and execution - all true")
    {
        GIVEN("tCallbackAnd with functions returning true")
        {
            tCallbackAnd* anchor = NULL;

            // Create callbacks that all return true
            tCallbackAnd cb1(anchor, boolFunc1);
            tCallbackAnd cb2(anchor, boolFunc1);

            // Execute and check result (should be true AND true = true)
            bool result = tCallbackAnd::Exec(anchor);
            THEN("Exec returns true when all callbacks return true")
            {
                CHECK(result == true);
            }
        }
    }

    TEST_CASE("tCallbackAnd construction and execution - one false")
    {
        GIVEN("tCallbackAnd with one false function")
        {
            tCallbackAnd* anchor = NULL;

            // Create callbacks: true AND false = false
            tCallbackAnd cb1(anchor, boolFunc1);
            tCallbackAnd cb2(anchor, boolFunc2);

            bool result = tCallbackAnd::Exec(anchor);
            THEN("Exec returns false when any callback returns false")
            {
                CHECK(result == false);
            }
        }
    }

    TEST_CASE("tCallbackAnd construction and execution - all false")
    {
        GIVEN("tCallbackAnd with functions returning false")
        {
            tCallbackAnd* anchor = NULL;

            // Create callbacks: false AND false = false
            tCallbackAnd cb1(anchor, boolFunc2);
            tCallbackAnd cb2(anchor, boolFunc2);

            bool result = tCallbackAnd::Exec(anchor);
            THEN("Exec returns false when all callbacks return false")
            {
                CHECK(result == false);
            }
        }
    }

    TEST_CASE("tCallbackOr construction and execution - all true")
    {
        GIVEN("tCallbackOr with functions returning true")
        {
            tCallbackOr* anchor = NULL;

            // Create callbacks: true OR true = true
            tCallbackOr cb1(anchor, boolFunc1);
            tCallbackOr cb2(anchor, boolFunc1);

            bool result = tCallbackOr::Exec(anchor);
            THEN("Exec returns true when all callbacks return true")
            {
                CHECK(result == true);
            }
        }
    }

    TEST_CASE("tCallbackOr construction and execution - one true")
    {
        GIVEN("tCallbackOr with one true function")
        {
            tCallbackOr* anchor = NULL;

            // Create callbacks: true OR false = true
            tCallbackOr cb1(anchor, boolFunc1);
            tCallbackOr cb2(anchor, boolFunc2);

            bool result = tCallbackOr::Exec(anchor);
            THEN("Exec returns true when any callback returns true")
            {
                CHECK(result == true);
            }
        }
    }

    TEST_CASE("tCallbackOr construction and execution - all false")
    {
        GIVEN("tCallbackOr with functions returning false")
        {
            tCallbackOr* anchor = NULL;

            // Create callbacks: false OR false = false
            tCallbackOr cb1(anchor, boolFunc2);
            tCallbackOr cb2(anchor, boolFunc2);

            bool result = tCallbackOr::Exec(anchor);
            THEN("Exec returns false when all callbacks return false")
            {
                CHECK(result == false);
            }
        }
    }

    TEST_CASE("tCallback multiple callbacks")
    {
        GIVEN("multiple callbacks registered")
        {
            tCallback* anchor = NULL;
            testFlag = false;

            // Create multiple callbacks
            tCallback cb1(anchor, testVoidFunc);
            tCallback cb2(anchor, testVoidFunc);

            // Execute all
            tCallback::Exec(anchor);

            // Both should have been called
            THEN("Exec calls all registered functions")
            {
                CHECK(testFlag == true);
            }
        }
    }
}
