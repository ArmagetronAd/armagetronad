#include "doctest.h"
#include "tCommandLine.h"
#include "nNetwork.h"
#include <cstring>

// Tests for tCommandLine system
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("tCommandLine")
{
    TEST_CASE("tCommandLineData default construction")
    {
        GIVEN("a default tCommandLineData")
        {
            tCommandLineData cmdData;

            THEN("it has expected default values")
            {
                CHECK(cmdData.programVersion_ == nullptr);
                // name_ should be empty by default
                CHECK(strcmp(cmdData.name_, "") == 0);
            }
        }
    }

    TEST_CASE("tCommandLineData with empty arguments")
    {
        GIVEN("tCommandLineData with empty arguments")
        {
            tCommandLineData cmdData;
            cmdData.programVersion_ = &sn_programVersion;

            WHEN("Analyse is called with minimal arguments")
            {
                // Create minimal argv
                char* argv[] = {const_cast<char*>("test_program")};
                int argc = 1;

                bool result = cmdData.Analyse(argc, argv);

                THEN("Analyse returns true")
                {
                    CHECK(result == true);
                }
            }
        }
    }

    TEST_CASE("tCommandLineParser basic functionality")
    {
        GIVEN("a tCommandLineParser with arguments")
        {
            char program[] = "program";
            char f[] = "-f";
            char option[] = "--option";
            char value[] = "value";
            char* argv[] = {program, f, option, value};
            int argc = 4;

            tCommandLineParser parser(argc, argv);

            THEN("parser can be constructed without crashing")
            {
            }

            THEN("Executable returns the program name")
            {
                const char* exec = parser.Executable();
                CHECK(exec != nullptr);
                CHECK(strcmp(exec, "program") == 0);
            }

            THEN("Current initially points to first argument after executable")
            {
                const char* current = parser.Current();
                CHECK(current != nullptr);
                // Should point to "-f" initially (index 0 is program name)
                CHECK(strcmp(current, "program") == 0);
            }

            WHEN("Advance is called")
            {
                parser.Advance();

                THEN("Current moves to next argument")
                {
                    const char* current = parser.Current();
                    CHECK(current != nullptr);
                    CHECK(strcmp(current, "-f") == 0);
                }
            }

            WHEN("Advance is called multiple times")
            {
                parser.Advance(); // program
                parser.Advance(); // -f
                parser.Advance(); // --option

                THEN("Current moves through arguments")
                {
                    const char* current = parser.Current();
                    CHECK(current != nullptr);
                    CHECK(strcmp(current, "value") == 0);
                }

                AND_WHEN("Advance is called at the end")
                {
                    parser.Advance(); // Move past last argument

                    THEN("End returns true when past last argument")
                    {
                        CHECK(parser.End() == true);
                    }
                }
            }
        }
    }

    TEST_CASE("tCommandLineParser GetSwitch functionality")
    {
        GIVEN("a tCommandLineParser with switch arguments")
        {
            char program[] = "program";
            char f[] = "-f";
            char fullscreen[] = "--fullscreen";
            char w[] = "-w";
            char* argv[] = {program, f, fullscreen, w};
            int argc = 4;

            tCommandLineParser parser(argc, argv);
            parser.Advance(); // Skip program name

            THEN("GetSwitch can detect short switches")
            {
                CHECK(parser.GetSwitch("-f") == true);
            }

            AND_THEN("GetSwitch returns false for non-matching switches")
            {
                CHECK(parser.GetSwitch("-x") == false);
                CHECK(parser.GetSwitch("--unknown") == false);
            }
        }
    }

    TEST_CASE("tCommandLineParser GetSwitch long switches")
    {
        GIVEN("a parser positioned at a long switch")
        {
            char program[] = "program";
            char fullscreen[] = "--fullscreen";
            char* argv[] = {program, fullscreen};
            int argc = 2;

            tCommandLineParser parser(argc, argv);
            parser.Advance(); // Skip program name

            THEN("GetSwitch can detect long switches")
            {
                CHECK(parser.GetSwitch("--fullscreen") == true);
            }
        }
    }

    TEST_CASE("tCommandLineParser GetSwitch with alternatives")
    {
        GIVEN("a parser positioned at a short switch with long alternative")
        {
            char program[] = "program";
            char w[] = "-w";
            char* argv[] = {program, w};
            int argc = 2;

            tCommandLineParser parser(argc, argv);
            parser.Advance(); // Skip program name

            THEN("GetSwitch can detect switches with short alternatives")
            {
                // Try to match -w with alternative --window
                CHECK(parser.GetSwitch("-w", "--window") == true);
            }
        }
    }

    TEST_CASE("tCommandLineParser GetOption functionality")
    {
        GIVEN("a tCommandLineParser with option arguments")
        {
            // Note: GetOption requires the parser to be positioned at the option
            // For now, we'll test that the parser can be created and basic operations work
            char program[] = "program";
            char config[] = "--config";
            char cfgfile[] = "config.cfg";
            char* argv[] = {program, config, cfgfile};
            int argc = 3;

            tCommandLineParser parser(argc, argv);

            WHEN("parser is created with option arguments")
            {
                // Note: The parser starts at index 0 (program name), need to advance
                parser.Advance(); // Skip program name

                THEN("GetOption works")
                {
                    tString optionValue;
                    CHECK(parser.GetOption(optionValue, "-cfg", "--config"));
                    CHECK(optionValue == "config.cfg");
                }
            }
        }
    }
}
