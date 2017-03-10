
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
            result = get_qvar_value(qvar_str, def_val);

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


void TWBaseScript::get_scriptparam_valuefalloff(char* design_note, const char* param, int* value, int* falloff, bool* limit)
{
    std::string workstr = param;
    std::string dummy;

    // Get the value
    if(value) {
        *value = get_scriptparam_int(design_note, param, 0, dummy);
    }

    // Allow uses to fall off over time
    if(falloff) {
        workstr += "Falloff";
        *falloff = get_scriptparam_int(design_note, workstr.c_str(), 0, dummy);
    }

    // And allow counting to be limited
    if(limit) {
        workstr = param;
        workstr += "Limit";
        *limit = get_scriptparam_bool(design_note, workstr.c_str());
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


int TWBaseScript::get_scriptparam_int(const char* design_note, const char* param, int def_val, std::string& qvar_str)
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
            qvar_str = &param[1];
            result = get_qvar_value(qvar_str, def_val);
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


int TWBaseScript::get_scriptparam_time(const char* design_note, const char* param, int def_val, std::string& qvar_str)
{
    // float is used internally for time handling, as the user may specify fractional
    // seconds or minutes
    float result = float(def_val);
    char* value = get_scriptparam_string(design_note, param);

    if(value) {
        char* workptr = value;

        // Skip any leading whitespace
        while(isspace(*workptr)) ++workptr;

        // If the string starts with a '$', it is a qvar, in theory
        if(*workptr == '$') {
            qvar_str = &param[1];
            result = get_qvar_value(qvar_str, float(def_val));
        } else {
            char* endstr;
            result = strtof(workptr, &endstr);

            // Restore the default if parsing failed
            if(endstr == workptr) {
                result = def_val;

            // otherwise, see if the character at the end is something we recognise
            } else if(*endstr) {
                // skip spaces, just in case
                while(isspace(*workptr)) ++workptr;

                switch(*endstr) {
                    // 's' indicates the time is in seconds, multiply up to milliseconds
                    case 's': result *= 1000.0f; break;

                    // 'm' indicates the time is in minutes, multiply up to milliseconds
                    case 'm': result *= 60000.0f; break;
                }
            }
        }

        g_pMalloc -> Free(value);
    }

    // Drop the fractional part on the way out - here result is in integer milliseconds
    return int(result);
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
    } else {
        vect.x = defx;
        vect.y = defy;
        vect.z = defz;
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
 *  qvar related
 */


void TWBaseScript::set_qvar(const std::string &qvar, const int value)
{
    SService<IQuestSrv> QuestSrv(g_pScriptManager);
    QuestSrv -> Set(qvar.c_str(), value, kQuestDataMission);
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
