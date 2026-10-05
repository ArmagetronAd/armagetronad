#include "doctest.h"
#include "tException.h"

// Tests for tException class hierarchy
// Purpose: Verify exception handling and message retrieval

TEST_SUITE("tException")
{
    TEST_CASE("tGenericException construction and methods")
    {
        GIVEN("a tGenericException with name and description")
        {
            tGenericException exc("Test error message", "TestError");

            // Test GetName
            tString name = exc.GetName();
            THEN("GetName returns the exception name")
            {
                CHECK(name == "TestError");
            }

            // Test GetDescription
            tString description = exc.GetDescription();
            THEN("GetDescription returns the error message")
            {
                CHECK(description == "Test error message");
            }
        }
    }

    TEST_CASE("tGenericException with NULL name")
    {
        GIVEN("a tGenericException with NULL name")
        {
            tGenericException exc("Test message only");

            tString name = exc.GetName();
            // When name is NULL, it should return empty or description
            // Check that it doesn't crash
            THEN("GetName returns a valid string")
            {
                CHECK(name.Len() >= 0);
            }

            tString description = exc.GetDescription();
            THEN("GetDescription returns the message")
            {
                CHECK(description == "Test message only");
            }
        }
    }

    TEST_CASE("tCleanQuit exception")
    {
        GIVEN("a tCleanQuit exception")
        {
            tCleanQuit quit;

            // Test GetName
            tString name = quit.GetName();
            // Should return "CleanQuit" or similar
            THEN("GetName returns a non-empty string")
            {
                CHECK(name.Len() > 0);
            }

            // Test GetDescription
            tString description = quit.GetDescription();
            THEN("GetDescription returns a valid string")
            {
                CHECK(description.Len() >= 0);
            }
        }
    }

    TEST_CASE("Exception polymorphism")
    {
        GIVEN("a tGenericException and a tException pointer")
        {
            // Test that tGenericException can be treated as tException
            tGenericException* ge = new tGenericException("Polymorphic test", "PolyError");
            tException* exc = ge;

            tString name = exc->GetName();
            THEN("GetName returns the exception name through base pointer")
            {
                CHECK(name == "PolyError");
            }

            tString description = exc->GetDescription();
            THEN("GetDescription returns the message through base pointer")
            {
                CHECK(description == "Polymorphic test");
            }

            delete ge;
        }
    }
}
