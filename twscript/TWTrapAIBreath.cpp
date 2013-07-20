#include "TWTrapAIBreath.h"
#include "ScriptLib.h"

/* =============================================================================
 *  TWTrapAIBreath Impmementation - protected members
 */

void TWTrapAIBreath::init(int time)
{
    TWBaseTrap::init(time);

    // Set up persisitent variables
//    in_cold.Init();
//    breath_timer.Init();
    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    if(!design_note) {
        debug_printf(DL_WARNING, "No Editor -> Design Note. Falling back on defaults.");
    } else {
        // Should the AI start off in the cold?
        in_cold = static_cast<int>(get_scriptparam_bool(design_note, "InCold", false));

        // Should the breath cloud stop immediately on entering the warm?
        stop_immediately = get_scriptparam_bool(design_note, "Immediate", true);

        // How long, in milliseconds, should the exhale last
        exhale_time = get_scriptparam_int(design_note, "ExhaleTime", 250);

        // Sort out the archetype for the particle
        char *sfx_name = get_scriptparam_string(design_note, "SFX", "AIBreath");
        if(sfx_name) {
            particle_arch_name = sfx_name;
            g_pMalloc -> Free(sfx_name);
        }

        g_pMalloc -> Free(design_note);
    }

    // Now work out the base rate
    SService<IPropertySrv> PropertySrv(g_pScriptManager);
    if(PropertySrv -> Possessed(ObjId(), "CfgTweqBlink")) {
        cMultiParm rate_param;

        PropertySrv -> Get(rate_param, ObjId(), "CfgTweqBlink", "Rate");
        base_rate = static_cast<int>(rate_param);
    } else {
        debug_printf(DL_WARNING, "CfgTweqBlink missing on object. Script will not function!");
    }

    // Make sure a base rate is set, just in case.
    if(!base_rate) {
        debug_printf(DL_WARNING, "No base rate set, falling back on default");
        base_rate = 2000;
    }

    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Initialised on object. Settings:");
        debug_printf(DL_DEBUG, "In cold: %s, stop breath immediately: %s", in_cold ? "yes" : "no", stop_immediately ? "yes" : "no");
        debug_printf(DL_DEBUG, "Exhale time: %dms, Rest rate: %dms", exhale_time, base_rate);
        debug_printf(DL_DEBUG, "SFX name: %s", particle_arch_name.c_str());
    }
}


TWBaseScript::MsgStatus TWTrapAIBreath::on_message(sScrMsg* msg, cMultiParm& reply)
{
    debug_printf(DL_DEBUG, "in on_message(%s): Debug is %d", msg -> message, debug_enabled());

    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseTrap::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    if(!::_stricmp(msg -> message, "Sim")) {
        if(static_cast<sSimMsg*>(msg) -> fStarting) init(msg -> time);

    } else if(!::_stricmp(msg -> message, "Timer")) {
        return stop_breath(static_cast<sScrTimerMsg*>(msg), reply);

    } else if(!::_stricmp(msg -> message, "TweqComplete")) {
        return start_breath(static_cast<sTweqMsg*>(msg), reply);

    } else if(!::_stricmp(msg -> message, "Alertness")) {
        return set_rate(static_cast<sAIAlertnessMsg*>(msg), reply);

    }

    // TODO: Handle death. Check knockout.
    debug_printf(DL_DEBUG, "out on_message(%s): Debug is %d", msg -> message, debug_enabled());
        debug_printf(DL_DEBUG, "In cold: %s, stop breath immediately: %s", in_cold ? "yes" : "no", stop_immediately ? "yes" : "no");
        debug_printf(DL_DEBUG, "Exhale time: %dms, Rest rate: %dms", exhale_time, base_rate);
        debug_printf(DL_DEBUG, "SFX name: %s", particle_arch_name.c_str());

    return result;
}


TWBaseScript::MsgStatus TWTrapAIBreath::on_onmsg(sScrMsg* msg, cMultiParm& reply)
{
    //if(debug_enabled())
        debug_printf(DL_DEBUG, "Received breath on message");

    // Mark the AI as being in the cold. The next TweqComplete should make the
    // breath puff fire
    in_cold = 1;

    return MS_CONTINUE;
}


TWBaseScript::MsgStatus TWTrapAIBreath::on_offmsg(sScrMsg* msg, cMultiParm& reply)
{
    SService<IPGroupSrv> SFXSrv(g_pScriptManager);

    //if(debug_enabled())
        debug_printf(DL_DEBUG, "Received breath off message");

    // Mark the AI as being in the warm. The timer will stop the breath puff at
    // the next firing.
    in_cold = 0;

    // Halt breath particle immediately on entering the warm?
    if(stop_immediately) {

        // Abort firing of the timed message
        if(breath_timer) {
            cancel_timed_message(breath_timer);
            breath_timer = NULL;
        }

        // Deactivate the particle group
        int breath_particles = get_breath_particles();
        if(breath_particles) SFXSrv -> SetActive(breath_particles, false);
    }

    return MS_CONTINUE;
}


/* =============================================================================
 *  TWTrapAIBreath Impmementation - private members
 */

