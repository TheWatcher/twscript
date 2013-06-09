# TWTrapPhysStateCtrl

TWTrapPhysStateCtrl provides direct control over the location, orientation,
velocity, and rotational velocity of objects in Thief 2. Note that this script
provides a means to set the physics state values, but the game may ignore these
values in some situations, and any changes you make will be subsequently subject
to the normal physics simulation performed by the game (so, for example, changing
an object's position may result in it either staying in the new location, or
falling to (or through!) the ground, depending on how the object has been set up).

Expect to have to experiment with this script!

## General setup

Add this script to a marker, link the marker to the object(s) whose physics
state you want to control using ControlDevice links. Whenever the marker is sent
a TurnOn message, the script will update the physics state of the objects linked
to the marker.

## Configuration

Parameters are specified using the Editor -> Design Note, please see the
main documentation for more about this.  Parameters supported by
TWTrapPhysStateCtrl are listed below. If a parameter is not specified,
the default value shown is used instead. Note that all the parameters are
optional, and if you do not specify a parameter, the script will attempt to use
a 'sane' default.

Note that, in addition to the parameters listed here, this script supports the
parameters described in the [TWBaseTrap.html](TWBaseTrap.html) file.

### Parameter: TWTrapPhysStateCtrlLocation
- Type: `float vector`
- Default: none (location is not changed)

Set the location of the controlled object(s) the position specified. If this
parameter is not specified, the location of the object(s) is not modified. If you
specify this parameter, but give it no value (ie: `TWTrapPhysStateCtrlLocation=;`)
then the default location of `0, 0, 0` is used.

### Parameter: TWTrapPhysStateCtrlFacing
- Type: `float vector`
- Default: none (orientation is not changed)

Set the orientation of the controlled object(s) the values specified. If this
parameter is not specified, the orientation of the object(s) is not modified. If you
specify this parameter, but give it no value (ie: `TWTrapPhysStateCtrlFacing=;`)
then the default orientation of `0, 0, 0` is used.IMPORTANT NOTE*: the values
specified for this parameter match the order found in Physics -> Model -> State,
so the first value is bank (B), the second is pitch (P), and the third is
heading (H). This is the opposite of the order most people would expect; if you
find yourself having problems orienting objects, check that you haven't mixed up
the bank and heading!

### Parameter: TWTrapPhysStateCtrlVelocity
- Type: `float vector`
- Default: none (velocity is not changed)

Set the velocity of the controlled object(s) the values specified. If this
parameter is not specified, the velocity of the object(s) is not modified. If you
specify this parameter, but give it no value (ie: `TWTrapPhysStateCtrlVelocity=;`)
then the default velocity of `0, 0, 0` is used.

### Parameter: TWTrapPhysStateCtrlRotVel
- Type: `float vector`
- Default: none (rotational velocity is not changed)

Set the rotational velocity of the controlled object(s) the values specified. If
this parameter is not specified, the rotational velocity of the object(s) is not
modified. If you specify this parameter, but give it no value
(ie: `TWTrapPhysStateCtrlRotVel=;`) then the default of `0, 0, 0` is used. Note
that, as with TWTrapPhysStateCtrlFacing, the first value of the vector is the
bank, the second is the pitch, and the third is the heading.
