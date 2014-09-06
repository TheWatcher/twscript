#include "TWTriggerAIAware.h"
#include "ScriptLib.h"

/* =============================================================================
 *  TWTriggerAIAware Impmementation - protected members
 */

/* ------------------------------------------------------------------------
 *  Initialisation related
 */

void TWTriggerAIAware::init(int time)
{
    TWBaseTrigger::init(time);

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    is_linked.Init(0);

    if(!design_note) {
        debug_printf(DL_WARNING, "No Editor -> Design Note. Falling back on defaults.");

    } else {
        std::string dummy;

        refresh = get_scriptparam_int(design_note, "Rate", 500, dummy);
        trigger_level = (eAIScriptAlertLevel)get_scriptparam_int(design_note, "Alertness" , 2, dummy);

        char *objname = get_scriptparam_string(design_note, "Object", "Garrett");
        if(objname) {
            SService<IObjectSrv>  obj_srv(g_pScriptManager);

            obj_srv -> Named(trigger_object, objname);
            if(!trigger_object) {
                debug_printf(DL_WARNING, "Unable to locate object named '%s'. Using default Garrett", objname);
                obj_srv -> Named(trigger_object, "Garrett");
            }

            g_pMalloc -> Free(objname);
        }

        g_pMalloc -> Free(design_note);
    }

    if(debug_enabled()) {
        std::string targ_name;
        get_object_namestr(targ_name, trigger_object);

        debug_printf(DL_DEBUG, "Initialised trigger level %d, match object '%s', check rate %d", trigger_level, targ_name.c_str(), refresh);
    }
}

/* ------------------------------------------------------------------------
 *  Message handling
 */

TWBaseScript::MsgStatus TWTriggerAIAware::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseTrigger::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    if(!::_stricmp(msg -> message, "Alertness")) {
        return on_alertness(static_cast<sAIAlertnessMsg*>(msg), reply);
    } else if(!::_stricmp(msg -> message, "Timer")) {
        return on_timer(static_cast<sScrTimerMsg*>(msg), reply);

    // Make sure death and knockout stops the check
    } else if(!::_stricmp(msg -> message, "Slain")) {
        return on_slain(static_cast<sSlayMsg*>(msg), reply);
    } else if(!::_stricmp(msg -> message, "IgnorePotion")) {
        return on_ignorepotion(msg, reply);
    }

    return result;
}


TWBaseScript::MsgStatus TWTriggerAIAware::on_alertness(sAIAlertnessMsg* msg, cMultiParm& reply)
{
    if(debug_enabled())
        debug_printf(DL_DEBUG, "Alertness change. New: %d, Old: %d\n", msg -> level, msg -> oldLevel);

    // Is the alertness going over the trigger level?
    if(msg -> level >= trigger_level && msg -> oldLevel < trigger_level) {
        if(debug_enabled())
            debug_printf(DL_DEBUG, "Alertness raised above trigger, starting link checks");

        check_awareness(msg);

    // Is the alertness going down below the trigger level?
    } else if(msg -> level < trigger_level && msg -> oldLevel >= trigger_level) {
        if(debug_enabled())
            debug_printf(DL_DEBUG, "Alertness fell below trigger, stopping link checks");

        stop_timer();

        // If the target has been linked during the high awareness, send the off message now.
        if(int(is_linked)) {
            is_linked = 0;
            send_off_message(msg);
        }
    }

    return MS_CONTINUE;
}


TWBaseScript::MsgStatus TWTriggerAIAware::on_timer(sScrTimerMsg* msg, cMultiParm& reply)
{
    // Only bother doing anything if the timer name is correct.
    if(!::_stricmp(msg -> name, "CheckLinks")) {
        check_awareness(msg);
    }

    return MS_CONTINUE;
}


TWBaseScript::MsgStatus TWTriggerAIAware::on_ignorepotion(sScrMsg* msg, cMultiParm& reply)
{
    if(debug_enabled())
        debug_printf(DL_DEBUG, "AI knocked out, halting link checks");

    stop_timer();

    // If the target has been linked during the high awareness, send the off message now.
    if(int(is_linked)) {
        is_linked = 0;
        send_off_message(msg);
    }
    return MS_CONTINUE;
}


TWBaseScript::MsgStatus TWTriggerAIAware::on_slain(sSlayMsg* msg, cMultiParm& reply)
{
    if(debug_enabled())
        debug_printf(DL_DEBUG, "AI slain, halting link checks");

    stop_timer();

    // If the target has been linked during the high awareness, send the off message now.
    if(int(is_linked)) {
        is_linked = 0;
        send_off_message(msg);
    }
    return MS_CONTINUE;
}


void TWTriggerAIAware::check_awareness(sScrMsg* msg)
{
    SService<IObjectSrv>     obj_srv(g_pScriptManager);
    SService<ILinkSrv>       link_srv(g_pScriptManager);
    SInterface<ILinkManager> link_mgr(g_pScriptManager);
    SService<ILinkToolsSrv>  link_tools(g_pScriptManager);

    bool target_linked = false;

    stop_timer(); // most of the time this is redundant, but be sure.

    linkset links;
    link_srv -> GetAll(links, link_tools -> LinkKindNamed("AIAwareness"), ObjId(), 0);
    for(; !target_linked && links.AnyLinksLeft(); links.NextLink()) {
		sLink link = links.Get();

        // Ignore links to objects with too low level
        sAIAwareness* awareness = static_cast<sAIAwareness*>(links.Data());
        if(awareness -> Level >= static_cast<eAIAwareLevel>(trigger_level)) {

            if(int(trigger_object) > 0 && link.dest == trigger_object) {
                target_linked = true;
            } else if(int(trigger_object) < 0) {
                true_bool inherits;
                obj_srv -> InheritsFrom(inherits, link.dest, trigger_object);

                target_linked = (bool)inherits;
            }
        }
    }

    if(debug_enabled())
        debug_printf(DL_DEBUG, "Target linked is %s", target_linked ? "true" : "false");

    if(target_linked && !int(is_linked)) {
        is_linked = 1;
        send_on_message(msg);
    } else if(!target_linked && int(is_linked)) {
        is_linked = 0;
        send_off_message(msg);
    }

    update_timer = set_timed_message("CheckLinks", refresh, kSTM_OneShot);
}


void TWTriggerAIAware::stop_timer(void)
{
    if(update_timer) {
        cancel_timed_message(update_timer);
        update_timer.Clear();
    }
}
