
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

void TWBaseScript::set_qvar(const std::string &name, const int value)
{
    SService<IQuestSrv> QuestSrv(g_pScriptManager);
    QuestSrv -> Set(name.c_str(), value, kQuestDataMission);
}


int TWBaseScript::get_qvar(const std::string& name, int def_val)
{
    SService<IQuestSrv> QuestSrv(g_pScriptManager);
    if(QuestSrv -> Exists(name.c_str()))
        return QuestSrv -> Get(name.c_str());

    return def_val;
}


float TWBaseScript::get_qvar(const std::string& name, float def_val)
{
    SService<IQuestSrv> QuestSrv(g_pScriptManager);
    if(QuestSrv -> Exists(name.c_str()))
        return static_cast<float>(QuestSrv -> Get(name.c_str()));

    return def_val;
}


/* ------------------------------------------------------------------------
 *  Design note support
 */


float TWBaseScript::get_scriptparam_float(const char* design_note, const char* param, QVarVariable& variable, float def_val, bool subscribe);
{
    float result = def_val;

    // Fetch the value as a string, if possible
    char* value = get_scriptparam_string(design_note, param);
    if(value) {
        variable.init_float(value, def_val, subscribe);
        result = static_cast<float>(variable);

        g_pMalloc -> Free(value);
    }

    return result;
}


int TWBaseScript::get_scriptparam_int(const char* design_note, const char* param, QVarVariable& variable, int def_val, bool subscribe);
{
    int result = def_val;

    // Fetch the value as a string, if possible
    char* value = get_scriptparam_string(design_note, param);
    if(value) {
        variable.init_integer(value, def_val, subscribe);
        result = static_cast<int>(variable);

        g_pMalloc -> Free(value);
    }

    return result;
}


int TWBaseScript::get_scriptparam_time(const char* design_note, const char* param, QVarVariable& variable, int def_val, bool subscribe);
{
    int result = def_val;

    // Fetch the value as a string, if possible
    char* value = get_scriptparam_string(design_note, param);
    if(value) {
        variable.init_time(value, def_val, subscribe);
        result = static_cast<int>(variable);

        g_pMalloc -> Free(value);
    }

    return result;
}


bool TWBaseScript::get_scriptparam_bool(const char* design_note, const char* param, QVarVariable& variable, bool def_val, bool subscribe);
{
    bool result = def_val;

    // Fetch the value as a string, if possible
    char* value = get_scriptparam_string(design_note, param);
    if(value) {
        variable.init_boolean(value, def_val, subscribe);
        result = static_cast<bool>(variable);

        g_pMalloc -> Free(value);
    }

    return result;
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

        vect.x = parse_float(value, defx);
        vect.y = parse_float( ystr, defy,); // Note these are safe even if ystr and zstr are NULL
        vect.z = parse_float( zstr, defz); // as parse_float checks for non-NULL

        parsed = true;

        g_pMalloc -> Free(value);
    } else {
        vect.x = defx;
        vect.y = defy;
        vect.z = defz;
    }

    return parsed;
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


void TWBaseScript::get_scriptparam_valuefalloff(char* design_note, const char* param, QVarVariable* value = NULL, QVarVariable* falloff = NULL, QVarVariable* limit = NULL)
{
    std::string workstr = param;
    std::string dummy;

    // Get the value
    if(value) {
        get_scriptparam_int(design_note, param, *value, 0, dummy);
    }

    // Allow uses to fall off over time
    if(falloff) {
        workstr += "Falloff";
        get_scriptparam_int(design_note, workstr.c_str(), *falloff, 0, dummy);
    }

    // And allow counting to be limited
    if(limit) {
        workstr = param;
        workstr += "Limit";
        get_scriptparam_bool(design_note, workstr.c_str(), *limit);
    }
}


/* ------------------------------------------------------------------------
 *  Initialisation related
 */

