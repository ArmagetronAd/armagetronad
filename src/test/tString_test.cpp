#include "doctest.h"
#include "tString.h"
#include <cstring>

// Tests for tString class
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("tString")
{
    TEST_CASE("tString default construction")
    {
        GIVEN("a default tString")
        {
            tString s;
            // tString includes null terminator, so Len() is 1 for empty string
            THEN("length is at least 1 and content is empty")
            {
                CHECK(s.Len() >= 1);
                CHECK(strcmp(s, "") == 0);
            }
        }
    }

    TEST_CASE("tString construction from C string")
    {
        GIVEN("a tString constructed from C string")
        {
            tString s("Hello");
            THEN("content matches the C string")
            {
                CHECK(strcmp(s, "Hello") == 0);
                // Len includes null terminator
                CHECK(s.Len() == 6); // "Hello" + '\0'
            }
        }
    }

    TEST_CASE("tString copy construction")
    {
        GIVEN("a tString copy")
        {
            tString original("Hello");
            tString copy = original;
            THEN("copy matches the original")
            {
                CHECK(strcmp(copy, "Hello") == 0);
                CHECK(copy.Len() == original.Len());
            }
        }
    }

    TEST_CASE("tString assignment from C string")
    {
        GIVEN("a tString assigned from C string")
        {
            tString s;
            s = "World";
            THEN("content matches the C string")
            {
                CHECK(strcmp(s, "World") == 0);
                CHECK(s.Len() == 6); // "World" + '\0'
            }
        }
    }

    TEST_CASE("tString assignment operator")
    {
        GIVEN("a tString assigned from another tString")
        {
            tString original("Hello");
            tString assigned;
            assigned = original;
            THEN("assigned string matches the original")
            {
                CHECK(strcmp(assigned, "Hello") == 0);
                CHECK(assigned.Len() == original.Len());
            }
        }
    }

    TEST_CASE("tString operator+= with C string")
    {
        GIVEN("a tString for operator+=")
        {
            tString s("Hello");
            s += " World";
            THEN("concatenation produces correct result")
            {
                CHECK(strcmp(s, "Hello World") == 0);
                CHECK(s.Len() == 12); // "Hello World" + '\0'
            }
        }
    }

    TEST_CASE("tString operator+ with C string")
    {
        GIVEN("a tString for operator+")
        {
            tString s("Hello");
            tString result = s + " World";
            // Original should be unchanged
            THEN("concatenation produces correct result")
            {
                CHECK(strcmp(result, "Hello World") == 0);
                CHECK(result.Len() == 12); // "Hello World" + '\0'
                CHECK(strcmp(s, "Hello") == 0);
            }
        }
    }

    TEST_CASE("tString operator<< with C string")
    {
        GIVEN("a tString for operator<<")
        {
            tString s;
            s << "Hello";
            THEN("appended C string produces correct result")
            {
                CHECK(strcmp(s, "Hello") == 0);
                CHECK(s.Len() == 6);
            }
        }
    }

    TEST_CASE("tString operator<< with char")
    {
        GIVEN("a tString for operator<<")
        {
            tString s;
            s << 'H' << 'e' << 'l' << 'l' << 'o';
            THEN("appended char produces correct result")
            {
                CHECK(strcmp(s, "Hello") == 0);
                CHECK(s.Len() == 6);
            }
        }
    }

    TEST_CASE("tString operator+ with char")
    {
        GIVEN("a tString for operator+")
        {
            tString s("Hell");
            tString result = s + 'o';
            THEN("concatenation with char produces correct result")
            {
                CHECK(strcmp(result, "Hello") == 0);
                CHECK(result.Len() == 6);
            }
        }
    }

    TEST_CASE("tString comparison operators - equal")
    {
        GIVEN("two tStrings for equality comparison")
        {
            tString s("Hello");
            THEN("operator== returns correct result")
            {
                CHECK(s == "Hello");
                CHECK(s != "World");
            }
        }
    }

    TEST_CASE("tString comparison operators - less than")
    {
        GIVEN("two tStrings for less-than comparison")
        {
            tString s("Hello");
            THEN("operator< returns correct result")
            {
                CHECK(s < "World");
                CHECK(!(s < "Hello"));
            }
        }
    }

    TEST_CASE("tString comparison operators - greater than")
    {
        GIVEN("two tStrings for greater-than comparison")
        {
            tString s("World");
            THEN("operator> returns correct result")
            {
                CHECK(s > "Hello");
                CHECK(!(s > "World"));
            }
        }
    }

    TEST_CASE("tString Compare method")
    {
        GIVEN("two tStrings for Compare method")
        {
            tString s("Hello");
            THEN("Compare returns correct result")
            {
                CHECK(s.Compare("Hello") == 0);
                CHECK(s.Compare("World") < 0);
                CHECK(s.Compare("Aardvark") > 0);
            }
        }
    }

    TEST_CASE("tString SetPos")
    {
        GIVEN("A string")
        {
            tString s("Hello");

// test strings
#define expandedTo10 "Hello     "
            CHECK(strlen(expandedTo10) == 10);
#define shrunkTo3 "He "
            CHECK(strlen(shrunkTo3) == 3);
#define shrunkTo6 "Hello "
            CHECK(strlen(shrunkTo6) == 6);
#define shrunkTo5 "Hell "
            CHECK(strlen(shrunkTo5) == 5);

            WHEN("String is expanded with SetPos")
            {
                s.SetPos(10, false);
                THEN("It is expanded to that length with spaces")
                {
                    CHECK(s == expandedTo10);

                    WHEN("It is then shrunk to 3")
                    {
                        s.SetPos(3, true);

                        THEN("It is cut to that length with an extra trailing space")
                        {
                            CHECK(s == shrunkTo3);
                        }
                    }

                    WHEN("It is then shrunk to 6")
                    {
                        s.SetPos(6, true);

                        THEN("It is cut to that length, no extra space required")
                        {
                            CHECK(s == shrunkTo6);
                        }
                    }

                    WHEN("It is then shrunk, but with cut == false")
                    {
                        s.SetPos(3, false);

                        THEN("Nothing changes")
                        {
                            CHECK(s == expandedTo10);
                        }
                    }

                    WHEN("It is expanded again with the same call")
                    {
                        s.SetPos(10, false);

                        THEN("Nothing changes")
                        {
                            CHECK(s == expandedTo10);
                        }
                    }

                    WHEN("It is then shrunk to 0")
                    {
                        s.SetPos(0, true);

                        THEN("It vanishes")
                        {
                            CHECK(s == "");
                        }
                    }
                }
            }

            WHEN("It is then set to 5, its current length")
            {
                s.SetPos(5, false);

                THEN("The last character is replaced with a space")
                {
                    CHECK(s == shrunkTo5);
                }
            }
        }
    }

    TEST_CASE("tString StartsWith")
    {
        GIVEN("a tString for StartsWith")
        {
            tString s("Hello World");
            THEN("StartsWith returns correct result")
            {
                CHECK(s.StartsWith("Hello"));
                CHECK(s.StartsWith("Hello W"));
                CHECK(!s.StartsWith("World"));
                CHECK(!s.StartsWith("hello")); // case sensitive
            }
        }
    }

    TEST_CASE("tString StrPos")
    {
        GIVEN("a tString for StrPos")
        {
            tString s("Hello World");
            THEN("StrPos returns correct position")
            {
                CHECK(s.StrPos("World") == 6);
                CHECK(s.StrPos("Hello") == 0);
                CHECK(s.StrPos("lo Wo") == 3);
                CHECK(s.StrPos("NotFound") == -1);
            }
        }
    }

    TEST_CASE("tString SubStr with start and length")
    {
        tString s("Hello World");
        tString sub = s.SubStr(0, 5);
        CHECK(strcmp(sub, "Hello") == 0);

        sub = s.SubStr(6, 5);
        CHECK(strcmp(sub, "World") == 0);
    }

    TEST_CASE("tString SubStr with start only")
    {
        GIVEN("a tString for SubStr with start only")
        {
            tString s("Hello World");
            tString sub = s.SubStr(6);
            THEN("SubStr returns correct substring")
            {
                CHECK(strcmp(sub, "World") == 0);
            }
        }
    }

    TEST_CASE("tString toInt")
    {
        GIVEN("a tString for toInt")
        {
            tString s("42");
            THEN("toInt converts string to integer correctly")
            {
                CHECK(s.toInt() == 42);
            }
        }
    }

    TEST_CASE("tString conversion to const char*")
    {
        GIVEN("a tString for conversion to const char*")
        {
            tString s("Hello");
            const char* cstr = s;
            THEN("conversion to const char* works correctly")
            {
                CHECK(strcmp(cstr, "Hello") == 0);
            }
        }
    }

    TEST_CASE("tString boundary conditions - empty string")
    {
        GIVEN("an empty tString")
        {
            tString s;
            THEN("empty string behaves correctly")
            {
                CHECK(strcmp(s, "") == 0);
                // Len includes null terminator
                CHECK(s.Len() >= 1);
            }
        }
    }

    TEST_CASE("tString boundary conditions - single character")
    {
        GIVEN("a single-character tString")
        {
            tString s("X");
            THEN("single character string behaves correctly")
            {
                CHECK(strcmp(s, "X") == 0);
                // Len includes null terminator
                CHECK(s.Len() == 2);
            }
        }
    }

    TEST_CASE("tString concatenation of multiple strings")
    {
        GIVEN("multiple tStrings for concatenation")
        {
            tString s("Hello");
            s += " ";
            s += "World";
            THEN("concatenation produces correct result")
            {
                CHECK(strcmp(s, "Hello World") == 0);
                CHECK(s.Len() == 12); // "Hello World" + '\0'
            }
        }
    }

    TEST_CASE("tString operator+ with tString")
    {
        GIVEN("two tStrings for operator+")
        {
            tString s1("Hello");
            tString s2(" World");
            tString result = s1 + s2;
            THEN("concatenation produces correct result")
            {
                CHECK(strcmp(result, "Hello World") == 0);
                CHECK(result.Len() == 12);
            }
        }
    }

    TEST_CASE("tString operator+= with tString")
    {
        GIVEN("a tString for operator+=")
        {
            tString s1("Hello");
            tString s2(" World");
            s1 += s2;
            THEN("concatenation produces correct result")
            {
                CHECK(strcmp(s1, "Hello World") == 0);
                CHECK(s1.Len() == 12);
            }
        }
    }
}
