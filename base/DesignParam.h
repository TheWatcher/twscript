/** @file
 * This file contains the base code for the DesignNote Parameter containers
 * and wrapper classes.
 *
 * @author Chris Page &lt;chris@starforge.co.uk&gt;
 *
 */
/* Okay, let's try to get this right this time. The full description of
 * what at design note can include is given in docs/design_note.abnf
 * Some observations from that:
 *
 * - string is the only parameter type that absolutely can never depend on the
 *   qvar rule directly or indirectly.
 *
 * - object is like int, except it also supports names for objects
 *
 * - target only allows for qvar-calc in distance arguments
 *
 * - boolean and time have specific fiddliness involved:
 *   - bool can either be t/f/y/n followed by any arbitrary text, or
 *     an implicitly integer qvar-calc
 *   - time can either be a positive float or int followed by s or m,
 *     or an implicitly integer qvar-calc
 *
 * - the remaining design note parameter types are integer, float, and
 *   floatvec. The integer and float ones are implicit integer or float
 *   qvar-calcs with no special treatment, while the floatvec type is
 *   three comma-separated implicitly float qvar-calc
 *
 * In an ideal world, the scripts should not need to care about any of
 * these issues: they should be able to just fetch the value of a
 * parameter and just get a value of the type appropriate for that
 * parameter. Inevitably, doing this is going to involve creating
 * classes that abstract away the ghastly implementation details for
 * each type of parameter. Hence we probably need a hierarchy of
 * parameter types, with one class per parameter type and a base
 * class that implements any common code.
 *
 * object, boolean, and time are specific sub-types of integer: they
 * all ultimately result in an integer result after some initial
 * type-specific handling (boolean may need to do a one-time translate
 * of t/f/y/n to 1 or 0, time may need to do multiplication of s or m
 * suffixed floats - but as we can't have fractional milliseconds, the
 * *value* of time is always integer milliseconds, and object may need
 * to do a one-time translate from object name to integer ID.
 *
 * <stike>target is a sub-type of string that must be able to register a qvar
 * change listener if appropriate. It should return the parameter
 * string as-is if a string is requested, and a float value for the
 * radius (or 0.0) if a float is requested?</strike>
 *
 * NOTE: is object really just a sub-type of target? If the target
 *       syntax is modified to include the object syntax?
 *
 * Adding `parameter=object`, `parameter=id`, and `parameter=qvar-calc`
 * to the target syntax allows target to work for object selection.
 * Important things:
 *
 * - function to retrieve object ID(s) must have a limit arg
 * - As individual object is a special case, perhaps own function
 *   that wraps object Id retrieval and returns first?
 *
 * What's the overhead here? Can we optimise the simple case where
 * an ID, object, or [me] is specified?
 *
 * float-vec is really just three floats wrapped up in something that
 * makes them easily accessible (say overload [] and treat them as
 * arrays of three values?)
 *
 * string, integer and float are, obviously, 'base' types here. What's
 * arguable is whether int should be considered as a sub-tye of float
 * (as any qvar-calc may internally produce a float result).
 *
 */
/*
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

#ifndef DESIGNPARAM_H
#define DESIGNPARAM_H

#include <string>
#include <cmath>
#include <random>
#include "QVarCalculation.h"

/** A base class for design note parameters. This collects the common
 *  code for all design note parameter types, and maintains the
 *  information that all design note parameter types need to keep
 *  track of.
 */
class DesignParam
{
public:
    /** Create a new DesignParam object. Generally this will never be called
     *  directly, but as part of constructing a child of this class.
     *
     * @param hostid The ID of the host object.
     * @param script The name of the script the parameter is attached to.
     * @param name   The name of the parameter.
     */
    DesignParam(const int hostid, const std::string& script, const std::string& name) :
        host(hostid), script_name(script), param_name(name), fullname(script + name), set(false)
        { /* fnord */ }


    /** Ensure that destruction works correctly through inheritance.
     */
    virtual ~DesignParam()
        { /* fnord */ }


    /** Was the value of this parameter set in the design note?
     *
     * @return true if the value of this parameter was set in the design note, false
     *         if it was not. If called before calling init(), this will always
     *         return false.
     */
    bool is_set(void) const
        { return set; }


protected:
    /** Set whether this design parameter has been set in the design note.
     *
     * @param status true if the parameter has been set in the design note, false otherwise.
     */
    void is_set(const bool status)
        { set = status; }


