
## Cantina pd

### Info

Implementation of Cantina as a pure-data external. Currently my only reference for testing purposes.

### Build 

#### Instructions

Using cmake (>= 3.12), from the project's root directory:
  
    mkdir build
    cd build
    cmake ..
    make

This will build the `cantina~` externals.

An example of use in Pure Data available [here](https://github.com/piptouque/cantina_pd_live.git), in the Alive patch.

#### Dependencies 

* pure-data 0.52 (submodule)
* pd.build       (submodule)
* SoundTouch  2.1.2
* FFTW3

*On Debian:*

    sudo apt install libsoundtouch-dev libfftw3-dev

### To do

#### Features 

###### ~ tut-tut-tut-tut-tulut-tut ~
