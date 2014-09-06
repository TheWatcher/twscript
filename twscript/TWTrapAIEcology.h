/** @file
 * This file contains the interface for the TWTrapAIEcology classe.
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

#ifndef TWTRAPAIECOLOGY_H
#define TWTRAPAIECOLOGY_H

#if !SCR_GENSCRIPTS
#include <lg/interface.h>
#include <lg/scrmanagers.h>
#include <lg/scrservices.h>
#include <lg/objects.h>
#include <lg/links.h>
#include <lg/properties.h>
#include <lg/propdefs.h>
#include <string>
#include "TWBaseScript.h"
#include "TWBaseTrap.h"

/** @class TWTrapAIEcology
 *
 * TWTrapAIEcology is a script that allows a controlled population of AIs to be
 * spawned dynamically during play.
 *
 * Periodically this script will check whether the number of currently spawned AIs
 * has reached a configurable population limit. If the count is less than the
 * limit, the script will select an AI archetype to spawn, and then choose a spawn
 * point at which the AI should be created.
 *
 * At most one AI is spawned each time the script checks the current population.
 * This is a deliberate restriction imposed to prevent AIs being spawned on top of
 * each other).
 *
 * The AI archetype is chosen based on links from the TWTrapAIEcology host object
 * to archetypes. By default the script will use weighted random selection if
 * multiple AI archetypes are linked, and if only a single AI archetype is linked
 * that is always used).
 *
 * The spawn point is chosen based on links from the TWTrapAIEcology to concrete
 * objects in the world. If multiple spawn points are linked, one is chosen at
 * random, whereas if only one spawn point is linked that one is always used. The
 * default behaviour is to try and find the first randomly selected spawn point
 * that is not visible on screen. if all spawn points are on screen, spawning is
 * aborted. This behaviour can be disabled if desired.
 *
 *
 *
 * It is important to note that, while a fnord (a Marker or similar) could be used
 * to mark the location of a spawn point, using a standard fnord is NOT recommended.
 * fnord objects are not rendered (do not see the fnord, if you can't see the fnord
 * it can't eat you), this means that the* engine can not determine whether the
 * object is on screen: it will always say it is not because, well, you can't see it;
 * it's not rendered! While this is a fine attribute for a normal fnord to have, if
 * you want to make sure that AIs are always spawned off-screen, you can't use a fnord
 * unless you give it a transparent, physicsless object and change its Render Type
 * to 'Normal'.



 *
 * Multiple spawn points may be defined, and the spawn point
 *
 * Configuration
 * -------------
 * Parameters are specified using the Editor -> Design Note, please see the
 * main documentation for more about this.  Parameters supported by TWTrapSetSpeed
 * are listed below. If a parameter is not specified, the default value shown is
 * used instead. Note that all the parameters are optional, and if you do not
 * specify a parameter, the script will attempt to use a 'sane' default.
 *
 * Parameter:
 *      Type:
 *   Default:
 */
class TWTrapAIEcology : public TWBaseTrap
{
public:
    TWTrapAIEcology(const char* name, int object) : TWBaseTrap(name, object), refresh(30000), refresh_qvar(), pop_limit(1), pop_qvar(), lives(0), lives_qvar(), spawned_qvar(), allow_visible_spawn(false),
                                                    archetype_link("&%Weighted"),
                                                    spawnpoint_link("&!#ScriptParams"),
                                                    SCRIPT_VAROBJ(TWTrapAIEcology, enabled, object),
                                                    SCRIPT_VAROBJ(TWTrapAIEcology, population, object),
                                                    SCRIPT_VAROBJ(TWTrapAIEcology, spawned, object),
                                                    SCRIPT_VAROBJ(TWTrapAIEcology, update_timer, object)
        { /* fnord */ }

protected:
    /* ------------------------------------------------------------------------
     *  Initialisation related
     */

