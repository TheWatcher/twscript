#include "TWTriggerAIEcologySlain.h"
#include "ScriptLib.h"

/* =============================================================================
 *  TWTriggerAIEcologySlain Impmementation - protected members
 */

/* ------------------------------------------------------------------------
 *  Initialisation related
 */

void TWTriggerAIEcologySlain::init(int time)
{
    TWBaseTrigger::init(time);

    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Initialised on object. Settings:");
    }
}


/* ------------------------------------------------------------------------
 *  Message handling
 */

TWBaseScript::MsgStatus TWTriggerAIEcologySlain::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseTrigger::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    if(!::_stricmp(msg -> message, "Slain")) {
        return on_slain(static_cast<sSlayMsg*>(msg), reply);
    }

    return result;
}


TWBaseScript::MsgStatus TWTriggerAIEcologySlain::on_slain(sSlayMsg* msg, cMultiParm& reply)
{
    if(debug_enabled())
        debug_printf(DL_DEBUG, "AI slain, Informing ecology");

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

    return MS_CONTINUE;
}
