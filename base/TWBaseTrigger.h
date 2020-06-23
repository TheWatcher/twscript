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

#include <random>
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
                                                  turnon_msg (object, name, "TOn"),
                                                  turnoff_msg(object, name, "TOff"),

                                                  isstim { false, false }, stimob { 0, 0 },
                                                  intensity_min { 0.0f, 0.0f },
                                                  intensity_max { -1.0f, -1.0f },

                                                  dest(object, name, "TDest"),
                                                  remove_links(object, name, "KillLinks"),

                                                  fail_chance(object, name, "FailChance"),

                                                  count_dp(object, name, "TCount"),
                                                  count(name, object, "count"),
                                                  count_mode(object, name, "TCountOnly"),

                                                  generator(0),
                                                  uni_dist(0, 100)
        { /* fnord */ }

protected:
    enum MessageMode {
        SEND_OFF,
        SEND_ON
    };

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
    bool send_on_message(sScrMsg* msg)
        { return send_trigger_message(true, msg); }


    /** Send the defined 'Off' message to the target objects.
     *
     * @return true if the message was sent, false otherwise.
     */
    bool send_off_message(sScrMsg* msg)
        { return send_trigger_message(false, msg); }


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
    bool send_trigger_message(bool send_on, sScrMsg* msg);


    /* ------------------------------------------------------------------------
     *  Miscellaneous
     */

    void process_designnote(const std::string& design_note, const int time);


    /** Determine whether the specified message is actually a stimulus request, and
     *  if so attempt to set up the settings based on the message.
     *
     */
    bool check_stimulus_message(const char* message, int* obj, float* int_min, float* int_max);


    /** Generate an intensity value between the two specified intensities.
     */
    float make_intensity(float min, float max);

    /* ------------------------------------------------------------------------
     *  Variables
     */

    // Message names
    DesignParamString turnon_msg;    //!< The name of the message that should be sent as 'TurnOn'
    DesignParamString turnoff_msg;   //!< The name of the message that should be sent as 'TurnOff'

    // Stimulus for on/off
    bool  isstim[2];         //!< Is the turnon/off message a stimulus rather than a message?
    int   stimob[2];         //!< The stimulus object to use on turnon/off.
    float intensity_min[2];  //!< The lower range of the stimulus intensity to use.
    float intensity_max[2];  //!< The upper range of the stumulus intensity to use.

    // Destination setting
    DesignParamTarget dest;       //!< Where should messages be sent?
    DesignParamBool remove_links; //!< Remove links after sending messages?

    // Full of fail?
    DesignParamInt fail_chance; //!< percentage chance of the trigger failing.

    // Count handling
    DesignParamCapacitor count_dp;
    SavedCounter         count;      //!< Control how many times the script will work

    DesignParamCountMode count_mode; //!< What counts as 'working'?

    // Randomness
    std::mt19937 generator;
    std::uniform_int_distribution<int> uni_dist;   //!< a uniform distribution for fail checking.
};

#else // SCR_GENSCRIPTS
GEN_FACTORY("TWBaseTrigger", "TWBaseScript", TWBaseTrigger)
#endif // SCR_GENSCRIPTS

#endif // TWBASETRIGGER_H
