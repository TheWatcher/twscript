## Not a Real Changelog, Really

Please note that this file is intended to be an end-user friendly list of
significant and important changes, it is **not** an exhaustive changelog. If
you want to see all of the changes in detail, you should look at the git
commit log.

### 2013-06-09 Version 2.0 Alpha (Blackfriars)

- Much of the PublicScripts-based framework has been removed, and replaced
  with a new set of classes. This has allowed me to cleanly implement many
  new features, including replicating and extending the 'NVTrap' behaviour
  controls found in NVScript. Please see the [TWBaseTrap.html](TWBaseTrap.html)
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
