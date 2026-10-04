# Visual Studio Code Configuration Starting Point

This is a sample configuration you can use to build and debug the game straight
from the source folder.

## Setup with customizations, but without updates

Run in the main source folder:

```bash
cp .vscode.example .vscode
```

This copies the configuration to where Visual Studio Code sees it, 
and you can customize it without risk of accidentally commiting your changes.

## Setup with updates, but no easy customizations

Link the folder instead:

```bash
ln -s .vscode.example .vscode
```

git updates will then also update your configuration. Any customizations you do
will go into git if you are not careful, though, but maybe that is what you want.

This is the configuration Z-Man currently is using for development on macOS and Linux.

## What it Provides

The configuration makes include directories known to the language server, so you
get meaningful code completion and instant linting.

You get tasks for building various configurations (client/server, debug/release).
The default build action builds both client and server in debug mode and runs all unit tests.

There is no special test configuration, the recommended "TestMate C++" works out of the box. 
It discovers the tests compiled by the build tasks and can automatically run them whenever they are rebuilt with "Continuous Run".
(The build tasks already fail on failing tests, but TestMate then tells you what failed where in the UI.)
Debugging individual tests from the test explorer works, but the extension will only ever run the pre-launch task of the first launch configuration; we chose this to be "Build Debug", so debugging both client and server in debug configuration works with a single click.

You get debug configurations that run the debug client/server in either record or playback mode.

The idea there is, since debugging a running game or even a client/server session is hard, that you just record the session without debugger, then play it back with the debugger.

## Code coverage

The debug builds also collect coverage data. With `make coverage`, you can create a html digest using lcov. Its intermediate files also
are consumed by the "Coverage Gutters" extension that can display coverage directly in the source editor.
