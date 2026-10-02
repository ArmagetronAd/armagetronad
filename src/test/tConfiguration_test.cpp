#include "doctest.h"
#include "tConfiguration.h"

#include "MockConsole.h"
#include "ConfItemTempSetter.h"

// Tests for tConfiguration system
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("tConfiguration")
{
    TEST_CASE("tConfiguration basic access level enum")
    {
        GIVEN("access level enum values")
        {
            THEN("access levels are ordered correctly")
            {
                // Lower numeric values should have higher privileges
                CHECK(tAccessLevel_Owner < tAccessLevel_Admin);
                CHECK(tAccessLevel_Admin < tAccessLevel_Moderator);
                CHECK(tAccessLevel_Moderator < tAccessLevel_TeamLeader);
                CHECK(tAccessLevel_TeamLeader < tAccessLevel_TeamMember);
                CHECK(tAccessLevel_TeamMember < tAccessLevel_Local);
                CHECK(tAccessLevel_Local < tAccessLevel_Remote);
                CHECK(tAccessLevel_Remote < tAccessLevel_Authenticated);
                CHECK(tAccessLevel_Authenticated < tAccessLevel_Invalid);
            }
        }
    }

    TEST_CASE("tCurrentAccessLevel basic functionality")
    {
        GIVEN("current access level management")
        {
            tCurrentAccessLevel access1{tAccessLevel::tAccessLevel_Admin, true};

            THEN("set access level can be retrieved")
            {
                // Just verify the methods exist by taking their address
                CHECK(tAccessLevel::tAccessLevel_Admin == tCurrentAccessLevel::GetAccessLevel());

                AND_WHEN("access level is lowered")
                {
                    tCurrentAccessLevel access2{tAccessLevel::tAccessLevel_Moderator, false};
                    THEN("lowered access level is current")
                    {
                        // Just verify the methods exist by taking their address
                        CHECK(tAccessLevel::tAccessLevel_Moderator == tCurrentAccessLevel::GetAccessLevel());
                    }
                }
            }
        }
    }
    
    #if false // requires language initialization, would be language dependent
    TEST_CASE("GetName works")
    {
        auto name = tCurrentAccessLevel::GetName(tAccessLevel::tAccessLevel_Admin);
        CHECK(name == "Administrator");
    }
    #endif

    TEST_CASE("tCasaclPreventer basic functionality")
    {
        GIVEN("CASACL prevention mechanism")
        {
            THEN("tCasaclPreventer can be constructed and destroyed")
            {
                // Test that we can create and destroy a preventer
                {
                    tCasaclPreventer preventer;
                    CHECK(true == tCasaclPreventer::InRInclude());
                }

                {
                    tCasaclPreventer preventer(true);
                    CHECK(true == tCasaclPreventer::InRInclude());
                }

                {
                    tCasaclPreventer preventer(false);
                    CHECK(false == tCasaclPreventer::InRInclude());
                }
            }

            THEN("InRInclude returns a boolean")
            {
                // no preventer, InRInclude should be false
                CHECK(false == tCasaclPreventer::InRInclude());
            }
        }
    }

    TEST_CASE("tConfItemBase map functionality")
    {
        GIVEN("configuration item map")
        {
            THEN("GetConfItemMap returns a a map")
            {
                auto& map = tConfItemBase::GetConfItemMap();
                AND_THEN("It is not empty")
                {
                    CHECK(100 < map.size()); // and in fact quite full, all the confitems in all the libraries self-register
                }
            }
        }
    }

    // This test also demonstrates how you can change setting values during a test;
    // just put a tConfItemTempValue<> on the stack, call SetValue, stop worrying
    TEST_CASE("tConfItemTempValue basic functionality")
    {
        MockConsole con;

        GIVEN("a configuration item temporary setter")
        {
            ConfItemTempValue<int> setter{"SERVER_PORT"};

            THEN("it has a default value")
            {
                CHECK(setter.GetOldValue() == 4534);
            }

            WHEN("it gets modified")
            {
                setter.SetValue(4535);

                THEN("the value changed")
                {
                    CHECK(setter.GetCurrentValue() == 4535);
                }
            }

            WHEN("another setter modifies the value")
            {
                {
                    ConfItemTempValue<int> otherSetter{"SERVER_PORT"};
                    otherSetter.SetValue(4555);
                    CHECK(otherSetter.GetCurrentValue() == 4555);
                }
                THEN("the value is reset afterwards")
                {
                    CHECK(setter.GetCurrentValue() == 4534);
                }
            }
        }
    }

    TEST_CASE("ConfItemTempValue expected fails" * doctest::should_fail())
    {
        GIVEN("a wrong confitem name")
        {
            auto const* const name = "NO_SUCH_ITEM";
            THEN("creating value setter with it should fail")
            {
                ConfItemTempValue<int>{name};
            }
        }
    }
}

// TODO: More comprehensive tConfiguration tests could be added, but the system
// has significant global state and dependencies that make isolated unit testing
// challenging. The current tests focus on:
// 1. Basic enum value verification
// 2. Access level management
// 3. CASACL prevention mechanism
// 4. Configuration item map access
//
// Additional tests that could be added:
// 1. Configuration file parsing (requires file system setup)
// 2. Setting retrieval and modification (requires initialization)
// 3. Hierarchical configuration loading (requires multiple config files)
// 4. CASACL enforcement (requires access level setup)
// 5. Notification callbacks (requires callback infrastructure)
//
// These would need to be integration tests rather than unit tests, or would
// require significant refactoring to make the configuration system more testable.