    /** Obtain the ID of the host object this design parameter is attached to.
     *
     * @return The host object ID
     */
    int hostid() const
        { return host; }


    /** Fetch the value of the parameter as a string, and store the string in the
     *  provided string reference. Note that this does not modify the 'set' value,
     *  as subclasses may do additional validation on the string that determines
     *  whether a valid value has be set.
     *
     * @param parameter A reference to a string to store the parameter in.
     * @return true if the parameter was set in the design note, false if it was not
     *         or could not be parsed from it.
     */
    bool get_param_string(const std::string& design_note, std::string& parameter);

private:
    int host;                //!< ID of the object this variable is attached to
    std::string script_name; //!< The name of the script the parameter is for.
    std::string param_name;  //!< The name of parameter this represents.
    std::string fullname;    //!< The full name of the parameter (script_name + param_name)
    bool        set;         //!< Was the value of this parameter set in the design note?
};


/** A design note parameter class to handle string parameters. This implements
 *  the basic operations needed to initialise and fetch the value of a basic
 *  string parameter that will not update after initialisation is completed.
 *
 * @note If the design note is changed after init is complete - say some other
 *       script modifies the string to store data, or change the behaviour
 *       of another script, *this will not pick up the change*. Whether this
 *       is "correct" behaviour is arguable, but it's the one I'm using. If
 *       picking up the change is really needed, the client /could/ call
 *       init() before each get() to get the desired behaviour.
 */
class DesignParamString : public DesignParam
{
public:
    /** Create a new DesignParamString. Note that little or no work can or
     *  should be done by this constructor: the client code should call
     *  init(), passing the design note data to initialise the parameter
     *  with after object creation is complete.
     *
     * @param hostid The ID of the host object.
     * @param script The name of the script the parameter is attached to.
     * @param name   The name of the parameter.
     */
    DesignParamString(const int hostid, const std::string& script, const std::string& name) :
        DesignParam(hostid, script, name), data("")
        { /* fnord */ }


    /** Initialise the DesignParamString based on the values specified.
     *
     * @param design_note   A reference to a string containing the design note to parse
     * @param default_value The default value to set for the string. If not specified,
     *                      the empty string is used.
     * @return true on successful init (which may include when no parameter was set
     *         in the design note!), false if init failed.
     */
    bool init(const std::string& design_note, const std::string& default_value = "");


    /** Obtain the value of this design parameter. This will return the current
     *  design parameter value, which may be an empty string if the parameter was not
     *  set in the design note.
     *
     * @return A reference to a string containing the design note parameter value.
     */
    const std::string& value() const
        { return data; }

    const char* c_str() const
        { return data.c_str(); }

private:
    std::string data; //!< The current value of the parameter.
};


/**
 */
class DesignParamFloat : public DesignParam
{
public:
    /** Create a new DesignParamFloat. Note that little or no work can or
     *  should be done by this constructor: the client code should call
     *  init(), passing the design note data to initialise the parameter
     *  with after object creation is complete.
     *
     * @param hostid The ID of the host object.
     * @param script The name of the script the parameter is attached to.
     * @param name   The name of the parameter.
     */
    DesignParamFloat(const int hostid, const std::string& script, const std::string& name) :
        DesignParam(hostid, script, name), data(hostid)
        { /* fnord */ }


    /** Initialise the DesignParamFloat based on the values specified.
     *
     * @param design_note   A reference to a string containing the design note to parse
     * @param default_value The default value to set for the float. If not specified,
     *                      0.0f is used.
     * @param add_listeners If true, request quest variable change messages for any quest
     *                      variables in the design param.
     * @return true on successful init (which may include when no parameter was set
     *         in the design note!), false if init failed.
     */
    bool init(const std::string& design_note, float default_value = 0.0f, const bool add_listeners = false);


    void unsubscribe()
        { data.unsubscribe(); }


    /** Obtain the value of this design parameter. This will return the current
     *  design parameter value, which may be 0.0f if the parameter was not
     *  set in the design note.
     *
     * @return A float containing the design note parameter value.
     */
    float value()
        { return data.value(); }

