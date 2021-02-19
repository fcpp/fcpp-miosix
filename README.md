# FCPP for MIOSIX

An integration of the FCPP distributed computing framework for the MIOSIX operative system for microcontroller architectures.

## Users: Getting Started

For further informations, you can inspect the main websites of the [FCPP](https://fcpp.github.io) and [MIOSIX](http://miosix.org) projects. The repository is set up to be run in a virtual environment through [Docker](https://www.docker.com). Alternatively, you can directly run it in your local machine through a custom build.

### Docker

Starting from the main folder of the repository, download the Docker container from GitHub by typing the following command in a terminal:
```
docker pull docker.pkg.github.com/fcpp/fcpp-miosix/container:1.0
```
Alternatively, you can build the container yourself with the following command:
```
docker build -t docker.pkg.github.com/fcpp/fcpp-miosix/container:1.0 .
```
Once you have the Docker container locally available, type the following commands:
```
docker run -it --volume $PWD:/fcpp-miosix --workdir /fcpp-miosix docker.pkg.github.com/fcpp/fcpp-miosix/container:1.0 make
```
Then you should get output about building the sample project together with FCPP and MIOSIX (in the Docker container). The resulting binaries can be flush to the microcontrollers to be directly executed. **TODO: mention the specific microcontroller that we are compiling for, and hint at how to change it to others.**

### Custom Build

In order to get started on your machine you need the following installed:

- [GCC](https://gcc.gnu.org) (tested with version 9.2.0)
- [MIOSIX](https://miosix.org/wiki/index.php?title=Quick_start)

In the main folder, you should be able to run `make`, getting output about building the sample project.

## Graphical Simulation

### Virtual Machines

The simulations in this repository have an OpenGL-based graphical interface. Common Virtual Machine software (e.g., VirtualBox) has faulty support for OpenGL, hence running the experiments in a VM is not supported. Based on preliminary testing, the simulations may not start on some VMs, while starting on others with graphical distortions (e.g., limited colors).

### Windows

Pre-requisites:
- [Git Bash](https://gitforwindows.org) (for issuing unix-style commands)
- [MinGW-w64 builds 8.1.0](http://mingw-w64.org/doku.php/download/mingw-builds)
- [CMake 3.9](https://cmake.org) (or higher)
- [Asymptote](http://asymptote.sourceforge.io) (for building the plots)

During MinGW installation, make sure you select "posix" threads (should be the default) and not "win32" threads. After installing MinGW, you need to add its path to the environment variable `PATH`. The default path should be:
```
C:\Program Files (x86)\mingw-w64\i686-8.1.0-posix-dwarf-rt_v6-rev0\mingw32\bin
```
but the actual path may vary depending on your installation.

Clone this repository, then go into its main directory to launch the `make.sh` script:
```
> ./make.sh windows simulation
```
You should see output about building the executables, then the graphical simulation should pop up. When the simulation closes, the resulting data will be plotted in folder `plot/`.

### Linux

Pre-requisites:
- Xorg-dev package (X11)
- G++ 9 (or higher)
- CMake 3.9 (or higher)
- Asymptote (for building the plots)

To install these packages in Ubuntu, type the following command:
```
sudo apt-get install xorg-dev g++ cmake asymptote
```

Clone this repository, then go into its main directory to launch the `make.sh` script:
```
> ./make.sh unix simulation
```
You should see output about building the executables, then the graphical simulation should pop up. When the simulation closes, the resulting data will be plotted in folder `plot/`.

### MacOS

Pre-requisites:
- Xcode Command Line Tools
- CMake 3.9 (or higher)
- [Asymptote](http://asymptote.sourceforge.io) (for building the plots)

To install them, assuming you have the [brew](https://brew.sh) package manager, type the following commands:
```
xcode-select --install
brew install cmake asymptote
```

Clone this repository, then go into its main directory to launch the `make.sh` script:
```
> ./make.sh unix simulation
```
You should see output about building the executables, then the graphical simulation should pop up. When the simulation closes, the resulting data will be plotted in folder `plot/`.

### Simulation

To launch the simulated scenario manually, move to the `bin` directory and run the `miosix_simulation` executable. This will open a window displaying the simulation scenario, initially still: you can start running the simulation by pressing `P` (current simulated time is displayed in the bottom-left corner). While the simulation is running, network statistics will be periodically printed in the console, and aggregated in form of an Asymptote plot at simulation end. You can interact with the simulation through the following keys:
- `Esc` to end the simulation
- `P` to stop/resume
- `O`/`I` to speed-up/slow-down simulated time
- `L` to show/hide connection links between nodes
- `G` to show/hide the grid on the reference plane
- `Q`,`W`,`E`,`A`,`S`,`D` to move the simulation area along orthogonal axes
- `C` resets the camera to the starting position
- `right-click`+`mouse drag` to rotate the camera
- `mouse scroll` for zooming in and out
-`left-shift` added to the commands above for precision control

## Authors

- [Giorgio Audrito](http://giorgio.audrito.info/#!/research)
- [Federico Terraneo](https://terraneo.faculty.polimi.it)
