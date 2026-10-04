# src/test/ Directory

## Summary
Unit and integration tests for Armagetron Advanced components, including coding style demonstration.

**Doxygen Group**: `@defgroup Tests` - Unit and integration tests (defined in README.md)

## Details

The test directory contains test programs for verifying the correctness of Armagetron Advanced components. Tests cover parsing logic, data structures, geometry, memory management, and other critical functionality. **CodingStyle.h and CodingStyle_test.cpp demonstrate ideal coding style with functional, testable examples.**

Tests are compiled as separate programs that link against the relevant project libraries. The main test executable `unit_tests` combines most tests using the doctest framework, while `chat_prefix_test` is a legacy standalone test. Tests can be run independently or as part of the test suite during development and CI/CD pipelines.

## Directory Structure

```
.
├── README.md                 # Doxygen @defgroup Tests definition
├── TODO.md                   # Known bugs, oddities, and unwritten tests
├── CodingStyle.cpp             # Coding style demonstration implementation
├── CodingStyle.h               # Ideal coding style demonstration header
├── CodingStyle_test.cpp        # Tests for coding style examples
├── MockConsole.h               # Helper to capture/suppress console output
├── Statics.h                   # Static initialization helper
├── unit_tests_main.cpp         # Main test harness entry point
├── chat_prefix_test.cpp        # Legacy chat prefix parsing tests
├── configuration_integration_test.cpp
├── eAxis_test.cpp              # Axis class tests
├── eCoord_test.cpp             # Coordinate system tests
├── eGameObject_test.cpp        # Game object tests
├── eGrid_test.cpp              # Grid tests
├── ePath_test.cpp              # Path tests
├── ePlayerNetID_test.cpp       # Player network ID tests
├── ePlayer_test.cpp            # Player tests
├── eRectangle_test.cpp         # Rectangle geometry tests
├── eTeam_test.cpp              # Team tests
├── eTimer_test.cpp             # Timer tests
├── engine_integration_test.cpp
├── geometry_system_test.cpp
├── md5_test.cpp                # MD5 hashing tests
├── memory_management_test.cpp
├── nMessage_test.cpp           # Network message tests
├── nNetObject_test.cpp         # Network object tests
├── nNetwork_test.cpp           # Network tests
├── nServerInfo_test.cpp        # Server info tests
├── nSocket_test.cpp            # Socket tests
├── network_integration_test.cpp
├── performance_test.cpp
├── tArray_test.cpp             # Dynamic array tests
├── tCallbackString_test.cpp    # String callback tests
├── tCallback_test.cpp          # Callback system tests
├── tColor_test.cpp             # Color handling tests
├── tCommandLine_test.cpp       # Command line parsing tests
├── tConfiguration_test.cpp     # Configuration system tests
├── tConsole_test.cpp           # Console tests
├── tCrypt_test.cpp             # Cryptography tests
├── tDirectories_test.cpp       # Directory management tests
├── tEventQueue_test.cpp        # Event queue tests
├── tException_test.cpp         # Exception system tests
├── tHeap_test.cpp              # Heap memory tests
├── tLinkedList_test.cpp        # Linked list tests
├── tList_test.cpp              # List container tests
├── tLocale_test.cpp            # Localization tests
├── tMath_test.cpp              # Math utilities tests
├── tMemStack_test.cpp          # Memory stack tests
├── tRandomizer_test.cpp        # Randomization tests
├── tResourceManager_test.cpp   # Resource manager tests
├── tRing_test.cpp              # Ring buffer tests
├── tString_test.cpp            # String class tests
├── tSysTime_test.cpp           # System time tests
└── time_system_test.cpp
```

## Technologies

- **Language**: C++ (C++11 features)
- **Build System**: Autotools (integrated with main src/Makefile.am)
- **Test Framework**: doctest (primary), custom harness (legacy)
- **Dependencies**: project libraries (libtools, libnetwork, libengine, libui, librender, libtron)

## Coding Conventions

- **Test Style**: doctest framework for most tests, BDD patterns (GIVEN/WHEN/THEN) encouraged
- **Coding Style Reference**: CodingStyle.h demonstrates ideal patterns including Rule of Zero/Five, reference counting, enum flags, smart pointers (tRefPtr), const-correctness, noexcept, and more
- **Assertions**: doctest assertions and project's assertion macros (tASSERT, etc.)
- **Integration**: Tests link against actual project libraries
- **File Naming**: Test files follow `_test.cpp` naming convention

## Key Patterns

