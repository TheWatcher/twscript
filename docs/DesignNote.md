# Design Note

These scripts uses the `Editor -> Design Note` feature in Dromed to store
their configuration, something you will have encountered in the past if
you have used NVScript, tnhScript, or PublicScripts.

For some scripts, the Design Note may be left empty: some scripts may use
the values set in other object attributes for configuration, or they may not
support any setup at all. However, in practice the Design Note will contain
one or more configuration parameters. Each parameter consists of three parts:

- a parameter name, something like `TWTrapSetSpeedDest`
- an equals sign (=)
- a value to set for the parameter

For example:

    TWTrapSetSpeedDest='SomeTerrPt'

The value you can specify for any given parameter depends on its *type*.
The documentation for each parameter says what type of value it expects you
to give it, and it may require any of the following:

- `float`: the value should be a real number, that is a number that can
  include a fractional part, like `3.1415`. Negative numbers are specified
  using `-`, eg: `-2.54`. Note that, in some cases, negatives can
  produce unexpected or undesirable behaviours if the script doesn't expect
  you to use them.
- `float vector`: three `float` values, separated by commas. You may place
  whitespace between the `float` values and the commas, but *do not* use
  quotes around each value, ie: `SomeParam=0 , 1.2, 5;` is fine, as is
  `SomeParam="1.2, 3.4,5.6";`, but `SomeParam="0","1.2","3";` **is not**
  valid. I generally recomment avoid using quotes when specifying the values
  for `float vector` types. Unless otherwise indicated in the documentation,
  the first value corresponds to the `x` component of the vector, the second
  to the `y` component, and the third to the `z`. Any components you do not
  specify a value for will be set to `0.0`. For example, the value `6,,10.5`
  will set `x` to `6.0`, `y` to `0.0`, and `z` to `10.5`. Similarly, `1,0.5`
  will set `x` to `1.0`, `y` to `0.5`, and `z` (which has been omitted
  entirely from the example here!) will be set to `0.0`.
- `integer`: a 'whole number', one without any decimal part, eg: `3`.
  Negative numbers can be specified using `-`, eg: `-42`.
- `boolean`: a true or false value. The following are considered to be 'true'
  values: Any word starting 't', 'T', 'y' or 'Y'; Any non-zero integer value.
  Any words that do not start as described, or the number `0`, are
  considered to be false.
- `time`: an integer that represents a period of time. Without any modifier,
  the the value is interpreted as a number of milliseconds, if you append
  `s` to the number (eg: `10s`) the value is interpreted as a number of
  seconds. If you place `m` after the number, it is interpreted as a number
  of minutes.
- `object`: a Dromed object name, or object ID.
- `string`: any text, no special meaning is attached to it. Note that, if
  the string needs to contain a semicolon (;) you *must* enclose the string
  in single or double quotes, `'like this'` or `"like this"`. If you need
  to include a single quote in a single-quoted string, you can do so by
  prefixing the `'` with `\`, ie: `\'`. Similarly, to include a double quote
  in a double-quoted string, prefix it with a backslash, `\"`. If you need
  to include a backslash in a quoted string, you should generally prefix it
  with another, eg: `\\`.

For the `integer`, `float`, `boolean`, and `time` types, you may also use a
quest variable in place of a literal value. To do this, prepend the quest
variable name with `$`. For example, this will use the value specified in
the quest variable `platform_speed`:

    TWTrapSetSpeedSpeed=$platform_speed

In addition, `integer` and `float` types can have simple mathematical
operations included in the parameter value if the parameter starts with a
quest variable. eg:

    TWTrapSetSpeedSpeed=$platform_speed * $speed_mult
    TWTrapSetSpeedSpeed=$platform_speed / 10.0
    TWTrapSetSpeedSpeed=$platform_speed + $base_speed

Supported operations are addition `+`, multiplication `*`, and division `/`.
Subtraction is not directly supported, but you can add a negative number
for the same effect.

Note that, for `float vector` types, each value may be a quest variable:

    TWTrapPhysStateCtrlLocation=$thingx, $thingy, $thingz;

Values *may* be enclosed in quotes, either single quotes or double quotes,
but this is not required *unless* you are specifying a string containing a
semicolon, in which case you must quote the string.

If more than one parameter is specified, semicolons are used to separate
them, for example:

    TWTrapSetSpeedSpeed=5;TWTrapSetSpeedDest='*TerrPt'
