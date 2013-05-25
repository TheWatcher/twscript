/** @file
 * This file contains the interface for the base script class. Note that
 * this is heavily based on Tom N Harris' "BaseScript" code, motified to
 * create the framework I want to work from, and incorporating the
 * utility functions that used to be provided in the TWScript class.
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
#include <lg/dynarray.h>
#include <lg/scrmanagers.h>
#include <vector>
#include "Script.h"

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
    TWBaseScript(const char* name, int object) : cScript(name, object), need_fixup(true), sim_running(false), message_time(0)
        { /* fnord */ }


    /** Destroy the TWBaseScript object.
     */
    virtual ~TWBaseScript();


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
        MS_ERROR,        //!< An error occured, don't continue processing
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
 	virtual MsgStatus on_message(sScrMsg* msg, cMultiParm& reply)
		{ return MS_CONTINUE; }


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
     *  (so you probably want to avoid indirect recusion...).
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
    void debug_printf(DebugLevel level, const char *format, ...);


    /** Obtain a string containing the specified object's name (or archetype name),
     *  and its ID number. This builds a 'human readable' version of the object
     *  id and name that can be used when generating debugging messages.
     *
     * @param name   A reference to a string object to store the name in.
     * @param obj_id The ID of the object to obtain the name and number of. If not
     *               provided, this defaults to the current object ID.
     */
    void get_object_namestr(cAnsiStr &name, object obj_id);


    /** Obtain a string containing the current object's name (or archetype name),
     *  and its ID number. This builds a 'human readable' version of the object
     *  id and name that can be used when generating debugging messages.
     *
     * @param name   A reference to a string object to store the name in.
     */
    void get_object_namestr(cAnsiStr &name);


    /* ------------------------------------------------------------------------
     *  QVar convenience functions
     */

    /** Fetch the value in the specified QVar if it exists, return the default if it
     *  does not.
     *
     * @param qvar    The name of the QVar to return the value of.
     * @param def_val The default value to return if the qvar does not exist.
     * @return The QVar value, or the default specified.
     */
    long get_qvar_value(const char *qvar, long def_val);


    /** A somewhat more powerful version of get_qvar_value() that allows the
     *  inclusion of simple calculations to be applied to the value set in the
     *  QVar by including *value or /value in the string. For example, if the
     *  qvar variable contains 'foo/100' this will take the value in foo and
     *  divide it by 100. If the quest variable does not exist, this returns
     *  the default value specified *without applying any calculations to it*.
     *  The value may optionally be another QVar by placing $ before its name,
     *  eg: foo/$bar will divide the value in foo by the value in bar.
     *
     * @param qvar    The name of the QVar to return the value of, possibly including simple maths.
     * @param def_val The default value to return if the qvar does not exist.
     * @return The QVar value, or the default specified.
     */
    float get_qvar_value(const char *qvar, float def_val);


    /* ------------------------------------------------------------------------
     *  Design note support
     */

    /** Parse a string containing either a float value, or a qvar name, and
     *  return the float value contained in the string or qvar. See the docs
     *  for get_param_float for more information.
     *
     * @param param       A string to parse.
     * @param def_val     The default value to use if the string does not contain
     *                    a parseable value, or it references a non-existent QVar
     *                    (note that qvar_str will contain the QVar name, even if
     *                    the QVar does not exist)
     * @param qvar_str    A reference to a cAnsiStr to store the quest var name, or
     *                    quest var and simple calculation string.
     * @return The value specified in the string, or the float version of a value
     *         read from the qvar named in the string.
     */
    float parse_float(const char *param, float def_val, cAnsiStr &qvar_str);


    /** Read a float parameter from a design note string. If the value specified
     *  for the parameter in the design note is a simple number, this behaves
     *  identically to GetParamFloat(). However, this allows the user to specify
     *  the name of a QVar to read the value from by placing $ before the QVar
     *  name, eg: `ExampleParam='$a_quest_var'`. If a qvar is specified in this
     *  way, the user may also include the simple calculations supported by
     *  get_qvar_value(). If a QVar is specified - with or without additional
     *  calculations - the parameter string, with the leading $ removed, is stored
     *  in the provided qvar_str for later use.
     *
     * @param design_note The design note string to parse the parameter from.
     * @param name        The name of the parameter to parse.
     * @param def_val     The default value to use if the parameter does not exist,
     *                    or it references a non-existent QVar (note that qvar_str
     *                    will contain the parameter string even if the QVar does
     *                    not exist)
     * @param qvar_str    A reference to a cAnsiStr to store the quest var name, or
     *                    quest var and simple calculation string.
     * @return The value specified in the parameter, or the float version of a value
     *         read from the qvar named in the parameter.
     */
    float get_param_float(const char *design_note, const char *name, float def_val, cAnsiStr& qvar_str);


    /** Read a float vector (triple of three floats) from a design note string. This
     *  behaves in the same way as get_param_float(), except that instead of a single
     *  float value or QVar string, this expects three comma-separated float or QVar
     *  strings, one for each component of a vector (x, y, and z, in that order).
     *  If components are missing, this will use the specified default values
     *  instead.
     *
     * @param design_note The design note string to parse the parameter from.
     * @param name        The name of the parameter to parse.
     * @param vect        A reference to a vector to store the values in.
     * @param defx        The default value to use if no value has been set for the x component.
     * @param defy        The default value to use if no value has been set for the y component.
     * @param defz        The default value to use if no value has been set for the z component.
     * @return true if the named parameter <b>is present in the design note</b>. false
     *         if it is not. Note that this returns true even if the user has simply
     *         provided the parameter with no actual values, and defaults have been
     *         used for all the vector components. This should not be treated as indicating
     *         whether any values were parsed, rather it should be used to determine
     *         whether the parameter has been found.
     */
    bool get_param_floatvec(const char *design_note, const char *name, cScrVec &vect, float defx = 0.0f, float defy = 0.0f, float defz = 0.0f);


    /** Given a target description string, generate a list of object ids the string
     *  corresponds to. If targ is '[me]', the current object is returned, if targ
     *  is '[source]' the source object is returned, if targ contains an object
     *  id or name, the id of that object is returned. If targ starts with * then
     *  the remainder of the string is used as an archetype name and all direct
     *  concrete descendents of that archetype are returned. If targ starts with
     *  @ then all concrete descendants (direct and indirect) are returned. If
     *  targ contains a < or > then a radius search is performed.
     *
     * @param targ The target description string
     * @param msg  A pointer to a script message containing the to and from objects.
     *             This is required when targ is "[me]", "[source]",
     * @return A vector of object ids the target string matches. The caller must
     *         free this when done with it.
     */
    std::vector<object>* get_target_objects(const char *targ, sScrMsg *msg = NULL);