    operator float() { return value(); }

protected:
    QVarCalculation data; //!< The current value of the parameter.
};



class DesignParamInt : public DesignParamFloat
{
public:
    /** Create a new DesignParamInt. This hands off the actual work to
     *  the DesignParamFloat constructor
     *
     * @param hostid The ID of the host object.
     * @param script The name of the script the parameter is attached to.
     * @param name   The name of the parameter.
     */
    DesignParamInt(const int hostid, const std::string& script, const std::string& name) :
        DesignParamFloat(hostid, script, name)
        { /* fnord */ }


    /** Initialise the DesignParamInt based on the values specified.
     *
     * @param design_note   A reference to a string containing the design note to parse
     * @param default_value The default value to set for the int. If not specified,
     *                      0 is used.
     * @param add_listeners If true, request quest variable change messages for any quest
     *                      variables in the design param.
     * @return true on successful init (which may include when no parameter was set
     *         in the design note!), false if init failed.
     */
    bool init(const std::string& design_note, int default_value = 0, const bool add_listeners = false)
        { return DesignParamFloat::init(design_note, static_cast<float>(default_value), add_listeners); }


    /** Obtain the value of this design parameter. This will return the current
     *  design parameter value, which may be 0 if the parameter was not
     *  set in the design note.
     *
     * @return An int containing the design note parameter value.
     */
    int value()
        { return static_cast<int>(round(data.value())); }


    operator int() { return value(); }
};


class DesignParamTime : public DesignParamInt
{
public:
    /** Create a new DesignParamTime. This hands off the actual work to
     *  the DesignParamInt constructor
     *
     * @param hostid The ID of the host object.
     * @param script The name of the script the parameter is attached to.
     * @param name   The name of the parameter.
     */
    DesignParamTime(const int hostid, const std::string& script, const std::string& name) :
        DesignParamInt(hostid, script, name)
        { /* fnord */ }


    /** Initialise the DesignParamTime based on the values specified.
     *
     * @param design_note   A reference to a string containing the design note to parse
     * @param default_value The default value to set for the time. If not specified,
     *                      0 is used.
     * @param add_listeners If true, request quest variable change messages for any quest
     *                      variables in the design param.
     * @return true on successful init (which may include when no parameter was set
     *         in the design note!), false if init failed.
     */
    bool init(const std::string& design_note, int default_value = 0, const bool add_listeners = false);


    operator unsigned long() { return static_cast<unsigned long>(value()); }

};


class DesignParamBool : public DesignParamInt
{
public:
    /** Create a new DesignParamBool. This hands off the actual work to
     *  the DesignParamInt constructor
     *
     * @param hostid The ID of the host object.
     * @param script The name of the script the parameter is attached to.
     * @param name   The name of the parameter.
     */
    DesignParamBool(const int hostid, const std::string& script, const std::string& name) :
        DesignParamInt(hostid, script, name)
        { /* fnord */ }


    /** Initialise the DesignParamBool based on the values specified.
     *
     * @param design_note   A reference to a string containing the design note to parse
     * @param default_value The default value to set for the time. If not specified,
     *                      0 is used.
     * @param add_listeners If true, request quest variable change messages for any quest
     *                      variables in the design param.
     * @return true on successful init (which may include when no parameter was set
     *         in the design note!), false if init failed.
     */
    bool init(const std::string& design_note, bool default_value = false, const bool add_listeners = false);


    /** Obtain the value of this design parameter. This will return the current
     *  design parameter value, which may be false if the parameter was not
     *  set in the design note.
     *
     * @return An bool containing the design note parameter value.
     */
    bool value()
        { return (static_cast<int>(data.value()) != 0); }


    operator bool() { return value(); }
};


class DesignParamCountMode: public DesignParamString
{
public:
    enum CountMode {
        CM_NOTHING = 0, //!< Count nothing
        CM_TURNON,      //!< Count only TurnOns
        CM_TURNOFF,     //!< Count only TurnOffs
        CM_BOTH         //!< Count both TurnOn and TurnOff
    };

