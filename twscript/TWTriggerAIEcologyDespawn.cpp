#include "TWTriggerAIEcologyDespawn.h"
#include "ScriptLib.h"

/* =============================================================================
 *  TWTriggerAIEcologyDespawn Impmementation - protected members
 */

/* ------------------------------------------------------------------------
 *  Initialisation related
 */

void TWTriggerAIEcologyDespawn::init(int time)
{
    TWBaseTrigger::init(time);

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    if(!design_note) {
        debug_printf(DL_WARNING, "No Editor -> Design Note. Falling back on defaults.");

    } else {
        // How often should the ecology update?
        refresh  = get_scriptparam_int(design_note, "Rate", 20000);

        g_pMalloc -> Free(design_note);
    }

    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Initialised on object. Settings:");
        debug_printf(DL_DEBUG, "Despawn rate %d", refresh);
    }
}


/* ------------------------------------------------------------------------
 *  Message handling
 */

TWBaseScript::MsgStatus TWTriggerAIEcologyDespawn::on_message(sScrMsg* msg, cMultiParm& reply)
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


TWBaseScript::MsgStatus TWTriggerAIEcologyDespawn::on_timer(sScrTimerMsg* msg, cMultiParm& reply)
{
    if(!::_stricmp(msg -> name, "Despawn")) {
        if(!attempt_despawn(msg)) {
            if(debug_enabled())
                debug_printf(DL_DEBUG, "Re-setting timed despawn");

            update_timer = set_timed_message("Despawn", refresh, kSTM_OneShot);
        }
    }

    return MS_CONTINUE;

}

TWBaseScript::MsgStatus TWTriggerAIEcologyDespawn::on_slain(sSlayMsg* msg, cMultiParm& reply)
{
    if(debug_enabled())
        debug_printf(DL_DEBUG, "AI slain, setting timed despawn");

    if(update_timer) {
        cancel_timed_message(update_timer);
    }
    update_timer = set_timed_message("Despawn", refresh, kSTM_OneShot);

    return MS_CONTINUE;
}


bool TWTriggerAIEcologyDespawn::attempt_despawn(sScrMsg *msg)
{
    if(debug_enabled())
        debug_printf(DL_DEBUG, "Attempting despawn of AI");

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
