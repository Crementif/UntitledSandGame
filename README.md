![Tank Trap Banner](./dist/has-banner.png)
# Tank Trap
**Tank Trap**, formerly Untitled Sand Game, is a Wii U homebrew game with a simple premise. You control a drill tank in a multiplayer arena with dynamic, 2D physics-based terrain, inspired by games like Noita.

Navigate through sand and lava, collect (explosive) items to use against opponents, and watch as the landscape changes with each interaction.
Create traps and obstacles for your opponents to fall into, or use the terrain to burn your enemies in all the chaos.

A sandbox mode is also available for solo play.

## Features
 - Play Solo In Sandbox Mode
 - Battle Up To 10+ Players Online
 - Fully Multiplayer Physics Simulation
 - Random Item Pickups For Terrain Manipulation
 - Cool, Dynamic Shader Effects (made possible with [CafeGLSL](https://github.com/Exzap/CafeGLSL))
 - Works (only) on Wii U and Cemu

## How To Install
#### For Wii U:
 1. Download Tank Trap  
    a. Download the game from the Homebrew App Store
    b. Download the latest **.zip version** from the [releases page](https://github.com/Crementif/UntitledSandGame/release) and extract it to the root of the SD card
 2. Launch Tank Trap from your home screen or homebrew launcher.

#### For Cemu (2.0 only!):
 1. Download the latest **.wua version** from the [releases page](https://github.com/Crementif/UntitledSandGame/release) and move it to your PC's Wii U games folder
 2. Refresh Cemu's game list and it should appear! You can also load the .wua file file through the File->Load File menu option

## How To Play
 - Use the left stick to move your tank
 - Use the right stick to aim your drill
 - Press A to drill/jump
 - Press B to use your item
 - Press Select to toggle the debug overlay

<sub><sup>Secret cheat: In sandbox mode, having the debug overlay open allows you to keep your item after using it. Infinite items!</sup></sub>

## How To Compile

 - Install [devkitPRO](https://devkitpro.org/wiki/Getting_Started)
 - Install devkitPPC and wut through devkitPro's pacman by using `pacman -S wiiu-dev`
 - Run `make` in the root of the project directory

### Credits
 - [@aboood40091](https://github.com/aboood40091) for the original GX2 framework!
 - [@Maschell](https://github.com/Maschell) for RPL loading support on the Wii U!
 - [@GaryOderNichts](https://github.com/GaryOderNichts) for testing and helping with the shader compiler!

### License
This project is licensed under the [MIT License](./LICENSE.md).
The pixel font is licensed under [Creative Commons Attribution license](./source/assets/font/license.txt).