    /** Create a new DesignParamCountMode. This hands off the actual work to
     *  the DesignParamInt constructor
     *
     * @param hostid The ID of the host object.
     * @param script The name of the script the parameter is attached to.
     * @param name   The name of the parameter.
     */
    DesignParamCountMode(const int hostid, const std::string& script, const std::string& name) :
        DesignParamString(hostid, script, name), mode(CM_BOTH)
        { /* fnord */ }


    /** Initialise the DesignParamCountMode based on the values specified.
     *
     * @param design_note   A reference to a string containing the design note to parse
     * @param default_value The default value to set for the mode. If not specified,
     *                      CM_BOTH is used.
     * @return true on successful init (which may include when no parameter was set
     *         in the design note!), false if init failed.
     */
    bool init(const std::string& design_note, CountMode default_value = CM_BOTH);


    /** Obtain the value of this design parameter.
     *
     * @return An CountMode containing the design note parameter value.
     */
    CountMode value() const
        { return mode; }


    operator int() const
        { return static_cast<int>(mode); }

private:
    CountMode mode;
};


/** POD class used by the link search code to keep track of link information.
 */
struct LinkScanWorker {
    int link_id;         //!< The ID of a link from the source object to another
    int dest_id;         //!< The ID of the object being linked to
    unsigned int weight;         //!< The link weight (only used when doing weighted link selection)
    unsigned int cumulative;     //!< The cumulative weight of this link, and earlier links in the list.

    /** Less-than operator to allow sorting by link IDs.
     */
    bool operator<(const LinkScanWorker& rhs) const
    {
        return link_id < rhs.link_id;
    }
};


/** POD class used by the targetting functions to keep track of information
 */
struct TargetObj {
    int  obj_id;   //!< The ID of the target object
    int  link_id;  //!< The ID of a the link to the object (may be zero, indicating no link)

    /** Less-than operator to allow sorting by object ID.
     */
    bool operator<(const TargetObj& rhs) const
    {
        return obj_id < rhs.obj_id;
    }

    /** Assignment operator to simplify the process of copying data from a LinkScanWorker
     */
    TargetObj& operator=(const LinkScanWorker& rhs)
    {
        obj_id = rhs.dest_id;
        link_id = rhs.link_id;

        return *this;
    }
};


class DesignParamTarget : public DesignParam
{
public:
    /** Possible targetting modes
     */
    enum TargetMode {
        TARGET_INVALID,        //!< No valid target mode set
        TARGET_INT,            //!< The target parameter describes a simple constant int object ID
        TARGET_QVAR,           //!< The target is a QVar or QVarCalculation
        TARGET_SOURCE,         //!< The target is the source of a message
        TARGET_LINK,           //!< The target is described by a link
        TARGET_ATYPE_DIRECT,   //!< Target is all direct concrete descendants of an archetype
        TARGET_ATYPE_INDIRECT, //!< Target is all direct and indirect concrete descendants of an archetype
        TARGET_RADIUS_LT,      //!< Target is all concrete descendants of an archetype within a given range
        TARGET_RADIUS_GT       //!< Target is all concrete descendants of an archetype outside a given range
    };

    /**
     *
     * @param hostid The ID of the host object.
     * @param script The name of the script the parameter is attached to.
     * @param name   The name of the parameter.
     */
    DesignParamTarget(const int hostid, const std::string& script, const std::string& name) :
        DesignParam(hostid, script, name),
        mode(TARGET_INVALID),
        objid_cache(0),
        qvar_calc(hostid),
        targetstr(""),
        randomiser(0)
        { /* fnord */ }


    /** Initialise the DesignParamTarget based on the values specified.
     *
     * @param design_note   A reference to a string containing the design note to parse
     * @param add_listeners If true, request quest variable change messages for any quest
     *                      variables in the design param.
     * @return true on successful init (which may include when no parameter was set
     *         in the design note!), false if init failed.
     */
    bool init(const std::string& design_note, const std::string& default_value, const bool add_listeners = false);


    /** Obtain a single object ID from the target string. Note that this
     *  only returns a value for 'simple' target types - [me], [source], a
     *  literal int ID or qvar calculation. Link/archetype/radius targets are
     *  not supported are result in 0.
     *
     * @return An int containing the object ID
     */
    int value(sScrMsg* msg = NULL);


    std::vector<TargetObj>* values(sScrMsg* msg);


