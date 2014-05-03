
#include <lg/interface.h>
#include <lg/scrmanagers.h>
#include <lg/scrservices.h>
#include <lg/objects.h>
#include <lg/links.h>
#include <lg/properties.h>
#include <lg/propdefs.h>
#include <exception>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <algorithm>    // std::sort and std::shuffle
#include <random>       // std::default_random_engine
#include <chrono>       // std::chrono::system_clock

#include "Version.h"
#include "TWBaseScript.h"
#include "ScriptModule.h"
#include "ScriptLib.h"

const char* const TWBaseScript::debug_levels[] = {"DEBUG", "WARNING", "ERROR"};
const uint TWBaseScript::NAME_BUFFER_SIZE = 256;

/* ------------------------------------------------------------------------
 *  Public interface exposed to the rest of the game
 */

STDMETHODIMP TWBaseScript::ReceiveMessage(sScrMsg* msg, sMultiParm* reply, eScrTraceAction trace)
{
    long result = 0;

    cScript::ReceiveMessage(msg, reply, trace);

    if(trace == kSpew)
        debug_printf(DL_DEBUG, "Got message '%s' at '%d'", msg -> message, msg -> time);

    message_time = msg -> time;
    if(!::_stricmp(msg -> message, "Sim"))
    {
        sim_running = static_cast<sSimMsg*>(msg) -> fStarting;
    }

    try {
        // Ensure that reply is always available, even if ReceiveMessage was called with it NULL
        sMultiParm fallback;
        if(reply == NULL)
            reply = &fallback;

        result = dispatch_message(msg, reply);
    }
    // Prevent exceptions from getting out into the rest of the game
    catch (std::exception& err) {
        debug_printf(DL_ERROR, "An error occurred, %s", err.what());
        result = S_FALSE;
    }
    catch (...) {
        debug_printf(DL_ERROR, "An unknown error occurred.");
        result = S_FALSE;
    }

    return result;
}


/* ------------------------------------------------------------------------
 *  Message handling
 */

TWBaseScript::MsgStatus TWBaseScript::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Handle setting up the script from the design note
    if(!done_init) {
        init(msg -> time);
        done_init = true;
    }

    return MS_CONTINUE;
}


/* ------------------------------------------------------------------------
 *  Message convenience functions
 */

cMultiParm* TWBaseScript::send_message(object dest, const char* message, cMultiParm& reply, const cMultiParm& data, const cMultiParm& data2, const cMultiParm& data3)
{
    return g_pScriptManager -> SendMessage2(reply, ObjId(), dest, message, data, data2, data3);
}


void TWBaseScript::post_message(object dest, const char* message, const cMultiParm& data, const cMultiParm& data2, const cMultiParm& data3)
{
    g_pScriptManager -> PostMessage2(ObjId(), dest, message, data, data2, data3, kScrMsgPostToOwner);
}


tScrTimer TWBaseScript::set_timed_message(const char* message, ulong time, eScrTimedMsgKind type, const cMultiParm& data)
{
    return g_pScriptManager -> SetTimedMessage2(ObjId(), message, time, type, data);
}


void TWBaseScript::cancel_timed_message(tScrTimer timer)
{
    g_pScriptManager -> KillTimedMessage(timer);
}


/* ------------------------------------------------------------------------
 *  Script data handling
 */

void TWBaseScript::set_script_data(const char* name, const cMultiParm& data)
{
    sScrDatumTag tag = { ObjId(), Name(), name };

    g_pScriptManager -> SetScriptData(&tag, &data);
}


bool TWBaseScript::is_script_data_set(const char* name)
{
    sScrDatumTag tag = { ObjId(), Name(), name };

    return g_pScriptManager -> IsScriptDataSet(&tag);
}


bool TWBaseScript::get_script_data(const char* name, cMultiParm& data)
{
    sScrDatumTag tag = { ObjId(), Name(), name };

    long result = g_pScriptManager -> GetScriptData(&tag, &data);

    return (result == 0);
}


