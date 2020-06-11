
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

    if(!::_stricmp(msg -> message, turnon_msg.c_str())) {
        if(debug_enabled())
            debug_printf(DL_DEBUG, "Received TurnOn");

        if(count.increment(msg -> time, (count_mode & CM_TURNON) ? 1 : 0)) {
            if(on_capacitor.increment(msg -> time)) {
                return on_onmsg(msg, reply);
            } else if(debug_enabled()) {
                debug_printf(DL_DEBUG, "TurnOn suppressed by on capacitor");
            }
        } else if(debug_enabled()) {
            debug_printf(DL_DEBUG, "TurnOn suppressed - count limit reached");
        }

        // Get here and one of the counters returned false, so halt further processing.
        return MS_HALT;

    } else if(!::_stricmp(msg -> message, turnoff_msg.c_str())) {

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Received TurnOff");

        if(count.increment(msg -> time, (count_mode & CM_TURNOFF) ? 1 : 0)) {
            if(off_capacitor.increment(msg -> time)) {
                return on_offmsg(msg, reply);
            } else if(debug_enabled()) {
                debug_printf(DL_DEBUG, "TurnOff suppressed by off capacitor");
            }
        } else if(debug_enabled()) {
            debug_printf(DL_DEBUG, "TurnOff suppressed - count limit reached");
        }

        // Get here and one of the counters returned false, so halt further processing.
        return MS_HALT;

    } else if(!::_stricmp(msg -> message, "ResetCount")) {
        count.reset(msg -> time);

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Use count reset to 0");
    }

    return MS_CONTINUE;
}


/* ------------------------------------------------------------------------
 *  Initialisation related
 */

void TWBaseTrap::init(int time)
{
    TWBaseScript::init(time);
    char *msg;
    char *design_note = GetObjectParams(ObjId());

    if(design_note) {
        // Work out what the turnon and turnoff messages should be
        turnon_msg.init(design_note, "TurnOn");
        turnoff_msg.init(design_note, "TurnOff");

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Trap initialised with on = '%s', off = '%s'", turnon_msg.c_str(), turnoff_msg.c_str());

        // Now for use limiting.
        int value, falloff;
        bool limit;
        get_scriptparam_valuefalloff(design_note, "Count", &value, &falloff, &limit);
        count.init(time, 0, value, falloff, false, limit);

        // Handle modes
        count_mode = get_scriptparam_countmode(design_note, "CountOnly");

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Count is %d%s with a falloff of %d milliseconds, count mode is %d", value, (value ? "" : " (no use limit)"), falloff, static_cast<int>(count_mode));

        // Now deal with capacitors
        get_scriptparam_valuefalloff(design_note, "OnCapacitor", &value, &falloff);
        on_capacitor.init(time, value, 0, falloff, true);

        if(debug_enabled())
            debug_printf(DL_DEBUG, "OnCapacitor is %d%s with a falloff of %d milliseconds", value, (value > 1 ? "" : " (every turnon fires)"), falloff);

        get_scriptparam_valuefalloff(design_note, "OffCapacitor", &value, &falloff);
        off_capacitor.init(time, value, 0, falloff, true);

        if(debug_enabled())
            debug_printf(DL_DEBUG, "OffCapacitor is %d%s with a falloff of %d milliseconds", value, (value > 1 ? "" : " (every turnoff fires)"), falloff);

        g_pMalloc -> Free(design_note);
    }
}
