#include <cstring>
#include "TWTrapAIBreath.h"
#include "ScriptLib.h"


/* =============================================================================
 *  TWTrapAIBreath Implementation - protected members
 */

void TWTrapAIBreath::init(int time)
{
    TWBaseTrap::init(time);

    // AIs generally start off alive, or they wouldn't have this script on them!
    still_alive.Init(1);

    // Default the base rate
    rates[0] = 3000;

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    if(!design_note) {
        debug_printf(DL_WARNING, "No Editor -> Design Note. Falling back on defaults.");

        // Work out rates based on the base using a simple division
        for(int level = 1; level < 4; ++level) {
            rates[level] = rates[0] / level;
        }

    } else {
        std::string dummy;

        // Should the AI start off in the cold?
        if(!in_cold.Valid()) {
            in_cold = static_cast<int>(get_scriptparam_bool(design_note, "InCold", false));
        }

        // Should the breath cloud stop immediately on entering the warm?
        stop_immediately = get_scriptparam_bool(design_note, "Immediate", true);

        // Should the breath cloud stop when knocked out?
        stop_on_ko = get_scriptparam_bool(design_note, "StopOnKO", false);

        // How long, in milliseconds, should the exhale last
        exhale_time = get_scriptparam_time(design_note, "ExhaleTime", 250, dummy);

        // Allow the base rate to be overridden, and recalculated the defaults
        rates[0] = get_scriptparam_time(design_note, "Rate0", rates[0], dummy);
        for(int level = 1; level < 4; ++level) {
            rates[level] = rates[0] / level;
        }

        // Allow override of the default rates
        rates[1] = get_scriptparam_time(design_note, "Rate1", rates[1], dummy);
        rates[2] = get_scriptparam_time(design_note, "Rate2", rates[2], dummy);
        rates[3] = get_scriptparam_time(design_note, "Rate3", rates[3], dummy);

        // Sort out the archetype for the particle
        char *sfx_name = get_scriptparam_string(design_note, "SFX", "AIBreath");
        if(sfx_name) {
            particle_arch_name = sfx_name;
            g_pMalloc -> Free(sfx_name);
        }

        char *link_name = get_scriptparam_string(design_note, "LinkType", "~ParticleAttachement");
        if(link_name) {
            particle_link_name = link_name;
            g_pMalloc -> Free(link_name);
        }

        // Sort out the archetype for the proxy
        char *proxy_name = get_scriptparam_string(design_note, "Proxy", "BreathProxy");
        if(proxy_name) {
            proxy_arch_name = proxy_name;
            g_pMalloc -> Free(proxy_name);
        }

        char *proxy_link = get_scriptparam_string(design_note, "ProxyLink", "~DetailAttachement");
        if(proxy_link) {
            proxy_link_name = proxy_link;
            g_pMalloc -> Free(proxy_link);
        }

        char *cold = get_scriptparam_string(design_note, "ColdRooms", NULL);
        if(cold) {
            parse_coldrooms(cold);
            g_pMalloc -> Free(cold);
        }

        g_pMalloc -> Free(design_note);
    }

    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Initialised on object. Settings:");
        debug_printf(DL_DEBUG, "In cold: %s, stop breath immediately: %s", in_cold ? "yes" : "no", stop_immediately ? "yes" : "no");
        debug_printf(DL_DEBUG, "Exhale time: %dms", exhale_time);
        debug_printf(DL_DEBUG, "Breathing rates (in ms) None: %d, Low: %d, Medium: %d, High: %d", rates[0], rates[1], rates[2], rates[3]);
        debug_printf(DL_DEBUG, "SFX name: %s", particle_arch_name.c_str());
    }

    // Now update the breathing rate based on alertness
    SService<IAIScrSrv> AISrv(g_pScriptManager);
    int new_rate = AISrv -> GetAlertLevel(ObjId());

    // The rate gets reset to 0 if the AI is dead or unconscious
    int knockedout = StrToObject("M-KnockedOut");
    if(knockedout) {
        SService<IObjectSrv> ObjectSrv(g_pScriptManager);
        true_bool just_resting;
        ObjectSrv -> HasMetaProperty(just_resting, ObjId(), knockedout);

        if(just_resting) new_rate = 0;
    }

    if(!still_alive) new_rate = 0;

    set_rate(new_rate);
}


TWBaseScript::MsgStatus TWTrapAIBreath::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseTrap::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    if(!::_stricmp(msg -> message, "Timer")) {
        return stop_breath(static_cast<sScrTimerMsg*>(msg), reply);

    } else if(!::_stricmp(msg -> message, "TweqComplete")) {
        return start_breath(static_cast<sTweqMsg*>(msg), reply);

    } else if(!::_stricmp(msg -> message, "Alertness")) {
        return on_aialertness(static_cast<sAIAlertnessMsg*>(msg), reply);

    } else if(!::_stricmp(msg -> message, "ObjRoomTransit")) {
        return on_objroomtransit(static_cast<sRoomMsg*>(msg), reply);

    } else if(!::_stricmp(msg -> message, "AIModeChange")) {
        return on_aimodechange(static_cast<sAIModeChangeMsg*>(msg), reply);

    } else if(!::_stricmp(msg -> message, "Slain")) {
        return on_slain(msg, reply);

    } else if(!::_stricmp(msg -> message, "IgnorePotion")) {
        return on_ignorepotion(msg, reply);

    }


    return result;
}


