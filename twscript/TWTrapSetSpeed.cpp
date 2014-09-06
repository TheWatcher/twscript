#include "TWTrapSetSpeed.h"
#include "ScriptLib.h"

/* =============================================================================
 *  TWTrapSetSpeed Impmementation - protected members
 */

void TWTrapSetSpeed::init(int time)
{
    TWBaseTrap::init(time);

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    if(!design_note) {
        debug_printf(DL_WARNING, "No Editor -> Design Note. Falling back on defaults.");
    } else {

        // Check whether the speed should come from a stim message intensity
        char *speed_data = get_scriptparam_string(design_note, "Speed");
        if(speed_data) {
            intensity = (::_stricmp(speed_data, "[intensity]") == 0);
            g_pMalloc -> Free(speed_data);
        }

        // Get the speed the user has set for this object (which could be a QVar string)
        speed = get_scriptparam_float(design_note, "Speed", 0.0f, qvar_name);

        // Is immediate mode enabled?
        immediate = get_scriptparam_bool(design_note, "Immediate", false);

        // Is qvar tracking enabled? (can only be turned on if there is a qvar to track, too)
        if(get_scriptparam_bool(design_note, "WatchQVar", false) && !qvar_name.empty()) {
            int namelen = get_qvar_namelen(qvar_name.c_str());

            if(namelen) {
                // Need to subscribe to, potentially, a substring of qvar_name
                qvar_sub = qvar_name.substr(0, namelen);

                if(debug_enabled())
                    debug_printf(DL_DEBUG, "Adding subscription to qvar '%s'.", qvar_sub.c_str());

                SService<IQuestSrv> quest_srv(g_pScriptManager);
                quest_srv -> SubscribeMsg(ObjId(), qvar_sub.c_str(), kQuestDataAny);
            } else {
                debug_printf(DL_WARNING, "Unable to subscribe to qvar with name '%s'", qvar_name.c_str());
            }
        }

        // Sort out the target string too.
        // IMPORTANT NOTE: While it is tempting to build the full target object list at this point,
        // doing so may possibly miss dynamically created terrpts.
        char *target = get_scriptparam_string(design_note, "Dest", "[me]");
        if(target) {
            set_target = target;
            g_pMalloc -> Free(target);
        }
        if(set_target.empty())
            debug_printf(DL_WARNING, "Target set failed!");

        g_pMalloc -> Free(design_note);
    }

    // If debugging is enabled, print some Helpful Information
    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Initialised. Initial speed: %.3f", speed);
        debug_printf(DL_DEBUG, "Immediate speed change: %s", immediate ? "enabled" : "disabled");
        if(intensity)  debug_printf(DL_DEBUG, "Speed will be taken from stim intensity");
        if(!qvar_name.empty())  debug_printf(DL_DEBUG, "Speed will be read from QVar: %s", qvar_name.c_str());
        if(!set_target.empty()) debug_printf(DL_DEBUG, "Targetting: %s", set_target.c_str());
    }
}


TWBaseScript::MsgStatus TWTrapSetSpeed::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseTrap::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    // Remove the qvar subscription during shutdown
    if(!::_stricmp(msg -> message, "EndScript")) {
        if(!qvar_sub.empty()) {
            if(debug_enabled())
                debug_printf(DL_DEBUG, "Removing subscription to '%s'", qvar_sub.c_str());

            SService<IQuestSrv> quest_srv(g_pScriptManager);
            quest_srv -> UnsubscribeMsg(ObjId(), qvar_sub.c_str());
        }

    // Handle updates on quest variable change
    } else if(!::_stricmp(msg -> message, "QuestChange")) {
        return on_questchange(static_cast<sQuestMsg *>(msg), reply);
    }

    return result;
}


TWBaseScript::MsgStatus TWTrapSetSpeed::on_onmsg(sScrMsg* msg, cMultiParm& reply)
{
    MsgStatus result = TWBaseTrap::on_onmsg(msg, reply);

    if(result == MS_CONTINUE)
        update_speed(msg);

    return result;
}


TWBaseScript::MsgStatus TWTrapSetSpeed::on_questchange(sQuestMsg* msg, cMultiParm& reply)
{
    // Only bother doing speed updates if the quest variable changes
    if(msg -> m_newValue != msg -> m_oldValue) {
        update_speed(msg);
    } else if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Quest variable %s value has not changed, skipping update.", msg -> m_pName);
    }

    return MS_CONTINUE;
}


/* =============================================================================
 *  TWTrapSetSpeed Impmementation - private members
 */

