# Extension (Battleships)

## Init

Clone as normal, then in the extension directory run the following command:

`git submodule update --init --recursive`

If *not* running on a lab machine (eg. under Ubuntu on WSL), run the following command to install the Raylib dependencies:

`sudo apt install libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev`

Then cd to the raylib/src folder. Compile Raylib:

`make`

## Compile

`make server` - compiles the dedicated server to bin/server

`make client-cli` - compiles the CLI client to bin/client-cli

`make client-gui` - compiles the GUI client to bin/client-cli

`make all` - compiles all of the above
