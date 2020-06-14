/** @file
 * This file contains the interface for the TWTrapAIBreath class.
 *
 * @author Chris Page &lt;chris@starforge.co.uk&gt;
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

#ifndef TWTRAPAIBREATH_H
#define TWTRAPAIBREATH_H

#if !SCR_GENSCRIPTS
#include <lg/interface.h>
#include <lg/scrmanagers.h>
#include <lg/scrservices.h>
#include <lg/objects.h>
#include <lg/links.h>
#include <lg/properties.h>
#include <lg/propdefs.h>

#include "scriptvars.h"
#include "TWBaseScript.h"
#include "TWBaseTrap.h"

#include <string>
#include <map>

/** A map type to make it easier to look up whether the AI has entered a
 *  room in which its breath should be visible. Maps room IDs to a flag
 *  indicating coldness (this will generally only ever store true, as
 *  a room not appearing in the map is assumed to be warm, but it may
 *  be useful to track warm rooms in future too...
 */
typedef std::map<int, bool> ColdRoomMap;
typedef ColdRoomMap::iterator   ColdRoomIter; //!< Convenience type for ColdRoomMap iterators
typedef ColdRoomMap::value_type ColdRoomPair; //!< Convenience type for ColdRoomMap key/value pairs


/** @class TWTrapAIBreath
 *
 * TWTrapAIBreath controls a particle attachment to an AI that allows the
 * simulation of visible breath in cold areas. Note that this script only
 * provides a general approximation of the effect: it does not attempt to
 * sync up with AI vocalisations (speech, whistles, etc), and probably never
 * will be able to. No message sent to scripts when an AI begins vocalising,
 * and in the absence of such a message there's no way to sync emission and
 * vocalisation.
 *
 * This script does the following:
 *
 * - Enables and disabled the attached particles automatically (no additional
 *   fnords and Rube Goldberg setup needed) when the AI enters and leaves
 *   concrete rooms listed as 'cold' in the script's design note.
 * - Scales the breath rate based on alertness level (the AI appears to
 *   breathe faster at higher alert levels).
 * - AIs continue breathing when knocked out, and the breath particles will be
 *   enabled and disabled automatically when the body is carried into or out
 *   of cold areas.
 * - When the AI is killed the particle group is disabled.
 *
 * There are a few remaining problems (in addition to the vocalisation sync
 * issue mentioned above):
 *
 * - When AIs are knocked out, the particle attachment can end up in the wrong
 *   place relative to the AI's face. This is difficult to solve because there
 *   is no vhot, joint, or other marker in front of the AI's face, and the
 *   orientation of the particle group does not seem to change when the AI is
 *   knocked out.
 * - The emitted particles are relative to the particle group, so sharp turns
 *   while the AI is breathing out can look a little odd from some angles. This
 *   is an unfortunate aspect of particle groups, and I'm not sure what the
 *   best way to handle it is.
 *
 * Initial Setup
 * -------------
 *
 * Before the script can be used, you need to set up the SFX and AI archetypes
 * to use it properly (you could set it up on a per-AI basis if you really like
 * pain).
 *
 * SFX Setup
 * =========
 *
 * Begin by creating a new SFX archetype for the breath particles. The following
 * instructions are for example only, you will want to adjust them to your own
 * requirements, aesthetics, and mission temperature.
 *
 * - Open the `Object Hierarchy` window
 * - Open the `Object -> SFX -> GasFX` tree
 * - Select `GasFX`
 * - Click `Add`, enter `AIBreath`
 * - Select `AIBreath`
 * - Click `Edit`
 * - Add `SFX -> Particles`
 * - Enter the following settings (if using fpuff, copy it from the demo map!):
 *
 *     Active: not checked  (**This is important! Do not set it active!**)
 *     Render Type: Bitmap disk
 *     Animation: Launched continually
 *     Group motion: Attached to object
 *     number of particles: 90
 *     size of particle: 0.5
 *     bitmap name: fpuff
 *     velocity: 0 0 0
 *     gravity vector: 0 0
 *     alpha: 45
 *     fixed-group radius: 1
 *     launch period: 0.01
 *     group-scale velocity: 1.25
 *     bm-disk flags: use lighting, fade-in at birth, grow-in at birth
 *     bm-disk birth time: 0.25
 *     bm-disk rot: 3 0 1.3
 *     bm-disk grow speed: 0.2
 *     bm-disk rgb: 245 245 245
 *
 * - Click `OK`
 * - Select the `SFX -> Particle Launch Info` parameter, click `Edit`
 * - Enter the following settings:
 *
 *     Launch type: Bounding Box
 *     Box min: 0.75 -0.1 0.2
 *     Box max: 0.75 0.1 0.2
 *     Velocity min: 0 -0.5 -2
 *     Velocity max: 3 0.5 2
 *     Min time: 0.4
 *     Max time: 1.0
 *
 * - Click `OK`
 * - Click `Done`
 *
 * AI Setup
 * ========
 *
 * Now that the particles are set up, you need to set up the AIs to use this
 * script and the particles. Note that what follows sets up the breath script
 * for all guards. This isn't really suitable to set up on the Animal archetype
 * (unless you have no spiders or rats in your mission, in which case it is
 * perfect to go on there!) and Human has no space left in its `S -> Scripts`.
 * If you need to set up the breath on other humans, you'll need to repeat
 * these steps as needed, unless you use `Animal` instead of `guard`.
 *
 * - Locate and select `Object -> physical -> Creature -> Animal -> Human -> guard`
 * - Click `Edit`
 * - Add `S -> Scripts`
 * - Set Script 0 to `TWTrapAIBreath`
 * - Click `OK`
 * - Add -> `Tweq -> Flicker`
 * - Enter the following settings (see note below about `Rate`):
 *
 *     Halt: Continue
 *     AnimC: [None]
 *     MiscC: Scripts
 *     CurveC: [None]
 *     Rate: 3000
 *
 * - Click `OK`
 * - Select the `FlickerState` parameter, click `Edit`
 * - Set `AnimS` to `On`
 * - Click `OK`
 * - Add `Links -> ~ParticleAttachement`
 * - Click `Add`
 * - Add a ~ParticleAttachement link from `guard` to `AIBreath`
 * - Select the new link, set its data to
 *
 *     Type: Joint
 *     vhot #: 0
 *     joint: Neck
 *     submod #: 0
 *
 * - Click `OK` twice
 * - Add `Editor -> Design Note`
 * - Enter any configuration settings you may want (this is a good place to set
 *   the `TWTrapAIBreathColdRooms` parameter).
 * - Click `Done`
 * - Click `Close`
 *
 * Configuration
 * -------------
 *
 * The script params are specified using the Editor -> Design Note, please see the
 * main documentation for more about this.  Parameters supported by TWTrapAIBreath
 * are listed below. If a parameter is not specified, the default value shown is
 * used instead. Note that all the parameters are optional, and if you do not
 * specify a parameter, the script will attempt to use a 'sane' default.
 *
 * Parameter: TWTrapAIBreathRate0
 *      Type: integer
 *   Default: 3000
 * This controls the the base breathing rate, the rate used when the AI is
 * at rest (alertness level 0). This is how long, in milliseconds, between one
 * inhale and another. For a normal human at rest that's generally about 3 to 5
 * seconds (3000 to 5000 milliseconds). As AIs will usually be walking around doing
 * things even 'at rest', going for the lower value is probably better, but you
 * should experiment to see what feels best for you. The base rate is used to
 * calculate the default values for the other three alertness levels (1, 2, and 3).
 *
 * Parameter: TWTrapAIBreathRate1, TWTrapAIBreathRate2, TWTrapAIBreathRate3
 *      Type: integer
 * These parameters allow the default values calculated from the base rate to be
 * overridden with rates you feel more appropriate to the alertness levels. Rate1
 * is used when the AI is at low alertness, Rate2 when the AI is at medium, and
 * Rate3 is used when the AI is at high alterness and actively searching, persuing,
 * or attacking the player. If the AI is at high alterness but is not actively
 * searching, persuing, or attacking then Rate2 is used instead.
 *
 * Parameter: TWTrapAIBreathInCold
 *      Type: boolean
 *   Default: false
 * If the AI starts out in a cold room, the script may not correctly detect this
 * fact. If you set this parameter to true, the AI is initialised as being in a
 * cold room, and everything should work as expected. Basically: if the AI is
 * in a cold room at the start of the mission, set this to true, otherwise you
 * can leave it out.
 *
 * Parameter: TWTrapAIBreathImmediate
 *      Type: boolean
 *   Default: true
 * If this is set to true, the breath particle group is deactivated as soon as
 * the AI exits from a cold room into a warm one. If it is false, the normal
 * exhale time will elapse before the group is deactivated. You might want to
 * set this to false if, for example, there is a very significant difference
 * in temperature between the cold and not-cold rooms, to simulate cold air
 * entering with the AI. This is something you should probably play around
 * with to see what works best for your mission.
 *
 * Parameter: TWTrapAIBreathStopOnKO
 *      Type: boolean
 *   Default: false
 * If this is false, the particle group attached to the AI continues to work
 * as normal when the AI has been knocked out (as knocked-out AIs need to
 * breathe too!). If set to true, the particle group is deactivated
 * permanently when the AI is knocked out. This has been provided because the
 * particle attachment will often place the particles in the wrong place when
 * the AI has been knocked out, making it look like the AI is breathing out
 * of their chest or armpit! If you prefer to just stop the particles on KO
 * rather than wait on a fix for this issue, set this to true.
 *
 * Parameter: TWTrapAIBreathExhaleTime
 *      Type: integer
 *   Default: 250
 * The amount of time, in milliseconds, that the AI will exhale for at rest.
 * This is the amount of time that the particle group will remain active for
 * during every breath cycle. It is hard limited so that you can not set it
 * to more than half of the breathing rate as set in the flicker tweq.
 *
 * Parameter: TWTrapAIBreathSFX
 *      Type: object
 *   Default: AIBreath
 * This should be the *archetype name* of the particle group attached to the
 * AI that should be used as the breath particles. If you followed the above
 * 'Initial Setup' instructions, you do not need to specify this, but if you
 * are using a different particle group for the breath you need to give the
 * name of its archetype here.
 *
 * Parameter: TWTrapAIBreathColdRooms
 *      Type: string
 *   Default: none
 * If you want to use the automatic activate/deactivate feature of this script
 * (which you do), rather that relying on a Heath Robinson/Rube Goldberg setup
 * to send the script TurnOn/TurnOff messages (which you don't want to do),
 * you need to list the rooms that should be considered to be cold rooms here.
 * You can specify multiple rooms by separating them with commas, and you can
 * use concrete room IDs or concrete room names here (use the ID if the name
 * contains a comma!). Note that you must set up concrete rooms for all your
 * designated cold areas.
 *
 * Parameter: TWTrapAIBreathLinkType
 *      Type: string
 *   Default: ~ParticleAttachement
 * Allows the link type used to attach the paricle group to the AI to be changed
 * from the default "~ParticleAttachement" to something else (like, for example,
 * Contains. If you use this parameter, be sure to check the spelling of the
 * link flavour - you will get errors in the monolog if the link type is
 * incorrect.
 */
