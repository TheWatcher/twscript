
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
        if((msg = get_scriptparam_string(design_note, "TOn", "TurnOn")) != NULL) {
            turnon_msg = msg;
            g_pMalloc -> Free(msg);
        }

        if((msg = get_scriptparam_string(design_note, "TOff", "TurnOff")) != NULL) {
            turnoff_msg = msg;
            g_pMalloc -> Free(msg);
        }

        if((msg = get_scriptparam_string(design_note, "TDest", "&ControlDevice")) != NULL) {
            dest_str = msg;
            g_pMalloc -> Free(msg);
        }

        remove_links = get_scriptparam_bool(design_note, "KillLinks");

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Trigger initialised with on = '%s', off = '%s', dest = '%s'.\nChosen links will%s be deleted.", turnon_msg.c_str(), turnoff_msg.c_str(), dest_str.c_str(), (remove_links ? "" : " not");

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
 *  Message handling - private functions
 */

bool TWBaseTrigger::send_trigger_message(bool send_on)
{
    std::vector<TargetObj>* targets = NULL;




}