    /** Initialise the TWTrapAIEcology instance. This parses the various
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


    /** Handle 'turn on' messages received by the script. This is invoked when
     *  the script receives the message it interprets as a 'turn on' instruction
     *  (TurnOn by default).
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_onmsg(sScrMsg* msg, cMultiParm& reply);


    /** Handle 'turn off' messages received by the script. This is invoked when
     *  the script receives the message it interprets as a 'turn off' instruction
     *  (TurnOff by default).
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_offmsg(sScrMsg* msg, cMultiParm& reply);


    /** Timer message handler, called whenever the script receives a timer message.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_timer(sScrTimerMsg* msg, cMultiParm& reply);


    /** AI despawn message handler, called whenever the script receives a "Despawned"
     *  message from an AI it has spawned.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_despawn(sScrMsg* msg, cMultiParm& reply);


    /** Spawn count reset message handler, called whenever the script receives a "ResetSpawned"
     *  message.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_resetspawned(sScrMsg* msg, cMultiParm& reply);

private:
    /** Enable the spawn timer. This starts a timer that will, when it fires, result in
     *  a spawn attempt by the ecology. If the immediate parameter is true, this will
     *  ignore the configured timer rate and start a 100ms timer - this is intended to
     *  be used as part of init() to ensure that the first spawn happens soon after init
     *  rather than a potentially multi-minute delay.
     *
     * @param immediate If true, the timer is set to go off in 100ms, otherwise the
     *                  configured spawn rate is used for the timer.
     */
    void start_timer(bool immediate = false);


    /** Disable the spawn timer, cancelling any currently pending timer.
     */
    void stop_timer(void);


    /** Determine whether a spawn is needed, and if one is attempt to spawn an AI at
     *  a spawn point. The exact behaviour of this function depends somewhat on the
     *  link definitions used for the archetype and spawn point queries, but it is
     *  capable of spawning a randomly chosen AI at a randomly chosen spawn point,
     *  choosing the spawn point that is not visible unless allow_visible_spawn is
     *  enabled.
     *
     * @param msg A pointer to the message that triggered this spawn (needed for
     *            certain linkdef types.
     */
    void attempt_spawn(sScrMsg* msg);


    /** Determine whether enough AIs are currently spawned by this ecology, or whether
     *  we MUST BUILD ADDITIONAL PYLONS.
     *
     * @return true if the number of spawned AIs is less than the population limit,
     *         false if the limit has been reached.
     */
    bool spawn_needed(void);


    /** Locate the object ID of the archetype to spawn at the selected spawn point.
     *  This will return an archetype, which will hopefully be an AI (it doesn't
     *  actually verify that the selected archetype is an AI, so you could potentially
     *  use it to spawn other things... no idea how well that'd work in practice though.)
     *
     * @param msg A pointer to the message that triggered this spawn (needed for
     *            certain linkdef types.
     * @return An archetype ID on success, 0 if the archetype link def doesn't actually
     *         match any archetypes somehow.
     */
    int select_archetype(sScrMsg *msg);


    /** Locate a spawn point to spawn the AI at. This will try to locate a concrete object
     *  to use as the reference location to spawn an AI at. If allow_visible_spawn is false,
     *  this will try to find a spawn point that is not on screen - if there are no off-screen
     *  spawn pointes, or the spawn point link definition does not match any concrete objects,
     *  this will return 0.
     *
     * @param msg A pointer to the message that triggered this spawn (needed for
     *            certain linkdef types.
     * @return The ID of the concrete object to spawn the AI at, or 0 if no suitable concrete
     *         objects can be located.
     */
    int select_spawnpoint(sScrMsg *msg);


    /** Attempt to spawn the AI with the specific archetype ID at the specified spawn point object.
     *  This will create the AI at the spawnpoint (potentially including any offset from the object
     *  if needed), and it will duplicate any AIWatchObj links on the spawn point onto the new AI.
     *
     * @param archetype  The ID of the archetype to spawn.
     * @param spawnpoint The ID of the object to use as the reference point for the spawn.
     */
    void spawn_ai(int archetype, int spawnpoint);


    /** Duplicate any AIWatchObj links on the src object onto the destination.
     *
     * @param src  The object to copy the AIWatchObj links from.
     * @param dest The object to copy the AIWatchObj links to.
     */
    void copy_spawn_aiwatch(object src, object dest);


