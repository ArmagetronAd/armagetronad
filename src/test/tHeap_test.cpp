#include "doctest.h"
#include "tHeap.h"

// Tests for tHeap class
// Purpose: Document the status quo behavior and detect regressions

namespace
{
class MockHeapElement : public tHeapElement
{
public:
    void SetVal(double val, tHeapBase& heap)
    {
        _heap = &heap;
        tHeapElement::SetVal(val, heap);
    }

protected:
    tHeapBase* Heap() const override { return _heap; };

private:
    tHeapBase* _heap{};
};
} // namespace

// For now, just test that we can include the header and basic types exist
TEST_SUITE("tHeap")
{
    TEST_CASE("tHeap management")
    {
        GIVEN("a heap")
        {
            // SO, THIS INTERFACE IS TERRIBLE

            MockHeapElement a, b; // construct first, so heap destructor doing the cleanup can come first
            tHeap<tHeapElement> heap;

            THEN("we can insert an object")
            {
                // SO, SO TERRIBLE, yes, this is how you insert
                a.SetVal(1, heap);

                THEN("we can remove an object")
                {
                    a.RemoveFromHeap();
                }
            }

            THEN("we can insert two objects")
            {
                a.SetVal(1, heap);
                b.SetVal(2, heap);

                THEN("we can remove two objects")
                {
                    a.RemoveFromHeap();
                    b.RemoveFromHeap();
                }
            }

            THEN("two objects can also just be left alone")
            {
                a.SetVal(1, heap);
                b.SetVal(2, heap);
            }

            THEN("actual heap function")
            {
                a.SetVal(1, heap);
                b.SetVal(2, heap);

                THEN("Remove items in order")
                {
                    auto const r1 = heap.Remove(0);
                    REQUIRE(r1);
                    auto const r2 = heap.Remove(0);
                    REQUIRE(r2);
                    CHECK(r1 == &a);
                    CHECK(r2 == &b);
                }
            }

            THEN("actual heap function, different order")
            {
                // different values
                a.SetVal(3, heap);
                b.SetVal(2, heap);

                THEN("Remove items in order")
                {
                    auto const r1 = heap.Remove(0);
                    REQUIRE(r1);
                    auto const r2 = heap.Remove(0);
                    REQUIRE(r2);
                    // different order
                    CHECK(r2 == &a);
                    CHECK(r1 == &b);
                }
            }
        }
    }
}