TWBaseScript::MsgStatus TWTrapAIBreath::on_onmsg(sScrMsg* msg, cMultiParm& reply)
{
    if(debug_enabled())
        debug_printf(DL_DEBUG, "Received breath on message");

    // Mark the AI as being in the cold. The next TweqComplete should make the
    // breath puff fire
    in_cold = 1;

    return MS_CONTINUE;
}


TWBaseScript::MsgStatus TWTrapAIBreath::on_offmsg(sScrMsg* msg, cMultiParm& reply)
{
    if(debug_enabled())
        debug_printf(DL_DEBUG, "Received breath off message");

    // Mark the AI as being in the warm. The timer will stop the breath puff at
    // the next firing.
    in_cold = 0;

    // Halt breath particle immediately on entering the warm?
    if(stop_immediately) {
        abort_breath();
    }

    return MS_CONTINUE;
}


/* =============================================================================
 *  TWTrapAIBreath Implementation - private members
 */

void TWTrapAIBreath::abort_breath(bool cancel_timer)
{
    SService<IPGroupSrv> SFXSrv(g_pScriptManager);

    if(debug_enabled())
        debug_printf(DL_DEBUG, "Deactivating particle group");

    // Abort firing of the timed message
    if(breath_timer) {
        if(cancel_timer) cancel_timed_message(breath_timer);
        breath_timer = NULL;
    }

    // Deactivate the particle group
    int breath_particles = get_breath_particles();
    if(breath_particles) SFXSrv -> SetActive(breath_particles, false);
}


TWBaseScript::MsgStatus TWTrapAIBreath::start_breath(sTweqMsg *msg, cMultiParm& reply)
{
    SService<IPropertySrv> PropertySrv(g_pScriptManager);
    SService<IPGroupSrv>   SFXSrv(g_pScriptManager);

    // Only process flicker complete messages, and only actually do anything at all
    // if the object is in the cold.
    if(still_alive && in_cold && msg -> Type == kTweqTypeFlicker && msg -> Op == kTweqOpFrameEvent) {
        if(debug_enabled())
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
        abort_breath(false);

        // Check the AI alertness, just in case the rate needs to be lowered
        check_ai_reallyhigh();
    }

    return MS_CONTINUE;
}


TWBaseScript::MsgStatus TWTrapAIBreath::on_objroomtransit(sRoomMsg *msg, cMultiParm& reply)
{
    ColdRoomIter iter = cold_rooms.find(msg -> ToObjId);

    if(iter != cold_rooms.end() && iter -> second) {
        return on_onmsg(msg, reply);
    }

    return on_offmsg(msg, reply);
}


TWBaseScript::MsgStatus TWTrapAIBreath::on_ignorepotion(sScrMsg *msg, cMultiParm& reply)
{
    // reset the rate: the AI is at rest (either temporarily or permanently!)
    // so there's no point triggering the tweq more often than needed.
    set_rate(0);

    // If stop_on_ko is true, it doesn't matter if the AI is knocked out, the
    // particles should be stopped.
    if(stop_on_ko) {
        if(debug_enabled())
            debug_printf(DL_DEBUG, "Treating AI as dead and stopping breath.");

        return on_slain(msg, reply);
    }

    if(debug_enabled())
        debug_printf(DL_DEBUG, "AI is pining for the fjords.");

    return MS_CONTINUE;
}


TWBaseScript::MsgStatus TWTrapAIBreath::on_aimodechange(sAIModeChangeMsg *msg, cMultiParm& reply)
{
    SService<IObjectSrv> ObjectSrv(g_pScriptManager);

    // If the AI is dead, they can't breathe!
    if(msg -> mode == kAIM_Dead) {
        // reset the rate: the AI is at rest (either temporarily or permanently!)
        // so there's no point triggering the tweq more often than needed.
        set_rate(0);

        // If stop_on_ko is true, it doesn't matter if the AI is knocked out...
        if(!stop_on_ko) {

            // Is the AI really dead, or just resting?
            int m_knockedout = StrToObject("M-KnockedOut");

            if(m_knockedout) {
                true_bool just_resting;
                ObjectSrv -> HasMetaProperty(just_resting, ObjId(), m_knockedout);

                if(just_resting) {
                    if(debug_enabled())
                        debug_printf(DL_DEBUG, "AI is pining for the fjords.");

                    return MS_CONTINUE;
                }
            } else if(debug_enabled()) {
                debug_printf(DL_WARNING, "Unable to find knocked-out metaprop, treating AI as slain.");
            }
        }

        return on_slain(msg, reply);
    }

    return MS_CONTINUE;
}