    const char *c_str()
        { return targetstr.c_str(); }

protected:

    /** Determine whether the specified parameter contains a target
     *  string that can not have its results cached. This inspects
     *  the provided parameter and returns true if it contains a
     *  target string  that may match different objects each time
     *  value() is called.
     *
     * @param param A reference to a string containing the parameter to check.
     * @return true if the parameter string contains an uncacheable
     *         target string, false otherwise.
     */
    bool is_complex_target(const std::string& param);


    /** Given a target description string, generate a list of object ids the string
     *  corresponds to. If targ is '[me]', the current object is returned, if targ
     *  is '[source]' the source object is returned, if targ contains an object
     *  id or name, the id of that object is returned. If targ starts with * then
     *  the remainder of the string is used as an archetype name and all direct
     *  concrete descendants of that archetype are returned. If targ starts with
     *  @ then all concrete descendants (direct and indirect) are returned. If
     *  targ contains a < or > then a radius search is performed. If the target
     *  starts with '&' it is considered to be a link search, in which case the
     *  remainder of the target string should be a linksearch definition.
     *
     * @param msg  A pointer to a script message containing the to and from objects.
     *             This is required when targ is "[me]", "[source]",
     * @return A vector of object ids the target string matches. The caller must
     *         free this when done with it.
     */


    /** Generate a list of objects linked to the host object based on the specified
     *  link definition. This will use the specified link definition to determine
     *  which links to search for, and adds the link destination objects to the
     *  specified list.
     *
     *  By default, the linkdef is the name of the link flavour to search for.
     *
     *  If the linkdef is preceded with '?' then one or more of the possible links
     *  is chosen at random, and the destination TargetObj aded to the list.
     *
     *  If the linkdef contains !, all links that match the linkdef are returned
     *  (and any count in [] will be ignored). Note that this is ignored if
     *  Weighted mode is enabled. If this is specified, and random mode has been
     *  enabled, the links are returned in a random order.
     *
     *  If the linkdef is preceded with '%', only links to archetype objects will
     *  be included in the results. Conversely, if the linkdef is preceded by '#'
     *  only links to concrete objects will be included. If neither sigil is set,
     *  the default is to include links to both archetypes and concrete objects.
     *
     *  If the linkdef specifies the flavour "Weighted", the search inspects all
     *  the ScriptParams links from the host object, and uses the integer values
     *  set in the link data to determine which random link to choose based on
     *  the weightings. If a ScriptParams link does not contain an integer weight,
     *  it defaults to 1.
     *
     *  Finally, the flavour may be prefixed with [N], where 'N' is the maximum
     *  number of linked objects to fetch. If the fetch count is not specified,
     *  and random mode is not enabled, all matching links are returned. If
     *  random or 'Weighted' mode is enabled, and no fetch count is given, only
     *  a single linked object is selected. If random or 'Weighted' mode is
     *  enabled, and a fetch count has been given, then the list of returned
     *  TargetObjs will contain the requested number of links, randomly selected
     *  from the possible links, and *repeats may be present in the list*.
     *
     *  For example, this will fetch three random ControlDevice linked objects:
     *
     *      ?[3]ControlDevice
     *
     *  The order of sigils doesn't matter, so the same could be expressed using
     *
     *      [3]?ControlDevice
     *
     *  This will fetch all ControlDevice linked concrete objects in a random order:
     *
     *      #?!ControlDevice
     *
     * @param matches A pointer to the vector to store object IDs in.
     * @param from    The ID of the object to search for links from.
     * @param linkdef A pointer to a string describing the links to fetch.
     */
    void link_search(std::vector<TargetObj>* matches, const int from, const char* linkdef);


    /* ------------------------------------------------------------------------
     *  Link targetting
     */

    /** Enum used to control link selection in searches.
     */
    enum LinkMode {
        LM_ARCHETYPE = 1, //!< Only include links to archetypes in results
        LM_CONCRETE,      //!< Only include links to concrete objects
        LM_BOTH           //!< Include links to both
    };

