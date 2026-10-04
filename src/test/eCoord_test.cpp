#include "doctest.h"
#include "eCoord.h"

// Tests for eCoord class
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("eCoord")
{
    TEST_CASE("eCoord construction and default values")
    {
        GIVEN("a default eCoord")
        {
            eCoord defaultCoord;
            THEN("its coordinates are zero")
            {
                CHECK(defaultCoord.x == 0);
                CHECK(defaultCoord.y == 0);
            }
        }

        GIVEN("an eCoord with explicit coordinates")
        {
            eCoord explicitCoord(1.0f, 2.0f);
            THEN("the coordinates match the constructor arguments")
            {
                CHECK(explicitCoord.x == 1.0f);
                CHECK(explicitCoord.y == 2.0f);
            }
        }

        GIVEN("an eCoord with a single argument")
        {
            eCoord singleArg(5.0f);
            THEN("x is set and y defaults to zero")
            {
                CHECK(singleArg.x == 5.0f);
                CHECK(singleArg.y == 0);
            }
        }
    }

    TEST_CASE("eCoord copy for copy construction and assignment")
    {
        GIVEN("an eCoord for copy construction and assignment")
        {
            eCoord original(3.0f, 4.0f);
            eCoord copy = original;
            THEN("the copy has the same coordinates as the original")
            {
                CHECK(copy.x == original.x);
                CHECK(copy.y == original.y);
            }

            eCoord assigned(0, 0);
            assigned = original;
            THEN("the assigned eCoord has the same coordinates as the original")
            {
                CHECK(assigned.x == original.x);
                CHECK(assigned.y == original.y);
            }
        }
    }

    TEST_CASE("eCoord arithmetic operators")
    {
        GIVEN("two eCoords for arithmetic operations")
        {
            eCoord a(1.0f, 2.0f);
            eCoord b(3.0f, 4.0f);

            // Addition
            eCoord sum = a + b;
            THEN("addition produces correct results")
            {
                CHECK(sum.x == 4.0f);
                CHECK(sum.y == 6.0f);
            }

            // Subtraction
            eCoord diff = a - b;
            THEN("subtraction produces correct results")
            {
                CHECK(diff.x == -2.0f);
                CHECK(diff.y == -2.0f);
            }

            // Negation
            eCoord neg = -a;
            THEN("negation produces correct results")
            {
                CHECK(neg.x == -1.0f);
                CHECK(neg.y == -2.0f);
            }

            // Scalar multiplication
            eCoord scaled = a * 2.0f;
            THEN("scalar multiplication produces correct results")
            {
                CHECK(scaled.x == 2.0f);
                CHECK(scaled.y == 4.0f);
            }

            // Scalar multiplication assignment
            eCoord a2(1.0f, 2.0f);
            a2 *= 3.0f;
            THEN("scalar multiplication assignment produces correct results")
            {
                CHECK(a2.x == 3.0f);
                CHECK(a2.y == 6.0f);
            }
        }
    }

    TEST_CASE("eCoord norm calculations")
    {
        GIVEN("an eCoord for norm calculations")
        {
            eCoord v(3.0f, 4.0f);

            // NormSquared
            REAL normSq = v.NormSquared();
            THEN("NormSquared returns the squared magnitude")
            {
                CHECK(normSq == 25.0f);
            }

            // Norm
            REAL norm = v.Norm();
            THEN("Norm returns the magnitude")
            {
                CHECK(norm == doctest::Approx(5.0f));
            }
        }
    }

    TEST_CASE("eCoord scalar product (F)")
    {
        GIVEN("two eCoords for scalar product")
        {
            eCoord a(1.0f, 2.0f);
            eCoord b(3.0f, 4.0f);

            REAL dot = eCoord::F(a, b);
            THEN("F returns the dot product")
            {
                CHECK(dot == 11.0f); // 1*3 + 2*4 = 11
            }
        }
    }

    TEST_CASE("eCoord cross product operator*")
    {
        GIVEN("two eCoords for cross product")
        {
            eCoord a(1.0f, 0.0f);
            eCoord b(0.0f, 1.0f);

            // Cross product in 2D: -x*a.y + y*a.x
            REAL cross = a * b;
            THEN("operator* returns the 2D cross product")
            {
                CHECK(cross == -1.0f); // 1*1 + 0*0 with the negative sign = -1
            }

            eCoord c(1.0f, 1.0f);
            eCoord d(1.0f, 1.0f);
            THEN("operator* returns zero for parallel vectors")
            {
                CHECK((c * d) == 0.0f); // parallel vectors
            }
        }
    }

    TEST_CASE("eCoord Turn (complex multiplication)")
    {
        GIVEN("an eCoord for complex multiplication rotation")
        {
            eCoord v(1.0f, 0.0f);
            // Turn by 90 degrees (0,1 is unit vector at 90 degrees)
            eCoord turned = v.Turn(eCoord(0.0f, 1.0f));
            THEN("Turn by 90 degrees rotates correctly")
            {
                CHECK(turned.x == doctest::Approx(0.0f));
                CHECK(turned.y == doctest::Approx(1.0f));
            }

            // Turn by -90 degrees (0,-1)
            eCoord turnedNeg = v.Turn(eCoord(0.0f, -1.0f));
            THEN("Turn by -90 degrees rotates correctly")
            {
                CHECK(turnedNeg.x == doctest::Approx(0.0f));
                CHECK(turnedNeg.y == doctest::Approx(-1.0f));
            }
        }
    }

    TEST_CASE("eCoord Conj (complex conjugation)")
    {
        GIVEN("an eCoord for complex conjugation")
        {
            eCoord v(1.0f, 2.0f);
            eCoord conj = v.Conj();
            THEN("Conj negates the imaginary component")
            {
                CHECK(conj.x == 1.0f);
                CHECK(conj.y == -2.0f);
            }
        }
    }

    TEST_CASE("eCoord V function")
    {
        GIVEN("three eCoords for V function")
        {
            // V(a, b, c) gives X coordinate of b in an orthogonal coordinate system when a is at (0,0) and c is at (1,0)
            eCoord a(0.0f, 0.0f);
            eCoord b(1.0f, 0.0f);
            eCoord c(2.0f, 0.0f);

            REAL result = eCoord::V(a, b, c);
            THEN("V returns the normalized coordinate")
            {
                CHECK(result == doctest::Approx(0.5f));
            }
        }
    }

    TEST_CASE("eCoord equality operators")
    {
        GIVEN("eCoords for equality comparison")
        {
            eCoord a(1.0f, 2.0f);
            eCoord b(1.0f, 2.0f);

            // Equality with EPS tolerance
            THEN("operator== returns true for equal coordinates")
            {
                CHECK(a == b);
            }

            // Test inequality
            eCoord c(3.0f, 4.0f);
            THEN("operator!= returns true for different coordinates")
            {
                CHECK(a != c);
            }
        }
    }

    TEST_CASE("eCoord se_EstimatedRangeOfMult")
    {
        GIVEN("two eCoords for se_EstimatedRangeOfMult")
        {
            eCoord a(1.0f, 2.0f);
            eCoord b(3.0f, 4.0f);

            REAL range = se_EstimatedRangeOfMult(a, b);
            THEN("se_EstimatedRangeOfMult returns a positive value")
            {
                // Just document that it returns a sensible positive value, exact value does not matter much
                CHECK(range >= 3);
                CHECK(range <= 30);
            }
        }
    }

    TEST_CASE("eCoord st_GetDifference")
    {
        GIVEN("two distinct eCoords for st_GetDifference")
        {
            eCoord a(1.0f, 2.0f);
            eCoord b(2.0f, 2.0f);

            REAL diff = st_GetDifference(a, b);
            THEN("st_GetDifference returns a non-negative value")
            {
                CHECK(diff > 0);
            }
        }

        GIVEN("two equal eCoords for st_GetDifference")
        {
            eCoord a(1.0f, 2.0f);
            eCoord b(1.0f, 2.0f);

            REAL diff = st_GetDifference(a, b);
            THEN("st_GetDifference returns null")
            {
                CHECK(diff == doctest::Approx(0.0));
            }
        }
    }
}
