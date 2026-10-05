#include "doctest.h"
#include "tLocale.h"

#include "Statics.h"

// Tests for tLocale system
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("tLocale")
{
    TEST_CASE("tLanguage basic functionality")
    {
        GIVEN("tLanguage class")
        {
            THEN("FirstLanguage returns a language pointer")
            {
                tLanguage* lang = tLanguage::FirstLanguage();
                CHECK(lang);
            }

            THEN("FindStrict method exists")
            {
                auto* english = tLanguage::FindStrict(tString("British English"));
                CHECK(english);

#if false // this would log an error to console
                auto *nope = tLanguage::FindStrict(tString("nope1"));
                CHECK(!nope); // should be missing
#endif
            }

            THEN("FindSloppy method exists")
            {
                auto* english = tLanguage::FindSloppy(tString("American English"));
                CHECK(english);

                auto* nope = tLanguage::FindSloppy(tString("nope3"));
                CHECK(!nope); // should be missing
            }

            THEN("Find method exists")
            {
                auto* german = tLanguage::Find(tString("Deutsch"));
                CHECK(german);

                auto* nope = tLanguage::Find(tString("nope2"));
                CHECK(nope); // should be created
            }
        }
    }

    TEST_CASE("tOutput basic functionality")
    {
        GIVEN("tOutput class")
        {
            THEN("tOutput can be constructed and destroyed")
            {
                tOutput output2;
            }

            tOutput output;
            THEN("AddLiteral works")
            {
                output.AddLiteral("test");
                CHECK(tString(output) == "test");
            }

            THEN("AddSpace works")
            {
                output.AddSpace();
                CHECK(tString(output) == " ");
            }

            THEN("AddString works")
            {
                output.AddString("test string");
                CHECK(tString(output) == "test string");
            }

            THEN("Translations work")
            {
                output.AddString("$first_use_help");
                CHECK(tString(output) != "$first_use_help");                 // may require adaption if item changes
                CHECK(tString(output).StartsWith("Is this the first time")); // may require adaption if text changes
            }

            THEN("Parameters work")
            {
                output.SetTemplateParameter(1, "BLARG");
                output.SetTemplateParameter(2, "BLE");
                output.AddString("$player_teamkill");
                CHECK(tString(output).StartsWith("BLARG ")); // may require adaption if text changes
            }
        }
    }

    TEST_CASE("Language change")
    {
        GIVEN("German and English")
        {
            auto* english = tLanguage::FindSloppy(tString("American English"));
            CHECK(english);

            auto* german = tLanguage::Find(tString("Deutsch"));
            CHECK(german);

            tOutput output;
            THEN("English translation works")
            {
                english->SetFirstLanguage();

                output.SetTemplateParameter(1, "BLARG");
                output.SetTemplateParameter(2, 17); // other template parameter types work, too
                output.AddString("$player_teamkill");
                CHECK(tString(output) == "BLARG core dumped teammate 17! Boo! No points for that!\n");
            }

            THEN("German translation works")
            {
                german->SetFirstLanguage();

                // convenient constructor
                output = tOutput("$player_teamkill", "BLARG", "BLE");
                bool worked = tString(output).StartsWith("BLARG hat den Teampartner BLE beseitigt!");
                CHECK(worked);
                if (!worked)
                {
                    // print full string
                    CHECK(tString(output) == "");
                }

                // restore default for other tests
                english->SetFirstLanguage();
            }
        }
    }
}
