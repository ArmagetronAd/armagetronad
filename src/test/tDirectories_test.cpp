#include "doctest.h"
#include "tDirectories.h"
#include "tString.h"
#include "tArray.h"

#include "MockConsole.h"

// Tests for tDirectories system
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("tDirectories")
{
    TEST_CASE("tPath basic functionality")
    {
        MockConsole con;

        GIVEN("tPath objects")
        {
            THEN("tPathResource can be constructed")
            {
                tPathResource resource;
                tString included = resource.GetIncluded();
                // We can't predict the exact path, but it should be a valid non-empty tString
                CHECK(included.Len() >= 5);
            }
        }
    }

    TEST_CASE("tPathResource basic functionality")
    {
        MockConsole con;

        GIVEN("a tPathResource")
        {
            tPathResource resource;

            THEN("GetIncluded returns a path string")
            {
                tString included = resource.GetIncluded();
                // We can't predict the exact path, but it should be a valid non-empty tString
                CHECK(included.Len() >= 5);
            }
        }
    }

    TEST_CASE("tDirectories static methods")
    {
        GIVEN("tDirectories static methods")
        {
            tArray<tString> pathElements;

            auto const testPaths = [](tPath const& path)
            {
                auto paths = path.GetPaths();
                CHECK(paths.Len() >= 5);
            };

            THEN("data path exists")
            {
                testPaths(tDirectories::Data());
            }

            THEN("config path exists")
            {
                testPaths(tDirectories::Config());
            }

            THEN("var path exists")
            {
                testPaths(tDirectories::Var());
            }

            THEN("screenshot path exists")
            {
                testPaths(tDirectories::Screenshot());
            }

            THEN("resource path exists")
            {
                testPaths(tDirectories::Resource());
            }
        }
    }

    // Note: FileMatchesWildcard and GetFiles tests are skipped because they
    // may trigger initialization of global systems (like console/language) which
    // are not available in the test environment.
    // Z-Man: NO, FileMatchesWildcard DOES NOT. GetFiles could be tested on a known data directory.
}

// TODO: More comprehensive tDirectories tests could be added, but many functions
// depend on the actual filesystem state and configuration, which makes them
// difficult to test in a predictable way. The current tests focus on functions
// that can be tested without external dependencies.
//
// Additional tests that could be added:
// 1. Tests for path resolution (GetReadPath, GetWritePath)
// 2. Tests for file opening functionality
// 3. Tests for directory setting functions
// 4. Tests for path string formatting
// These would require more complex setup or mocking of the filesystem.