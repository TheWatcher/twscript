# TWScript

TWScript is a collection of script modules for Thief 2. These scripts are
provided under the terms of the GNU GPL v2, a full copy of which may be found
in the LICENSE file in the distribution directory, along with other license
information.

TWScript would not be possible without the Public Scripts package released by
Telliamed (Tom N. Harris), and I would like to express my sincere thanks for
his generous contribution to the community.

I'd also like to thank LarryG for his suggestions, patience, feedback, and
willingness to be a guinea-pig.

## Installing

In order to use TWScript with missions, it must be made available to Thief 2
in a location it recognises.

Players can place the .osm in the Thief 2 directory and, if it is not provided
with a mission, Thief 2 will use that version.

For editors, the simplest method to include TWScript is to place the .osm in
your mission's directory inside the Thief2\FMs\ directory, and use FMSel to
select your mission when editing it. When you distribute your mission you
can simply zip up your mission directory and distribute that.[^1]

## Documentation

The documentation for the scripts consists of a number of files in addition
to this one:

- [DesignNote.html](DesignNote.html) describes the way in which the parameters
  for the scripts are specified in Dromed's `Editor -> Design Note` property.
- The remaining HTML files describe each of the script modules and the
  parameters they suppport.

Currently available scripts are:

- [TWTrapSetSpeed.html](TWTrapSetSpeed.html)
- [TWTrapPhysStateControl.html](TWTrapPhysStateControl.html)

## Development information

The current version of TWScript is essentially an early beta release, and some
significant changes are in development. Some features that should be in the
next release are:

- Support for defining which script messages are interpreted as `TurnOn` and
  `TurnOff` for traps and triggers (much like the features provided by NVScript)
- A `TWTweqSmooth` script to support more flexible ease-in and ease-out on
  joint tweqs.
- A `TWTrapRequireMessages` script that will send a `TurnOn` when it has
  recieved a set of messages, and `TurnOff` when it recieves a different set.


## The source

The source code for TWScript, including supporting files and instructions for
building the scripts is available at github:

<https://github.com/TheWatcher/twscript>

[^1]: Note that doing this does have the downside that the version of the osm
included with your mission will not get any bugfixes or updates unless you
repackage your mission. Another, albeit less reliable, method is to simply state
that players must install the .osm in their Thief2 directory - but if you do
that, expect to run into problems if players do not follow the instructions!
