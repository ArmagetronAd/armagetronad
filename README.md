# Armagetron Advanced

## Easy Installation Instructions

You may have just stumbled here into this git repository by accident and just want to play the game. 
If that is the case, you do not need to bother with the source code here. 
The easiest ways to get the game running are over 
[Steam, it is called Retrocycles there](https://store.steampowered.com/app/1306180/Retrocycles/) and 
[Itch.io](https://armagetronad.itch.io/armagetronad). Grab current release downloads from [the project website](https://www.armagetronad.org/downloads.php).

There are more installation paths; check them out on [the download archive](https://download.armagetronad.org/docs/install/).

## User Documentation

All the documentation for Armagetron is now in the src/doc/ directory and in HTML.
Starting point is index.html. In source distributions, all these files are in
a homemade template format using the m4 macro language. 

In installed version, the documentation directory is usually located in /usr/local/games/armagetronad.

Better documentation is on our [Wiki](https://wiki.armagetronad.org/index.php?title=PlayingGettingStarted).

## Community

- Most of the activity nowadays is on Discord, use [this invite](https://discord.gg/dcpaauj).
- Much slower: [The Forums](https://forums3.armagetronad.net/).
- In IRC, some of the old guard are in the channel `#armagetron` on the OFTC network. Have patience there, responses can take awhile.

## Developer Documentation

### Unix Systems (Linux, *BSD, macOS)

This project uses autoconf/automake. Run `.\bootstrap.sh` to generate the scripts, then `configure` as appropriate. The most important argument is `--enable-dedicated`, which switches compilation from the default client to the dedicated server.

If you do do not mind opinionated builds, run `./batch/test_builds.sh default`, this makes server and client builds in the `build` subdirectory. Check the script for more options.

Read more on dependencies you may need [Wiki](https://wiki.armagetronad.org/index.php?title=Linux_Development).

### Windows

Too much for this readme, check the [Wiki](https://wiki.armagetronad.org/index.php?title=Windows_Development).

### IDEs

You don't need any specific IDE for development, builds can be done from the command line and you can use whatever editor you like (Exception: Windows, see above). We do have some support to make setup easier:

#### Visual Studio Code/Codium

Check the sample configuration in `.vscode.example/`, the `README.md` in there explains how to activate it.

There is also a `devcontainer` setup that includes everything needed to develop the game [in a separate git](https://gitlab.com/zmanuel/armagetron-advanced-devcontainer). Running the client is probably not possible in there, but it can serve as a playpen for AI coding agents.

## Contributing

This is an open source game, contributions are generally welcome! Read [CONTRIBUTING.md](CONTRIBUTING.md) before jumping in head first.
