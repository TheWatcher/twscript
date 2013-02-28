TWTrapSetSpeed
==============

TWTrapSetSpeed allows mission authors to control the speed setting of `TPath`
waypoints while a mission is being played. This script lets the editor
control how fast a vator (a moving terrain object) moves between `TerrPt`s
dynamically, either using literal speed values or taking speed information out
of a quest variable.

By default, the speed changes made by this script will not be picked up by
any moving terrain objects until they reach their next waypoint. However, if
you want the speed of any moving terrain object to be updated by this script
before it reaches the next `TerrPt`, link the object this script is placed on
to the moving terrain object with a `ScriptParams` link, and set the data for
the link to `SetSpeed`. Note that, if you set the speed to `0` at any point,
you **must** set up the link from the object on which `TWTrapSetSpeed` is set
to the moving terrain object, otherwise it will be impossible to make the
moving terrain object move again!


General setup
-------------

In the most simple case, set up your vator and path as normal, then add a
button and a marker to your level, and then do the following:

- Open the marker's properties
- Give it a useful name, for example `SetSpeed5`
- Add `S -> Scripts`, set `Script 0` to `TWTrapSetSpeed`
- Add `Editor -> Design Note`
- In the design note put `TWTrapSetSpeed=5;TWTrapSetSpeedDest='*TerrPt'`
- Link `SetSpeed5` to your moving terrain object with a `ScriptParams` link,
  and set the data for the link to `SetSpeed`
- Open the button's properties.
- Give it a useful name like `SpeedButton5`
- Link `SpeedButton5` to `SetSpeed5` with a `ControlDevice`

Now, when you go into the game, every time `SpeedButton5` is pressed it will
send a `TurnOn` signal to `SetSpeed5`. When `SetSpeed5` receives the `TurnOn`
it will set the speed value of all concrete descendants of `TerrPt` to `5`,
and updates the speed of the moving terrain object to `5`.

With several buttons and markers you can change the speed of the moving
terrain object to different values as desired.

More complex setups are obviously possible, and the example given above will
set the speed of **all** `TerrPts` in the level - if you have different networks
of `TerrPt`s, this is likely to be Undesirable Behaviour. One method to work
around that without requiring an explosion of markers and links is to create
new archetypes under `TerrPt`, one for each group of `TerrPt`s in your level
(say, `Loop1TerrPt`, `Loop2TerrPt`, and `Loop3TerrPt`), and then place concrete
instances of those in your level as appropriate. Then you can use `*Loop1TerrPt`
to update one group, `*Loop2TerrPt` to update the second, and so on.

Configuration
-------------

All parameters are specified using the `Editor -> Design Note` as described
in the DesignNote document. If a parameter is not specified, the default
value shown is used instead. Note that while all the parameters are optional,
and if you do not specify a parameter the script will attempt to use a 'sane'
default, you probably want to always set at least the `TWTrapSetSpeed`
parameter to something.

Parameter: `TWTrapSetSpeed`
     Type: `float`
  Default: `0.0`
The speed to set the target objects' TPath speed values to when triggered. All
TPath links on the target object are updated to reflect the speed given here.
The value provided for this parameter may be taken from a QVar by placing a $
before the QVar name, eg: `TWTrapSetSpeed='$speed_var'`. If you set a QVar
as the speed source in this way, each time the script receives a `TurnOn`, it
will read the value out of the QVar and then copy it to the destination
object(s). Using a simple QVar as in the previous example will restrict your
speeds to integer values; if you need fractional speeds, you can include a
simple calculation after the QVar name to scale it, for example,
`TWTrapSetSpeed='$speed_var / 10'` will divide the value in speed_var by `10`,
so if `speed_var` contains `55`, the speed set by the script will be `5.5`. You
can even specify a QVar as the second operand if needed, again by prefixing
the name with '$', eg: `TWTrapSetSpeed='$speed_var / $speed_div'`.


Parameter: `TWTrapSetSpeedWatchQVar`
     Type: `boolean`
  Default: `false`
If TWTrapSetSpeed is set to read the speed from a QVar, you can make the
script trigger whenever the QVar is changed by setting this to true. Note
that this will only watch changes to the first QVar specified in
`TWTrapSetSpeed`: if you set `TWTrapSetSpeed='$speed_var / $speed_div'` then
changes to `speed_var` will be picked up, but any changes to `speed_div`
will not trigger this script.


Parameter: `TWTrapSetSpeedDest`
     Type: `string`
  Default: `[me]`
Specify the target object(s) to update when triggered. This can either be
an object name or id, [me] to update the object the script is on, [source] to
update the object that triggered the change (if you need that, for some odd
reason), or you may specify an archetype name preceeded by * or @ to update all
objects that inherit from the specified archetype. If you use *Archetype then
only concrete objects that directly inherit from that archetype are updated, if
you use @Archetype then all concrete objects that inherit from the archetype
directly or indirectly are updated.


Parameter: `TWTrapSetSpeedDebug`
     Type: `boolean`
  Default: `false`
If this is set to true, debugging messages will be written to the monolog to
help trace problems with the script. Note that if you set this parameter to true,
and see no new output in the monolog, double-check that you have twscript loaded!


Parameter: `TWTrapSetSpeedImmediate`
     Type: `boolean`
  Default: `false`
If this is set to true, the speed of any linked moving terrain objects is
immediately set to the speed value applied to the TerrPts. If it is false, the
moving terrain object will smoothly change its speed to the new speed
(essentially, setting this to true breaks the appearance of momentum and inertia
on the moving object. It is very rare that you will want to set this to true.)
