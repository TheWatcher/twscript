# TWTrapAIBreath

**TWTrapAIBreath** controls a particle attachment to an AI that allows the
simulation of visible breath in cold areas. Note that this script only
provides a general approximation of the effect: it does not attempt to
sync up with AI vocalisations (speech, whistles, etc), and probably never
will be able to. No message sent to scripts when an AI begins vocalising,
and in the absence of such a message there's no way to sync emission and
vocalisation.

This script does the following:

- Enables and disabled the attached particles automatically (no additional
  fnords and Rube Goldberg setup needed) when the AI enters and leaves
  concrete rooms listed as 'cold' in the script's design note.
- Scales the breath rate based on alertness level (the AI appears to
  breathe faster at higher alert levels).
- AIs continue breathing when knocked out, and the breath particles will be
  enabled and disabled automatically when the body is carried into or out
  of cold areas.
- When the AI is killed the particle group is disabled.

There are a few remaining problems (in addition to the vocalisation sync
issue mentioned above):

- When AIs are knocked out, the particle attachment can end up in the wrong
  place relative to the AI's face. This is difficult to solve because there
  is no vhot, joint, or other marker in front of the AI's face, and the
  orientation of the particle group does not seem to change when the AI is
  knocked out.
- The emitted particles are relative to the particle group, so sharp turns
  while the AI is breathing out can look a little odd from some angles. This
  is an unfortunate aspect of particle groups, and I'm not sure what the
  best way to handle it is.

## Initial Setup

Before the script can be used, you need to set up the SFX and AI archetypes
to use it properly (you could set it up on a per-AI basis if you really like
pain).

### SFX Setup

Begin by creating a new SFX archetype for the breath particles. The following
instructions are for example only, you will want to adjust them to your own
requirements, aesthetics, and mission temperature.

- Open the `Object Hierarchy` window
- Open the `Object -> SFX -> GasFX` tree
- Select `GasFX`
- Click `Add`, enter `AIBreath`
- Select `AIBreath`
- Click `Edit`
- Add `SFX -> Particles`
- Enter the following settings (if using fpuff, copy it from the demo map!):

        Active: not checked  (**This is important! Do not set it active!**)
        Render Type: Bitmap disk
        Animation: Launched continually
        Group motion: Attached to object
        number of particles: 90
        size of particle: 0.5
        bitmap name: fpuff
        velocity: 0 0 0
        gravity vector: 0 0
        alpha: 45
        fixed-group radius: 1
        launch period: 0.01
        group-scale velocity: 1.25
        bm-disk flags: use lighting, fade-in at birth, grow-in at birth
        bm-disk birth time: 0.25
        bm-disk rot: 3 0 1.3
        bm-disk grow speed: 0.2
        bm-disk rgb: 245 245 245

- Click `OK`
- Select the `SFX -> Particle Launch Info` parameter, click `Edit`
- Enter the following settings:

        Launch type: Bounding Box
        Box min: 0.75 -0.1 0.2
        Box max: 0.75 0.1 0.2
        Velocity min: 0 -0.5 -2
        Velocity max: 3 0.5 2
        Min time: 0.4
        Max time: 1.0

- Click `OK`
- Click `Done`

### AI Setup

Now that the particles are set up, you need to set up the AIs to use this
script and the particles. Note that what follows sets up the breath script
for all guards. This isn't really suitable to set up on the `Animal` archetype
(unless you have no spiders or rats in your mission, in which case it is
perfect to go on there!) and `Human` has no space left in its `S -> Scripts`.
If you need to set up the breath on other humans, you'll need to repeat
these steps as needed, unless you use `Animal` instead of `guard`.

- Locate and select `Object -> physical -> Creature -> Animal -> Human -> guard`
- Click `Edit`
- Add `S -> Scripts`
- Set Script 0 to `TWTrapAIBreath`
- Click `OK`
- Add -> `Tweq -> Flicker`
- Enter the following settings (see note below about `Rate`):

        Halt: Continue
        AnimC: [None]
        MiscC: Scripts
        CurveC: [None]
        Rate: 3000

- Click `OK`
- Select the `FlickerState` parameter, click `Edit`
- Set `AnimS` to `On`
- Click `OK`
- Add `Links -> ~ParticleAttachement`
- Click `Add`
- Add a ~ParticleAttachement link from `guard` to `AIBreath`
- Select the new link, set its data to

        Type: Joint
        vhot #: 0
        joint: Neck
        submod #: 0