- Unit testing with doctest framework
- BDD-style test organization with GIVEN/WHEN/THEN
- Coding style examples serving as both documentation and testable code
- Direct library testing against production code
- Geometry and data structure verification
- Memory management testing with reference counting
- Exception safety testing

## Automated Tests

- `unit_tests`: Main test suite with doctest
  - **CodingStyle tests**: Demonstrates ideal coding patterns (cCounter, cReferenceCounted, smart pointers, enum flags) with BDD-style tests
  - Tests coordinate systems (eCoord, eAxis, eRectangle)
  - Tests data structures (tArray, tList, tLinkedList, tRing)
  - Tests memory management (tHeap, tMemStack)
  - Tests utilities (tString, tColor, tCallback, tRandomizer, tMath, tSysTime)
  - Tests exception handling
  - Tests command line parsing (tCommandLine)
  - Tests event queue system (tEventQueue)
  - Tests directory management (tDirectories)
  - Tests cryptographic utilities (MD5)
  - Tests engine components (eTimer, eGrid, ePath, ePlayer, eTeam, ePlayerNetID)
  - Tests network components (nSocket, nNetObject, nMessage, nServerInfo, nNetwork)
  - Tests configuration and resource management (tConfiguration, tResourceManager)
- `chat_prefix_test`: Legacy test for XML parsing
  - Tests parsing of player chat messages
  - Validates prefix formatting
  - Ensures proper handling of special characters

## Notes from Humans
### MAIN GUARDRAIL: The AI Agents keep out of this section.

### GUARDRAILS

- Do run `batch/test_builds.sh debug` to verify all tests pass.

### Guidelines

- Code touched by AI agents must be largely covered by automated tests.
- Employ Test Driven Development whenever appropriate.
- Changes to the tested code are discouraged when writing tests, but sometimes required. All changes should be reported to the user and be noted in commit messages.
- It is likely you will find classes that are not currently testable in isolation. Your choices then, before doing anything else:
  - Refactor them to make them testable. On legacy branches, only do this if it is possible with minimal changes.
  - Test them integrated with the parts they cannot be removed from.
- If not in conflict with user preferences, do the adaptations that make code testable in separate commits.
- **Test Framework**: Use doctest as the default test framework. Only write ad-hoc custom test programs when explicitly demanded by the user.
- Test files end in `_test.cpp`.
- If, during writing of tests, you find bugs or odd behavior, or cannot write a test because there is something blocking it, write them down in `TODO.md`.
- Tests should test runtime behavior.
- Tests generally **NEED** nontrivial `CHECK`s or `REQUIRE`s. Tautological checks like `CHECK(true)`, `CHECK(a == a)` and `REQUIRE(true)` are **outlawed**.
  - If the only thing a test can check is "yep, this operation did not crash", just write a comment to that effect.
- If a test you wrote **SHOULD** work, but does not because of some roadblock you currently have no control over, disable the test via `#if false`, describe the roadblock in a comment, and make a note in `TODO.md`.
- Tests and most of the actual program are single threaded; consider access to globals safe. We usually have methods available to reset them to the default state after tests.
- Keep tests for one class in a single file. Use multiple `TEST_SUITE`s to group tests if appropriate.

### Allowed Operations During Tests
- All regular in-memory operations
- File read access to known files from this repository
- Initialize static systems, such as network base, translation, directories (it's unfortunate that we have them, the main program should take care of the common ones)

### Avoid During Tests
- Console output; use a local `MockConsole` object to swallow most of it

### **Forbidden** Operations During Tests
- Network access
- File write access

### Do not Write These Nonsense Tests
- Existence of functions or variables
- "Correct" values for enums

### Test Coverage
- To collect test coverage, run `COVERAGE=2 ./batch/test_builds.sh server_debug`.
- Coverage files will be either in lcov digest format in `build/test_vs_server_debug/coverage/lcov.info` or raw gcov `.gcno/.gcda` files in `build/test_vs_server_debug/src` and subdirectories.
- We do not chase 100% test coverage, but the coverage data can be used to decide which test to write next, and, more importantly, to verify that **new** or **changed** code is covered.

### Remarks

- `chat_prefix_test` is just a relic, not actively used.
- CodingStyle.h and CodingStyle_test.cpp serve as both style documentation and functional tests. The classes defined in CodingStyle.h (cCounter, cReferenceCounted, cShallowCopy, cDeepCopy, cRandomStuff) are fully functional and demonstrate proper use of project patterns.
