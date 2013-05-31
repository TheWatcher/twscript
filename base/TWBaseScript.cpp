
#include <lg/interface.h>
#include <lg/scrmanagers.h>
#include <lg/scrservices.h>
#include <lg/objects.h>
#include <lg/links.h>
#include <lg/properties.h>
#include <lg/propdefs.h>
#include <exception>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>

#include "TWBaseScript.h"
#include "ScriptModule.h"
#include "ScriptLib.h"

const char * const TWBaseScript::debug_levels[] = {"DEBUG", "WARNING", "ERROR"};


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

void TWBaseScript::debug_printf(TWBaseScript::DebugLevel level, const char *format, ...)
{
    va_list args;
    cAnsiStr name;
    char buffer[1000]; // Temporary buffer, 1000 is the limit imposed by MPrint.

    // Need the name of the current object
    get_object_namestr(name);

    // build the string the caller requested
    va_start(args, format);
    _vsnprintf(buffer, 999, format, args);
    va_end(args);

    // And spit out the result to monolog
	g_pfnMPrintf("%s[%s(%s)]: %s\n", debug_levels[level], Name(), static_cast<const char *>(name), buffer);
}


void TWBaseScript::get_object_namestr(cAnsiStr &name, object obj_id)
{
    // NOTE: obj_name isn't freed when GetName sets it to non-NULL. As near
    // as I can tell, it doesn't need to, it only needs to be freed when
    // using the version of GetName in ObjectSrv... Probably. Maybe. >.<
    // The docs for this are pretty shit, so this is mostly guesswork.

    SInterface<IObjectSystem> ObjSys(g_pScriptManager);
    const char *obj_name = ObjSys -> GetName(obj_id);

    // If the object system has returned a name here, the concrete object
    // has been given a name, so use it
    if(obj_name) {
        name.FmtStr("%s (%d)", obj_name, int(obj_id));

    // Otherwise, the concrete object has no name, get its archetype name
    // if possible and use that instead.
    } else {
        SInterface<ITraitManager> TraitMan(g_pScriptManager);
        object archetype_id = TraitMan -> GetArchetype(obj_id);
        const char *archetype_name = ObjSys -> GetName(archetype_id);

        // Archetype name found, use it in the string
        if(archetype_name) {
            name.FmtStr("A %s (%d)", archetype_name, int(obj_id));

        // Can't find a name or archetype name (!), so just use the ID
        } else {
            name.FmtStr("%d", int(obj_id));
        }
    }
}


void TWBaseScript::get_object_namestr(cAnsiStr &name)
{
    get_object_namestr(name, ObjId());
}


/* ------------------------------------------------------------------------
 *  QVar convenience functions
 */

long TWBaseScript::get_qvar_value(const char *qvar, long def_val)
{
    SService<IQuestSrv> QuestSrv(g_pScriptManager);
    if(QuestSrv -> Exists(qvar))
        return QuestSrv -> Get(qvar);

    return def_val;
}


float TWBaseScript::get_qvar_value(const char *qvar, float def_val)
{
    float qvarval = def_val;
    char *endstr  = NULL;

    // Blegh, copy. Irritating, but we may need to tinker with it so that const has to go.
    char *tmpvar = static_cast<char *>(g_pMalloc -> Alloc(strlen(qvar) + 1));
    if(!tmpvar) return def_val;
    strcpy(tmpvar, qvar);

    // Check whether the user has included a multiply or divide operator
    char op = '\0';
    char *valstr = tmpvar;
    while(*valstr) {
        if(*valstr == '/' || *valstr == '*') {
            op = *valstr;
            endstr = valstr - 1; // Keep track of the last character before the operator, for space trimming
            *valstr = '\0';      // null terminate the qvar name so tmpvar might be used 'as is'
            ++valstr;
            break;
        }
        ++valstr;
    }
    // Skip spaces before the second operand if needed
    while(*valstr == ' ') {
        ++valstr;
    }

    // Trim spaces before the operator if needed
    if(endstr) {
        while(*endstr == ' ') {
            *endstr = '\0';
            --endstr;
        }
    }

    // Check the QVar exists before trying to use it...
    SService<IQuestSrv> QuestSrv(g_pScriptManager);
    if(QuestSrv -> Exists(tmpvar)) {
        qvarval = (float)QuestSrv -> Get(tmpvar);

        // If an operator has been specified, try to parse the second operand and apply it
        if(op) {
            float adjval;
            endstr = NULL;

            // Is the value another QVar? If so, pull its value
            if(*valstr == '$') {
                adjval = (float)get_qvar_value(&valstr[1], (long)1); // default needs to be explicitly set to long.
            } else {
                // Doesn't appear to be a qvar, treat it as a float and see what happens...
                adjval = strtof(valstr, &endstr);
            }

            // Has a value been parsed, and is not zero? (zeros are bad here...) If so, apply the operator.
            if(endstr != valstr && adjval) {
                switch(op) {
                    case '/': qvarval /= adjval; break;
                    case '*': qvarval *= adjval; break;
                }
            }
        }
    }

    // Done with the copy now
    g_pMalloc -> Free(tmpvar);

    return qvarval;
}


