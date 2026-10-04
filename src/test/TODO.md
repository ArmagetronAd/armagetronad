# Bugs and stuff noticed while writing tests

## Bugs
Things that are definitely wrong and should be fixed:

## Oddities
Things that smell kind of bad, maybe have a look:
- tString::Len() includes the trailing \0, so is one more than one would expect.
- ePlayerNetId::LastActivity is badly named, it is the time SINCE the last activity, not the time OF last activity.

## TODOs
Tests we would like to have written, but could not yet:
- **tCommandLine**: The system uses global state and has complex dependencies that make isolated unit testing difficult. The parser functionality can be tested, but full command line execution requires initialization of other subsystems.
- **tEventQueue**: The queue system doesn't expose a public interface for adding events, making it difficult to test event management functionality in isolation. Only basic construction and Timestep with empty queues can be tested.
- **eTimer**: Tightly coupled with the network system through nNetObject inheritance, requiring network subsystem initialization for proper testing.
- **eGrid**: Complex system with dependencies on game objects and walls. Some functions can be tested in isolation (the constituents eFace and eEdge are needed). For others, we need mock walls and game objects. No biggie, we already have some in eGameObject_test.cpp.
- **ePath**: Make a valid one through the grid.
- **nSocket**: Low-level networking component requiring network subsystem initialization, platform-specific setup, and potentially privileged execution context.
- **tDirectories**: More tests are possible.
- **nNetwork**: More tests of individual classes are possible.
