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
 * For full documentation on features/design note parameters, see the docs:
 * https://thief.starforge.co.uk/wiki/Scripting:TWTrapAIBreath
 *
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
