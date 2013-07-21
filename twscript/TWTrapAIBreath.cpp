#include <cstring>
#include "TWTrapAIBreath.h"
#include "ScriptLib.h"

static char* strtok_r(char *str, const char *delim, char **nextp);


/* =============================================================================
 *  TWTrapAIBreath Impmementation - protected members
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
    } else {
        // Should the AI start off in the cold?
        if(!in_cold.Valid()) {
            in_cold = static_cast<int>(get_scriptparam_bool(design_note, "InCold", false));
        }

        // Should the breath cloud stop immediately on entering the warm?
        stop_immediately = get_scriptparam_bool(design_note, "Immediate", true);

        // Should the breath cloud stop when knocked out?
        stop_on_ko = get_scriptparam_bool(design_note, "StopOnKO", false);

        // How long, in milliseconds, should the exhale last
        exhale_time = get_scriptparam_int(design_note, "ExhaleTime", 250);

        // Sort out the archetype for the particle
        char *sfx_name = get_scriptparam_string(design_note, "SFX", "AIBreath");
        if(sfx_name) {
            particle_arch_name = sfx_name;
            g_pMalloc -> Free(sfx_name);
        }

        char *cold = get_scriptparam_string(design_note, "ColdRooms", NULL);
        if(cold) {
            parse_coldrooms(cold);
            g_pMalloc -> Free(cold);
        }

        g_pMalloc -> Free(design_note);
    }

    if(!base_rate.Valid()) {
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

            // Average human at-rest breating rate is 12 to 24 breaths per minute. AIs are usually moving, so the
            // higher end is more likely as a sane value. 60000 milliseconds / 20 breaths per minute =
            base_rate = 3000;
        }
    }

    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Initialised on object. Settings:");
        debug_printf(DL_DEBUG, "In cold: %s, stop breath immediately: %s", in_cold ? "yes" : "no", stop_immediately ? "yes" : "no");
        debug_printf(DL_DEBUG, "Exhale time: %dms, Rest rate: %dms", exhale_time, int(base_rate));
        debug_printf(DL_DEBUG, "SFX name: %s", particle_arch_name.c_str());
    }
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
 *  TWTrapAIBreath Impmementation - private members
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


TWBaseScript::MsgStatus TWTrapAIBreath::on_aimodechange(sAIModeChangeMsg *msg, cMultiParm& reply)
{
    SService<IObjectSrv> ObjectSrv(g_pScriptManager);

    // If the AI is dead, they can't breathe!
    if(msg -> mode == kAIM_Dead) {
        // reset the rate: the AI is at rest (either temporarily or permanently!)
        // so there's no point triggering the tweq more often than needed.
        set_rate(1);

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
    // If the AI has its tweq set up, update the rate. Awareness levels are listed in
    // lg/defs.h eAIScriptAlertLevel, with the lowest level at 0 (kNoAlert) and highest
    // at 3 (kHighAlert). As the level will be used as a divisor, it must be 1 to 4.
    set_rate(msg -> level + 1);

    return MS_CONTINUE;
}


void TWTrapAIBreath::set_rate(int new_level)
{
    SService<IPropertySrv> PropertySrv(g_pScriptManager);

    if(new_level && PropertySrv -> Possessed(ObjId(), "CfgTweqBlink")) {

        // POSSIBLE EXPANSION: instead of doing a basic divisor thing here, possibly
        // allow the user to specify scaling values per alerness level?
        int new_rate = base_rate / new_level;

        PropertySrv -> Set(ObjId(), "CfgTweqBlink", "Rate", new_rate);
        PropertySrv -> Set(ObjId(), "StTweqBlink", "Cur Time", new_rate - 1);
    }
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
        int archetype = StrToObject(particle_arch_name.c_str());
        if(archetype) {
            // Is there a particle attach(e)ment to this object?
            true_bool has_attach;
            LinkSrv -> AnyExist(has_attach, LinkToolsSrv -> LinkKindNamed("~ParticleAttachement"), ObjId(), 0);

            // Only do anything if there is at least one particle attachment.
            if(has_attach) {
                linkset links;
                true_bool inherits;

                // Check all the particle attachment links looking for a link from a particle that inherits
                // from the archetype.
                LinkSrv -> GetAll(links, LinkToolsSrv -> LinkKindNamed("~ParticleAttachement"), ObjId(), 0);
                while(links.AnyLinksLeft()) {
                    sLink link = links.Get();
                    ObjectSrv -> InheritsFrom(inherits, link.dest, archetype);

                    // Found a link from a concrete instance of the archetype? Return that object.
                    if(inherits) {
                        return link.dest;
                    }
                    links.NextLink();
                }

                if(debug_enabled())
                    debug_printf(DL_WARNING, "Object has no link to a particle inheriting from %s", particle_arch_name.c_str());
            }
        } else if(debug_enabled()) {
            debug_printf(DL_WARNING, "Unable to find particle named '%s'", particle_arch_name.c_str());
        }
    } else {
        debug_printf(DL_ERROR, "particle_arch_name name is empty?!");
    }

    return 0;
}


void TWTrapAIBreath::parse_coldrooms(char *coldstr)
{
    char *room;
    char *rest = NULL;

    for(room = ::strtok_r(coldstr, ",", &rest); room; room = ::strtok_r(NULL, ",", &rest)) {
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
 * public domain strtok_r() by Charlie Gordon
 *
 *   from comp.lang.c  9/14/2007
 *
 *      http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *
 *     (Declaration that it's public domain):
 *      http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */

static char* strtok_r(char *str, const char *delim, char **nextp)
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
