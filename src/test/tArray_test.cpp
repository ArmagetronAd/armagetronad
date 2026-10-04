#include "doctest.h"
#include "tArray.h"

// Tests for tArray class
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("tArray")
{
    TEST_CASE("tArray default construction and basic properties")
    {
        GIVEN("a default tArray")
        {
            tArray<int> arr;
            THEN("Len returns 0 for a default array")
            {
                CHECK(arr.Len() == 0);
            }
            THEN("capacity is non-negative")
            {
                // capacity may be non-zero due to allocation strategy
                CHECK(arr.capacity() >= 0);
            }
        }
    }

    TEST_CASE("tArray with an initial capacity")
    {
        tArray<int> arr(10);
        CHECK(arr.Len() == 10);
        CHECK(arr.capacity() >= 10);

        // Elements should be default-initialized
        for (int i = 0; i < 10; i++)
        {
            CHECK(arr[i] == 0);
        }
    }

    TEST_CASE("tArray copy construction")
    {
        tArray<int> original(5);
        for (int i = 0; i < 5; i++)
        {
            original[i] = i * 10;
        }

        tArray<int> copy = original;
        CHECK(copy.Len() == original.Len());
        for (int i = 0; i < 5; i++)
        {
            CHECK(copy[i] == original[i]);
        }
    }

    TEST_CASE("tArray assignment operator with empty target")
    {
        GIVEN("a tArray for assignment with empty target")
        {
            tArray<int> original(3);
            original[0] = 1;
            original[1] = 2;
            original[2] = 3;

            tArray<int> assigned;
            assigned = original;
            THEN("the results are as expected")
            {
                CHECK(assigned.Len() == 3);
                CHECK(assigned[0] == 1);
                CHECK(assigned[1] == 2);
                CHECK(assigned[2] == 3);
            }
        }
    }

    TEST_CASE("tArray operator[] with auto-recapacity")
    {
        tArray<int> arr;
        CHECK(arr.Len() == 0);

        arr[0] = 42;
        CHECK(arr.Len() == 1);
        CHECK(arr[0] == 42);

        arr[5] = 100;
        CHECK(arr.Len() == 6);
        CHECK(arr[5] == 100);
    }

    TEST_CASE("tArray operator() const access")
    {
        GIVEN("a tArray for operator() const access")
        {
            tArray<int> arr(3);
            arr[0] = 10;
            arr[1] = 20;
            arr[2] = 30;

            // operator() does bounds checking with assertion
            THEN("the results are as expected")
            {
                CHECK(arr(0) == 10);
                CHECK(arr(1) == 20);
                CHECK(arr(2) == 30);
            }
        }
    }

    TEST_CASE("tArray operator+ pointer access")
    {
        GIVEN("a tArray for operator+ pointer access")
        {
            tArray<int> arr(3);
            arr[0] = 10;
            arr[1] = 20;
            arr[2] = 30;

            THEN("the results are as expected")
            {
                CHECK(*(arr + 0) == 10);
                CHECK(*(arr + 1) == 20);
                CHECK(*(arr + 2) == 30);
            }
        }
    }

    TEST_CASE("tArray SetLen")
    {
        tArray<int> arr;
        arr.SetLen(5);
        CHECK(arr.Len() == 5);
        CHECK(arr.capacity() >= 5);

        // Recapacity larger
        arr.SetLen(10);
        CHECK(arr.Len() == 10);
        CHECK(arr.capacity() >= 10);
    }

    TEST_CASE("tArray Insert")
    {
        tArray<int> arr;
        arr.Insert(10);
        CHECK(arr.Len() == 1);
        CHECK(arr[0] == 10);

        arr.Insert(20);
        CHECK(arr.Len() == 2);
        CHECK(arr[1] == 20);

        arr.Insert(30);
        CHECK(arr.Len() == 3);
        CHECK(arr[2] == 30);
    }

    TEST_CASE("tArray RemoveAt")
    {
        tArray<int> arr;
        arr.Insert(10);
        arr.Insert(20);
        arr.Insert(30);
        CHECK(arr.Len() == 3);

        arr.RemoveAt(1);
        CHECK(arr.Len() == 2);
        // After RemoveAt, the last element moves to the removed position
        CHECK(arr[0] == 10);
        CHECK(arr[1] == 30);
    }

    TEST_CASE("tArray Remove")
    {
        tArray<int> arr;
        arr.Insert(10);
        arr.Insert(20);
        arr.Insert(30);

        bool removed = arr.Remove(20);
        CHECK(removed == true);
        CHECK(arr.Len() == 2);

        removed = arr.Remove(99);
        CHECK(removed == false);
        CHECK(arr.Len() == 2);
    }

    TEST_CASE("tArray Clear via SetLen")
    {
        tArray<int> arr;
        arr.Insert(10);
        arr.Insert(20);
        arr.Insert(30);
        CHECK(arr.Len() == 3);

        arr.SetLen(0);
        CHECK(arr.Len() == 0);
    }

    TEST_CASE("tArray boundary conditions - empty array")
    {
        GIVEN("an empty tArray")
        {
            tArray<int> arr;
            THEN("the results are as expected")
            {
                CHECK(arr.Len() == 0);
                // capacity may be non-zero
                CHECK(arr.capacity() >= 0);
            }
        }
    }

    TEST_CASE("tArray boundary conditions - single element")
    {
        tArray<int> arr;
        arr.Insert(42);
        CHECK(arr.Len() == 1);
        CHECK(arr[0] == 42);

        arr.RemoveAt(0);
        CHECK(arr.Len() == 0);
    }

    TEST_CASE("tArray with custom type")
    {
        GIVEN("a tArray with custom type")
        {
            struct CustomType
            {
                int value;
                CustomType() : value(0) {}
                CustomType(int v) : value(v) {}
                bool operator==(const CustomType& other) const { return value == other.value; }
            };

            tArray<CustomType> arr;
            arr.Insert(CustomType(42));
            THEN("the results are as expected")
            {
                CHECK(arr.Len() == 1);
                CHECK(arr[0].value == 42);
            }
        }
    }
}
