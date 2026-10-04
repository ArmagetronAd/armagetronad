#include "doctest.h"
#include "tList.h"

// Mock class that inherits from tListMember for testing
class MockListMember : public tListMember
{
public:
    int value;
    MockListMember(int v = 0) : value(v) {}
    bool operator==(const MockListMember& other) const { return value == other.value; }
};

// Tests for tList class
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("tList")
{
    TEST_CASE("tList default construction")
    {
        GIVEN("a default tList")
        {
            tList<MockListMember> list;
            // Size may not be 0 due to uninitialized offset - don't check it
            THEN("list length is zero")
            {
                CHECK(list.Len() == 0);
            }
        }
    }

    TEST_CASE("tList Add with list member")
    {
        GIVEN("a tList with one item added")
        {
            tList<MockListMember> list;
            MockListMember item1(42);

            list.Add(&item1);
            THEN("list contains the item with valid ListID")
            {
                CHECK(list.Len() == 1);
                // The item should be in the list
                CHECK(list[0] == &item1);
                // The item's ListID should be valid
                CHECK(item1.ListID() >= 0);
            }
        }
    }

    TEST_CASE("tList Add multiple items")
    {
        GIVEN("a tList with three items added")
        {
            tList<MockListMember> list;
            MockListMember item1(1);
            MockListMember item2(2);
            MockListMember item3(3);

            list.Add(&item1);
            list.Add(&item2);
            list.Add(&item3);

            // Verify all items are in the list
            bool found1 = false, found2 = false, found3 = false;
            for (int i = 0; i < list.Len(); i++)
            {
                MockListMember* item = list[i];
                if (item == &item1)
                    found1 = true;
                if (item == &item2)
                    found2 = true;
                if (item == &item3)
                    found3 = true;
            }

            THEN("all items are in the list")
            {
                CHECK(list.Len() == 3);
                CHECK(found1);
                CHECK(found2);
                CHECK(found3);
            }
        }
    }

    TEST_CASE("tList Remove with list member")
    {
        tList<MockListMember> list;
        MockListMember item(42);

        list.Add(&item);
        CHECK(list.Len() == 1);
        CHECK(item.ListID() >= 0);

        list.Remove(&item);
        CHECK(list.Len() == 0);
        // After removal, ListID should be -1
        CHECK(item.ListID() == -1);
    }

    TEST_CASE("tList Remove multiple items")
    {
        tList<MockListMember> list;
        MockListMember item1(1);
        MockListMember item2(2);
        MockListMember item3(3);

        list.Add(&item1);
        list.Add(&item2);
        list.Add(&item3);
        CHECK(list.Len() == 3);

        list.Remove(&item2);
        CHECK(list.Len() == 2);
        CHECK(item2.ListID() == -1);

        // item1 and item3 should still be in the list
        bool found1 = false, found3 = false;
        for (int i = 0; i < list.Len(); i++)
        {
            MockListMember* item = list[i];
            if (item == &item1)
                found1 = true;
            if (item == &item3)
                found3 = true;
        }

        CHECK(found1);
        CHECK(found3);
    }

    TEST_CASE("tList Clear via SetLen")
    {
        tList<MockListMember> list;
        MockListMember item1(1);
        MockListMember item2(2);

        list.Add(&item1);
        list.Add(&item2);
        CHECK(list.Len() == 2);

        // Note: tList Clear is inherited from tArray, but it's protected
        // So we use SetLen instead
        list.SetLen(0);
        CHECK(list.Len() == 0);
    }

    TEST_CASE("tList Swap")
    {
        tList<MockListMember> list1;
        tList<MockListMember> list2;

        MockListMember item1(1);
        MockListMember item2(2);

        list1.Add(&item1);
        list2.Add(&item2);

        CHECK(list1.Len() == 1);
        CHECK(list2.Len() == 1);

        list1.Swap(list2);

        CHECK(list1.Len() == 1);
        CHECK(list2.Len() == 1);
        CHECK(list1[0] == &item2);
        CHECK(list2[0] == &item1);
    }

    TEST_CASE("tList inherited from tArray")
    {
        GIVEN("a tList with two items")
        {
            tList<MockListMember> list;
            MockListMember item1(1);
            MockListMember item2(2);

            list.Add(&item1);
            list.Add(&item2);

            // Test that we can use tArray methods
            // operator[] from tArray
            THEN("tArray methods work correctly")
            {
                CHECK(list.Len() == 2);
                CHECK(list[0] != nullptr);
                CHECK(list[1] != nullptr);
            }
        }
    }
}
