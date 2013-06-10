## TWBaseTrap

If you have used NVScript you will be familiar with the facilities supported by
the`NVTrap` base script. I have replicated the features provided by `NVTrap`
and added some new features. All scripts whose names begin with `TWTrap`, or link
to this document from their documentation page, understand the paramters listed
below (`[ScriptName]` here is the name of the script as you entered it into the
`S -> Scripts` property)

### Parameter: [ScriptName]On
- Type: `string`
- Default: `TurnOn`

Allows you to control which script message should activate the trap.

### Parameter: [ScriptName]Off
- Type: `string`
- Default: `TurnOff`

Allows you to control which script message should deactivate the trap.

### Parameter: [ScriptName]Count
- Type: `integer`
- Default: `0` (no limit)

Lets you specify how many times the script will work. You can replicate the
behaviour of the `Once` Trap Control Flag by setting this to `[ScriptName]Count=1`.
To reset the count to zero, sent the trap a `ResetCount` message.

### Parameter: [ScriptName]CountOnly
- Type: `string`
- Default: `Both`

Controls whether On, Off, or both count against the limit counter. This can
be set to 'On' or 1 so that only 'On' messages increase the count, or 'Off'
or 2 so that only 'Off' messages increase the count.

### Parameter: [ScriptName]CountFalloff
- Type: `integer`
- Default: `0` (no falloff)

Lets you make the use count decrease over time. This specifies the time, in
milliseconds (actually, sim time ticks) that it takes to decrease the count
by 1.

### Parameter: [ScriptName][On/Off]Capacitor
- Type: `integer`
- Default: `1`

Specify the number of times the trap needs to receive an on or off message
before it will actually activate or deactivate.

### Parameter: [ScriptName][On/Off]CapacitorFalloff
- Type: `integer`
- Default: `0` (no falloff)

How long it takes for the capacitor to 'lose charge'. This is the time in
milliseconds that it takes to lose one activation.

### Parameter: [ScriptName]Debug
- Type: `boolean`
- Default: `false`

Enable or disable debugging output from the script. If this is set to true,
the script will write debugging information to the monolog.