bool TWBaseScript::clear_script_data(const char* name, cMultiParm& data)
{
    sScrDatumTag tag = { ObjId(), Name(), name };

    long result = g_pScriptManager -> ClearScriptData(&tag, &data);
    return (result == 0);
}


/* ------------------------------------------------------------------------
 *  Debugging support
 */

void TWBaseScript::debug_printf(TWBaseScript::DebugLevel level, const char* format, ...)
{
    va_list args;
    std::string name;
    char buffer[900]; // Temporary buffer, 900 is the limit imposed by MPrint. This is really nasty and needs fixing.

    // Need the name of the current object
    get_object_namestr(name);

    // build the string the caller requested
    va_start(args, format);
    _vsnprintf(buffer, 899, format, args);
    va_end(args);

    // And spit out the result to monolog
	g_pfnMPrintf("%s[%s(%s)]: %s\n", debug_levels[level], Name(), name.c_str(), buffer);
}


void TWBaseScript::get_object_namestr(std::string& name, object obj_id)
{
    char namebuffer[NAME_BUFFER_SIZE];

    // NOTE: obj_name isn't freed when GetName sets it to non-NULL. As near
    // as I can tell, it doesn't need to, it only needs to be freed when
    // using the version of GetName in ObjectSrv... Probably. Maybe. >.<
    // The docs for this are pretty shit, so this is mostly guesswork.

    SInterface<IObjectSystem> ObjSys(g_pScriptManager);
    const char* obj_name = ObjSys -> GetName(obj_id);

    // If the object system has returned a name here, the concrete object
    // has been given a name, so use it
    if(obj_name) {
        snprintf(namebuffer, NAME_BUFFER_SIZE, "%s (%d)", obj_name, int(obj_id));

    // Otherwise, the concrete object has no name, get its archetype name
    // if possible and use that instead.
    } else {
        SInterface<ITraitManager> TraitMan(g_pScriptManager);
        object archetype_id = TraitMan -> GetArchetype(obj_id);
        const char* archetype_name = ObjSys -> GetName(archetype_id);

        // Archetype name found, use it in the string
        if(archetype_name) {
            snprintf(namebuffer, NAME_BUFFER_SIZE, "A %s (%d)", archetype_name, int(obj_id));

        // Can't find a name or archetype name (!), so just use the ID
        } else {
            snprintf(namebuffer, NAME_BUFFER_SIZE, "%d", int(obj_id));
        }
    }

    name = namebuffer;
}


void TWBaseScript::get_object_namestr(std::string& name)
{
    get_object_namestr(name, ObjId());
}


/* ------------------------------------------------------------------------
 *  QVar convenience functions
 */

int TWBaseScript::get_qvar_value(const char* qvar, int def_val)
{
    int value = def_val;
    char  op;
    char* lhs_qvar, *rhs_data, *endstr = NULL;
    char* buffer = parse_qvar(qvar, &lhs_qvar, &op, &rhs_data);

    if(buffer) {
        value = get_qvar(lhs_qvar, def_val);

        // If an operation and right hand side value/qvar were found, use them
        if(op && rhs_data) {
            int adjval = 0;

            // Is the RHS a qvar itself? If so, fetch it, otherwise treat it as an int
            if(*rhs_data == '$') {
                adjval = get_qvar(&rhs_data[1], 0);
            } else {
                adjval = strtol(rhs_data, &endstr, 10);
            }

            // If a value was parsed in some way, apply it (this also avoids
            // division-by-zero problems for / )
            if(endstr != rhs_data && adjval) {
                switch(op) {
                    case('+'): value += adjval; break;
                    case('-'): value -= adjval; break;
                    case('*'): value *= adjval; break;
                    case('/'): value /= adjval; break;
                }
            }
        }

        delete[] buffer;
    }

    return value;
}


