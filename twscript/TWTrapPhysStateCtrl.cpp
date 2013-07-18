

#include "TWTrapPhysStateCtrl.h"
#include "ScriptLib.h"

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
