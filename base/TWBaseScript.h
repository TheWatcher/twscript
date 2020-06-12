/** @file
 * This file contains the interface for the base script class.
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

#ifndef TWBASESCRIPT_H
#define TWBASESCRIPT_H

#if !SCR_GENSCRIPTS

#include <lg/config.h>
#include <lg/objstd.h>
#include <vector>
#include <string>
#include "Script.h"
#include "DesignParam.h"


/** A replacement for cBaseScript from Public Scripts. This class is a replacement
 *  for the cBaseScript found in Public Scripts that modifies the way in which
 *  message handling is performed by the script, and introduces a significant
 *  number of advanced features for subclasses to take advantage of.
 */
class TWBaseScript : public cScript
{
public:
    /* ------------------------------------------------------------------------
     *  Public interface exposed to the rest of the game
     */

    /** Create a new TWBaseScript object. This sets up a new TWBaseScript object
     *  that is attached to a concrete object in the game world.
     *
     * @param name   The name of the script.
     * @param object The ID of the client object to add the script to.
     * @return A new TWBaseScript object.
     */
    TWBaseScript(const char* name, int object) : cScript(name, object), debug(object, name, "Debug"), need_fixup(true), sim_running(false), message_time(0), done_init(false)
        { /* fnord */ }


    /** Entrypoint for messages recieved from the game. All messages sent to
     *  the object a script is placed on get sent to this function for handling.
     *  This internally provides debugging and exception handling to prevent
     *  exceptions getting back into the game code. Subclasses should generally
     *  never attempt to override this function: they should override on_message
     *  instead.
     *
     * @param msg   A pointer to the script message data.
     * @param reply A pointer to somewhere to store reply data. The type this is
     *              pointing to depends on the type of message, in horribly-undocumented
     *              fashion.
     * @param trace A trace control flag, currently supports kNoAction (do nothing),
     *              or kSpew (write debugging info to monolog). Other values are
     *              ignored.
     * @return S_OK on success, S_FALSE on error.
     */
    STDMETHOD(ReceiveMessage)(sScrMsg* msg, sMultiParm* reply, eScrTraceAction trace);


protected:
    /* ------------------------------------------------------------------------
     *  Initialisation related
     */

    /** Initialise the debug flag and anything else that couldn't be handled in
     *  the constructor. This should be called as part of processing BeginScript,
     *  before any attempt to use the class' features is made.
     *
     * @param time The current sim time.
     */
    virtual void init(int time);


    /* ------------------------------------------------------------------------
     *  Message handling
     */

    /** Values that can be returned from on_message to indicate whether the caller
     *  should continue processing the message. Note that the caller is free to
     *  ignore these values and process the message anyway, but they are needed
     *  to allow for convenient handling of Count/Capacitor behaviour.
     */
    enum MsgStatus {
        MS_CONTINUE = 0, //!< Continue processing, msg hasn't been handled, or has but can be further processed
        MS_HALT,         //!< Indicates that the message has been handled, and needs no further processing
        MS_ERROR,        //!< An error occurred, don't continue processing
    };


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


    /* ------------------------------------------------------------------------
     *  Sim checking functions
     */

    /** A simple way to determine whether the game is currently running. This
     *  will return true when in game, false otherwise.
     *
     * @return true if the game is running, false if it is not.
     */
    bool in_sim(void) const { return sim_running; }


    /** Obtain the sim time in the last received message. Each message contains
     *  a sim time, this function provides a means of getting at that time even
     *  if a given member function doesn't have direct access to the message.
     *  Note that sim time ticks roughly 1 per millisecond, except when the game
     *  is paused (where it stops ticking), but you should probably not rely on
     *  it as a millisecond timer!
     *
     * @return The current sim time.
     */
    uint get_sim_time(void) const { return message_time; }


    /* ------------------------------------------------------------------------
     *  Message convenience functions
     */

    /** Send a message to the specified object, and store the result in the
     *  reply parameter. This sends a message and waits for it to be processed
     *  (so you probably want to avoid indirect recursion...).
     *
     * @warning The arguments to this function are in a different order to those
     *          for Telliamed's BaseScript::SendMessage()!
     *
     * @param dest    The ID of the object to send the message to.
     * @param message The message string, eg "TurnOn", "TurnOff", etc.
     * @param reply   A variable to store the reply in. String or vector returns
     *                should be freed by the caller.
     * @param data    Optional data to pass to the destination in sScrMsg::data
     * @param data2   Optional data to pass to the destination in sScrMsg::data2
     * @param data3   Optional data to pass to the destination in sScrMsg::data3
     * @return A pointer to the variable passed in via `reply` (I think!)
     */
    cMultiParm* send_message(object dest, const char* message, cMultiParm& reply, const cMultiParm& data = cMultiParm::Undef, const cMultiParm& data2 = cMultiParm::Undef, const cMultiParm& data3 = cMultiParm::Undef);


