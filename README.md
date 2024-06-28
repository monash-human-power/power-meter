# Cycling power meter FYP
Oscar Varney and Jotham Gates, 2024

This is the code repository for an FYP to develop a crankset and power meter for Human Powered Vehicles (HPVs).

## Getting started
If you are a member of MHP, then the Notion page for this project can be found [here](https://www.notion.so/monashhumanpower/Power-Pedals-Cranks-FYP-3e6eb409a05642b1ad961b32c2f40aa7).

## Schematic and PCB layout
The schematic and PCB layout were completed using [KiCAD 8](https://www.kicad.org/). The design files are in the [`power-meter-pcb`](./power-meter-pcb/) folder.

## Code
The firmware is written to use the Arduino environment and is located in the [`power-meter-code`](power-meter-code) directory.

### IDE
Development was principally undertaken using VS Code, however the Arduino IDE can also be used to compile and upload this project to the device.

### Tools for updating dates of changed files and versions
A bash script has been written to update the dates on all modified or staged files (not including untracked files). Run `./tools/update_date.sh` to perform this operation.

Another script is included to update the version info on all files and definitions for the code. The files and elements updated include:
- Any `@version` tags in the code directory.
- The `VERSION` definition in [`defines.h`](./power-meter-code/defines.h).
- The Doxygen version number in [`Doxyfile`](./Doxyfile).

As an example:
```bash
./tools/update_version.sh 1.2.3
```

#### Git Hook for checking dates are correct before committing
Copy the [`pre-commit`](./tools/pre-commit) file into [`.git/hooks/pre-commit`](.git/hooks/pre-commit). This will check the date of code files and make sure they are up to date.

### Doxygen documentation
Documentation can be automatically generated using Doxygen by running `doxygen` from the root folder of the repository. You might need to install doxygen and graphviz first. The output will be placed in the [`docs`](docs) folder. GitHub actions may be set up later down the track to automatically generate and deploy to GitHub pages on commit- however that would require a public repository and documentation.

To generate a massive appendix for the report, run `./tools/generate_refman.sh`

The output file will be [`refman.pdf`](docs/latex/refman.pdf).