    /** Determine whether the spawn point object is visible. This will return the provided
     *  object ID if the object is not visible, otherwise it will return 0.
     *
     * @param target The ID of the object to check the visibility of.
     * @return The object ID if the object is not visible, otherwise 0.
     */
    int check_spawn_visibility(int target);


    /** Obtain the location and facing rotation to spawn the AI at based on the specified spawn
     *  point. This will use the spawn point's x,y,z as the location, potentially adjusted by a
     *  TWTrapAIEcologySpawnOffset, and the spawn point's heading for the facing direction (pitch
     *  and bank are both hard-forced to zero to prevent problems with AIs behaving incorrectly
     *  when P or B are non-zero.)
     *
     * @param spawnpoint The ID of the spawn point object.
     * @param location   A reference to a vector to store the spawn location in.
     * @param facing     A reference to a vector to store the spawn rotation in.
     */
    void get_spawn_location(int spawnpoint, cScrVec& location, cScrVec& facing);


    /** Handle increasing the current count of spawned AIs after a spawn.
     */
    void increase_spawncount(void);


    /** Build links between the AI and the ecology for firer counting, and copy any
     *  AIWatchObj links from the spawn point to the AI.
     *
     * @param combined The combined IDs of the AI and the spawn point. The lower 16
     *                 bits are the spawnpoint ID, the upper 16 are the spawn object ID.
     */
    void fixup_links(int combined);


    /** Determine whether the population limit should be read from a qvar rather than the design note,
     *  and if so update its value if needed.
     */
    void update_pop_limit(void);


    /** Determine whether the update rate should be read from a qvar rather than the design note,
     *  and if so update its value if needed.
     */
    void update_refresh(void);


    /** Calculate a combined ID using the provided spawnpoint and spaned AI IDs.
     *
     * @param spawnid      The ID of the spawned AI.
     * @param spawnpointid The ID of the spawn point the AI was spawned from.
     * @return A combined ID.
     */
    inline int combined_id(int spawnid, int spawnpointid) {
        return(spawnid << 16 | spawnpointid);
    }


    /** Given a combined ID, extract the ID of the spawned AI.
     *
     * @param combined The combined ID as generated by combined_id()
     * @return The ID of the spawned AI.
     */
    inline int spawn_id(int combined) {
        return((combined & 0xFFFF0000) >> 16);
    }


    /** Given a combined ID, extract the ID of the spawn point.
     *
     * @param combined The combined ID as generated by combined_id()
     * @return The ID of the spawn point object.
     */
    inline int spawnpoint_id(int combined) {
        return(combined & 0xFFFF);
    }


    int  refresh;                          //!< How frequently should the ecology be updated?
    std::string refresh_qvar;              //!< If the refresh rate is controlled by a qvar, the name goes here.
    int  pop_limit;                        //!< How many AIs should this ecology allow?
    std::string pop_qvar;                  //!< If the population is controlled by a qvar, the name goes here.
    int  lives;                            //!< Should there be an upper limit to the number of AIs that are ever spawned?
    std::string lives_qvar;                //!< If the number of lives is controlled by a qvar, the name goes here.
    std::string spawned_qvar;              //!< The name of the qvar to store the total number of spawned AIs.

    bool allow_visible_spawn;              //!< Should spawns be allowed to happen on-screen?

    std::string archetype_link;            //!< The string to use as a linkdef when searching for the archetype to spawn.
    std::string spawnpoint_link;           //!< The string to use as a linkdef when searching for spawn points.

    script_int               enabled;      //!< Is the ecology enabled?
    script_int               population;   //!< The number of currently spawned AIs
    script_int               spawned;      //!< The number of AIs spawned from the start.
    script_handle<tScrTimer> update_timer; //!< A timer used to update the ecology.
};

#else // SCR_GENSCRIPTS

GEN_FACTORY("TWTrapAIEcology", "TWBaseTrap", TWTrapAIEcology)

#endif // SCR_GENSCRIPTS

#endif // TWTRAPAIECOLOGY_H
