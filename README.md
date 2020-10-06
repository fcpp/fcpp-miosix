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

## Authors

- [Giorgio Audrito](http://giorgio.audrito.info/#!/research)
- [Federico Terraneo](https://terraneo.faculty.polimi.it)