// I could probably avoid this hideous duplication of the above through
// templating, but it's possible that float and int handling may support
// different features in future, so I'm leaving the duplication for now...
float TWBaseScript::get_qvar_value(const char* qvar, float def_val)
{
    float value = def_val;
    char  op;
    char* lhs_qvar, *rhs_data, *endstr = NULL;
    char* buffer = parse_qvar(qvar, &lhs_qvar, &op, &rhs_data);

    if(buffer) {
        value = get_qvar(lhs_qvar, def_val);

        // If an operation and right hand side value/qvar were found, use them
        if(op && rhs_data) {
            float adjval = 0;

            // Is the RHS a qvar itself? If so, fetch it, otherwise treat it as an int
            if(*rhs_data == '$') {
                adjval = get_qvar(&rhs_data[1], 0.0f);
            } else {
                adjval = strtof(rhs_data, &endstr);
            }

            // If a value was parsed in some way, apply it (this also avoids
            // division-by-zero problems for / )
            if(endstr != rhs_data && adjval) {
                switch(op) {
                    case('+'): value += adjval; break;
                    case('-'): value -= adjval; break;
                    case('*'): value *= adjval; break;
                    case('/'): value /= adjval; break;
                }
            }
        }

        delete[] buffer;
    }

    return value;
}


/* ------------------------------------------------------------------------
 *  Design note support
 */

float TWBaseScript::parse_float(const char* param, float def_val, std::string& qvar_str)
{
    float result = def_val;

    // Gracefully handle the situation where the parameter is NULL or empty
    if(param || *param == '\0') {
        // Skip any leading whitespace
        while(isspace(*param)) {
            ++param;
        }

        // Starting with $ indicates that the string contains a qvar, fetch it
        if(*param == '$') {
            // Store the qvar string for later
            qvar_str = &param[1];
            result = get_qvar_value(&param[1], def_val);

        // Otherwise assume it's a float string
        } else {
            char* endstr;
            result = strtof(param, &endstr);

            // Restore the default if parsing failed
            if(endstr == param) result = def_val;
        }
    }

    return result;
}


void TWBaseScript::get_scriptparam_valuefalloff(char* design_note, const char* param, int* value, int* falloff)
{
    std::string workstr = param;

    // Get the value
    if(value) {
        *value = get_scriptparam_int(design_note, param);
    }

    // Allow uses to fall off over time
    if(falloff) {
        workstr += "Falloff";
        *falloff = get_scriptparam_int(design_note, workstr.c_str());
    }
}


TWBaseScript::CountMode TWBaseScript::get_scriptparam_countmode(char* design_note, const char* param, CountMode def_mode)
{
    TWBaseScript::CountMode result = def_mode;

    // Get the value the editor has set for countonly (or the default) and process it
    char* mode = get_scriptparam_string(design_note, param, "Both");
    if(mode) {
        char* end = NULL;

        // First up, art thou an int?
        int parsed = strtol(mode, &end, 10);
        if(mode != end) {
            // Well, something parsed, only update the result if it is in range!
            if(parsed >= CM_NOTHING && parsed <= CM_BOTH) {
                result = static_cast<CountMode>(parsed);
            }

        // Doesn't appear to be numeric, so search for known modes
        } else if(!::_stricmp(mode, "None")) {
            result = CM_NOTHING;
        } else if(!::_stricmp(mode, "On")) {
            result = CM_TURNON;
        } else if(!::_stricmp(mode, "Off")) {
            result = CM_TURNOFF;
        } else if(!::_stricmp(mode, "Both")) {
            result = CM_BOTH;
        }

        g_pMalloc -> Free(mode);
    }

    return result;
}


float TWBaseScript::get_scriptparam_float(const char* design_note, const char* param, float def_val, std::string& qvar_str)
{
    float result = def_val;

    // Fetch the value as a string, if possible
    char* value = get_scriptparam_string(design_note, param);
    if(value) {
        result = parse_float(value, def_val, qvar_str);

        g_pMalloc -> Free(value);
    }

    return result;
}


int TWBaseScript::get_scriptparam_int(const char* design_note, const char* param, int def_val)
{
    int result = def_val;
    char* value = get_scriptparam_string(design_note, param);

    if(value) {
        char* workptr = value;

        // Skip any leading whitespace
        while(isspace(*workptr)) {
            ++workptr;
        }

        // If the string starts with a '$', it is a qvar, in theory
        if(*workptr == '$') {
            result = get_qvar_value(&workptr[1], def_val);
        } else {
            char* endstr;
            result = strtol(workptr, &endstr, 10);

            // Restore the default if parsing failed
            if(endstr == workptr) result = def_val;
        }

        g_pMalloc -> Free(value);
    }

    return result;
}


