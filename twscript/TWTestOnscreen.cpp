#include "TWTestOnscreen.h"
#include "ScriptLib.h"

void TWTestOnscreen::init(int time)
{
    TWBaseScript::init(time);

    // And schedule the next update.
    update_timer = set_timed_message("CheckOnScreen", 500, kSTM_OneShot);
}


TWBaseScript::MsgStatus TWTestOnscreen::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseScript::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    if(!::_stricmp(msg -> message, "Timer")) {
        return on_timer(static_cast<sScrTimerMsg*>(msg), reply);
    }

    return result;
}


TWBaseScript::MsgStatus TWTestOnscreen::on_timer(sScrTimerMsg *msg, cMultiParm& reply)
{
    // Only bother doing anything if the timer name is correct.
    if(!::_stricmp(msg -> name, "CheckOnScreen")) {
        SService<IObjectSrv> obj_srv(g_pScriptManager);
        SService<IPropertySrv> prop_srv(g_pScriptManager);

        // Check what the render type is
        if(prop_srv -> Possessed(ObjId(), "RenderType")) {
            cMultiParm prop;
            prop_srv -> Get(prop, ObjId(), "RenderType", NULL);

            debug_printf(DL_DEBUG, "Render Type: %d", static_cast<int>(prop));

        } else {
            debug_printf(DL_WARNING, "No Render Type property");
        }

        true_bool onscreen;
        obj_srv -> RenderedThisFrame(onscreen, ObjId());
        if(onscreen) {
            debug_printf(DL_DEBUG, "On screen");
        } else {
            debug_printf(DL_DEBUG, "Off screen");
        }

        update_timer = set_timed_message("CheckOnScreen", 500, kSTM_OneShot);
    }

    return MS_CONTINUE;
}
