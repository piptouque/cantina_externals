
#### Externals

* cantina~

todo:

* midimachine

----

#### Patches

* Alive: live set-up
* cantroller~: post-processed cantina~
* cantina_patch~: cantina~ implementation with patches

----

#### Whatever

doing:

* [] refactoring MidiControl

todo:

* EXPLAIN
* add backtrace handling 
| see: http://www.gnu.org/software/libc/manual/html_node/Backtraces.html
* add garbage-collection behaviour to PitchShifter Voice when respective note isn't playing.
* only update relevant Voice/Controller when receiving midi input 

#### Build

##### dependencies:

	FFTW
	libsamplerate
	sndfile
	
###### on debian:

	sudo apt install libsndfile-dev vamp-plugin-sdk
	sudo apt install libfftw3-dev libsamplerate0-dev
	sudo apt install rubberband-ladspa

###### ~ tut-tut-tut-tut-tulut-tut ~