    /** Process any sigils included in the specified linkdef. This will scan the
     *  specified linkdef for recognised sigils, and set the options for the link
     *  search appropriately.
     *
     * @note Settings are only updated if an appropriate sigil appears in the
     *       linkdef. The caller must ensure that the settings values are set to
     *       Sane Default Values before calling this function.
     *
     * @param linkdef     A pointer to the string describing the links to fetch.
     * @param is_random   A pointer to a bool that will be set to true if the linkdef
     *                    contains the '?' sigil, or the flavour "Weighted".
     * @param is_weighted A pointer to a bool that will be set to true if the linkdef
     *                    contains the flavour "Weighted"
     * @param fetch_count A pointer to an int that will be set to the number of
     *                    objects to return from link_search.
     * @param fetch_all   A pointer to a book that will be set to true if the linkdef
     *                    contains the '!' sigil.
     * @param mode        A pointer to a LinkMode to store the link selection mode in.
     *                    If a mode is not set in the string, this is set to LM_BOTH.
     * @return A pointer to the start of the link flavour specified in linkdef. Note
     *         that if the linkdef specifies the flavour "Weighted", this will be a
     *         link to a string containing "ScriptParams" which *should not* be freed.
     */
    const char* link_search_setup(const char* linkdef, bool* is_random, bool* is_weighted, uint* fetch_count, bool *fetch_all, LinkMode *mode);


    /** Parse the number of linked objects to return from the specified link definition.
     *  This assumes that the linkdef provided starts pointing to the '[' in the link
     *  definition. If this is not the case, it returns the pointer as-is.
     *
     * @param linkdef     A pointer to the count marker in the linkdef.
     * @param fetch_count A pointer to the int to update with the link count.
     * @return A pointer to the ] after the link count, or the first usable character
     *         after the parsed number if the ] is missing.
     */
    const char* parse_link_count(const char* linkdef, uint* fetch_count);


    /** Generate a list of current links of the specified flavour from this object, recording
     *  the link ID and destination, and possibly weighting information if needed and
     *  weighting is enabled.
     *
     * @param flavour  The link flavour to include in the list.
     * @param from     The ID of the object to fetch links from.
     * @param weighted If true, weighting is enabled. `flavour` must be `ScriptParams` or `~ScriptParams`.
     * @param mode     The link selection mode.
     * @param links    A reference to a vector in which the list of links should be stored.
     * @return The accumulated weights if weighting is enabled, the number of links if it is
     *         not enabled, 0 indicates no matching links found.
     */
    uint link_scan(const char* flavour, const int from, const bool weighted, LinkMode mode, std::vector<LinkScanWorker>& links);


    /** Select a link from the specified vector of links such that it has the target
     *  cumulative weight, or is the closest greater weight.
     *
     * @param links  A reference to a list of LinkScanWorker structures containing weighted
     *               link information. This must be ordered by ascending cumulative weight.
     * @param target The target weight to fetch in the list.
     * @param store  A refrence to a TargetObj structure to store the link and object id in.
     * @return true if an item with the appropriate weight is located, false otherwise.
     */
    bool pick_weighted_link(std::vector<LinkScanWorker>& links, const uint target, TargetObj& store);


    /** Compute the cumulative weightings for the links in the supplied vector.
     *
     * @param links A reference to a vector of links.
     * @return The sum of all the weights specified in the links
     */
    uint build_link_weightsums(std::vector<LinkScanWorker>& links);


    /** Choose an appropriate number of links at random from the specified links list.
     *  This will randomise the list, and then choose the requested number of links
     *  from it. Note that if fetch_count > 1, this can produce duplicate entries in
     *  the matches list. The links are chosen *at random*, with no exclusion of
     *  already selected links!
     *
     * @param matches       A pointer to the vector to store object IDs in.
     * @param links         A reference to a vector of links.
     * @param fetch_count   The number of links to fetch.
     * @param fetch_all     Fetch all the links in a random order?
     * @param total_weights The total of all the weights of the links in the links vector.
     * @param is_weighted   If true, do a weighted random selection, otherwise all links
     *                      can be selected equally.
     */
    void select_random_links(std::vector<TargetObj>* matches, std::vector<LinkScanWorker>& links, const uint fetch_count, const bool fetch_all, const uint total_weights, const bool is_weighted);