    /** Post a message to the specified object, continuing immediately without
     *  waiting for the message to be processed.
     *
     * @param dest    The ID of the object to send the message to.
     * @param message The message string, eg "TurnOn", "TurnOff", etc.
     * @param data    Optional data to pass to the destination in sScrMsg::data
     * @param data2   Optional data to pass to the destination in sScrMsg::data2
     * @param data3   Optional data to pass to the destination in sScrMsg::data3
     */
    void post_message(object dest, const char* message, const cMultiParm& data = cMultiParm::Undef, const cMultiParm& data2 = cMultiParm::Undef, const cMultiParm& data3 = cMultiParm::Undef);


    /** Create a timed message to be sent to the client object after the
     *  specified delay. This creates a timed message that will be sent to
     *  the object the script is attached to after the specified millisecond
     *  delay has elapsed.
     *
     * @param message The message string to send.
     * @param time    How many milliseconds to wait before sending the message.
     * @param type    Either kSTM_OneShot to send the message once, or kSTM_Periodic
     *                to repeat the message every `time` milliseconds.
     * @param data    Optional data to pass to the destination in sScrMsg::data
     * @return A timer struct for the queued message (really a pointer, but hidden
     *         inside a typedef).
     */
    tScrTimer set_timed_message(const char* message, ulong time, eScrTimedMsgKind type, const cMultiParm& data = cMultiParm::Undef);


    /** Abort sending of a timed message. This stops a timed message from being sent,
     *  including cancelling any periodic repeats of the message.
     *
     * @param time The timer struct for the timed message to kill.
     */
    void cancel_timed_message(tScrTimer timer);


    /* ------------------------------------------------------------------------
     *  Script data handling
     */

    /** Store arbitrary data in the persistent script database. If scripts need
     *  to store data that will persist across save games they can store arbitrary
     *  data in the script manager's database. This is a convenience function that
     *  simplifies the process of setting data in the database with a name that
     *  can be used to retrieve it.
     *
     * @param name The name to store the data under.
     * @param data The data to store in the database. Note that strings and vectors
     *             will be duplicated into the database.
     */
    void set_script_data(const char* name, const cMultiParm& data);


    /** Determine whether script data with the specified name has been set.
     *
     * @param name The name of the script data to look for,
     * @return true if the script data is set, false otherwise.
     */
    bool is_script_data_set(const char* name);


    /** Obtain script data from the persistent script database. This attempts to
     *  fetch the script data with the specified name and store it in the provided
     *  multiparm. Note that string and vector data will be copied into the data
     *  if successful.
     *
     * @param name The name of the script data to fetch.
     * @param data A reference to a multiparm variable to store the data in.
     * @return true if the data with the specified name has been fetched, false
     *         otherwise.
     */
    bool get_script_data(const char* name, cMultiParm& data);


    /** Erase script data from the persistent script database. This removes the
     *  data with the specified name from the persistent store.
     *
     * @param name The name of the script data to fetch.
     * @param data A reference to a multiparm variable in which to store
     *             the data that was removed from the database.
     * @return true if the data was erased, false otherwise.
     */
    bool clear_script_data(const char* name, cMultiParm& data);


    /* ------------------------------------------------------------------------
     *  Debugging support
     */

    /** Different levels of debug messages supported by the debug_printf() function.
     *  Note that this enum is used to index into the `debug_levels` array: ensure
     *  that the array has matching entries for each level specified here!
     */
    enum DebugLevel {
        DL_DEBUG = 0, //!< General debugging message, informational content only.
        DL_WARNING,   //!< An important debugging message indicating there may be a problem.
        DL_ERROR      //!< A serious problem has been encountered.
    };

    static const char*  const debug_levels[]; //!< An array of debug level strings


    /** Has the editor enabled debugging via the design note?
     *
     * @return true if the editor has enabled debugging, false otherwise.
     */
    inline bool debug_enabled(void) const
        { return debug; }


    /** Print out a debugging message to the monolog. This prints out a formatted
     *  string to the monolog, suitable for status and debugging messages in
     *  scripts. Note that the format string provided will be modified to
     *  include the script name, object name (or archetype name) and ID number
     *  of the object the script is attached to - the caller does not need to
     *  include this information explicitly.
     *
     * @param level  The debug message level
     * @param format A sprintf compatible format string.
     * @param ...    Optional additional arguments for the format.
     */
    void debug_printf(DebugLevel level, const char* format, ...);


    /** Obtain a string containing the specified object's name (or archetype name),
     *  and its ID number. This builds a 'human readable' version of the object
     *  id and name that can be used when generating debugging messages.
     *
     * @param name   A reference to a string object to store the name in.
     * @param obj_id The ID of the object to obtain the name and number of. If not
     *               provided, this defaults to the current object ID.
     */
    void get_object_namestr(std::string& name, object obj_id);