bool TWBaseScript::get_scriptparam_bool(const char* design_note, const char* param, bool def_val)
{
    std::string namestr = Name();
    namestr += param;

    return GetParamBool(design_note, namestr.c_str(), def_val);
}


char* TWBaseScript::get_scriptparam_string(const char* design_note, const char* param, const char* def_val)
{
    std::string namestr = Name();
    namestr += param;

    return GetParamString(design_note, namestr.c_str(), def_val);
}


bool TWBaseScript::get_scriptparam_floatvec(const char* design_note, const char* param, cScrVec& vect, float defx, float defy, float defz)
{
    bool parsed = false;
    char* value = get_scriptparam_string(design_note, param, NULL);

    if(value) {
        char* ystr = comma_split(value);              // Getting y is safe...
        char* zstr = ystr ? comma_split(ystr) : NULL; // z needs to be handled more carefully

        std::string tmp; // This is actually throw-away, needed for parse_float

        vect.x = parse_float(value, defx, tmp);
        vect.y = parse_float( ystr, defy, tmp); // Note these are safe even if ystr and zstr are NULL
        vect.z = parse_float( zstr, defz, tmp); // as parse_float checks for non-NULL

        parsed = true;

        g_pMalloc -> Free(value);
    }

    return parsed;
}


int TWBaseScript::get_qvar_namelen(const char* namestr)
{
    const char* workptr = namestr;

    // Work along the string looking for /, * or null
    while(*workptr && *workptr != '/' && *workptr != '*') ++workptr;

    // not gone anywhere? No name available...
    if(workptr == namestr) return 0;

    // Go back a char, and strip spaces
    do {
        --workptr;
    } while(*workptr == ' ');

     // Return length + 1, as the above loop always backs up 1 char too many
    return (workptr - namestr) + 1;
}


/* ------------------------------------------------------------------------
 *  Initialisation related
 */

void TWBaseScript::init(int time)
{
    char* design_note = GetObjectParams(ObjId());

    if(design_note) {
        debug = get_scriptparam_bool(design_note, "Debug");

        if(debug_enabled()) {
            debug_printf(DL_DEBUG, "Attached %s version %s", Name(), SCRIPT_VERSTRING);
            debug_printf(DL_DEBUG, "Script debugging enabled");
        }

        g_pMalloc -> Free(design_note);
    }

    // Set up randomisation
    uint seed = std::chrono::system_clock::now().time_since_epoch().count();
    randomiser.seed(seed);
}


/* ------------------------------------------------------------------------
 *  Message handling
 */

long TWBaseScript::dispatch_message(sScrMsg* msg, sMultiParm* reply)
{
    // Only bother checking for fixup stuff if it hasn't been done.
    if(need_fixup) {
        // On starting sim, fix any links if possible
        if(!::_stricmp(msg -> message, "Sim") && static_cast<sSimMsg*>(msg) -> fStarting) {
            fixup_player_links();

        // Catch and handle the deferred player link fixup if needed
        } else if(!::_stricmp(msg -> message, "Timer") &&
                  !::_stricmp(static_cast<sScrTimerMsg*>(msg) -> name, "DelayInit") &&
                  static_cast<sScrTimerMsg*>(msg) -> data == "FixupPlayerLinks") { // Note: data is a cMultiParm, so == does a strcmp internally
            fixup_player_links();
        }
    }

    // Capture and bin Null messages (to prevent TornOn/TurnOff triggering in
    // subclasses when the TirnOn/TurnOff message has been set to Null)
    if(!::_stricmp(msg -> message, "Null")) {
        return S_OK;
    }

    // Invoke the message handling!
    return (on_message(msg, static_cast<cMultiParm&>(*reply)) != MS_ERROR);
}


