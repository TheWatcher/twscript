
#include <lg/interface.h>
#include <lg/scrmsgs.h>
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


/* ------------------------------------------------------------------------
 *  Link inspection
 */

/* NOTE: Retain as this does not fall directly under normal targetting - the
 *       obj_name and link_name values can come from DesignParamString vars,
 *       but do not need to - hence subclasses may need to use this directly.
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
 *  Initialisation related
 */

void TWBaseScript::init(int time)
{
    std::string design_note = GetObjectParams(ObjId());

    if(design_note) {
        debug.init(design_note);

        if(debug_enabled()) {
            debug_printf(DL_DEBUG, "Attached %s version %s", Name(), SCRIPT_VERSTRING);
            debug_printf(DL_DEBUG, "Script debugging enabled");
        }

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
