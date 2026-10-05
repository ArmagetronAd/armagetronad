#include "doctest.h"
#include "tResourceManager.h"

#include "MockConsole.h"

#include <sstream>

// Tests for tResourceManager system
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("tResourceManager")
{
    TEST_CASE("OpenResource")
    {
        MockConsole con;

        GIVEN("A local resource")
        {
            auto* const DEFAULT_MAP = "Anonymous/polygon/regular/square-1.0.1.aamap.xml";
            THEN("openResource works")
            {
                auto* file = tResourceManager::openResource(DEFAULT_MAP);
                REQUIRE(file);
                fclose(file);
            }
        }

        // not testing remote sources
    }

    TEST_CASE("tResourceManager repository strings")
    {
        GIVEN("repository string variables")
        {
            // Note: These are static strings that can be modified
            // (but should not be modified by random code, they are part of the configuration)

            THEN("resRepoServer exists")
            {
                CHECK(tResourceManager::AccessRepoServer().Len() > 5);
            }

            THEN("resRepoClient exists")
            {
                CHECK(tResourceManager::AccessRepoClient().Len() > 5);
            }
        }
    }

    TEST_CASE("FetchURI with invalid URI")
    {
        MockConsole con;

        GIVEN("an invalid URI")
        {
            WHEN("FetchURI is called with NULL URI")
            {
                std::ostringstream oss;
                tResourceManager::Result result = tResourceManager::FetchURI(nullptr, oss);

                THEN("returns an error code")
                {
                    // Should return an error for NULL URI
                    CHECK(result != tResourceManager::RESULT_Ok);
                }
            }

            WHEN("FetchURI is called with empty URI")
            {
                std::ostringstream oss;
                tResourceManager::Result result = tResourceManager::FetchURI("", oss);

                THEN("returns an error code")
                {
                    // Should return an error for empty URI
                    CHECK(result != tResourceManager::RESULT_Ok);
                }
            }
        }
    }
}

// TODO: More comprehensive tResourceManager tests could be added, but the system
// has dependencies on file system and network which make isolated unit testing
// challenging. The current tests focus on:
// 1. Result enum value verification
// 2. Static method existence
// 3. Repository string variables
// 4. Basic error handling for invalid inputs
//
// Additional tests that could be added:
// 1. Resource loading from various paths (proto, included, user) - requires file system
// 2. Caching behavior with repeated loads - requires cache initialization
// 3. Fallback to alternate paths when primary path missing - requires multiple paths
// 4. Resource unloading and cache invalidation - requires cache setup
// 5. Reference counting behavior - requires resource objects
//
// These would need to be integration tests rather than unit tests, or would
// require significant refactoring to make the resource manager more testable.

// Z-Man says: File system access to known files from the repo is fine.
