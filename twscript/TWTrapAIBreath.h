/** @file
 * This file contains the interface for the TWTrapAIBreath classe.
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
 */
class TWTrapAIBreath : public TWBaseTrap
{
public:
    TWTrapAIBreath(const char* name, int object) : TWBaseTrap(name, object), stop_immediately(false), exhale_time(250), particle_arch_name(), cold_rooms(),
                                                   SCRIPT_VAROBJ(TWTrapAIBreath, base_rate, object),
                                                   SCRIPT_VAROBJ(TWTrapAIBreath, in_cold, object),
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
    MsgStatus set_rate(sAIAlertnessMsg *msg, cMultiParm& reply);


    /** Obtain the object ID of the particle group used to show the AI's breath.
     *  This looks for the first ~ParticleAttachement link to a particle group that
     *  inherits from particle_arch_name, and returns the ID of the object at the
     *  end of the link.
     *
     * @return The ID of the particle group on success, 0 if not found.
     */
    int get_breath_particles();


    /** Parse the list of cold rooms defined by the Design Note into the cold_rooms
     *  map for later lookup. The cold rooms string should contain a comma separated
     *  list of room ID numbers or names.
     *
     * @param coldstr A string containing the list of cold room names/ids.
     */
    void parse_coldrooms(char* coldstr);


    // DesignNote configured options
    bool                     stop_immediately;   //!< Stop the particle group immediately on leaving the cold?
    int                      exhale_time;        //!< How long to leave the particle group active for at a time
    std::string              particle_arch_name; //!< The name of the particle group archetype to use
    ColdRoomMap              cold_rooms;         //!< Which rooms are marked as cold?

    // Taken from TweqBlink rate
    script_int               base_rate;          //!< What is the base breathing rate?

    // Persistent variables
    script_int               in_cold;            //!< Is the AI in a cold area?
    script_handle<tScrTimer> breath_timer;       //!< A timer used to deactivate the group after exhale_time
};

#else // SCR_GENSCRIPTS

GEN_FACTORY("TWTrapAIBreath", "TWBaseTrap", TWTrapAIBreath)

#endif // SCR_GENSCRIPTS

#endif // TWTRAPAIBREATH_H
