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
        // Watch for QVar changes?
        subscribe.init(design_note, false);

        // Check whether the speed should come from a stim message intensity
        speed.init(design_note, 0.0f, subscribe.value());
        intensity.init(design_note);

        // Is immediate mode enabled?
        immediate.init(design_note);

        // And targetting
        set_target.init(design_note, "[me]");

        g_pMalloc -> Free(design_note);
    }

    // If debugging is enabled, print some Helpful Information
    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Initialised. Initial speed: %.3f", speed.value());
        debug_printf(DL_DEBUG, "Immediate speed change: %s", immediate.value() ? "enabled" : "disabled");
        if(intensity.value())  debug_printf(DL_DEBUG, "Speed will be taken from stim intensity");
        debug_printf(DL_DEBUG, "Targetting: %s", set_target.c_str());
    }
}


TWBaseScript::MsgStatus TWTrapSetSpeed::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseTrap::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    // Remove the qvar subscription during shutdown
    if(!::_stricmp(msg -> message, "EndScript")) {
        if(subscribe.value()) {
            if(debug_enabled())
                debug_printf(DL_DEBUG, "Removing subscription to qvars");

            speed.unsubscribe();
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

    // Speed from an intensity value?
    if(intensity.value()) {
        debug_printf(DL_ERROR, "Parsing speed from intensity.", msg);
        set_speed = static_cast<sStimMsg *>(msg) -> intensity;

        if(debug_enabled()) debug_printf(DL_DEBUG, "Using speed %.3f from stim intensity.", set_speed);

    // Otherwise calculate the speed?
    } else {
        set_speed = speed.value();

        if(debug_enabled())
             debug_printf(DL_DEBUG, "Using speed %.3f.", set_speed);
    }

    // If a target has been parsed, fetch all the objects that match it
    if(debug_enabled())
        debug_printf(DL_DEBUG, "Looking up targets matched by %s.", set_target.c_str());

    std::vector<TargetObj>* targets = set_target.values(msg);

    if(targets) {
        if(!targets -> empty()) {
            // Process the target list, setting the speeds accordingly
            std::vector<TargetObj>::iterator it;
            std::string targ_name;

            for(it = targets -> begin() ; it != targets -> end(); it++) {
                set_tpath_speed(it -> obj_id);

                if(debug_enabled()) {
                    get_object_namestr(targ_name, it -> obj_id);
                    debug_printf(DL_DEBUG, "Setting speed %.3f on %s.", set_speed, targ_name.c_str());
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
    cMultiParm setspeed = set_speed;

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
        client -> debug_printf(DL_DEBUG, "setting speed %.3f on %s", client -> set_speed, mterr_name.c_str());
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
                direction *= client -> set_speed;
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
