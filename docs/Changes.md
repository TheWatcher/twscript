## Not a Real Changelog, Really

Please note that this file is intended to be an end-user friendly list of
significant and important changes, it is **not** an exhaustive changelog. If
you want to see all of the changes in detail, you should look at the git
commit log.

### 2013-07-21 Version 2.0.4 ()

- Significant changes to the way in which **TWBaseScript** and **TWBaseTrap**
  handle initialisation, strings, and other settings.

- Added [TWTrapAIBreath](TWTrapAIBreath.html)

### 2013-06-18 Version 2.0.2 (Marble Arch)

- Modified simple arithmetic applied to quest vars when processing int and
  float parameters. Subtraction via explicit '-' is no longer possible; it
  must be done using addition of a negative number, so `Foo=$qvar+-10` will
  subtract 10 from the value in the `qvar` quest variable.

- `TWTrapSetSpeedSpeed` now supports the `[intensity]` value, allowing the
  script to use the intensity set in stimulus messages when set to be
  activated by such a message.

### 2013-06-09 Version 2.0 Alpha (Blackfriars)

- Much of the PublicScripts-based framework has been removed, and replaced
  with a new set of classes. This has allowed me to cleanly implement many
  new features, including replicating and extending the 'NVTrap' behaviour
  controls found in NVScript. Please see the [TWBaseTrap](TWBaseTrap.html)
  file for more information about this.

- TWTrapPhysStateControl has been renamed to TWTrapPhysStateCtrl, because the
  even the shorter version of the name is borderline painfully long, and the
  full version just makes it worse.

- TWTrapSetSpeed and TWTrapPhysStateCtrl have been ported to the new framework
  and support all the TWBaseTrap parameters, plus their previously documented
  parameters.

- The `TWTrapSetSpeed` parameter for the TWTrapSetSpeed script has been
  renamed to `TWTrapSetSpeedSpeed`, which is unfortunately repetitious, but
  necessary under the rules of the new framework.

### 2013-03-06 Version 1.0 (Fulham Broadway)

- Initial release based around PublicScripts.
