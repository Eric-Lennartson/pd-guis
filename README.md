# pd-guis
Extra guis for puredata. Currently only compatible for pdnext.

--------------------------------------------------------------------------

   This work is free. You can redistribute it and/or modify it under the
   terms of the GNU General Public License v3.0. See LICENSE for more details

--------------------------------------------------------------------------

###   About guis

​guis requires using pdnext. The colors are set using the tcl file that is used to theme pdnext.
A default darkmode theme is included in the repository. In the future I hope to add more guis, but for now
it will just be the dial. Don't cross your fingers.
The repository for pdnext can be found here <https://github.com/sebshader/pdnext>.
​This library's repository resides at <https://github.com/Eric-Lennartson/pd-guis/>.
A repository for more themes for pdnext can be found here <https://github.com/Eric-Lennartson/pd-themes>.

--------------------------------------------------------------------------

### Downloading guis:

​	You can get guis from https://github.com/Eric-Lennartson/pd-guis/releases - where all releases are available, but guis is also found via Pd's external manager (In Pd, just go for Help => Find Externals and search for 'guis').  In any case, you should download the folder to a place Pd automatically searches for, and the common place is the ~/documents/pd/externals folder.

​	Instructions on how to build guis are provided below.

--------------------------------------------------------------------------

### Building guis for Pd Vanilla:

guis relies on the build system called "pd-lib-builder" by Katja Vetter (check the project in: <https://github.com/pure-data/pd-lib-builder>). PdLibBuilder tries to find the Pd source directory at several common locations, but when this fails, you have to specify the path yourself using the pdincludepath variable. Example:

<pre>make pdincludepath=~/pd-0.51-1/src/  (for Windows/MinGW add 'pdbinpath=~/pd-0.51-1/bin/)</pre>

* Installing with pdlibbuilder

Go to the pd-guis folder and use "objectsdir" to set a relative path for your build, something like:

<pre>make install objectsdir=../guis-build</pre>
Then move it to your preferred install folder for Pd and add it to the path.

Cross compiling is also possible with something like this

<pre>make CC=arm-linux-gnueabihf-gcc target.arch=arm7l install objectsdir=../</pre>

--------------------------------------------------------------------------
### Theming guis

The following options are available for theming. Just change the hex color in the .tcl file
and reload pdnext for the changes to take effect.

- dial_background
- dial_track
- dial_ticks
- dial_thumb
- dial_thumb_highlight (highlight for the dial, shown when dial is active)
- dial_active (contrasting region showing the area the dial has already covered)
- dial_label
- tooltip_fill
- tooltip_border
- tooltip_text
- dial_iolet
- dial_iolet_border

--------------------------------------------------------------------------
## Current Object list (1 object):

- [dial]