class TWTrapAIBreath : public TWBaseTrap
{
public:
    TWTrapAIBreath(const char* name, int object) : TWBaseTrap(name, object),
                                                   start_cold(object, name, "InCold"),
                                                   stop_immediately(object, name, "Immediate"),
                                                   stop_on_ko(object, name, "StopOnKO"),
                                                   exhale_time(object, name, "ExhaleTime"),

                                                   rates{ { object, name, "BreathRate0" },
                                                          { object, name, "BreathRate1" },
                                                          { object, name, "BreathRate2" },
                                                          { object, name, "BreathRate3" }
                                                   },

                                                   particle_arch_name(object, name, "SFX"),
                                                   particle_link_name(object, name, "LinkType"),
                                                   proxy_arch_name(object, name, "Proxy"),
                                                   proxy_link_name(object, name, "ProxyLink"),

                                                   rooms(object, name, "ColdRooms"),
                                                   cold_rooms(),

                                                   last_level(-1),

                                                   SCRIPT_VAROBJ(TWTrapAIBreath, in_cold, object),
                                                   SCRIPT_VAROBJ(TWTrapAIBreath, still_alive, object),
                                                   SCRIPT_VAROBJ(TWTrapAIBreath, breath_timer, object)
        { /* fnord */ }

protected:
    /* ------------------------------------------------------------------------
     *  Initialisation related
     */

