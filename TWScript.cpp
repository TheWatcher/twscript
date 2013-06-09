

#include "TWScript.h"
#include "ScriptLib.h"


/* =============================================================================
 *  TWTrapSetSpeed Impmementation - protected members
 */

TWBaseScript::MsgStatus TWTrapSetSpeed::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseTrap::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    // Remove the qvar subscription during shutdown
    if(!::_stricmp(msg -> message, "EndScript")) {
        if(qvar_sub) {
            if(debug_enabled())
                debug_printf(DL_DEBUG, "Removing subscription to '%s'", static_cast<const char *>(qvar_sub));

            SService<IQuestSrv> quest_srv(g_pScriptManager);
            quest_srv -> UnsubscribeMsg(ObjId(), static_cast<const char *>(qvar_sub));
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

/** A convenience structure used to pass speed and control
 *  data from the TWTrapSetSpeed::OnTurnOn() function
 *  to the link iterator callback.
 */
struct TWSetSpeedData
{
    float speed;     //!< The speed set by the user.
    bool  immediate; //!< Whether the speed change should be immediate.
};


void TWTrapSetSpeed::init(int time)
{
    TWBaseTrap::init(time);

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    if(!design_note)
        debug_printf(DL_WARNING, "No Editor -> Design Note. Falling back on defaults.");

    // Get the speed the user has set for this object (which could be a QVar string)
    speed = get_scriptparam_float(design_note, "Speed", 0.0f, qvar_name);

    // Is immediate mode enabled?
    immediate = get_scriptparam_bool(design_note, "Immediate", false);

    // Is qvar tracking enabled? (can only be turned on if there is a qvar to track, too)
    if(get_scriptparam_bool(design_note, "WatchQVar", false) && qvar_name) {
        int namelen = get_qvar_namelen(static_cast<const char *>(qvar_name));

        if(namelen) {
            qvar_sub.Assign(namelen, static_cast<const char *>(qvar_name));

            if(debug_enabled())
                debug_printf(DL_DEBUG, "Adding subscription to qvar '%s'.", static_cast<const char *>(qvar_sub));

            SService<IQuestSrv> quest_srv(g_pScriptManager);
            quest_srv -> SubscribeMsg(ObjId(), static_cast<const char *>(qvar_sub), kQuestDataAny);
        } else {
            debug_printf(DL_WARNING, "Unable to subscribe to qvar with name '%s'", static_cast<const char *>(qvar_name));
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
    if(!set_target)
        debug_printf(DL_WARNING, "Target set failed!");

    // If a design note was obtained, free it now
    if(design_note)
        g_pMalloc -> Free(design_note);

    // If debugging is enabled, print some Helpful Information
    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Initialised. Initial speed: %.3f", speed);
        debug_printf(DL_DEBUG, "Immediate speed change: %s", immediate ? "enabled" : "disabled");
        if(qvar_name)  debug_printf(DL_DEBUG, "Speed will be read from QVar: %s", static_cast<const char *>(qvar_name));
        if(set_target) debug_printf(DL_DEBUG, "Targetting: %s", static_cast<const char *>(set_target));
    }
}


void TWTrapSetSpeed::update_speed(sScrMsg* pMsg)
{
    SInterface<IObjectSystem> obj_sys(g_pScriptManager);

    if(debug_enabled())
        debug_printf(DL_DEBUG, "Updating speed.");

    // If the user has specified a QVar to use, read that
    if(qvar_name) {
        speed = get_qvar_value(qvar_name, (float)speed);
    }

    if(debug_enabled())
        debug_printf(DL_DEBUG, "Using speed %.3f.", speed);

    // If a target has been parsed, fetch all the objects that match it
    if(set_target) {
        if(debug_enabled())
            debug_printf(DL_DEBUG, "Looking up targets matched by %s.", static_cast<const char *>(set_target));

        std::vector<object>* targets = get_target_objects(static_cast<const char *>(set_target), pMsg);

        if(!targets -> empty()) {
            // Process the target list, setting the speeds accordingly
            std::vector<object>::iterator it;
            cAnsiStr targ_name;
            for(it = targets -> begin() ; it < targets -> end(); it++) {
                set_tpath_speed(*it);

                if(debug_enabled()) {
                    get_object_namestr(targ_name, *it);
                    debug_printf(DL_DEBUG, "Setting speed %.3f on %s.", speed, static_cast<const char *>(targ_name));
                }
            }
        } else {
            debug_printf(DL_WARNING, "Dest '%s' did not match any objects.", static_cast<const char *>(set_target));
        }

        // And clean up
        delete targets;
    }

    // Copy the speed and immediate setting so it can be made available to the iterator
    TWSetSpeedData data;
    data.speed     = speed;
    data.immediate = immediate;

    // And now update any moving terrain objects linked to this one via ScriptParams with data set to "SetSpeed"
    IterateLinksByData("ScriptParams", ObjId(), 0, "SetSpeed", 9, set_mterr_speed, this, static_cast<void*>(&data));
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


int TWTrapSetSpeed::set_mterr_speed(ILinkSrv*, ILinkQuery* link_query, IScript* script, void* data)
{
    TWSetSpeedData *speed_data = static_cast<TWSetSpeedData *>(data);

    // Get the scriptparams link - dest should be a moving terrain object
    sLink current_link;
    link_query -> Link(&current_link);

    // For readability
    object mterr_obj = current_link.dest;

    if(static_cast<TWTrapSetSpeed *>(script) -> debug_enabled()) {
        cAnsiStr mterr_name;
        static_cast<TWTrapSetSpeed *>(script) -> get_object_namestr(mterr_name, mterr_obj);
        static_cast<TWTrapSetSpeed *>(script) -> debug_printf(DL_DEBUG, "setting speed %.3f on %s", speed_data -> speed, static_cast<const char *>(mterr_name));
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
                direction *= speed_data -> speed;
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
            if(speed_data -> immediate) phys_srv -> SetVelocity(mterr_obj, direction);
        }
    }

    return 1;
}



/* =============================================================================
 *  TWTrapPhysStateCtrl Impmementation - protected members
 */

TWBaseScript::MsgStatus TWTrapPhysStateCtrl::on_onmsg(sScrMsg* msg, cMultiParm& reply)
{
    MsgStatus result = TWBaseTrap::on_onmsg(msg, reply);

    if(result == MS_CONTINUE)
        update();

    return result;
}


/* =============================================================================
 *  TWTrapPhysStateCtrl Impmementation - private members
 */

/** A convenience structure used to pass vectors and control data
 *  from the TWTrapPhysStateCtrl::update() function to the
 *  link iterator callback.
 */
struct TWStateData
{
    bool    set_location;     //!< Update the object position?
    cScrVec location;         //!< The x,y,z coordinates to set the object at.

    bool    set_facing;       //!< Update the heading, bank, and pitch of the object?
    cScrVec facing;           //!< The orientation to set, x=bank, y=pitch, z=heading (as in Physics -> Model -> State)

    bool    set_velocity;     //!< Update the object's velocity?
    cScrVec velocity;         //!< The velocity to set for the object

    bool    set_rotvel;       //!< Update the rotational velocity?
    cScrVec rotvel;           //!< The rotational velocity to set, x=bank, y=pitch, z=heading (as in Physics -> Model -> State)
};


void TWTrapPhysStateCtrl::update()
{
    struct TWStateData data;

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    if(!design_note)
        debug_printf(DL_WARNING, "No Editor -> Design Note. Falling back on defaults.");

    data.set_location = get_scriptparam_floatvec(design_note, "Location", data.location);
    data.set_facing   = get_scriptparam_floatvec(design_note, "Facing"  , data.facing  );
    data.set_velocity = get_scriptparam_floatvec(design_note, "Velocity", data.velocity);
    data.set_rotvel   = get_scriptparam_floatvec(design_note, "RotVel"  , data.rotvel  );

    if(data.set_location || data.set_facing || data.set_velocity || data.set_rotvel) {
        IterateLinks("ControlDevice", ObjId(), 0, set_state, this, static_cast<void*>(&data));
    } else if(debug_enabled()) {
        debug_printf(DL_WARNING, "Design note will not update linked objects, skipping.");
    }

    // If a design note was obtained, free it now
    if(design_note)
        g_pMalloc -> Free(design_note);
}


int TWTrapPhysStateCtrl::set_state(ILinkSrv*, ILinkQuery* link_query, IScript* script, void* data)
{
    TWStateData *state_data = static_cast<TWStateData *>(data);

    // Get the ControlDevice link - dest should be a moving terrain object
    sLink current_link;
    link_query -> Link(&current_link);
    object target_obj = current_link.dest; // For readability

    // Names are only needed for debugging, but meh.
    cAnsiStr target_name;
    static_cast<TWTrapPhysStateCtrl *>(script) -> get_object_namestr(target_name, target_obj);

    if(static_cast<TWTrapPhysStateCtrl *>(script) -> debug_enabled())
        static_cast<TWTrapPhysStateCtrl *>(script) -> debug_printf(DL_DEBUG, "Setting state of %s", static_cast<const char *>(target_name));

    // Obtain the current location and orientation - both are needed, even if one is being updated,
    // so that teleport will work
    SService<IObjectSrv> obj_srv(g_pScriptManager);
    cScrVec position, facing;
    obj_srv -> Position(position, target_obj);
    obj_srv -> Facing(facing, target_obj);

    // Update the location if needed
    if(state_data -> set_location) {
        position = state_data -> location;
        if(static_cast<TWTrapPhysStateCtrl *>(script) -> debug_enabled())
            static_cast<TWTrapPhysStateCtrl *>(script) -> debug_printf(DL_DEBUG, "Setting Location of %s to X: %.3f Y: %.3f Z: %.3f", static_cast<const char *>(target_name), position.x, position.y, position.z);
    }

    // And the orientation
    if(state_data -> set_facing) {
        facing = state_data -> facing;
        if(static_cast<TWTrapPhysStateCtrl *>(script) -> debug_enabled())
            static_cast<TWTrapPhysStateCtrl *>(script) -> debug_printf(DL_DEBUG, "Setting Facing of %s to H: %.3f P: %.3f B: %.3f", static_cast<const char *>(target_name), facing.z, facing.y, facing.x);
    }

    // Move and orient the object
    obj_srv -> Teleport(target_obj, position, facing, 0);

    // Now fix up the object velocities.
    SService<IPropertySrv> prop_srv(g_pScriptManager);
    if(prop_srv -> Possessed(target_obj, "PhysState")) {

        if(state_data -> set_velocity) {
            cMultiParm prop = state_data -> velocity;
            prop_srv -> Set(target_obj, "PhysState", "Velocity", prop);

            if(static_cast<TWTrapPhysStateCtrl *>(script) -> debug_enabled())
                static_cast<TWTrapPhysStateCtrl *>(script) -> debug_printf(DL_DEBUG, "Setting Velocity of %s to X: %.3f Y: %.3f Z: %.3f", static_cast<const char *>(target_name), state_data -> velocity.x, state_data -> velocity.y, state_data -> velocity.z);
        }

        if(state_data -> set_rotvel) {
            cMultiParm prop = state_data -> rotvel;
            prop_srv -> Set(target_obj, "PhysState", "Rot Velocity", prop);

            if(static_cast<TWTrapPhysStateCtrl *>(script) -> debug_enabled())
                static_cast<TWTrapPhysStateCtrl *>(script) -> debug_printf(DL_DEBUG, "Setting Rot Velocity of %s to H: %.3f P: %.3f B: %.3f", static_cast<const char *>(target_name), state_data -> rotvel.z, state_data -> rotvel.y, state_data -> rotvel.x);
        }

    } else if(static_cast<TWTrapPhysStateCtrl *>(script) -> debug_enabled()) {
        static_cast<TWTrapPhysStateCtrl *>(script) -> debug_printf(DL_DEBUG, "%s has no PhysState property. This should not happen!", static_cast<const char *>(target_name));
    }

    return 1;
}
