/** @file
 * This file contains the interface for the TWTriggerAIEcologySlain class.
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

#ifndef TWTRIGGERAIECOLOGYSLAIN_H
#define TWTRIGGERAIECOLOGYSLAIN_H

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

/** @class TWTriggerAIEcologySlain
 * This script should be used on AIs spawned by a TWTrapAIEcology that vanish
 * when slain (like elementals). This script informs the AIEcology that the AI
 * has been removed as soon as it is slain, and it does not attempt any despawn
 * behaviour on its own.
 *
 * For full documentation on features/design note parameters, see the docs:
 * https://thief.starforge.co.uk/wiki/Scripting:TWTriggerAIEcologySlain
 */
class TWTriggerAIEcologySlain : public TWBaseTrigger
{
public:
    TWTriggerAIEcologySlain(const char* name, int object) : TWBaseTrigger(name, object)
        { /* fnord */ }

protected:
    /* ------------------------------------------------------------------------
     *  Initialisation related
     */

    /** Initialise the TWTriggerAIEcologySlain instance. This parses the various
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


    /** Slain message handler, called whenever the script receives a slain message.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_slain(sSlayMsg* msg, cMultiParm& reply);
};

#else // SCR_GENSCRIPTS

GEN_FACTORY("TWTriggerAIEcologySlain", "TWBaseTrigger", TWTriggerAIEcologySlain)

#endif // SCR_GENSCRIPTS

#endif // TWTRIGGERAIECOLOGYSLAIN_H