    /** Initialise the TWTrapAIBreath instance. This parses the various
     *  parameters from the design note, and sets up the script so that
     *  it can be used correctly.
     */
    void init(int time);


    /* ------------------------------------------------------------------------
     *  Message handling
     */

    /** Handle messages passed to the script. This is invoked whenever the
     *  script receives a message, and subclasses of this class will generally
     *  override or extend this function to provide script-specific behaviour.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_message(sScrMsg* msg, cMultiParm& reply);


    /** On message handler, called whenever the script receives an on message.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_onmsg(sScrMsg* msg, cMultiParm& reply);


    /** Off message handler, called whenever the script receives an off message.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_offmsg(sScrMsg* msg, cMultiParm& reply);

private:
    /** Abort the AI breath, deactivates the breth particle group immediately.
     *
     * @param cancel_timer If true, and the breath timer is active, this will
     *                     cancel the timer.
     */
    void abort_breath(bool cancel_timer = true);


    /** Start the display of the AI's breath. This will check that the tweq
     *  provided is appropriate and, if it is, the particle group attached to
     *  the AI showing the breath is activated.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus start_breath(sTweqMsg *msg, cMultiParm& reply);


    /** Deactivate the breath particle group. This is triggered by a timer
     *  set when the breath group was activated.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus stop_breath(sScrTimerMsg *msg, cMultiParm& reply);


    /** Called when the AI moves from one room to another. May call on_onmsg()
     *  of on_offmsg() depending on whether the new room is cold.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_objroomtransit(sRoomMsg *msg, cMultiParm& reply);


    /** Handle the IgnorePotion message - this is set when the AI gets
     *  knocked out, so use it as an indicator of knockedoutedness.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_ignorepotion(sScrMsg *msg, cMultiParm& reply);


    /** Handle changes in the AI's mode. This is primarily needed to stop
     *  the breath output if the AI is dead, but can be called on knockout.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_aimodechange(sAIModeChangeMsg *msg, cMultiParm& reply);


    /** Yep, he's not getting any better - deal with dead AIs by stopping
     *  their breath.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_slain(sScrMsg *msg, cMultiParm& reply);


    /** Update the breathing rate in response to AI Alertness changes. This
     *  will modify the breathing rate such that higher alertness levels will
     *  increase the breathing rate.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_aialertness(sAIAlertnessMsg *msg, cMultiParm& reply);


    /** Update the tweq rate in response to alertness changes.
     *
     * @param new_level The new alertness level, must be in the range 1 to 4.
     */
    void set_rate(int new_level = 1);


