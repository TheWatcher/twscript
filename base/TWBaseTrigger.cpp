
#include <lg/objects.h>
#include "TWBaseTrigger.h"
#include "ScriptLib.h"


/* ------------------------------------------------------------------------
 *  Message handling
 */

TWBaseScript::MsgStatus TWBaseTrigger::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseScript::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    // nothing here... yet.

    return MS_CONTINUE;
}


/* ------------------------------------------------------------------------
 *  Initialisation related
 */

void TWBaseTrigger::init(int time)
{
    TWBaseScript::init(time);

    char *msg;
    char *design_note = GetObjectParams(ObjId());

    if(design_note) {
        // Work out what the turnon and turnoff messages should be
        if((msg = get_scriptparam_string(design_note, "TOff", "TurnOff")) != NULL) {
            messages[0] = msg;
            isstim[0] = check_stimulus_message(msg, &stimob[0], &intensity[0]);

            g_pMalloc -> Free(msg);
        }

        if((msg = get_scriptparam_string(design_note, "TOn", "TurnOn")) != NULL) {
            messages[1] = msg;
            isstim[1] = check_stimulus_message(msg, &stimob[1], &intensity[1]);

            g_pMalloc -> Free(msg);
        }

        if((msg = get_scriptparam_string(design_note, "TDest", "&ControlDevice")) != NULL) {
            dest_str = msg;
            g_pMalloc -> Free(msg);
        }

        remove_links = get_scriptparam_bool(design_note, "KillLinks");

        if(debug_enabled()) {
            debug_printf(DL_DEBUG, "Trigger initialised with on = '%s', off = '%s', dest = '%s'.\nChosen links will%s be deleted.", messages[1].c_str(), messages[0].c_str(), dest_str.c_str(), (remove_links ? "" : " not"));
            debug_printf(DL_DEBUG, "On is%s a stimulus", (isstim[1] ? "" : " not"));
            if(isstim[1]) debug_printf(DL_DEBUG, "    Stim object: %d, Intensity: %.3f", stimob[1], intensity[1]);

            debug_printf(DL_DEBUG, "Off is%s a stimulus", (isstim[0] ? "" : " not"));
            if(isstim[0]) debug_printf(DL_DEBUG, "    Stim object: %d, Intensity: %.3f", stimob[0], intensity[0]);
        }

        fail_chance = get_scriptparam_int(design_note, "FailChance", 0);
        if(debug_enabled())
            debug_printf(DL_DEBUG, "Chance of failure is %d%%%s", fail_chance, (fail_chance ? "" : " (will always trigger)"));

        // Now for use limiting.
        int value, falloff;
        get_scriptparam_valuefalloff(design_note, "Count", &value, &falloff);
        count.init(time, 0, value, falloff);

        // Handle modes
        count_mode = get_scriptparam_countmode(design_note, "CountOnly");

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Count is %d%s with a falloff of %d milliseconds, count mode is %d", value, (value ? "" : " (no use limit)"), falloff, static_cast<int>(count_mode));

        g_pMalloc -> Free(design_note);
    }
}


/* ------------------------------------------------------------------------
 *  Miscellaneous - private functions
 */


bool TWBaseTrigger::check_stimulus_message(char *message, int *obj, float *intensity)
{
    char *end;

    // If the first character is not a '[', the message is not a stimulus
    if(*message != '[') return false;

    ++message;
    *intensity = strtof(message, &end);

    // If number parsing fails, the message is still not a stimulus.
    if(end == message) return false;

    // Something has been parsed out, so traverse the string looking for the ] or end
    while(*end && *end != ']') ++end;

    // Hit the end of string? Not something we can use, then
    if(!*end) return false;

    // Skip the ], and then check that we haven't hit the end of the string
    ++end;
    if(!*end) return false;

    // end now contains the name of an object, so try to locate it
    SInterface<IObjectSystem> ObjectSys(g_pScriptManager);
    *obj = ObjectSys -> GetObjectNamed(end);

    // The stimulus must be a negative (ie: a stimulus archetype)
    if(*obj >= 0) return false;

    // Okay, looks like an archetype has been found, so this is a stim message
    return true;
}


/* ------------------------------------------------------------------------
 *  Message handling - private functions
 */

bool TWBaseTrigger::send_trigger_message(bool send_on, sScrMsg* msg)
{
    std::vector<TargetObj>* targets = NULL;

    if(debug_enabled())
        debug_printf(DL_DEBUG, "Doing %s trigger", (send_on ? "On" : "Off"));

    // Do failure checking; should be done before count checking as failed
    // firings should not be counted
    if(fail_chance && (uni_dist(randomiser) > fail_chance)) return false;

    CountMode mode = (send_on ? CM_TURNON : CM_TURNOFF);
    if(count.increment(msg -> time, (count_mode & mode) ? 1 : 0)) {
        targets = get_target_objects(dest_str.c_str(), msg);

        if(!targets -> empty()) {
            std::vector<TargetObj>::iterator it;
            SService<IActReactSrv> ar_srv(g_pScriptManager);

            // Convert the bool to an index into the various arrays
            int send = (send_on ? 1 : 0);

            for(it = targets -> begin(); it != targets -> end(); it++) {
                // If sending a stim instead of a message, do that...
                if(isstim[send]) {
                    ar_srv -> Stimulate(it -> obj_id, stimob[send], intensity[send], ObjId());

                // otherwise, send the message to the target
                } else {
                    post_message(it -> obj_id, messages[send].c_str());

                    // TODO: Handle link delete
                }
            }
        }

        // No longer need the target list
        delete targets;

        // Indicate messages have been sent
        return true;
    }

    return false;
}