void TWTrapSetSpeed::update_speed(sScrMsg* msg)
{
    SInterface<IObjectSystem> obj_sys(g_pScriptManager);

    if(debug_enabled())
        debug_printf(DL_DEBUG, "Updating speed.");

    // If the user has specified a QVar to use, read that
    if(!qvar_name.empty()) {
        speed = get_qvar_value(qvar_name, (float)speed);

        if(debug_enabled()) debug_printf(DL_DEBUG, "Using speed %.3f from %s.", speed, qvar_name.c_str());

    // If using intensity value, try that...
    } else if(intensity) {
        debug_printf(DL_ERROR, "Parsing speed from intensity.", msg);
        speed = static_cast<sStimMsg *>(msg) -> intensity;

        if(debug_enabled()) debug_printf(DL_DEBUG, "Using speed %.3f from stim intensity.", speed);

    // Otherwise just print out debugging if needed.
    } else if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Using speed %.3f.", speed);
    }

    // If a target has been parsed, fetch all the objects that match it
    if(!set_target.empty()) {
        if(debug_enabled())
            debug_printf(DL_DEBUG, "Looking up targets matched by %s.", set_target.c_str());

        std::vector<TargetObj>* targets = get_target_objects(set_target.c_str(), msg);

        if(!targets -> empty()) {
            // Process the target list, setting the speeds accordingly
            std::vector<TargetObj>::iterator it;
            std::string targ_name;
            for(it = targets -> begin() ; it != targets -> end(); it++) {
                set_tpath_speed(it -> obj_id);

                if(debug_enabled()) {
                    get_object_namestr(targ_name, it -> obj_id);
                    debug_printf(DL_DEBUG, "Setting speed %.3f on %s.", speed, targ_name.c_str());
                }
            }
        } else {
            debug_printf(DL_WARNING, "Dest '%s' did not match any objects.", set_target.c_str());
        }

        // And clean up
        delete targets;
    }

    // And now update any moving terrain objects linked to this one via ScriptParams with data set to "SetSpeed"
    IterateLinksByData("ScriptParams", ObjId(), 0, "SetSpeed", 9, set_mterr_speed, this, NULL);
}


void TWTrapSetSpeed::set_tpath_speed(object obj_id)
{
    SService<ILinkSrv> link_srv(g_pScriptManager);
    SService<ILinkToolsSrv> link_tools_srv(g_pScriptManager);

    // Convert to a multiparm here for ease
    cMultiParm setspeed = speed;

    // Fetch all TPath links from the specified object to any other
    linkset lsLinks;
    link_srv -> GetAll(lsLinks, link_tools_srv -> LinkKindNamed("TPath"), obj_id, 0);

    // Set the speed for each link to the set speed.
    for(; lsLinks.AnyLinksLeft(); lsLinks.NextLink()) {
        link_tools_srv -> LinkSetData(lsLinks.Link(), "Speed", setspeed);
    }
}


int TWTrapSetSpeed::set_mterr_speed(ILinkSrv*, ILinkQuery* link_query, IScript* script, void*)
{
    TWTrapSetSpeed *client = static_cast<TWTrapSetSpeed *>(script);

    // Get the scriptparams link - dest should be a moving terrain object
    sLink current_link;
    link_query -> Link(&current_link);

    // For readability
    object mterr_obj = current_link.dest;

    if(client -> debug_enabled()) {
        std::string mterr_name;
        client -> get_object_namestr(mterr_name, mterr_obj);
        client -> debug_printf(DL_DEBUG, "setting speed %.3f on %s", client -> speed, mterr_name.c_str());
    }

    // Find out where the moving terrain is headed to
    SInterface<ILinkManager> link_mgr(g_pScriptManager);
    SInterface<IRelation> path_next_rel = link_mgr -> GetRelationNamed("TPathNext");

    // Try to get the link to the next waypoint
    long id = path_next_rel -> GetSingleLink(mterr_obj, 0);
    if(id != 0) {

        // dest in this link should be where the moving terrain is going
        sLink target_link;
        path_next_rel -> Get(id, &target_link);
        object terrpt_obj = target_link.dest;   // For readability

        if(terrpt_obj) {
            SService<IObjectSrv> obj_srv(g_pScriptManager);
            SService<IPhysSrv> phys_srv(g_pScriptManager);

            // Get the location of the terrpt
            cScrVec target_pos;
            cScrVec terrain_pos;
            obj_srv -> Position(target_pos, terrpt_obj);
            obj_srv -> Position(terrain_pos, mterr_obj);

            // Now work out what the velocity vector should be, based on the
            // direction to the target and the speed.
            cScrVec direction = target_pos - terrain_pos;
            if(direction.MagSquared() > 0.0001) {
                // The moving terrain is not on top of the terrpt
                direction.Normalize();
                direction *= client -> speed;
            } else {
                // On top of it, the game should pick this up and move the mterr to a
                // new path.
                direction = cScrVec::Zero;
            }

            // Set the speed. Note that UpdateMovingTerrainVelocity does something with
            // 'ClearTransLimits()' and 'AddTransLimit()' here - that seems to be something
            // to do with setting the waypoint trigger, so we should be okay to just update the
            // speed here as we're not changing the target waypoint.
            phys_srv -> ControlVelocity(mterr_obj, direction);
            if(client -> immediate) phys_srv -> SetVelocity(mterr_obj, direction);
        }
    }

    return 1;
}