/* ------------------------------------------------------------------------
 *  Design note support
 */

float TWBaseScript::parse_float(const char *param, float def_val, cAnsiStr &qvar_str)
{
    float result = def_val;

    // Gracefully handle the situation where the parameter is NULL or empty
    if(param || *param == '\0') {

        // Starting with $ indicates that the string contains a qvar, fetch it
        if(*param == '$') {
            // Store the qvar string for later
            qvar_str = &param[1];
            result = get_qvar_value(&param[1], def_val);

        // Otherwise assume it's a float string
        } else {
            char *endstr;
            result = strtof(param, &endstr);

            // Restore the default if parsing failed
            if(endstr == param) result = def_val;
        }
    }

    return result;
}


bool TWBaseScript::get_param_floatvec(const char *design_note, const char *name, cScrVec &vect, float defx, float defy, float defz)
{
    bool parsed = false;
    char *param = GetParamString(design_note, name, NULL);

    if(param) {
        char *ystr = comma_split(param);              // Getting y is safe...
        char *zstr = ystr ? comma_split(ystr) : NULL; // z needs to be handled more carefully

        cAnsiStr tmp; // This is actually throw-away, needed for parse_float

        vect.x = parse_float(param, defx, tmp);
        vect.y = parse_float( ystr, defy, tmp); // Note these are safe even if ystr and zstr are NULL
        vect.z = parse_float( zstr, defz, tmp); // as parse_float checks for non-NULL

        parsed = true;

        g_pMalloc -> Free(param);
    }

    return parsed;
}


float TWBaseScript::get_param_float(const char *design_note, const char *name, float def_val, cAnsiStr &qvar_str)
{
    float result = def_val;

    // Fetch the value as a string, if possible
    char *param = GetParamString(design_note, name, NULL);
    if(param) {
        result = parse_float(param, def_val, qvar_str);

        g_pMalloc -> Free(param);
    }

    return result;
}


std::vector<object>* TWBaseScript::get_target_objects(const char *target, sScrMsg *msg)
{
    std::vector<object>* matches = new std::vector<object>;

    // Make sure target is actually set before doing anything
    if(!target || *target == '\0') return matches;

    float radius;
    bool  lessthan;
    const char *archname;

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
        const char *realname = archname;
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


void TWBaseScript::archetype_search(std::vector<object> *matches, const char *archetype, bool do_full, bool do_radius, object from_obj, float radius, bool lessthan)
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


bool TWBaseScript::radius_search(const char *target, float *radius, bool *lessthan, const char **archetype)
{
    char mode = 0; // This gets set to '>' or '<' if a mode is found in the string
    const char *search = target;

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
    char *end;
    *radius = strtof(target, &end);

    // If the value didn't parse, or parsing stopped before the < or >, give up (the latter
    // is actually probably 'safe', but meh)
    if(end == target || *end != mode) return false;

    // Okay, this should be a radius search!
    return true;
}


int TWBaseScript::get_qvar_namelen(const char *namestr)
{
    const char *workptr = namestr;

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


char *TWBaseScript::comma_split(char *src)
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
