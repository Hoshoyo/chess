## Project

The project is just a simple chess interface in OpenGL with UDP server support.
The client is Windows only, the server has support for Linux and Windows.

## Loading Fen

A fen string can be specified through as the first command line argument within double quotes.
If nothing is specified, the interface starts with the default position.

```bash
chess.exe "rnbqkbnr/ppp2ppp/3p4/3Pp3/8/8/PPP1PPPP/RNBQKBNR w KQkq e6 0 3"
```

## Compile client (Windows only)

Visual Studio is necessary to compile the client.

- Open `x64 Native Tools Command Prompt for VS` in the project directory.
- Run the command `build.bat`.
- The executable will be built in the `bin\` directory and copied to the root.

## Compile server (Windows)

- Open `x64 Native Tools Command Prompt for VS` in the project `server/` directory.
- Run the command `build.bat`.
- The executable will be built in the `server\bin\` directory.

## Compile server (Linux)

- Have gcc and make installed.
- Run `make` in the `server/` directory.
- The executable will be built in the `server/` directory.

## Configuration file

The server, port and board background can be configured in the `config.txt` file.