TWBaseScript::MsgStatus TWTrapAIBreath::on_slain(sScrMsg *msg, cMultiParm& reply)
{
    if(debug_enabled())
        debug_printf(DL_DEBUG, "AI is dead; deactivating breath permanently");

    abort_breath();
    still_alive = false;

    return MS_CONTINUE;
}


TWBaseScript::MsgStatus TWTrapAIBreath::on_aialertness(sAIAlertnessMsg *msg, cMultiParm& reply)
{
    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "AI Alertness changed to %d from %d", msg -> level, msg -> oldLevel);
    }

    // If the AI has its tweq set up, update the rate. Awareness levels are listed in
    // lg/defs.h eAIScriptAlertLevel, with the lowest level at 0 (kNoAlert) and highest
    // at 3 (kHighAlert).
    set_rate(msg -> level);

    return MS_CONTINUE;
}


void TWTrapAIBreath::set_rate(int new_level)
{
    SService<IPropertySrv> PropertySrv(g_pScriptManager);

    if(new_level != last_level && new_level >= 0 && new_level <= 3 && PropertySrv -> Possessed(ObjId(), "CfgTweqBlink")) {
        last_level = new_level;

        if(debug_enabled()) {
            debug_printf(DL_DEBUG, "New rate is %d", rates[new_level]);
        }

        PropertySrv -> Set(ObjId(), "CfgTweqBlink", "Rate", rates[new_level]);
        PropertySrv -> Set(ObjId(), "StTweqBlink", "Cur Time", rates[new_level] - 1);
    }
}


void TWTrapAIBreath::check_ai_reallyhigh()
{
    SService<IAIScrSrv>     AISrv(g_pScriptManager);
    SService<ILinkSrv>      LinkSrv(g_pScriptManager);
    SService<ILinkToolsSrv> LinkToolsSrv(g_pScriptManager);

    // First obtain the AI's alertness level
    eAIScriptAlertLevel level = AISrv -> GetAlertLevel(ObjId());

    // If the AI is on high alert, check whether it has an AIInvest link. If it
    // does, the AI is on high alert and in search/pursuit/attack mode, otherwise
    // it has gone back on patrol, but hasn't updated its awareness yet
    if(level == kHighAlert) {

        // Knocked out AIs can be on high alert, so check for that...
        int knockedout = StrToObject("M-KnockedOut");
        if(knockedout) {
            SService<IObjectSrv> ObjectSrv(g_pScriptManager);
            true_bool just_resting;
            ObjectSrv -> HasMetaProperty(just_resting, ObjId(), knockedout);

            // If the AI is not knocked out, check whether it is searching/attacking
            if(!just_resting) {
                true_bool has_invest;
                LinkSrv -> AnyExist(has_invest, LinkToolsSrv -> LinkKindNamed("AIInvest"), ObjId(), 0);

                // AI Doesn't have an invest link? Pretend the AI is a level lower
                if(has_invest) {
                    set_rate(level);
                } else if(last_level != (level - 1)) {
                    if(debug_enabled()) {
                        debug_printf(DL_DEBUG, "AI has no AIInvest link at high alert, downgrading to medium");
                    }

                    set_rate(level - 1);
                }
            } else {
                set_rate(0);
            }
        }
    }
}


int TWTrapAIBreath::get_breath_particles()
{
    object from = get_breath_proxy(ObjId());

    return get_breath_particlegroup(from);
}


int TWTrapAIBreath::get_breath_proxy(object fallback)
{
    return get_linked_object(ObjId(), proxy_arch_name, proxy_link_name, fallback);
}


int TWTrapAIBreath::get_breath_particlegroup(object from)
{
    return get_linked_object(from, particle_arch_name, particle_link_name);
}


void TWTrapAIBreath::parse_coldrooms(char *coldstr)
{
    char *room;
    char *rest = NULL;

    for(room = tok_r(coldstr, ",", &rest); room; room = tok_r(NULL, ",", &rest)) {
        int room_id = StrToObject(room);
        if(room_id) {
            cold_rooms.insert(ColdRoomPair(room_id, true));

            if(debug_enabled())
                debug_printf(DL_DEBUG, "Marking room %s (%d) as cold", room, room_id);
        } else if(debug_enabled()) {
            debug_printf(DL_WARNING, "Unable to map '%s' to a room id, ignoring", room);
        }
    }

}


/*
 * public domain tok_r() by Charlie Gordon
 *
 *   from comp.lang.c  9/14/2007
 *
 *      http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *
 *     (Declaration that it's public domain):
 *      http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */

char* TWTrapAIBreath::tok_r(char *str, const char *delim, char **nextp)
{
    char *ret;

    if(str == NULL) {
        str = *nextp;
    }

    str += strspn(str, delim);

    if (*str == '\0') {
        return NULL;
    }

    ret = str;

    str += strcspn(str, delim);

    if (*str) {
        *str++ = '\0';
    }

    *nextp = str;

    return ret;
}
