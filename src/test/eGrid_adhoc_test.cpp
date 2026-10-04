// eGrid_adhoc_test.cpp - Tests converted from src/engine/test.cpp
// This file contains doctest-based tests for eGrid functionality
// Purpose: Stress test the grid engine by drawing random patterns
//   not very targeted...
// Original: src/engine/test.cpp

#include "doctest.h"

#include "eGrid.h"

#include "MockConsole.h"

/// Better cleanup than plain tStackObject<eGrid>
class eStackGrid: public tStackObject<eGrid>
{
public:
    ~eStackGrid()
    {
        Clear();
    }
};

// ============================================================================
// TEST_CASE: eGrid Creation and Initial Validation
// BDD: GIVEN empty state, WHEN create grid, THEN check passes
// Original: lines 33-37 in test.cpp
// ============================================================================
TEST_CASE("eGrid: Creation and Initial Validation") {
    // GIVEN: empty state
    eStackGrid grid;
    
    // WHEN: create grid
    grid.Create();
    
    // THEN: check passes (should not throw)
    CHECK_NOTHROW(grid.Check());
    CHECK_NOTHROW(grid.Check()); // called twice in original
}

// ============================================================================
// TEST_CASE: eGrid Simplification Operations
// BDD: GIVEN grid with lines, WHEN simplify all, THEN grid remains valid
// Original: lines 40-41, 68 in test.cpp
// Note: This is extracted from the loop structure
// ============================================================================
TEST_CASE("eGrid: Simplification Operations") {
    // GIVEN: grid with some lines
    eStackGrid grid;
    grid.Create();
    
    ePoint *p = grid.Insert(eCoord(0, 0));
    p = grid.DrawLine(p, eCoord(1000, 0), NULL);
    
    // WHEN: simplify all
    CHECK_NOTHROW(grid.SimplifyAll(10));
    
    // THEN: grid remains valid
    CHECK_NOTHROW(grid.Check());
}

// ============================================================================
// TEST_CASE: eGrid Line Drawing Operations
// BDD: GIVEN grid, WHEN draw various lines, THEN all lines added correctly
// Original: lines 42-66 in test.cpp
// Note: This tests the line drawing loop with i=2,1,0
// ============================================================================
TEST_CASE("eGrid: Line Drawing Operations") {
    MockConsole con;

    // GIVEN: grid
    eStackGrid grid;
    grid.Create();
    
    for (int i=2;i>=0;i--)
    {
        grid.SimplifyAll(10);

        ePoint *p =grid.Insert(eCoord(0,0));
        p = grid.DrawLine(p, eCoord(1000+2*i,i), NULL);
        REQUIRE(p);

#ifdef DEBUG
        if (i == -1)
        {
            grid.doCheck = true;
            grid.Check();
        }
#endif


        p = grid.DrawLine(p, eCoord(10+2*i,10+i), NULL);
        REQUIRE(p);
        p = grid.DrawLine(p, eCoord(-10+2*i,10+i), NULL);
        REQUIRE(p);
        p = grid.DrawLine(p, eCoord(-10+2*i,-10+i), NULL);
        REQUIRE(p);
        p = grid.DrawLine(p, eCoord(-1000+2*i,1000+i), NULL);
        REQUIRE(p);
        p = grid.DrawLine(p, eCoord(10,500+i), NULL);
        REQUIRE(p);
        p = grid.DrawLine(p, eCoord(10,0+i), NULL);
        REQUIRE(p);
        p = grid.DrawLine(p, eCoord(10,700+i), NULL);
        REQUIRE(p);
        p = grid.DrawLine(p, eCoord(10,10+i), NULL);
        REQUIRE(p);
    }

    CHECK_NOTHROW(grid.Check());
}

// ============================================================================
// TEST_CASE: eGrid Final State Validation
// BDD: GIVEN modified grid, WHEN check and clear, THEN no memory leaks
// Original: line 68 in test.cpp
// ============================================================================
TEST_CASE("eGrid: Final State Validation") {
    // GIVEN: modified grid
    tStackObject<eGrid> grid;
    grid.Create();
    
    ePoint *p = grid.Insert(eCoord(0, 0));
    p = grid.DrawLine(p, eCoord(100, 100), NULL);
    
    // WHEN: check and clear
    CHECK_NOTHROW(grid.Check());
    CHECK_NOTHROW(grid.Clear());
    
    // THEN: no memory leaks (implicit via tStackObject)
    // If we get here without crashing, the clear succeeded
}
