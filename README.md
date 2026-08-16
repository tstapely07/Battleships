# Battleships in C

## Background
This repository contains the multiplayer game extension from a larger university systems project (which also involved building an ARM emulator, a two-pass assembler, and bare-metal Raspberry Pi GPIO programs). 

To comply with academic integrity rules regarding the coursework's starter code, only the Battleships extension is published here.

**Note on Commit History:** As required by our university conventions, all commit messages in this repository are prepended with the author's name.

## Overview
A fully playable, networked multiplayer Battleships game written in C. It uses an authoritative server and supports both a graphical client (Raylib/OpenGL) and a terminal-based CLI client.

## Architecture
* **Client-Server Model:** The server is the single source of truth. It validates all moves and manages the board state to prevent cheating.
* **Custom Binary Protocol:** Network communication runs over non-blocking TCP sockets using a custom binary protocol. Every transmission uses a fixed header and message-specific payload to prevent fragmented reads.
* **Dual Clients:** Game state and networking logic are shared, making it easy to swap between the Raylib GUI and the CLI fallback.
* **State Encapsulation:** The board is completely encapsulated using an opaque pointer (`typedef struct Board *Board`). This hides the internal memory layout from the networking and rendering layers.

---

## Building and Running

### 1. Clone
This project uses `raylib` and `raygui`, which are included as Git submodules. You need to clone the repository recursively:

```bash
git clone --recurse-submodules https://github.com/tstapely/Battleships.git
```
*(If you already cloned the repo normally, fetch the submodules by running `git submodule update --init --recursive`)*

### 2. Install Dependencies (Linux)
On Linux systems (like Debian/Ubuntu), Raylib requires several libraries for audio, graphics, and windowing. Install them using your package manager:

```bash
sudo apt update
sudo apt install libasound2-dev mesa-common-dev libx11-dev libxrandr-dev libxi-dev xorg-dev libgl1-mesa-dev libglu1-mesa-dev
```

### 3. Compile Raylib
Before compiling the game, you must manually build the Raylib library from the submodule:

```bash
cd Battleships/raylib/src
make
cd ../../extension
```

### 4. Compile the Game
The project uses a standard `Makefile` in the `extension/` directory. Compile the server and both clients by running:

```bash
make
```
This generates the `server`, `client-gui`, and `client-cli` executables.

### 5. Usage
You need to start the server before connecting clients.

**Start the Server:**
```bash
./server [-p PORT | -l LOGFILE]
```
*(Defaults to port 8080)*

**Start a Client:**
Open a new terminal and run either the GUI or CLI client:

```bash
./client-gui [-p PORT | -h HOST | -l LOGFILE]
```
*or*
```bash
./client-cli [-p PORT | -h HOST | -l LOGFILE]
```
*(Defaults to connecting to `localhost`)*

---

## License
This code is tied to university coursework and is provided here strictly for portfolio purposes. 

All rights reserved. You are welcome to review the code, but please do not copy, distribute, or reuse it.
