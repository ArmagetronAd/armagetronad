#include "doctest.h"
#include "nNetObject.h"
#include "nProtoBuf.h"

// Tests for nNetObject system
// Purpose: Document the status quo behavior and detect regressions

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include "nNetObject.pb.h"
#include "nNetObjectPrivate.pb.h"
#pragma GCC diagnostic pop

namespace
{
class MockNetObject : public nNetObject
{
public:
    using nNetObject::nNetObject;

    nNetObjectDescriptorBase& DoGetDescriptor() const override;
};

// static nNOInitialisator<MockNetObject> s_mockDescriptor(99, "MockNetObject");
static nNetObjectDescriptor<MockNetObject, Network::NetObjectSync> s_mockDescriptor(99);

nNetObjectDescriptorBase& MockNetObject::DoGetDescriptor() const { return s_mockDescriptor; }
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