void TWBaseScript::init(int time)
{
    char* design_note = GetObjectParams(ObjId());

    if(design_note) {
        get_scriptparam_bool(design_note, "Debug", &debug);

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
    } else if(*target == '<' || *target == '>') {
        if(radius_search(target, &radius, &lessthan, &archname)) {
            const char* realname = archname;
            // Jump filter controls if needed...
            if(*archname == '*' || *archname == '@') ++realname;

            // Default behaviour for radius search is to get all decendants unless * is specified.
            archetype_search(matches, realname, *archname != '*', true, msg -> to, radius, lessthan);
        }
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
    bool is_random = false, is_weighted = false, fetch_all = false;;
    uint fetch_count = 0;
    LinkMode mode = LM_BOTH;

    // Parse the link definition, and fetch the list of possible matching links
    const char* flavour = link_search_setup(linkdef, &is_random, &is_weighted, &fetch_count, &fetch_all, &mode);
    uint count = link_scan(flavour, from, is_weighted, mode, links);

    if(count) {
        // If no fetch count has been explicitly set, use the whole size, unless random is set
        if(fetch_count < 1) fetch_count = is_random ? 1 : links.size();

        // if fetch_all has been set, set the count to the link count even in random mode
        if(fetch_all) fetch_count = links.size();

        if(is_random) {
            select_random_links(matches, links, fetch_count, fetch_all, count, is_weighted);
        } else {
            select_links(matches, links, fetch_count);
        }
    }
}


const char* TWBaseScript::link_search_setup(const char* linkdef, bool* is_random, bool* is_weighted, uint* fetch_count, bool *fetch_all, LinkMode *mode)
{
    while(*linkdef) {
        switch(*linkdef) {
            // The ? sigil indicates that the link mode should be random
            case '?': *is_random = true;
                break;

            // The ! sigil indicates that all links should be returned
            case '!': *fetch_all = true;
                break;

            // [ indicates the start of a [N] block, probably
            case '[': linkdef = parse_link_count(linkdef, fetch_count);

                // If the linkdef char is still [, what follows is not a number,
                // the ++linkdef below will skip the [.
                break;

            case '%': *mode = LM_ARCHETYPE;
                break;

            case '#': *mode = LM_CONCRETE;
                break;

            // Not a recognised sigil? Assume that it's the start of a link flavour
            // name (or "Weighted", in which case enabled weighted random mode)
            default: if(!strcasecmp(linkdef, "Weighted")) {
                        *is_weighted = *is_random = true;
                        *fetch_all = false; // Can't use fetch_all mode for weighted random
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


uint TWBaseScript::link_scan(const char* flavour, const int from, const bool weighted, LinkMode mode, std::vector<LinkScanWorker>& links)
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
            temp.weight  = 1;
            temp.link_id = matching_links.Link();
            temp.dest_id = matching_links.Get().dest;

            if(mode == LM_BOTH ||                            // If linkmode is both, let through any destination
               (temp.dest_id < 0 && mode == LM_ARCHETYPE) || // Otherwisse, only let through archetypes or concrete if set
               (temp.dest_id > 0 && mode == LM_CONCRETE)) {

                // If weighting is enabled, fetch the weighting information from the link.
                if(weighted) {
                    const char* data = static_cast<const char* >(matching_links.Data());

                    temp.weight = strtol(data, NULL, 10);
                    if(temp.weight < 1) temp.weight = 1; // Force positive non-zero weights
                    accumulator += temp.weight;
                }

                links.push_back(temp);
            }

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


bool TWBaseScript::pick_weighted_link(std::vector<LinkScanWorker>& links, const uint target, TargetObj& store)
{
    std::vector<LinkScanWorker>::iterator it;

    for(it = links.begin(); it < links.end(); it++) {
        if((*it).cumulative >= target) {
            store = *it;
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


void TWBaseScript::select_random_links(std::vector<TargetObj>* matches, std::vector<LinkScanWorker>& links, const uint fetch_count, const bool fetch_all, const uint total_weights, const bool is_weighted)
{
    // Yay for easy randomisation
    std::shuffle(links.begin(), links.end(), randomiser);

    if(!is_weighted) {
        // Work out how many links to fetch, limiting it to the number available.
        uint count = fetch_all ? links.size() : fetch_count;
        if(count > links.size()) count = links.size();

        select_links(matches, links, count);

    } else {
        // Weighted selection needs cumulative weight information
        if(is_weighted) build_link_weightsums(links);

        TargetObj chosen;

        // Pick the requested number of links
        for(uint pass = 0; pass < fetch_count; ++pass) {
            // Weighted mode needs more work to pick the item
            pick_weighted_link(links, 1 + (randomiser() % total_weights), chosen);

            // Store the chosen item
            matches -> push_back(chosen);
        }
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
    // Check for < or > here
    *lessthan = (*target++ == '<');

    // try to parse the radius
    char* end;
    *radius = strtof(target, &end);

    if(!end || end == target) return false;

    // Look for the ':' in the string
    while(*end && *end != ':') ++end;

    // Hit end of string without finding a ':'? Give up.
    if(!*end) return false;

    // Archetype starts right after the ':'
    *archetype = ++end;

    // Make sure that end of string after ':' doesn't bite us.
    if(!**archetype) return false;

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


float TWBaseScript::parse_float(const char *srcstr, float def_val)
{
    char *endstr = NULL;

    float value = strtof(srcstr, &endstr);
    if(endstr == srcstr) {
        value = defval;
    }

    return value;
}
