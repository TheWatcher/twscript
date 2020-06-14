#include <lg/propdefs.h>
#include <lg/iids.h>
#include "TWTriggerVisible.h"
#include "ScriptLib.h"

/* =============================================================================
 *  TWTriggerVisible Implementation - protected members
 */

void TWTriggerVisible::init(int time)
{
    TWBaseTrigger::init(time);

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    if(!design_note) {
        debug_printf(DL_WARNING, "No Editor -> Design Note. Falling back on defaults.");

        lowlight_threshold.init("", 35);
        highlight_threshold.init("", 55);
        refresh.init("", 500);

    } else {
        std::string dummy;

        lowlight_threshold.init(design_note, 35);
        highlight_threshold.init(design_note, 55);
        refresh.init(design_note, 500);

        g_pMalloc -> Free(design_note);
    }

    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Initialised with low theshold: %d, high threshold: %d", lowlight_threshold.value(), highlight_threshold.value());
        debug_printf(DL_DEBUG, "Update rate set to %dms", refresh.value());
    }

    if(update_timer) {
        cancel_timed_message(update_timer);
    }

    // And schedule the next update.
    update_timer = set_timed_message("CheckVis", refresh, kSTM_OneShot);
}


TWBaseScript::MsgStatus TWTriggerVisible::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseScript::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    if(!::_stricmp(msg -> message, "Timer")) {
        return on_timer(static_cast<sScrTimerMsg*>(msg), reply);
    }

    return result;
}


TWBaseScript::MsgStatus TWTriggerVisible::on_timer(sScrTimerMsg *msg, cMultiParm& reply)
{
    // Only bother doing anything if the timer name is correct.
    if(!::_stricmp(msg -> name, "CheckVis")) {
        check_visible(msg);

        update_timer = set_timed_message("CheckVis", refresh, kSTM_OneShot);
    }

    return MS_CONTINUE;
}


void TWTriggerVisible::check_visible(sScrMsg* msg)
{
    SService<IPropertySrv> prop_serv(g_pScriptManager);
    if(prop_serv -> Possessed(ObjId(), "AI_Visibility")) {
        cMultiParm light;
        prop_serv -> Get(light, ObjId(), "AI_Visibility", "Light rating");

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Light: %d", int(light));

        // Check whether the object has changed from light to dark or vice versa
        if(int(light) < lowlight_threshold.value() && int(is_litup)) {
            if(debug_enabled())
                debug_printf(DL_DEBUG, "Object is now invisible, sending off");

            is_litup = 0;
            send_off_message(msg);

        } else if(int(light) > highlight_threshold.value() && !int(is_litup)) {
            if(debug_enabled())
                debug_printf(DL_DEBUG, "Object is now visible, sending on");

            is_litup = 1;
            send_on_message(msg);
        }
    }
}
