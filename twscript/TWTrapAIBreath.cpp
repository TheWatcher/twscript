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

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    if(!design_note) {
        debug_printf(DL_WARNING, "No Editor -> Design Note. Falling back on defaults.");

        // Work out rates based on the base using a simple division
        rates[0].init("", 3000);
        for(int level = 1; level < 4; ++level) {
            rates[level].init("", rates[0].value() / level);
        }

    } else {
        std::string dummy;

        // Should the AI start off in the cold?
        if(!in_cold.Valid()) {
            start_cold.init(design_note, false);
            in_cold = start_cold.value();
        }

        // Should the breath cloud stop immediately on entering the warm?
        stop_immediately.init(design_note, true);

        // Should the breath cloud stop when knocked out?
        stop_on_ko.init(design_note, false);

        // How long, in milliseconds, should the exhale last
        exhale_time.init(design_note, 250);

        // Allow the base rate to be overridden, and recalculated the defaults
        rates[0].init(design_note, 3000);
        for(int level = 1; level < 4; ++level) {
            rates[level].init(design_note, rates[0].value() / level);
        }

        // Sort out the archetype for the particle
        particle_arch_name.init(design_note, "AIBreath");
        particle_link_name.init(design_note, "~ParticleAttachement");

        // Sort out the archetype for the proxy
        proxy_arch_name.init(design_note, "BreathProxy");
        proxy_link_name.init(design_note, "~DetailAttachement");


        rooms.init(design_note);
        if(rooms.value().length() > 0) {
            parse_coldrooms(rooms.value());
        }

        g_pMalloc -> Free(design_note);
    }

    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Initialised on object. Settings:");
        debug_printf(DL_DEBUG, "In cold: %s, stop breath immediately: %s", in_cold ? "yes" : "no", stop_immediately ? "yes" : "no");
        debug_printf(DL_DEBUG, "Exhale time: %dms", exhale_time.value());
        debug_printf(DL_DEBUG, "Breathing rates (in ms) None: %d, Low: %d, Medium: %d, High: %d", rates[0].value(), rates[1].value(), rates[2].value(), rates[3].value());
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
            debug_printf(DL_DEBUG, "New rate is %d", rates[new_level].value());
        }

        PropertySrv -> Set(ObjId(), "CfgTweqBlink", "Rate", rates[new_level].value());
        PropertySrv -> Set(ObjId(), "StTweqBlink", "Cur Time", rates[new_level].value() - 1);
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
    return get_linked_object(ObjId(), proxy_arch_name.value(), proxy_link_name.value(), fallback);
}


int TWTrapAIBreath::get_breath_particlegroup(object from)
{
    return get_linked_object(from, particle_arch_name.value(), particle_link_name.value());
}


void TWTrapAIBreath::parse_coldrooms(const std::string& coldstr)
{
    std::size_t from = 0;
    std::size_t to = coldstr.find(",");

    do {
        std::string room = coldstr.substr(from, to - from);

        int room_id = StrToObject(room.c_str());
        if(room_id) {
            cold_rooms.insert(ColdRoomPair(room_id, true));

            if(debug_enabled())
                debug_printf(DL_DEBUG, "Marking room %s (%d) as cold", room.c_str(), room_id);
        } else if(debug_enabled()) {
            debug_printf(DL_WARNING, "Unable to map '%s' to a room id, ignoring", room.c_str());
        }

        from = to + 1;
        to = coldstr.find(",", from);
    } while(to != std::string::npos);

}
