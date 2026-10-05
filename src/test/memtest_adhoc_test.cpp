// memtest_adhoc_test.cpp - Tests converted from src/tools/memtest.cpp
// This file contains doctest-based tests for memory manager functionality
// Original: src/tools/memtest.cpp

#include <cmath> // for fabs
#include "tMemManager.h"
#include "tLinkedList.h"
#include "tDefer.h"
#include "doctest.h"

namespace
{
class test3 : public tListItem<test3>
{
public:
    test3() : tListItem<test3>(first) {}

    static test3* first;
};

test3* test3::first{};

}; // namespace

// ============================================================================
// TEST_CASE: Memory Linked List Node Creation
// BDD: GIVEN anchor, WHEN create test3 instances, THEN all linked correctly
// Original: lines 1-13, static initialization
// Note: test3 class and anchor are from the original file
// ============================================================================
TEST_CASE("Memory: Linked List Node Creation")
{
    // anchor should start and end empty
    INVARIANT_CHECK(!test3::first);

    // GIVEN: anchor and test3 class
    WHEN("we create four linked objects")
    {
        test3 a, b, c, d;
        THEN("the last one becomes the anchor")
        {
            CHECK(&d == test3::first);
        }
    }
}

// ============================================================================
// TEST_CASE: Memory Virtual Inheritance
// BDD: GIVEN class hierarchy, WHEN create C, THEN x equals 2
// Original: lines 148-156 in memtest.cpp
// This just tests my understanding of virtual inheritance: The topmost constructor call to the virtual base is the one actually executed
// ============================================================================
TEST_CASE("Memory: Virtual Inheritance")
{
    GIVEN("a virtual inheritance hierarchy")
    {
        class A
        {
        public:
            int x;
            A(int X) : x(X) {}
        };

        class B : virtual public A
        {
        public:
            B() : A(1) {}
        };

        class C : public B
        {
        public:
            C() : A(2) {}
        };

        WHEN("we spawn a leaf object")
        {
            C c;

            THEN("the constructor call from the leaf class is executed")
            {
                CHECK(c.x == 2);
            }
        }
    }
}

// ============================================================================
// TEST_CASE: Memory Allocation and Deallocation
// BDD: GIVEN memory manager, WHEN allocate and free objects, THEN no leaks
// Original: lines 158-171 in memtest.cpp
// Purpose: Stress test our custom memory manager, which is not in use much any more
// ============================================================================
TEST_CASE("Memory: Allocation and Deallocation")
{
    // GIVEN: memory manager
    // Classes from original file
    class test_base
    {
        int x;

    public:
        virtual ~test_base() {}
        tMEMMANAGER(test_base);
    };

    class test_derived : public test_base
    {
        int y;

    public:
        virtual ~test_derived() {}
        tMEMMANAGER(test_derived);
    };

#define MAX 100

    // WHEN: allocate and free objects
    test_base* x[MAX];

    test_derived* y = new test_derived;
    delete y;

    for (int i = 0; i < MAX; i++)
    {
        x[i] = new test_base;
    }

    for (int i = 0; i < MAX; i++)
    {
        if (i % 4 != 0)
        {
            delete x[i];
            x[i] = NULL;
        }
    }

    for (int i = 0; i < MAX; i++)
    {
        if (!x[i])
        {
            x[i] = new test_base;
        }
    }

    for (int i = 0; i < MAX; i++)
    {
        delete x[i];
    }

    // THEN: no leaks (implicit - if we get here, no crashes)
}