private:
    /* ------------------------------------------------------------------------
     *  Message handling
     */

    /** Handle message despatch. This enforces some low-level vital message processing
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


    /** Search for concrete objects that are descendants of the specified archetype,
     *  either direct only (if do_full is false), or directly and indirectly. This
     *  can also filter the results based on the distance the concrete objects are
     *  from the specified object.
     *
     * @param matches   A pointer to the vector to store object ids in.
     * @param archetype The name of the archetype to search for. *Must not* include
     *                  and filtering (* or @) directives.
     * @param do_full   If false, only concrete objects that are direct descendants of
     *                  the archetype are matched. If true, all concrete objects that
     *                  are descendants of the archetype, or any descendant of that
     *                  archetype, are matched.
     * @param do_radius If false, concrete objects are matched regardless of distance
     *                  from the `from_obj`. If true, objects must be either inside
     *                  the specified radius from the `from_obj`, or outside out depending
     *                  on the `lessthan` flag.
     * @param from_obj  When filtering objects based on their distance, this is the
     *                  object that distance is measured from.
     * @param radius    The radius of the sphere that matched objects must fall inside
     *                  or outside.
     * @param lessthan  If true, objects must fall within the sphere around from_obj,
     *                  if false they must be outside it.
     */
    void archetype_search(std::vector<object> *matches, const char *archetype, bool do_full = false, bool do_radius = false, object from_obj = 0, float radius = 0.0f, bool lessthan = false);


    /** Determine whether the specified target string is a radius search, and if so
     *  pull out its components. This will take a string like `5.00<Chest` and set
     *  the radius to 5.0, set the lessthan variable to true, and set the archetype
     *  string pointer to the start of the archetype name.
     *
     * @param target    The target string to check
     * @param radius    A pointer to a float to store the radius value in.
     * @param lessthan  A pointer to a bool. If the radius search is a < search
     *                  this is set to true, otherwise it is set to false.
     * @param archetype A pointer to a char pointer to set to the start of the
     *                  archetype name.
     * @return true if the target string is a radius search, false otherwise.
     */
    bool radius_search(const char *target, float *radius, bool *lessthan, const char **archetype);


    /** Establish the length of the name of the qvar in the specified string. This
     *  will determine the length of the qvar name by looking for the end of the
     *  name string, or the presence of a simple calculation, and then working back
     *  until it hits the end of the name
     *
     * @param namestr A string containing a QVar name, and potentially a simple calculation.
     * @return The length of the QVar name, or 0 if the length can not be established.
     */
    int get_qvar_namelen(const char *namestr);


    /** Split a string at a comma. This locates the first comma in the specified string,
     *  replaces the comma with a null character, and returns a pointer to the character
     *  immediately following the replaced comma.
     *
     * @param src A pointer to a string to split at the first comma.
     * @return A pointer to the character immediately after a comma split, or NULL if
     *         the src string does not contain any commas.
     */
    char *comma_split(char *src);

    bool need_fixup;   //!< Does the script need to fix links to the player?
    bool sim_running;  //!< Is the sum currently running?
    uint message_time; //!< The sim time stored in the last recieved message

    static const char * const debug_levels[]; //!< An array of debug level strings
};

#else // SCR_GENSCRIPTS
GEN_FACTORY("TWBaseScript", "CustomScript", cBaseScript)
#endif // SCR_GENSCRIPTS

#endif // TWBASESCRIPT_H
