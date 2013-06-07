
#include "TWBaseTrap.h"
#include "ScriptLib.h"


/* ------------------------------------------------------------------------
 *  Message handling
 */

TWBaseScript::MsgStatus TWBaseTrap::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseScript::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    // Handle setting up the script from the design note
    if(!::_stricmp(msg -> message, "BeginScript")) {
        init(msg -> time);
    }

    if(!::_stricmp(msg -> message, static_cast<const char *>(turnon_msg))) {

        if(count.increment(msg -> time, (count_mode & CM_TURNON) ? 1 : 0)) {
            if(on_capacitor.increment(msg -> time)) {
                return on_turnon(msg, reply);
            }
        }

        // Get here and one of the counters returned false, so halt further processing.
        return MS_HALT;

    } else if(!::_stricmp(msg -> message, static_cast<const char *>(turnoff_msg))) {

        if(count.increment(msg -> time, (count_mode & CM_TURNOFF) ? 1 : 0)) {
            if(off_capacitor.increment(msg -> time)) {
                return on_turnoff(msg, reply);
            }
        }

        // Get here and one of the counters returned false, so halt further processing.
        return MS_HALT;

    }

    return MS_CONTINUE;
}


/* ------------------------------------------------------------------------
 *  Initialisation related
 */

void TWBaseTrap::init(int time)
{
    char *msg;
    char *design_note = GetObjectParams(ObjId());

    if(design_note) {
        // Work out what the turnon and turnoff messages should be
        if((msg = get_scriptparam_string(design_note, "On", "TurnOn")) != NULL) {
            turnon_msg = msg;
            g_pMalloc -> Free(msg);
        }

        if((msg = get_scriptparam_string(design_note, "Off", "TurnOff")) != NULL) {
            turnoff_msg = msg;
            g_pMalloc -> Free(msg);
        }

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Trap initialised with on = '%s', off = '%s'", static_cast<const char *>(turnon_msg), static_cast<const char *>(turnoff_msg));

        // Now for use limiting.
        int value, falloff;
        get_scriptparam_valuefalloff(design_note, "Count", &value, &falloff);
        count.init(time, 0, value, falloff);

        // Handle modes
        count_mode = get_scriptparam_countmode(design_note, "CountOnly");

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Count is %d %s with a falloff of %d milliseconds, count mode is %d", value, (value ? "" : "(no use limit)"), falloff, static_cast<int>(count_mode));

        // Now deal with capacitors
        get_scriptparam_valuefalloff(design_note, "OnCapacitor", &value, &falloff);
        on_capacitor.init(time, value, 0, falloff);

        if(debug_enabled())
            debug_printf(DL_DEBUG, "OnCapacitor is %d %s with a falloff of %d milliseconds", value, (value > 1 ? "" : "(every turnon fires)"), falloff);

        get_scriptparam_valuefalloff(design_note, "OffCapacitor", &value, &falloff);
        off_capacitor.init(time, value, 0, falloff);

        if(debug_enabled())
            debug_printf(DL_DEBUG, "OffCapacitor is %d %s with a falloff of %d milliseconds", value, (value > 1 ? "" : "(every turnoff fires)"), falloff);

        g_pMalloc -> Free(design_note);
    }
}
