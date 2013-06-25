
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

#include "TWBaseScript.h"
#include "ScriptModule.h"
#include "ScriptLib.h"

const char* const TWBaseScript::debug_levels[] = {"DEBUG", "WARNING", "ERROR"};


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
        sim_running = static_cast<sSimMsg *>(msg) -> fStarting;
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
    if(!::_stricmp(msg -> message, "BeginScript")) {
        init(msg -> time);
    }

    return MS_CONTINUE;
}


const char* TWBaseScript::get_message_type(sScrMsg* msg) const
{
    /* Okay, seriously, what the fuck. Attempting actual RTTI on msg with typeid(*msg)
     * will crash, I guess due to a corrupt vtable. At least, that's the only thing I
     * can think of that will cause that in this case. So, calling msg -> GetName()?
     * That crashes too, at least when using liblg.a compiled in gcc, again I suspect
     * because of vtable corruption or simply :fullofspiders:. However, going into the
     * "hack" vtable seems to work, because madness.
     *
     * Lasciate ogne speranza, voi ch'intrate, damnit.
     */
#ifdef _MSC_VER
    return msg -> GetName();  // No idea if this actually works in MSVC, see scrmsgs.h for more
#else
    // This is the horrible mess that seems to work fine under GCC. Don't ask me,
    // this shit is messed up.
    return msg -> persistent_hack -> thunk_GetName();
#endif
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
    cAnsiStr name;
    char buffer[900]; // Temporary buffer, 900 is the limit imposed by MPrint. This is really nasty and needs fixing.

    // Need the name of the current object
    get_object_namestr(name);

    // build the string the caller requested
    va_start(args, format);
    _vsnprintf(buffer, 899, format, args);
    va_end(args);

    // And spit out the result to monolog
	g_pfnMPrintf("%s[%s(%s)]: %s\n", debug_levels[level], Name(), static_cast<const char* >(name), buffer);
}


void TWBaseScript::get_object_namestr(cAnsiStr& name, object obj_id)
{
    // NOTE: obj_name isn't freed when GetName sets it to non-NULL. As near
    // as I can tell, it doesn't need to, it only needs to be freed when
    // using the version of GetName in ObjectSrv... Probably. Maybe. >.<
    // The docs for this are pretty shit, so this is mostly guesswork.

    SInterface<IObjectSystem> ObjSys(g_pScriptManager);
    const char* obj_name = ObjSys -> GetName(obj_id);

    // If the object system has returned a name here, the concrete object
    // has been given a name, so use it
    if(obj_name) {
        name.FmtStr("%s (%d)", obj_name, int(obj_id));

    // Otherwise, the concrete object has no name, get its archetype name
    // if possible and use that instead.
    } else {
        SInterface<ITraitManager> TraitMan(g_pScriptManager);
        object archetype_id = TraitMan -> GetArchetype(obj_id);
        const char* archetype_name = ObjSys -> GetName(archetype_id);

        // Archetype name found, use it in the string
        if(archetype_name) {
            name.FmtStr("A %s (%d)", archetype_name, int(obj_id));

        // Can't find a name or archetype name (!), so just use the ID
        } else {
            name.FmtStr("%d", int(obj_id));
        }
    }
}


void TWBaseScript::get_object_namestr(cAnsiStr& name)
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

        g_pMalloc -> Free(buffer);
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

        g_pMalloc -> Free(buffer);
    }

    return value;
}


/* ------------------------------------------------------------------------
 *  Design note support
 */

float TWBaseScript::parse_float(const char* param, float def_val, cAnsiStr& qvar_str)
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