/* ------------------------------------------------------------------------
 *  Targetting
 */

std::vector<TargetObj>* TWBaseScript::get_target_objects(const char* target, sScrMsg* msg)
{
    std::vector<TargetObj>* matches = new std::vector<TargetObj>;

    // Make sure target is actually set before doing anything
    if(!target || *target == '\0') return matches;

    float radius;
    bool  lessthan;
    const char* archname;
    TargetObj newtarget = { 0, 0 };

    // Simple target/source selection.
    if(!_stricmp(target, "[me]")) {
        newtarget.obj_id = msg -> to;
        matches -> push_back(newtarget);

    } else if(!_stricmp(target, "[source]")) {
        newtarget.obj_id = msg -> from;
        matches -> push_back(newtarget);

    // linked objects
    } else if(*target == '&') {
        link_search(matches, ObjId(), &target[1]);

    // Archetype search, direct concrete and indirect concrete
    } else if(*target == '*' || *target == '@') {
        archetype_search(matches, &target[1], *target == '@');

    // Radius archetype search
    } else if(radius_search(target, &radius, &lessthan, &archname)) {
        const char* realname = archname;
        // Jump filter controls if needed...
        if(*archname == '*' || *archname == '@') ++realname;

        // Default behaviour for radius search is to get all decendants unless * is specified.
        archetype_search(matches, realname, *archname != '*', true, msg -> to, radius, lessthan);

    // Named destination object
    } else {
        SInterface<IObjectSystem> ObjectSys(g_pScriptManager);

        newtarget.obj_id = ObjectSys -> GetObjectNamed(target);
        if(newtarget.obj_id)
            matches -> push_back(newtarget);
    }

    return matches;
}


/* ------------------------------------------------------------------------
 *  Link Targetting
 */

void TWBaseScript::link_search(std::vector<TargetObj>* matches, const int from, const char* linkdef)
{
    std::vector<LinkScanWorker> links;
    bool is_random = false, is_weighted = false;
    uint fetch_count = 0;

    // Parse the link definition, and fetch the list of possible matching links
    const char* flavour = link_search_setup(linkdef, &is_random, &is_weighted, &fetch_count);
    uint count = link_scan(flavour, from, is_weighted, links);

    if(count) {
        // If no fetch count has been explicitly set, use the whole size, unless random is set
        if(fetch_count < 1) fetch_count = is_random ? 1 : links.size();

        if(is_random) {
            select_random_links(matches, links, fetch_count, count, is_weighted);
        } else {
            select_links(matches, links, fetch_count);
        }
    }
}


const char* TWBaseScript::link_search_setup(const char* linkdef, bool* is_random, bool* is_weighted, uint* fetch_count)
{
    while(*linkdef) {
        switch(*linkdef) {
            // The ? sigil indicates that the link mode should be random
            case '?': *is_random = true;
                break;

            // [ indicates the start of a [N] block, probably
            case '[': linkdef = parse_link_count(linkdef, fetch_count);

                // If the linkdef char is still [, what follows is not a number,
                // the ++linkdef below will skip the [.
                break;

            // Not a recognised sigil? Assume that it's the start of a link flavour
            // name (or "Weighted", in which case enabled weighted random mode)
            default: if(!strcasecmp(linkdef, "Weighted")) {
                        *is_weighted = *is_random = true;
                        return "ScriptParams"; // Weighted mode looks at scriptparams
                     }
                     return linkdef;
                break;
        }
        ++linkdef;
    }

    // Fallback for a horribly broken string is always "ControlDevice"
    return "ControlDevice";
}


