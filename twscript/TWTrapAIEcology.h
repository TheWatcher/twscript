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

/* Basic requirements:
 *
 * Maintain count of number of AIs currently spawned (based on links?)
 * Respawn one AI every X seconds
 * Link to AI archetypes via scriptparams, weighted by param data.
 * Multiple spawn points linked to the script by script params links?
 * Respawn via a randomly chosen spawn point, weighted by script param data
 *
 * AIs need to have a script on them that optionally despawns them
 *    Respawn in place? (scriptparam link with "RespawnMe" in data?)
 *
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
 */
class TWTrapAIEcology : public TWBaseTrap
{
public:
    TWTrapAIEcology(const char* name, int object) : TWBaseTrap(name, object), refresh(500), poplimit(1),
                                                    archetype_link("%Weighted"),
                                                    spawnpoint_link("#Weighted"),
                                                    SCRIPT_VAROBJ(TWTrapAIEcology, enabled, object),
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

private:
    void start_timer(bool immediate = false);


    void stop_timer(void);

    /**
     */
    void attempt_spawn(sScrMsg* msg);


    bool spawn_needed(void);


    int select_archetype(sScrMsg *msg);


    int select_spawnpoint(sScrMsg *msg);


    void spawn_ai(int archetype, int spawnpoint);


    void copy_spawn_aiwatch(object src, object dest);

    int refresh;  //!< How frequently should the ecology be updated?
    int poplimit; //!< How many AIs should this ecology allow?

    std::string archetype_link;            //!< The string to use as a linkdef when searching for the archetype to spawn.
    std::string spawnpoint_link;           //!< The string to use as a linkdef when searching for spawn points.

    script_int               enabled;      //!< Is the ecology enabled?
    script_handle<tScrTimer> update_timer; //!< A timer used to update the ecology.
};

#else // SCR_GENSCRIPTS

GEN_FACTORY("TWTrapAIEcology", "TWBaseTrap", TWTrapAIEcology)

#endif // SCR_GENSCRIPTS

#endif // TWTRAPAIECOLOGY_H
