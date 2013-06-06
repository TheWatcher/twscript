
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

    // Work out what the turnon and turnoff messages should be
    if((msg = get_scriptparam_string(design_note, "On", "TurnOn")) != NULL) {
        turnon_msg = msg;
        g_pMalloc -> Free(msg);
    }

    if((msg = get_scriptparam_string(design_note, "Off", "TurnOff")) != NULL) {
        turnoff_msg = msg;
        g_pMalloc -> Free(msg);
    }

    // Now for use limiting.
    int value, falloff;
    get_scriptparam_valuefalloff(design_note, "Count", &value, &falloff);
    count.init(time, 0, value, falloff);

    // Handle modes
    count_mode = get_scriptparam_countmode(design_note, "CountOnly");

    // Now deal with capacitors
    get_scriptparam_valuefalloff(design_note, "OnCapacitor", &value, &falloff);
    on_capacitor.init(time, value, 0, falloff);

    get_scriptparam_valuefalloff(design_note, "OffCapacitor", &value, &falloff);
    off_capacitor.init(time, value, 0, falloff);
}
