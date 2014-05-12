/** @file
 * This file contains the interface for the base trigger class. This class
 * adds trigger-sepcific facilities, including the ability to remap the
 * TurnOn and TurnOff messages sent when triggered, limit the number of
 * times the script will perform the TurnOn and TurnOff actions, and other
 * advanced trigger functions.
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

#ifndef TWBASETRIGGER_H
#define TWBASETRIGGER_H

#if !SCR_GENSCRIPTS

#include <string>
#include "TWBaseScript.h"
#include "SavedCounter.h"

class TWBaseTrigger : public TWBaseScript
{
public:
    /* ------------------------------------------------------------------------
     *  Public interface exposed to the rest of the game
     */

    /** Create a new TWBaseTrigger object. This sets up a new TWBaseTrigger object
     *  that is attached to a concrete object in the game world.
     *
     * @param name   The name of the script.
     * @param object The ID of the client object to add the script to.
     * @return A new TWBaseTrigger object.
     */
    TWBaseTrigger(const char* name, int object) : TWBaseScript(name, object),
                                                  turnon_msg("TurnOn"), turnoff_msg("TurnOff"), dest_str("&ControlDevice"),
                                                  fail_chance(0),
                                                  count(name, object), count_mode(CM_BOTH)
        { /* fnord */ }

protected:
    /* ------------------------------------------------------------------------
     *  Initialisation related
     */

    /** Initialise the trigger counters, message names, and other aspects of
     *  the trigger class that couldn't be handled in the constructor. This
     *  should be called as part of processing BeginScript, before any
     *  attempt to use the class' features is made.
     *
     * @param time The current sim time.
     */
    virtual void init(int time);


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
    virtual MsgStatus on_message(sScrMsg* msg, cMultiParm& reply);


    /** Send the defined 'On' message to the target objects.
     *
     * @return true if the message was sent, false otherwise.
     */
    bool send_on_message()
        { return send_trigger_message(true); }


    /** Send the defined 'Off' message to the target objects.
     *
     * @return true if the message was sent, false otherwise.
     */
    bool send_off_message()
        { return send_trigger_message(false); }


private:
    /* ------------------------------------------------------------------------
     *  Message handling
     */

    /** Send the trigger message to the trigger destination object(s). This should
     *  be called when the trigger fires to send the appropriate message to the
     *  destination object(s).
     *
     * @param send_on If true, this will send the 'on' message to the destination,
     *                otherwise it will send the 'off' message.
     * @return true if the message was sent, false otherwise.
     */
    bool send_trigger_message(bool send_on);


    /* ------------------------------------------------------------------------
     *  Variables
     */

    // Message names
    std::string turnon_msg;  //!< The name of the message that should be sent as a 'turnon'
    std::string turnoff_msg; //!< The name of the message that should be sent as a 'turnoff'

    // Destination setting
    std::string dest_str;    //!< Where should messages be sent?

    // Full of fail?
    int fail_chance;         //!< percentage chance of the trigger failing.

    // Count handling
    SavedCounter count;      //!< Control how many times the script will work
    CountMode    count_mode; //!< What counts as 'working'?

};

#else // SCR_GENSCRIPTS
GEN_FACTORY("TWBaseTrigger", "TWBaseScript", TWBaseTrigger)
#endif // SCR_GENSCRIPTS

#endif // TWBASETRIGGER_H