    /** Obtain a string containing the current object's name (or archetype name),
     *  and its ID number. This builds a 'human readable' version of the object
     *  id and name that can be used when generating debugging messages.
     *
     * @param name   A reference to a string object to store the name in.
     */
    void get_object_namestr(std::string& name);


    /* ------------------------------------------------------------------------
     *  QVar convenience functions
     */

    /** Update the value stored in the specified QVar. This updates the value set
     *  for the qvar to the specified value, hopefully creating the qvar if it does
     *  not already exist.
     *
     * @param qvar   The name of the qvar to store the value in.
     * @param value  The value to store in the qvar.
     */
    void set_qvar(const std::string &qvar, const int value);


    /** Fetch the value stored in a qvar, potentially applying a simple calculation
     *  to the value set in the QVar. This takes a string of the form
     *  qvarname([*+/](value|$qvarname)) and parses out the name, and operation
     *  and value or qvar name if they are present, and returns the value in the
     *  left hand side qvar, potentially modified by the operator and value or
     *  the value in the right hand qvar. For example, if the qvar variable
     *  contains 'foo/100' this will take the value in foo and divide it by 100.
     *  If the quest variable does not exist, this returns the default value
     *  specified *without applying any calculations to it*. The value may
     *  optionally be another QVar by placing $ before its name, eg: foo/$bar will
     *  divide the value in foo by the value in bar.
     *
     * @param qvar    The name of the QVar to return the value of, possibly including simple maths.
     * @param def_val The default value to return if the qvar does not exist.
     * @return The QVar value, or the default specified.
     */
    int get_qvar_value(std::string& qvar, int def_val);


    /** Fetch the value stored in a qvar, potentially applying a simple calculation
     *  to the value set in the QVar. This takes a string of the form
     *  qvarname([*+/](value|$qvarname)) and parses out the name, and operation
     *  and value or qvar name if they are present, and returns the value in the
     *  left hand side qvar, potentially modified by the operator and value or
     *  the value in the right hand qvar. For example, if the qvar variable
     *  contains 'foo/2.5' this will take the value in foo and divide it by 2.5.
     *  If the quest variable does not exist, this returns the default value
     *  specified *without applying any calculations to it*. The value may
     *  optionally be another QVar by placing $ before its name, eg: foo/$bar will
     *  divide the value in foo by the value in bar.
     *
     * @param qvar    The name of the QVar to return the value of, possibly including simple maths.
     * @param def_val The default value to return if the qvar does not exist.
     * @return The QVar value, or the default specified.
     */
    float get_qvar_value(std::string& qvar, float def_val);


    /* ------------------------------------------------------------------------
     *  Link inspection
     */

    /** Inspect the links with the specified flavour from the 'from' object, and locate the first link to an object
     *  with either the specified name, or an object that descends from an archetype with the specified name. If an
     *  appropriate link is found, return the ID of the object at the end of it.
     *
     * @param from      The ID of the object to look at links from.
     * @param obj_name  The name of the object to look for a link to, or the name of an archetype the target object should
     *                  be a concrete instance of.
     * @param link_name The name of the link flavour to look for.
     * @param fallback  An optional default ID to return if no matching object has been located.
     * @return The target object ID, or the fallback ID if no match has been located.
     */
    int get_linked_object(const int from, const std::string& obj_name, const std::string& link_name, const int fallback = 0);

private:
    /* ------------------------------------------------------------------------
     *  Message handling
     */

    /** Handle message dispatch. This enforces some low-level vital message processing
     *  before passing the message to on_message to actually handle.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return S_OK (zero) if the message has been processed successfully,
     *         S_FALSE if an error occurred.
     */
    long dispatch_message(sScrMsg* msg, sMultiParm* reply);

    /* ------------------------------------------------------------------------
     *  Miscellaneous stuff
     */

    /** Ensure that links from this script to the PlayerFactory actually go to
     *  Garrett instead. This is inherited from PublicScripts, I've no idea how
     *  vital it actually is, and including it is easier than finding out what
     *  breaks without it, so here it is. Mysterious functions. Yey.
     */
    void fixup_player_links(void);

    /* ------------------------------------------------------------------------
     *  Variables
     */
    DesignParamBool debug; //!< Is debugging enabled?
    bool need_fixup;       //!< Does the script need to fix links to the player?
    bool sim_running;      //!< Is the sim currently running?
    uint message_time;     //!< The sim time stored in the last recieved message

    bool done_init;        //!< Has the script run its init?

    static const uint NAME_BUFFER_SIZE;
};

#else // SCR_GENSCRIPTS
GEN_FACTORY("TWBaseScript", "CustomScript", TWBaseScript)
#endif // SCR_GENSCRIPTS

#endif // TWBASESCRIPT_H
