# Solar System Simulator

An infinitely scalable, procedurally generated N-body simulation of the Solar System built from scratch in C++ and pure OpenGL.

## Features
- **Accurate N-body Physics:** Tracks the gravitational interactions of planets. Features both purely Newtonian physics ($1/r^2$) and an unconditionally stable Einstein General Relativity (GR) 1st Post-Newtonian ($1/r^4$) simulator for testing perihelion precession.
- **Procedural Rendering:** Fully un-textured! All planets, the dynamic surface of the Sun (granulation, solar spots), and the majestic backdrop Nebula Skybox are driven entirely by custom 3D mathematical noise (Seamless Simplex Noise, FBM).
- **Time Dilation Sub-stepping:** High-stability Velocity Verlet integrator dynamically breaks frames down into mathematical sub-steps, allowing you to rocket the simulation speed up to months per second while maintaining perfect mathematical integration.
- **Dear ImGui Telemetry:** Highly refined, dockable telemetry overlay and floating 3D planet distance labels built directly into the OpenGL pipeline.
- **Full Camera Control**: Features locked-on perspective tracking or free-range planetary exploration.

## Building and Running (Windows)

This simulator requires **MSYS2 (UCRT64)** and `g++`. 
All dependencies (GLFW, GLEW, GLM, ImGui) are neatly packaged right out of the box within the `src`, `lib`, and `include` paths.

### 1. Requirements
Ensure you have MSYS2 installed with UCRT64 `g++` on your path. 

### 2. Execution
If your `g++` is located at `C:\msys64\ucrt64\bin\g++.exe`, you can seamlessly compile the codebase using the included bat file:

1. Double-click `build_and_run.bat` or run `.\build_and_run.bat` via Powershell.
2. The game will automatically re-build taking your local changes into effect and boot `bin/app.exe`.

*Note: Dedicated graphic drivers (Nvidia/AMD) are requested forcibly upon load due to embedded DLL exports bypassing laptop integrated Intel GPUs to ensure hyper-smooth 60+ FPS.*

## Controls
- **W/A/S/D / Up/Down** - Move around in Free-Camera
- **Shift** - Speed Boost Camera
- **Scroll Wheel** - Zoom
- **`** - Cycle to focus on the next planet
- **V** - Cycle view modes (Sun-locked vs Surface-Observer)
- **F** - Free Look ON/OFF
- **P** - Pause physics simulation
- **[ / ]** - Decrease/Increase Time Scale locally 
- **G** - Swap Gravity (Newtonian vs Einstein GR)
- **H** - Toggle this Help Menu onscreen

Use the ImGui overlay in the top right to customize your Base Time Scale (e.g. 1 Second IRL = 1 Day Simulation).
