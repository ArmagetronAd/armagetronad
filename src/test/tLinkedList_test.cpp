#include "doctest.h"
#include "tLinkedList.h"

// Tests for tListItem (linked list functionality)
// Purpose: Document the status quo behavior and detect regressions

// Mock class that inherits from tListItem for testing
class MockLinkedListItem : public tListItem<MockLinkedListItem>
{
public:
    int value;
    MockLinkedListItem(int v = 0) : value(v) {}

    // For sorting
    static bool Compare(const MockLinkedListItem* a, const MockLinkedListItem* b)
    {
        return a->value < b->value;
    }
};

TEST_SUITE("tLinkedList")
{
    TEST_CASE("tListItem default construction")
    {
        GIVEN("a MockLinkedListItem")
        {
            MockLinkedListItem item;
            THEN("Next returns nullptr")
            {
                CHECK(item.Next() == nullptr);
            }
            THEN("IsInList returns false")
            {
                CHECK(item.IsInList() == false);
            }
        }
    }

    TEST_CASE("tListItem Insert and Remove")
    {
        GIVEN("items for insertion and removal")
        {
            MockLinkedListItem* anchor = nullptr;

            MockLinkedListItem item1(1);
            item1.Insert(anchor);
            THEN("after first Insert, anchor points to item1")
            {
                CHECK(anchor == &item1);
                CHECK(item1.IsInList() == true);
                CHECK(item1.Next() == nullptr);
            }

            MockLinkedListItem item2(2);
            item2.Insert(anchor);
            THEN("after second Insert, anchor points to item2 and item2 points to item1")
            {
                CHECK(anchor == &item2);
                CHECK(item2.Next() == &item1);
                CHECK(item1.IsInList() == true);
            }

            item2.Remove();
            THEN("after Remove, anchor points back to item1 and item2 is not in list")
            {
                CHECK(anchor == &item1);
                CHECK(item2.IsInList() == false);
            }
        }
    }

    TEST_CASE("tListItem Len static method")
    {
        GIVEN("items for length calculation")
        {
            MockLinkedListItem* anchor = nullptr;

            MockLinkedListItem item1(1);
            item1.Insert(anchor);
            THEN("Len returns 1 for one item")
            {
                CHECK(MockLinkedListItem::Len(anchor) == 1);
            }

            MockLinkedListItem item2(2);
            item2.Insert(anchor);
            THEN("Len returns 2 for two items")
            {
                CHECK(MockLinkedListItem::Len(anchor) == 2);
            }
        }
    }

    TEST_CASE("tListItem InsertAfter")
    {
        GIVEN("items for InsertAfter")
        {
            MockLinkedListItem* anchor = nullptr;

            MockLinkedListItem item1(1);
            item1.Insert(anchor);

            MockLinkedListItem item2(2);
            item2.InsertAfter(item1);
            THEN("item1 Next points to item2")
            {
                CHECK(item1.Next() == &item2);
            }
            THEN("the list length is 2")
            {
                CHECK(MockLinkedListItem::Len(anchor) == 2);
            }
        }
    }

    TEST_CASE("tListItem SwapLists")
    {
        GIVEN("two lists for swapping")
        {
            MockLinkedListItem* list1 = nullptr;
            MockLinkedListItem* list2 = nullptr;

            MockLinkedListItem item1(1);
            item1.Insert(list1);

            MockLinkedListItem item2(2);
            item2.Insert(list2);

            MockLinkedListItem::SwapLists(list1, list2);
            THEN("SwapLists exchanges the list anchors")
            {
                CHECK(list1 == &item2);
                CHECK(list2 == &item1);
            }
        }
    }
}