    /** Copy the requested number of links from the link worker vector into the TargetObj
     *  list. Note that, as the links vector is sorted by link id, the chosen links will
     *  always be the same, given the same links list.
     *
     * @param matches       A pointer to the vector to store object IDs in.
     * @param links         A reference to a vector of links.
     * @param fetch_count   The number of links to fetch.
     */
    void select_links(std::vector<TargetObj>* matches, std::vector<LinkScanWorker>& links, const uint fetch_count);


    /* ------------------------------------------------------------------------
     *  Search methods
     */

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
    bool radius_search(const char* target, float* radius, bool* lessthan, const char** archetype);


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
    void archetype_search(std::vector<TargetObj>* matches, const char* archetype, bool do_full = false, bool do_radius = false, object from_obj = 0, float radius = 0.0f, bool lessthan = false);


private:
    TargetMode      mode;         //!< Which mode is this target parameter working in?

    // FIXME: Can we just union this?
    int             objid_cache;  //!< Used by TARGET_INT to store the target object id
    QVarCalculation qvar_calc;    //!< In TARGET_QVAR, this stores the qvar/qvar calc
    std::string     targetstr;    //!< In TARGET_COMPLEX, this is the target string

    std::minstd_rand0 randomiser; //!< a random number generator for... random numbers.
};


/** A class that encapsulates multiple DesignParams to form a compound
 *  type representing a capacitor description. Note that this does not
 *  implement the capacitor behaviour - that is done by SavedCounter,
 *  rather this collects together the [ScriptName][Param]Falloff/Limit
 *  handling into one class.
 */
class DesignParamCapacitor
{
public:
    /** Create a new DesignParamCapacitor. This creates the capacitor and
     *  the additional design parameter variables needed for capacitor
     *  operation.
     *
     * @param hostid The ID of the host object.
     * @param script The name of the script the parameter is attached to.
     * @param name   The name of the parameter.
     */
    DesignParamCapacitor(const int hostid, const std::string& script, const std::string& name) :
        count(hostid, script, name),
        falloff(hostid, script, name + "Falloff"),
        limit(hostid, script, name + "Limit")
        { /* fnord */ }


    /** Initialise the DesignParamCapacitor based on the values specified. Note
     *  that this explicitly *DOES NOT* allow listeners to be attached to any
     *  possible qvar calculations in the design note parameter for the capacitor,
     *  as we'd need to allow for readjustment after init and that's a cavern of
     *  woe and spiders.
     *
     * @param design_note   A reference to a string containing the design note to parse

     * @return true on successful init (which may include when no parameter was set
     *         in the design note!), false if init failed.
     */
    bool init(const std::string& design_note, int default_count = 0, int default_falloff = 0, bool default_limit = false);


    /** Retrieve the value set for the capacitor count
     */
    int get_count() { return count.value(); }


    /** Get the value set for the capacitor falloff, in milliseconds
     */
    int get_falloff() { return falloff.value(); }


    /** Get the value set for the capacitor limiter
     */
    bool get_limit() { return limit.value(); }


    /** Determine wheter the design param is set. Uses the count, as
     *  that's the major value here - without count, nothing works,
     *  and with it falloff and limit are defaulted.
     */
    bool is_set() const
        { return count.is_set(); }

private:
    DesignParamInt  count;   //!< The count for the capacitor, meaning varies on Trap/Trigger context!
    DesignParamTime falloff; //!< The rate at which the count falls off.
    DesignParamBool limit;   //!< Should the count be limited?
};



class DesignParamFloatVec : public DesignParam
{
public:
    DesignParamFloatVec(const int hostid, const std::string& script, const std::string& name) :
        DesignParam(hostid, script, name),
        x_calc(hostid), y_calc(hostid), z_calc(hostid),
        vect()
        { /* fnord */ }


    /** Initialise the DesignParamFloatVec based on the values specified.
     *
     * @param design_note   A reference to a string containing the design note to parse

     * @return true on successful init (which may include when no parameter was set
     *         in the design note!), false if init failed.
     */
    bool init(const std::string& design_note, const float def_x = 0.0f, const float def_y = 0.0f, const float def_z = 0.0f, const bool add_listeners = false );


    const cScrVec& value();

private:
    QVarCalculation x_calc;
    QVarCalculation y_calc;
    QVarCalculation z_calc;

    cScrVec vect;
};


#endif // DESIGNPARAM_H