TWBaseScript::MsgStatus TWTrapAIBreath::start_breath(sTweqMsg *msg, cMultiParm& reply)
{
    SService<IPropertySrv> PropertySrv(g_pScriptManager);
    SService<IPGroupSrv>   SFXSrv(g_pScriptManager);

//    if(debug_enabled())
        debug_printf(DL_DEBUG, "Starting breathe out");

    // Only process flicker complete messages, and only actually do anything at all
    // if the object is in the cold.
    if(in_cold && msg -> Type == kTweqTypeFlicker && msg -> Op == kTweqOpFrameEvent) {
        debug_printf(DL_DEBUG, "Doing breathe out");
        int turn_off = exhale_time;

        cMultiParm rate_param;
        PropertySrv -> Get(rate_param, ObjId(), "CfgTweqBlink", "Rate");
        int rate = static_cast<int>(rate_param);

        // Limit exhale time to at most half the breath rate
        if((turn_off * 2) > rate) {
            turn_off = rate / 2;
        }

        // Now get the particles going if possible
        int breath_particles = get_breath_particles();
        if(breath_particles) {
            SFXSrv -> SetActive(breath_particles, true);

            // This shouldn't be needed, but check anyway
            if(breath_timer) {
                cancel_timed_message(breath_timer);
            }

            // And queue up the message to turn the breath off
            breath_timer = set_timed_message("StopBreath", turn_off, kSTM_OneShot);
        } else {//if(debug_enabled()) {
            debug_printf(DL_WARNING, "Unable to obtain breath SFX object.");
        }
    }

    return MS_CONTINUE;
}


TWBaseScript::MsgStatus TWTrapAIBreath::stop_breath(sScrTimerMsg *msg, cMultiParm& reply)
{
    // Only bother doing anything if the timer name is correct.
    if(!::_stricmp(msg -> name, "StopBreath")) {
        SService<IPGroupSrv> SFXSrv(g_pScriptManager);

        breath_timer = NULL;

        int breath_particles = get_breath_particles();
        if(breath_particles) SFXSrv -> SetActive(breath_particles, false);
    }

    return MS_CONTINUE;
}


TWBaseScript::MsgStatus TWTrapAIBreath::set_rate(sAIAlertnessMsg *msg, cMultiParm& reply)
{
    SService<IPropertySrv> PropertySrv(g_pScriptManager);

    // If the AI has its tweq set up, update the rate. Awareness levels are listed in
    // lg/defs.h eAIScriptAlertLevel, with the lowest level at 0 (kNoAlert) and highest
    // at 3 (kHighAlert). As the level will be used as a divisor, it must be 1 to 4.
    int new_level = msg -> level + 1;
    if(new_level && PropertySrv -> Possessed(ObjId(), "CfgTweqBlink")) {

        // POSSIBLE EXPANSION: instead of doing a basic divisor thing here, possibly
        // allow the user to specify scaling values per alerness level?
        int new_rate = base_rate / new_level;

        PropertySrv -> Set(ObjId(), "CfgTweqBlink", "Rate", new_rate);
        PropertySrv -> Set(ObjId(), "StTweqBlink", "Cur Time", new_rate - 1);
    }

    return MS_CONTINUE;
}


int TWTrapAIBreath::get_breath_particles()
{
    SInterface<IObjectSystem> ObjectSys(g_pScriptManager);
    SService<IObjectSrv>      ObjectSrv(g_pScriptManager);
    SService<ILinkSrv>        LinkSrv(g_pScriptManager);
    SService<ILinkToolsSrv>   LinkToolsSrv(g_pScriptManager);

    // Can't do anything if there is no archytype name set
    if(!particle_arch_name.empty()) {

        // Attempt to locate the archetype requested
        object archetype = ObjectSys -> GetObjectNamed(particle_arch_name.c_str());
        if(ObjectSys -> Exists(archetype)) {

            // Is there a particle attach(e)ment to this object?
            true_bool has_attach;
            LinkSrv -> AnyExist(has_attach, LinkToolsSrv -> LinkKindNamed("~ParticleAttachement"), ObjId(), 0);

            // Only do anything if there is at least one particle attachment.
            if(has_attach) {
                linkset links;
                true_bool inherits;
                debug_printf(DL_DEBUG, "Has attachment, looking for correct sfx");

                // Check all the particle attachment links looking for a link from a particle that inherits
                // from the archetype.
                LinkSrv -> GetAll(links, LinkToolsSrv -> LinkKindNamed("~ParticleAttachement"), ObjId(), 0);
                while(links.AnyLinksLeft()) {
                    sLink link = links.Get();

                    debug_printf(DL_DEBUG, "Checking %d against %d", link.dest, archetype);
                    ObjectSrv -> InheritsFrom(inherits, link.dest, archetype);

                    // Found a link from a concrete instance of the archetype? Return that object.
                    if(inherits) {
                        return link.dest;
                    }
                    links.NextLink();
                }
            }
        } else { //if(debug_enabled()) {
            debug_printf(DL_WARNING, "Unable to find particle named '%s'", particle_arch_name.c_str());
        }
    } else {
        debug_printf(DL_WARNING, "particle_arch_name name is empty?!");
    }

    return 0;
}
