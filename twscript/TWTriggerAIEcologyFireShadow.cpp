#include "TWTriggerAIEcologyFireShadow.h"
#include "ScriptLib.h"

/* =============================================================================
 *  TWTriggerAIEcologyFireShadow Impmementation - protected members
 */

/* ------------------------------------------------------------------------
 *  Initialisation related
 */

void TWTriggerAIEcologyFireShadow::init(int time)
{
    TWBaseTrigger::init(time);

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    if(!design_note) {
        debug_printf(DL_WARNING, "No Editor -> Design Note. Falling back on defaults.");

    } else {
        // How often should the fireshadow update?
        refresh.init(design_note, 1000);

        // parse the timewarp settings
        speed_factor.init(design_note, 0.8125);
        min_timewarp.init(design_note, 0.03);

        g_pMalloc -> Free(design_note);
    }

    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Initialised on object. Settings:");
        debug_printf(DL_DEBUG, "Speedup rate %d", refresh.value());
    }
}


/* ------------------------------------------------------------------------
 *  Message handling
 */

TWBaseScript::MsgStatus TWTriggerAIEcologyFireShadow::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseTrigger::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    if(!::_stricmp(msg -> message, "Timer")) {
        return on_timer(static_cast<sScrTimerMsg*>(msg), reply);

    } else if(!::_stricmp(msg -> message, "Slain")) {
        return on_slain(static_cast<sSlayMsg*>(msg), reply);

    }

    return result;
}


TWBaseScript::MsgStatus TWTriggerAIEcologyFireShadow::on_timer(sScrTimerMsg* msg, cMultiParm& reply)
{
    if(!::_stricmp(msg -> name, "FireShadow")) {
        speedup();

        if(!attempt_despawn(msg)) {
            if(debug_enabled())
                debug_printf(DL_DEBUG, "Re-setting timed despawn");

            update_timer = set_timed_message("FireShadow", refresh.value(), kSTM_OneShot);
        }
    }

    return MS_CONTINUE;

}

TWBaseScript::MsgStatus TWTriggerAIEcologyFireShadow::on_slain(sSlayMsg* msg, cMultiParm& reply)
{
    if(debug_enabled())
        debug_printf(DL_DEBUG, "AI slain, setting up slain behaviour");

    if(update_timer) {
        cancel_timed_message(update_timer);
    }
    update_timer = set_timed_message("FireShadow", refresh.value(), kSTM_OneShot);

    fireshadow_flee();

    return MS_CONTINUE;
}


bool TWTriggerAIEcologyFireShadow::attempt_despawn(sScrMsg *msg)
{
    if(debug_enabled())
        debug_printf(DL_DEBUG, "Attempting despawn of AI");

    fireshadow_flee();

    true_bool onscreen;
    SService<IObjectSrv> obj_srv(g_pScriptManager);

    // If the AI is visible, it can't be despawned
    obj_srv -> RenderedThisFrame(onscreen, ObjId());
    if(!onscreen) {
        if(debug_enabled())
            debug_printf(DL_DEBUG, "AI is offscreen, despawning");

        // Try to locate the ecology that controls this AI
        int ecology = GetObjectParamInt(ObjId(), "EcologyID", 0);
        if(ecology) {
            if(debug_enabled())
                debug_printf(DL_DEBUG, "Sending 'Despawned' message to ecology %d", ecology);

            // Tell the ecology that the AI is despawned.
            post_message(ecology, "Despawned", ObjId());
        } else if(debug_enabled()) {
            debug_printf(DL_WARNING, "Unable to find ecology ID to notify about despawn");
        }

        // Send any on messages needed
        send_on_message(msg);

        // And get rid of the AI
        obj_srv -> Destroy(ObjId());

        return true;
    } else if(debug_enabled()) {
        debug_printf(DL_DEBUG, "AI is visible, despawn failed this time");
    }

    return false;
}


void TWTriggerAIEcologyFireShadow::fire_corseparts(void)
{
    SService<IPhysSrv>      phys_srv(g_pScriptManager);
    SService<ILinkSrv>      link_srv(g_pScriptManager);
    SService<ILinkToolsSrv> link_tools(g_pScriptManager);
    linkset links;

    link_srv -> GetAllInheritedSingle(links, link_tools -> LinkKindNamed("CorpsePart"), ObjId(), 0);
    for(; links.AnyLinksLeft(); links.NextLink()) {
        sLink link = links.Get();
        object fired;
        phys_srv -> LaunchProjectile(fired, ObjId(), link.dest, 0, 10, cScrVec::Zero);
    }
}


void TWTriggerAIEcologyFireShadow::fireshadow_flee(void)
{
    SService<IObjectSrv>   obj_srv(g_pScriptManager);
    SService<IPropertySrv> prop_srv(g_pScriptManager);

    object metaprop;
    obj_srv -> Named(metaprop, "M-FireShadowFlee");
    if(metaprop) {
        true_bool has_prop;

        obj_srv -> HasMetaProperty(has_prop, ObjId(), metaprop);
        if(!has_prop) {
            obj_srv -> AddMetaProperty(ObjId(), metaprop);

            if(debug_enabled())
                debug_printf(DL_DEBUG, "Added M-FireShadowFlee to AI %d", ObjId());

            fire_corseparts();

            prop_srv -> Add(ObjId(), "TimeWarp");
            prop_srv -> SetSimple(ObjId(), "TimeWarp", speed_factor.value());
        }
    }
}


void TWTriggerAIEcologyFireShadow::speedup(void)
{
    SService<IObjectSrv>   obj_srv(g_pScriptManager);
    SService<IPropertySrv> prop_srv(g_pScriptManager);

    cMultiParm timewarp;
    prop_srv -> Get(timewarp, ObjId(), "TimeWarp", NULL);

    timewarp = float(timewarp) * speed_factor.value();
    if(float(timewarp) < min_timewarp.value()) timewarp = min_timewarp.value();

    prop_srv -> SetSimple(ObjId(), "TimeWarp", timewarp);
}