- Click `OK` twice
- Add `Editor -> Design Note`
- Enter any configuration settings you may want (this is a good place to set
  the `TWTrapAIBreathColdRooms` parameter).
- Click `Done`
- Click `Close`

The rate set in the `Tweq -> Flicker` is the 'base breathing rate' for the
AI: this is how long, in milliseconds, between one inhale and another. For
a normal human at rest that's generally about 3 to 5 seconds (3000 to 5000
milliseconds). As AIs will usually be walking around doing things even 'at
rest', going for the lower value is probably better, but you should
experiment to see what feels best for you.

## Configuration

The base breathing rate for the AI (the breathing rate when the AI is at the
lowest alertness level, or when knocked out) is taken from the `Rate` setting
in the `Tweq -> Flicker` set on the AI. This should have been created as part
of the Initial Setup discussed above.

Remaining params are specified using the Editor -> Design Note, please see the
main documentation for more about this.  Parameters supported by **TWTrapAIBreath**
are listed below. If a parameter is not specified, the default value shown is
used instead. Note that all the parameters are optional, and if you do not
specify a parameter, the script will attempt to use a 'sane' default, but
you probably want to set at least the `TWTrapAIBreathColdRooms` parameter.

Note that, in addition to the parameters listed here, this script supports the
parameters described in the [TWBaseTrap.html](TWBaseTrap.html) file.

### Parameter: `TWTrapAIBreathInCold`
- Type: `boolean`
- Default: `false`

If the AI starts out in a cold room, the script may not correctly detect this
fact. If you set this parameter to true, the AI is initialised as being in a
cold room, and everything should work as expected. Basically: if the AI is
in a cold room at the start of the mission, set this to true, otherwise you
can leave it out.

### Parameter: `TWTrapAIBreathImmediate`
- Type: `boolean`
- Default: `true`

If this is set to true, the breath particle group is deactivated as soon as
the AI exits from a cold room into a warm one. If it is false, the normal
exhale time will elapse before the group is deactivated. You might want to
set this to false if, for example, there is a very significant difference
in temperature between the cold and not-cold rooms, to simulate cold air
entering with the AI. This is something you should probably play around
with to see what works best for your mission.

### Parameter: `TWTrapAIBreathStopOnKO`
- Type: `boolean`
- Default: `false`

If this is false, the particle group attached to the AI continues to work
as normal when the AI has been knocked out (as knocked-out AIs need to
breathe too!). If set to true, the particle group is deactivated
permanently when the AI is knocked out. This has been provided because the
particle attachment will often place the particles in the wrong place when
the AI has been knocked out, making it look like the AI is breathing out
of their chest or armpit! If you prefer to just stop the particles on KO
rather than wait on a fix for this issue, set this to true.

### Parameter: `TWTrapAIBreathExhaleTime`
- Type: `integer`
- Default: `250`

The amount of time, in milliseconds, that the AI will exhale for at rest.
This is the amount of time that the particle group will remain active for
during every breath cycle. It is hard limited so that you can not set it
to more than half of the breathing rate as set in the flicker tweq.

### Parameter: `TWTrapAIBreathSFX`
- Type: `object`
- Default: `AIBreath`

This should be the *archetype name* of the particle group attached to the
AI that should be used as the breath particles. If you followed the above
'Initial Setup' instructions, you do not need to specify this, but if you
are using a different particle group for the breath you need to give the
name of its archetype here.

### Parameter: `TWTrapAIBreathColdRooms`
- Type: `string`
- Default: `none`

If you want to use the automatic activate/deactivate feature of this script
(which you do), rather that relying on a Heath Robinson/Rube Goldberg setup
to send the script TurnOn/TurnOff messages (which you don't want to do),
you need to list the rooms that should be considered to be cold rooms here.
You can specify multiple rooms by separating them with commas, and you can
use concrete room IDs or concrete room names here (use the ID if the name
contains a comma!). Note that you must set up concrete rooms for all your
designated cold areas.

### Parameter: `TWTrapAIBreathLinkType`
- Type: `string`
- Default: `~ParticleAttachement`

Allows the link type used to attach the paricle group to the AI to be changed
from the default `~ParticleAttachement` to something else (like, for example,
`Contains`. If you use this parameter, be sure to check the spelling of the
link flavour - you will get errors in the monolog if the link type is
incorrect.
