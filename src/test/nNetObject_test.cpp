#include "doctest.h"
#include "nNetObject.h"

// Tests for nNetObject system
// Purpose: Document the status quo behavior and detect regressions

namespace
{
class MockNetObject : public nNetObject
{
public:
    using nNetObject::nNetObject;

    nDescriptor& CreatorDescriptor() const override;
};

static nNOInitialisator<MockNetObject> s_mockDescriptor(99, "MockNetObject");

nDescriptor& MockNetObject::CreatorDescriptor() const { return s_mockDescriptor; }
} // namespace

TEST_SUITE("nNetObject")
{
    TEST_CASE("nNetObject basic types")
    {
        GIVEN("a mock network object")
        {
            auto p = tRefPtr<MockNetObject>::Make();

            THEN("You can do absolutely nothing with it")
            {
            }
        }
    }

    TEST_CASE("nNetObject observer")
    {
        GIVEN("a mock network object")
        {
            auto p = tRefPtr<MockNetObject>::Make();
            THEN("we can observe it")
            {
                // another weird interface choice
                nObserver& observer = p->GetObserver();
                tRefPtr<nObserver> keeper(&observer);

                CHECK(observer.GetNetObject());

                WHEN("we delete the object")
                {
                    p = nullptr;

                    THEN("the observer notices it lost contact")
                    {
                        CHECK(nullptr == observer.GetNetObject());
                    }
                }
            }
        }
    }
}
