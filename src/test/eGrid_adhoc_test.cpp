// eGrid_adhoc_test.cpp - Tests converted from src/engine/test.cpp
// This file contains doctest-based tests for eGrid functionality
// Original: src/engine/test.cpp

#include "eGrid.h"
#include "doctest.h"

// ============================================================================
// TEST_CASE: eGrid Creation and Initial Validation
// BDD: GIVEN empty state, WHEN create grid, THEN check passes
// Original: lines 33-37 in test.cpp
// ============================================================================
TEST_CASE("eGrid: Creation and Initial Validation") {
    // GIVEN: empty state
    tStackObject< eGrid > grid;
    
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
    tStackObject< eGrid > grid;
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
    // GIVEN: grid
    tStackObject< eGrid > grid;
    grid.Create();
    
    // WHEN: draw lines for i=2 (first iteration)
    ePoint *p = grid.Insert(eCoord(0, 0));
    p = grid.DrawLine(p, eCoord(1000 + 2*2, 2), NULL);
    REQUIRE(p != NULL);
    
    p = grid.DrawLine(p, eCoord(10 + 2*2, 10 + 2), NULL);
    REQUIRE(p != NULL);
    
    p = grid.DrawLine(p, eCoord(-10 + 2*2, 10 + 2), NULL);
    REQUIRE(p != NULL);
    
    p = grid.DrawLine(p, eCoord(-10 + 2*2, -10 + 2), NULL);
    REQUIRE(p != NULL);
    
    p = grid.DrawLine(p, eCoord(-1000 + 2*2, 1000 + 2), NULL);
    REQUIRE(p != NULL);
    
    p = grid.DrawLine(p, eCoord(10, 500 + 2), NULL);
    REQUIRE(p != NULL);
    
    p = grid.DrawLine(p, eCoord(10, 0 + 2), NULL);
    REQUIRE(p != NULL);
    
    p = grid.DrawLine(p, eCoord(10, 700 + 2), NULL);
    REQUIRE(p != NULL);
    
    p = grid.DrawLine(p, eCoord(10, 10 + 2), NULL);
    REQUIRE(p != NULL);
    
    // THEN: all operations succeed and grid is valid
    CHECK_NOTHROW(grid.Check());
    
    // Repeat for i=1
    grid.Clear();
    p = grid.Insert(eCoord(0, 0));
    p = grid.DrawLine(p, eCoord(1000 + 2*1, 1), NULL);
    p = grid.DrawLine(p, eCoord(10 + 2*1, 10 + 1), NULL);
    p = grid.DrawLine(p, eCoord(-10 + 2*1, 10 + 1), NULL);
    p = grid.DrawLine(p, eCoord(-10 + 2*1, -10 + 1), NULL);
    p = grid.DrawLine(p, eCoord(-1000 + 2*1, 1000 + 1), NULL);
    p = grid.DrawLine(p, eCoord(10, 500 + 1), NULL);
    p = grid.DrawLine(p, eCoord(10, 0 + 1), NULL);
    p = grid.DrawLine(p, eCoord(10, 700 + 1), NULL);
    p = grid.DrawLine(p, eCoord(10, 10 + 1), NULL);
    CHECK_NOTHROW(grid.Check());
    
    // Repeat for i=0
    grid.Clear();
    p = grid.Insert(eCoord(0, 0));
    p = grid.DrawLine(p, eCoord(1000 + 2*0, 0), NULL);
    p = grid.DrawLine(p, eCoord(10 + 2*0, 10 + 0), NULL);
    p = grid.DrawLine(p, eCoord(-10 + 2*0, 10 + 0), NULL);
    p = grid.DrawLine(p, eCoord(-10 + 2*0, -10 + 0), NULL);
    p = grid.DrawLine(p, eCoord(-1000 + 2*0, 1000 + 0), NULL);
    p = grid.DrawLine(p, eCoord(10, 500 + 0), NULL);
    p = grid.DrawLine(p, eCoord(10, 0 + 0), NULL);
    p = grid.DrawLine(p, eCoord(10, 700 + 0), NULL);
    p = grid.DrawLine(p, eCoord(10, 10 + 0), NULL);
    CHECK_NOTHROW(grid.Check());
}

// ============================================================================
// TEST_CASE: eGrid Final State Validation
// BDD: GIVEN modified grid, WHEN check and clear, THEN no memory leaks
// Original: line 68 in test.cpp
// ============================================================================
TEST_CASE("eGrid: Final State Validation") {
    // GIVEN: modified grid
    tStackObject< eGrid > grid;
    grid.Create();
    
    ePoint *p = grid.Insert(eCoord(0, 0));
    p = grid.DrawLine(p, eCoord(100, 100), NULL);
    
    // WHEN: check and clear
    CHECK_NOTHROW(grid.Check());
    CHECK_NOTHROW(grid.Clear());
    
    // THEN: no memory leaks (implicit via tStackObject)
    // If we get here without crashing, the clear succeeded
}