    /** Determine whether an AI on high alert is in active search/pursuit/attack
     *  mode, or really in a high alert patrol. If the AI is not at high alert,
     *  this does nothing. Similartly, if the AI is at high alert and is actively
     *  searching, pursuing, or attacking then this does nothing. If the AI as at
     *  high alert but is on patrol, this reduces the breathing rate to medium.
     */
    void check_ai_reallyhigh();


    /** Obtain the object ID of the particle group used to show the AI's breath.
     *  This looks for the first ~ParticleAttachement link to a particle group that
     *  inherits from particle_arch_name, and returns the ID of the object at the
     *  end of the link.
     *
     * @return The ID of the particle group on success, 0 if not found.
     */
    int get_breath_particles();


    int get_breath_proxy(object fallback);

    int get_breath_particlegroup(object from);

    /** Parse the list of cold rooms defined by the Design Note into the cold_rooms
     *  map for later lookup. The cold rooms string should contain a comma separated
     *  list of room ID numbers or names.
     *
     * @param coldstr A string containing the list of cold room names/ids.
     */
    void parse_coldrooms(const std::string& coldstr);

    // DesignNote configured options
    DesignParamBool start_cold;         //!< Start the AI off in a cold are?
    DesignParamBool stop_immediately;   //!< Stop the particle group immediately on leaving the cold?
    DesignParamBool stop_on_ko;         //!< Deactivate the particle group on knockout
    DesignParamTime  exhale_time;        //!< How long to leave the particle group active for at a time
    DesignParamTime  rates[4];           //!< Breathing rates, in millisecods, for each awareness level.
    DesignParamString              particle_arch_name; //!< The name of the particle group archetype to use
    DesignParamString              particle_link_name; //!< The link flavour used to link the particles to the AI (or proxy)
    DesignParamString              proxy_arch_name;    //!< The name of the particle proxy archetype to use
    DesignParamString              proxy_link_name;    //!< The link flavour used to link the proxy to the AI
    DesignParamString rooms;
    ColdRoomMap              cold_rooms;         //!< Which rooms are marked as cold?

    int                      last_level;         //!< Which level is currently set?

    // Persistent variables
    script_int               in_cold;            //!< Is the AI in a cold area?
    script_int               still_alive;        //!< Is the AI alive?
    script_handle<tScrTimer> breath_timer;       //!< A timer used to deactivate the group after exhale_time
};

#else // SCR_GENSCRIPTS

GEN_FACTORY("TWTrapAIBreath", "TWBaseTrap", TWTrapAIBreath)

#endif // SCR_GENSCRIPTS

#endif // TWTRAPAIBREATH_H
