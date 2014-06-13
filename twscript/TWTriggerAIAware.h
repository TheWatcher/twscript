/** @file
 * This file contains the interface for the TWTriggerAIAware class.
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

#ifndef TWTRIGGERAIAWARE_H
#define TWTRIGGERAIAWARE_H

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
#include "TWBaseTrigger.h"


/** @class TWTriggerAIAware
 *
 */
class TWTriggerAIAware : public TWBaseTrigger
{
public:
    TWTriggerAIAware(const char* name, int object) : TWBaseTrigger(name, object), refresh(500), trigger_level(kModerateAlert), trigger_object(0),
                                                     SCRIPT_VAROBJ(TWTriggerAIAware, update_timer, object),
                                                     SCRIPT_VAROBJ(TWTriggerAIAware, is_linked, object)
        { /* fnord */ }

protected:
    /* ------------------------------------------------------------------------
     *  Initialisation related
     */

    /** Initialise the TWTriggerAIAware instance. This parses the various
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


    /** Alertness message handler.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_alertness(sAIAlertnessMsg* msg, cMultiParm& reply);


    /** Timer message handler, called whenever the script receives a timer message.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_timer(sScrTimerMsg* msg, cMultiParm& reply);


    /** Slain message handler, called whenever the script receives a slain message.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_slain(sSlayMsg* msg, cMultiParm& reply);


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

private:
    void stop_timer(void);
    void check_awareness(sScrMsg* msg);

    int refresh;                           //!< How frequently should the state be updated?
    eAIScriptAlertLevel trigger_level;     //!< The level at which the trigger should fire an On message
    object trigger_object;                 //!< The object (or archetype) that must be linked before the trigger happens

    script_handle<tScrTimer> update_timer; //!< A timer used to update the trigger
    script_int               is_linked;    //!< Is the target currently linked?
};

#else // SCR_GENSCRIPTS

GEN_FACTORY("TWTriggerAIAware", "TWBaseTrigger", TWTriggerAIAware)

#endif // SCR_GENSCRIPTS

#endif // TWTRIGGERAIAWARE_H