void TWBaseScript::get_scriptparam_valuefalloff(char* design_note, const char* param, int *value, int *falloff)
{
    cAnsiStr workstr;

    // Get the value
    if(value) {
        *value = get_scriptparam_int(design_note, param);
    }

    // Allow uses to fall off over time
    if(falloff) {
        workstr.FmtStr("%sFalloff", param);
        *falloff = get_scriptparam_int(design_note, static_cast<const char* >(workstr));
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


float TWBaseScript::get_scriptparam_float(const char* design_note, const char* param, float def_val, cAnsiStr& qvar_str)
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


int TWBaseScript::get_scriptparam_int(const char *design_note, const char *param, int def_val)
{
    int result = def_val;
    char *value = get_scriptparam_string(design_note, param);

    if(value) {
        char *workptr = value;

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


bool TWBaseScript::get_scriptparam_bool(const char *design_note, const char *param, bool def_val)
{
    cAnsiStr namestr = Name();
    namestr += param;

    return GetParamBool(design_note, static_cast<const char* >(namestr), def_val);
}


char* TWBaseScript::get_scriptparam_string(const char* design_note, const char* param, const char* def_val)
{
    cAnsiStr namestr = Name();
    namestr += param;

    return GetParamString(design_note, static_cast<const char* >(namestr), def_val);
}


bool TWBaseScript::get_scriptparam_floatvec(const char* design_note, const char* param, cScrVec& vect, float defx, float defy, float defz)
{
    bool parsed = false;
    char* value = get_scriptparam_string(design_note, param, NULL);

    if(value) {
        char* ystr = comma_split(value);              // Getting y is safe...
        char* zstr = ystr ? comma_split(ystr) : NULL; // z needs to be handled more carefully

        cAnsiStr tmp; // This is actually throw-away, needed for parse_float

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
 *  Targetting
 */

std::vector<object>* TWBaseScript::get_target_objects(const char* target, sScrMsg *msg)
{
    std::vector<object>* matches = new std::vector<object>;

    // Make sure target is actually set before doing anything
    if(!target || *target == '\0') return matches;

    float radius;
    bool  lessthan;
    const char* archname;

    // Simple target/source selection.
    if(!_stricmp(target, "[me]")) {
        matches -> push_back(msg -> to);

    } else if(!_stricmp(target, "[source]")) {
        matches -> push_back(msg -> from);

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

        object obj = ObjectSys -> GetObjectNamed(target);
        if(obj) matches -> push_back(obj);
    }

    return matches;
}


/* ------------------------------------------------------------------------
 *  Initialisation related
 */

void TWBaseScript::init(int time)
{
    char *design_note = GetObjectParams(ObjId());

    if(design_note) {
        debug = get_scriptparam_bool(design_note, "Debug");

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Script debugging enabled");

        g_pMalloc -> Free(design_note);
    }
}


/* ------------------------------------------------------------------------
 *  Message handling
 */

long TWBaseScript::dispatch_message(sScrMsg* msg, sMultiParm* reply)
{
    // Only bother checking for fixup stuff if it hasn't been done.
    if(need_fixup) {
        // On starting sim, fix any links if possible
        if(!::_stricmp(msg -> message, "Sim") && static_cast<sSimMsg *>(msg) -> fStarting) {
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


void TWBaseScript::archetype_search(std::vector<object> *matches, const char* archetype, bool do_full, bool do_radius, object from_obj, float radius, bool lessthan)
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

            // Process each object, adding it to the match list if it's concrete.
            for(; !query -> Done(); query -> Next()) {
                object obj = query -> Object();
                if(int(obj) > 0) {

                    // Object is concrete, do we need to check it for distance?
                    if(do_radius) {
                        // Get the provisionally matched object's position, and work out how far it
                        // is from the 'from' object.
                        ObjectSrv -> Position(to_pos, obj);
                        distance = (float)from_pos.Distance(to_pos);

                        // If the distance check passes, store the object.
                        if((lessthan && (distance < radius)) || (!lessthan && (distance > radius))) {
                            matches -> push_back(obj);
                        }

                    // No radius check needed, add straight to the list
                    } else {
                        matches -> push_back(obj);
                    }
                }
            }
        }
    }
}


bool TWBaseScript::radius_search(const char* target, float *radius, bool *lessthan, const char* *archetype)
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


char* TWBaseScript::parse_qvar(const char* qvar, char** lhs, char* op, char **rhs)
{
    char* buffer = static_cast<char* >(g_pMalloc -> Alloc(strlen(qvar) + 1));
    if(!buffer) return NULL;
    strcpy(buffer, qvar);

    char *workstr = buffer;

    // skip any leading spaces or $
    while(*workstr && (isspace(*workstr) || *workstr == '$')) {
        ++workstr;
    }

    *lhs = workstr;

    // Search for an operator
    char *endstr = NULL;
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
