/*
Armagetron Advanced -- a Tron clone in 3D
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// This file demonstrates the *ideal* coding style for tests.

// Comments starting with `FYI` in them are meant as comments just to describe
// what is going on HERE, they are not meant as templates to include in actual code.

// FYI: own include comes first, so we know it works standalone
#include "CodingStyle.h"

#include "doctest.h"
#include "tDefer.h"

#include "tSysTime.h"

// FYI tests use BDD patterns whenever appropriate
TEST_SUITE("CodingStyle")
{
    // FYI TEST_SUITE and TEST_CASE use Title Case
    TEST_CASE("Counting Works")
    {
        // FYI The BDD macros use regular sentence case, starting with lower case
        GIVEN("a counter")
        {
            // FYI local varaibles are camelCase.
            cCounter counter; // the counter

            WHEN("left at default")
            {
                THEN("it starts at zero")
                {
                    CHECK(0 == counter.GetCount());
                }
                AND_THEN("it cannot count down")
                {
                    CHECK(!counter.TryCountDown());
                    CHECK(0 == counter.GetCount());
                }
            }

            WHEN("counting up")
            {
                counter.CountUp();

                THEN("it goes to one")
                {
                    CHECK(1 == counter.GetCount());
                }
                AND_THEN("it can count down")
                {
                    CHECK(counter.TryCountDown());
                    CHECK(0 == counter.GetCount());
                }
            }
        }
    }

    TEST_CASE("Shallow Copy")
    {
        GIVEN("a filled shallow copy holder")
        {
            {
                // we start and end with zero objects
                INVARIANT_CHECK(0 == cReferenceCounted::GetNumberOfObjects())

                cShallowCopy holder{new cReferenceCountedDerived};

                CHECK(1 == cReferenceCounted::GetNumberOfObjects());

                WHEN("doing nothing")
                {
                    THEN("we have one object")
                    {
                        CHECK(1 == cReferenceCounted::GetNumberOfObjects());
                    }
                }

                WHEN("making a copy")
                {
                    cShallowCopy copy{holder};

                    THEN("we still have one object")
                    {
                        CHECK(1 == cReferenceCounted::GetNumberOfObjects());
                    }
                    AND_WHEN("resetting the original")
                    {
                        holder.SetTarget(nullptr);

                        THEN("we still have one object")
                        {
                            CHECK(1 == cReferenceCounted::GetNumberOfObjects());
                        }
                        AND_WHEN("we also reset the copy")
                        {
                            copy.SetTarget(nullptr);

                            THEN("we have no objects left")
                            {
                                CHECK(0 == cReferenceCounted::GetNumberOfObjects());
                            }
                        }
                    }
                }
            }
        }
    }

    TEST_CASE("Deep Copy")
    {
        GIVEN("a filled deep copy holder")
        {
            // we start and end with zero objects (alternative manual version)
            CHECK(0 == cReferenceCounted::GetNumberOfObjects());
            auto guard = tDefer([]
                                { CHECK(0 == cReferenceCounted::GetNumberOfObjects()); });

            // FYI the Make(...) function is the equivalent to std::make_shared or std::make_unique
            auto referenceCounted = tRefPtr<cReferenceCountedDerived>::Make();

            cDeepCopy holder{std::move(referenceCounted)};

            // FYI the moved-from pointer should be zero now,
            // but do not rely on that in production code,
            // it is not strictly guaranteed (moved-from must be destructible, that is all)
            CHECK(!referenceCounted);
            referenceCounted = nullptr;

            CHECK(1 == cReferenceCounted::GetNumberOfObjects());

            WHEN("doing nothing")
            {
                THEN("we have one object")
                {
                    CHECK(1 == cReferenceCounted::GetNumberOfObjects());
                }
            }

            WHEN("making a copy")
            {
                cDeepCopy copy{holder};

                THEN("we have two derived objects")
                {
                    CHECK(2 == cReferenceCounted::GetNumberOfObjects());
                    CHECK(dynamic_cast<cReferenceCountedDerived*>(copy.GetTarget()));
                }
                AND_WHEN("resetting the original")
                {
                    holder.SetTarget(nullptr);

                    THEN("we have one object left")
                    {
                        CHECK(1 == cReferenceCounted::GetNumberOfObjects());
                    }
                    AND_WHEN("we also reset the copy")
                    {
                        copy.SetTarget(nullptr);

                        THEN("we have no objects left")
                        {
                            CHECK(0 == cReferenceCounted::GetNumberOfObjects());
                        }
                    }
                }
            }

            WHEN("moving the holder")
            {
                cDeepCopy copy{std::move(holder)};

                THEN("we still have one object")
                {
                    CHECK(1 == cReferenceCounted::GetNumberOfObjects());
                }
                AND_WHEN("resetting the original")
                {
                    holder.SetTarget(nullptr);

                    // FYI it is an implementation detail what would happen if we reset the copy instead.

                    THEN("we still have the same object")
                    {
                        CHECK(1 == cReferenceCounted::GetNumberOfObjects());
                    }
                    AND_WHEN("we also reset the copy")
                    {
                        copy.SetTarget(nullptr);

                        THEN("we have no objects left")
                        {
                            CHECK(0 == cReferenceCounted::GetNumberOfObjects());
                        }
                    }
                }
            }
        }
    }

    TEST_CASE("Flag Enums")
    {
        SUBCASE("Enum flag combinations are valid enum values")
        {
            cRandomStuff stuff;
            stuff.SetLights(cRandomStuff::TailLights | cRandomStuff::Underlighting);
            stuff.SetLights(~cRandomStuff::TailLights);
            stuff.SetLights(~cRandomStuff::TailLights & cRandomStuff::HeadLights);
            // no check, just see that it compiles
        }

        SUBCASE("'or' works")
        {
            auto const combo1 = cRandomStuff::TailLights | cRandomStuff::HeadLights;
            CHECK(3 == combo1);
            CHECK((cRandomStuff::HeadLights & combo1));
            CHECK((cRandomStuff::TailLights & combo1));
            CHECK_FALSE((cRandomStuff::Underlighting & combo1));
        }
        SUBCASE("'and' and 'complement' work")
        {
            auto const combo2 = (cRandomStuff::TailLights | cRandomStuff::HeadLights) & ~(cRandomStuff::HeadLights | cRandomStuff::Underlighting);
            CHECK(2 == combo2);
            CHECK(!(cRandomStuff::HeadLights & combo2));
            CHECK((cRandomStuff::TailLights & combo2));
            CHECK_FALSE((cRandomStuff::Underlighting & combo2));
        }
    }

    TEST_CASE("Time")
    {
        // FYI this test demonstrates how to advance time in tests without actually adding delays.
        auto timeStart = tSysTimeFloat(); // get time
        tMockAdvanceFrame(1);             // go forward one second
        auto timeEnd = tSysTimeFloat();   // get time again

        CHECK(timeEnd - timeStart >= doctest::Approx(1));
    }
}