const char* TWBaseScript::parse_link_count(const char* linkdef, uint* fetch_count)
{
    // linkdef should be a pointer to a '[' - check to be sure
    if(*linkdef == '[') {
        ++linkdef;

        char* endptr;
        int value = strtol(linkdef, &endptr, 10);

        // Has anything been parsed at all?
        if(endptr != linkdef) {
            // A value was parsed, but only positive non-zero values make any sense
            if(value > 0) {
                *fetch_count = value;
            }

            // copy the end pointer so that we can try to find the ]
            const char *close = endptr;

            // Skip anything up to the ] if possible
            while(*close && *close != ']')
                ++close;

            // If the close ] was found, return the pointer to it. If it wasn't,
            // return the pointer to the last character in the number, as link_search_setup
            // will immediately ++ this on return.
            return *close ? close : --endptr;
        }

        // The data after the [ was not numeric, so return the pointer to the [
        // so that link_search_setup can skip it.
        return --linkdef;
    }

    // Not a number block, why was this even called?
    return linkdef;
}


uint TWBaseScript::link_scan(const char* flavour, const int from, const bool weighted, std::vector<LinkScanWorker>& links)
{
    // If there is no link flavour, do nothing
    if(!flavour || !*flavour) return 0;

    SService<ILinkToolsSrv> LinkToolsSrv(g_pScriptManager);

    uint accumulator = 0;
    long flavourid =  LinkToolsSrv -> LinkKindNamed(flavour);

    if(flavourid) {
        // At this point, we need to locate all the linked objects that match the flavour and mode
        SService<ILinkSrv>      LinkSrv(g_pScriptManager);
        linkset matching_links;
        LinkScanWorker temp = { 0, 0, 0, 0 };

        // Traverse the list of links that match the selected flavour.
        LinkSrv -> GetAll(matching_links, flavourid, from, 0);
        while(matching_links.AnyLinksLeft()) {
            // Get the common link information
            temp.link_id = matching_links.Link();
            temp.dest_id = matching_links.Get().dest;

            // If weighting is enabled, fetch the weighting information from the link.
            if(weighted) {
                const char* data = static_cast<const char* >(matching_links.Data());

                temp.weight = strtol(data, NULL, 10);
                if(temp.weight < 1) temp.weight = 1; // Force positive non-zero weights
                accumulator += temp.weight;
            }

            links.push_back(temp);

            matching_links.NextLink();
        }
    }

    // Ensure that the list is sorted by link IDs. In theory it already should be, but
    // this will guarantee it.
    if(links.size() > 0)
        std::sort(links.begin(), links.end());

    if(weighted) {
        return accumulator;
    } else {
        return links.size();
    }
}


bool pick_weighted_link(std::vector<LinkScanWorker>& links, const uint target, TargetObj& store)
{
    std::vector<LinkScanWorker>::iterator it;

    for(it = links.begin(); it < links.end(); it++) {
        if((*it).cumulative >= target) {
            store = (*it);
            return true;
        }
    }

    return false;
}


uint TWBaseScript::build_link_weightsums(std::vector<LinkScanWorker>& links)
{
    uint accumulator = 0;
    std::vector<LinkScanWorker>::iterator it;

    for(it = links.begin(); it < links.end(); it++) {
        accumulator += it -> weight;
        it -> cumulative = accumulator;
    }

    return accumulator;
}


void TWBaseScript::select_random_links(std::vector<TargetObj>* matches, std::vector<LinkScanWorker>& links, const uint fetch_count, const uint total_weights, const bool is_weighted)
{
    // Yay for easy randomisation
    std::shuffle(links.begin(), links.end(), randomiser);

    // Weighted selection needs cumulative weight information
    if(is_weighted) build_link_weightsums(links);

    TargetObj chosen;

    // Pick the requested number of links
    for(uint pass = 0; pass < fetch_count; ++pass) {
        // Weighted mode needs more work to pick the item
        if(is_weighted) {
            pick_weighted_link(links, 1 + (randomiser() % total_weights), chosen);
        } else {
            chosen = links[randomiser() % total_weights]; // at this point, total_weights is actually links.size()
        }

        // Store the chosen item
        matches -> push_back(chosen);
    }
}


void TWBaseScript::select_links(std::vector<TargetObj>* matches, std::vector<LinkScanWorker>& links, const uint fetch_count)
{
    uint copied = 0;
    TargetObj newtemp = { 0, 0 };
    std::vector<LinkScanWorker>::iterator it;

    for(it = links.begin(); it < links.end() && copied < fetch_count; it++, copied++) {
        newtemp = *it;
        matches -> push_back(newtemp);
    }
}


