/** @file
 * This file contains the interface for the TWTriggerAIEcologyFireShadow classe.
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

/*
 *
 */

#ifndef TWTRIGGERAIECOLOGYFIRESHADOW_H
#define TWTRIGGERAIECOLOGYFIRESHADOW_H

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

/** @class TWTriggerAIEcologyFireShadow
 *
 */
class TWTriggerAIEcologyFireShadow : public TWBaseTrigger
{
public:
    TWTriggerAIEcologyFireShadow(const char* name, int object) : TWBaseTrigger(name, object), refresh(1000),
                                                                 SCRIPT_VAROBJ(TWTriggerAIEcologyFireShadow, update_timer, object)
        { /* fnord */ }

protected:
    /* ------------------------------------------------------------------------
     *  Initialisation related
     */

    /** Initialise the TWTriggerAIEcologyFireShadow instance. This parses the various
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

private:
    bool attempt_despawn(sScrMsg *msg);

    void fire_corseparts(void);

    void fireshadow_flee(void);

    void speedup(void);

    int  refresh;                          //!< How frequently should the despawn happen after death?

    script_handle<tScrTimer> update_timer; //!< A timer used to despawn the AI
};

#else // SCR_GENSCRIPTS

GEN_FACTORY("TWTriggerAIEcologyFireShadow", "TWBaseTrigger", TWTriggerAIEcologyFireShadow)

#endif // SCR_GENSCRIPTS

#endif // TWTRIGGERAIECOLOGYFIRESHADOW_H
