# Project Guidelines

## Overview
This project is a POSIX-compliant shell implementation in C.

## Project Structure
- `src/`: Core source code implementation.
  - `main.c`: Entry point.
  - `parser.c/h`: Command parsing logic.
  - `exec.c/h`: Command execution logic.
  - `builtins.c/h`: Built-in command implementation.
  - `completion.c/h`: Tab completion support.
  - `expand.c/h`: Shell expansion handling.
- `CMakeLists.txt`: Build system configuration.
- `Parsing.mmd`: Parsing logic diagram.
- `README.md`: Challenge overview and submission instructions.

## Development Standards
- **Memory Management**: Always pair allocation functions with corresponding `cleanup_*` functions (e.g., `init_command` with `cleanup_command`).
- **Code Organization**: Keep logic modular. Use clear separation between modules for parsing, execution, and built-ins.
- **Documentation**: 
  - Add Doxygen-style comments to all public-facing functions in header files (`.h`).
  - Prioritize documenting existing `TODO: DOCS` placeholders in header files.
  - Keep documentation updated as code changes.
- **Testing**: Ensure any new features or bug fixes are tested. Use `cmake` to build the project.

## Contribution Workflow
1. Follow existing code style (indentation, naming).
2. Ensure the code compiles before submitting.
3. Keep changes focused and modular.