/* ------------------------------------------------------------------------
 *  Search methods
 */

bool TWBaseScript::radius_search(const char* target, float* radius, bool* lessthan, const char** archetype)
{
    char mode = 0; // This gets set to '>' or '<' if a mode is found in the string
    const char* search = target;

    // Search the string for a < or >, if found, record it and the start of the archetype
    while(*search) {
        if(*search == '<' || *search == '>') {
            mode = *search;
            *lessthan = (mode == '<');
            *archetype = search + 1;
            break;
        }
        ++search;
    }

    // If the mode hasn't been found, or there's no archetype set, it's not a radius search
    if(!mode || !*archetype) return false;

    // It's a radius search, so try to parse the radius
    char* end;
    *radius = strtof(target, &end);

    // If the value didn't parse, or parsing stopped before the < or >, give up (the latter
    // is actually probably 'safe', but meh)
    if(end == target || *end != mode) return false;

    // Okay, this should be a radius search!
    return true;
}


void TWBaseScript::archetype_search(std::vector<TargetObj>* matches, const char* archetype, bool do_full, bool do_radius, object from_obj, float radius, bool lessthan)
{
    // Get handles to game interfaces here for convenience
    SInterface<IObjectSystem> ObjectSys(g_pScriptManager);
	SService<IObjectSrv>      ObjectSrv(g_pScriptManager);
    SInterface<ITraitManager> TraitMgr(g_pScriptManager);

    // These are only needed when doing radius searches
    cScrVec from_pos, to_pos;
    float   distance;
    if(do_radius) ObjectSrv -> Position(from_pos, from_obj);

    // Find the archetype named if possible
    object arch = ObjectSys -> GetObjectNamed(archetype);
    if(int(arch) <= 0) {

        // Build the query flags
        ulong flags = kTraitQueryChildren;
        if(do_full) flags |= kTraitQueryFull; // If dofull is on, query direct and indirect descendants

        // Ask for the list of matching objects
        SInterface<IObjectQuery> query = TraitMgr -> Query(arch, flags);
        if(query) {
            TargetObj newtarget = { 0, 0 };

            // Process each object, adding it to the match list if it's concrete.
            for(; !query -> Done(); query -> Next()) {
                newtarget.obj_id = query -> Object();
                if(newtarget.obj_id > 0) {

                    // Object is concrete, do we need to check it for distance?
                    if(do_radius) {
                        // Get the provisionally matched object's position, and work out how far it
                        // is from the 'from' object.
                        ObjectSrv -> Position(to_pos, newtarget.obj_id);
                        distance = (float)from_pos.Distance(to_pos);

                        // If the distance check passes, store the object.
                        if((lessthan && (distance < radius)) || (!lessthan && (distance > radius))) {
                            matches -> push_back(newtarget);
                        }

                    // No radius check needed, add straight to the list
                    } else {
                        matches -> push_back(newtarget);
                    }
                }
            }
        }
    }
}


/* ------------------------------------------------------------------------
 *  Link inspection
 */

