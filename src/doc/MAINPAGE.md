# Main Page
@mainpage

## Core Classes to Check Out

### For the Game
 - eGrid: Our spatial data structure. Well, planar. 
   It divides the plane into triangles, game objects move on the faces,
   the edges can be walls.
 - eGameObject: Base class for objects moving around the grid.
 - gCycle: Specialized game object representing a lightcycle.
 - gPlayerWall: The lightwalls the cycles leave behind.
 - gZone: Areas on the grid that have special effects.
 - gGame: Holds it all together.
 - All classes in \ref Game

### Administration
 - eTeam: Team of humans or AIs.
 - ePlayerNetID: Network visible player, organized in eTeam.
 - ePlayer: Local representation of a player, holds configuration such as keyboard settings.
 - All classes in \ref Engine

### Network
 - nMessage: Network message, sent between clients and server.
 - nNetObject: Network aware object, with sync methods.
 - All classes in \ref Network

### Other Modules
 - \ref Tools : Utility classes
 - \ref Render : Graphics output
 - \ref UI : User Interface

## More to Read
 - [How to Contribute](../../CONTRIBUTING.md)
 - [Main README.md](assemble/MAIN_README.md)
 - [Classes by Topic](topics.html)
