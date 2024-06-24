# Cycling power meter FYP
Oscar Varney and Jotham Gates, 2024

This is the code repository for an FYP to develop a crankset and power meter for Human Powered Vehicles (HPVs).

## Getting started
If you are a member of MHP, then the Notion page for this project can be found [here](https://www.notion.so/monashhumanpower/Power-Pedals-Cranks-FYP-3e6eb409a05642b1ad961b32c2f40aa7).

## Schematic and PCB layout
The schematic and PCB layout were completed using [KiCAD 8](https://www.kicad.org/). The design files are in the [`power-meter-pcb`](./power-meter-pcb/) folder.

## Code
### Tool for updating dates of changed files
A bash script has been written to update the dates on all modified or staged files (not including untracked files). Run `./tools/update_date.sh` to perform this operation. If an argument is provided, i.e.
```bash
./tools/update_date.sh 0.1.0
```
The version field of all code files will be edited to match this.