int TWBaseScript::get_linked_object(const int from, const std::string& obj_name, const std::string& link_name, const int fallback)
{
    SInterface<IObjectSystem> ObjectSys(g_pScriptManager);
    SService<IObjectSrv>      ObjectSrv(g_pScriptManager);
    SService<ILinkSrv>        LinkSrv(g_pScriptManager);
    SService<ILinkToolsSrv>   LinkToolsSrv(g_pScriptManager);

    // Can't do anything if there is no archytype name set
    if(!obj_name.empty()) {

        // Attempt to locate the object requested
        int object = StrToObject(obj_name.c_str());
        if(object) {

            // Convert the link to a liny type ID
            long flavourid = LinkToolsSrv -> LinkKindNamed(link_name.c_str());

            if(flavourid) {
                // Does the object have a link of the specified flavour?
                true_bool has_link;
                LinkSrv -> AnyExist(has_link, flavourid, from, 0);

                // Only do anything if there is at least one particle attachment.
                if(has_link) {
                    linkset links;
                    true_bool inherits;

                    // Check all the links of the appropriate flavour, looking for a link either to
                    // the named object, or to an object that inherits from it
                    LinkSrv -> GetAll(links, flavourid, from, 0);
                    while(links.AnyLinksLeft()) {
                        sLink link = links.Get();

                        // If the object is an archetype, check whether the destination inherits from it.
                        if(object < 0) {
                            ObjectSrv -> InheritsFrom(inherits, link.dest, object);

                            // Found a link from a concrete instance of the archetype? Return that object.
                            if(inherits) {
                                return link.dest;
                            }

                        // If the target object is concrete, is the link to that object?
                        } else if(link.dest == object) {
                            return link.dest;
                        }

                        links.NextLink();
                    }

                    if(debug_enabled())
                        debug_printf(DL_WARNING, "Object has no %s link to a object named or inheriting from %s", link_name.c_str(), obj_name.c_str());
                }
            } else {
                debug_printf(DL_ERROR, "Request for non-existent link flavour %s", link_name.c_str());
            }
        } else if(debug_enabled()) {
            debug_printf(DL_WARNING, "Unable to find object named '%s'", obj_name.c_str());
        }
    } else {
        debug_printf(DL_ERROR, "obj_name name is empty.");
    }

    return fallback;
}


/* ------------------------------------------------------------------------
 *  qvar related
 */

int TWBaseScript::get_qvar(const char* qvar, int def_val)
{
    SService<IQuestSrv> QuestSrv(g_pScriptManager);
    if(QuestSrv -> Exists(qvar))
        return QuestSrv -> Get(qvar);

    return def_val;
}


float TWBaseScript::get_qvar(const char* qvar, float def_val)
{
    SService<IQuestSrv> QuestSrv(g_pScriptManager);
    if(QuestSrv -> Exists(qvar))
        return static_cast<float>(QuestSrv -> Get(qvar));

    return def_val;
}


char* TWBaseScript::parse_qvar(const char* qvar, char** lhs, char* op, char** rhs)
{
    char* buffer = new char[strlen(qvar) + 1];
    if(!buffer) return NULL;
    strcpy(buffer, qvar);

    char* workstr = buffer;

    // skip any leading spaces or $
    while(*workstr && (isspace(*workstr) || *workstr == '$')) {
        ++workstr;
    }

    *lhs = workstr;

    // Search for an operator
    char* endstr = NULL;
    *op = '\0';
    while(*workstr) {
        // NOTE: '-' is not included here. LarryG encountered problems with using QVar names containing
        // '-' as this was interpreting it as an operator.
        if(*workstr == '+' || *workstr == '*' || *workstr == '/') {
            *op = *workstr;
            endstr = workstr - 1; // record the character before the operator, for space trimming
            *workstr = '\0';      // terminate so that lhs can potentially be used 'as is'
            ++workstr;
            break;
        }
        ++workstr;
    }

    // Only bother doing any more work if an operator was found
    if(op && endstr) {
        // Trim spaces before the operator if needed
        while(isspace(*endstr)) {
            *endstr = '\0';
            --endstr;
        }

        // Skip spaces before the second operand
        while(*workstr && isspace(*workstr)) {
            ++workstr;
        }

        // If there is anything left on the right side, store the pointer to it
        if(*workstr) {
            *rhs = workstr;
        }
    }

    return buffer;
}


/* ------------------------------------------------------------------------
 *  Miscellaneous stuff
 */

void TWBaseScript::fixup_player_links(void)
{
    int player = StrToObject("Player");
    if (player) {
        ::FixupPlayerLinks(ObjId(), player);
        need_fixup = false;
    } else {
        g_pScriptManager -> SetTimedMessage2(ObjId(), "DelayInit", 1, kSTM_OneShot, "FixupPlayerLinks");
    }
}


char* TWBaseScript::comma_split(char* src)
{
    while(*src) {
        if(*src == ',') {
            *src = '\0';
            return (src + 1);
        }
        ++src;
    }

    return NULL;
}
