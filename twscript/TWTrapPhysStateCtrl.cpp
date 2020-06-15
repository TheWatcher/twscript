

#include "TWTrapPhysStateCtrl.h"
#include "ScriptLib.h"

/* =============================================================================
 *  TWTrapPhysStateCtrl Impmementation - protected members
 */

void TWTrapPhysStateCtrl::init(int time)
{
    TWBaseTrap::init(time);

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    if(!design_note) {
        debug_printf(DL_WARNING, "No Editor -> Design Note. Nothing will happen!");
    } else {

        location.init(design_note);
        facing.init(design_note);
        velocity.init(design_note);
        rotvel.init(design_note);

        g_pMalloc -> Free(design_note);
    }

    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Initialised on object. Settings:");

        cScrVec values = location.value();
        debug_printf(DL_DEBUG, "Location: %s values = %.3f, %.3f, %.3f", location.is_set() ? "set" : "not set",
                     values.x, values.y, values.z);
        values = facing.value();
        debug_printf(DL_DEBUG, "Facing: %s values = %.3f, %.3f, %.3f", facing.is_set() ? "set" : "not set",
                     values.x, values.y, values.z);
        values = velocity.value();
        debug_printf(DL_DEBUG, "Velocity: %s values = %.3f, %.3f, %.3f", velocity.is_set() ? "set" : "not set",
                     values.x, values.y, values.z);
        values = rotvel.value();
        debug_printf(DL_DEBUG, "Rotvel: %s values = %.3f, %.3f, %.3f", rotvel.is_set() ? "set" : "not set",
                     values.x, values.y, values.z);
    }
}


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

    data.set_location = location.is_set();
    data.location     = location.value();
    data.set_facing   = facing.is_set();
    data.facing       = facing.value();
    data.set_velocity = velocity.is_set();
    data.velocity     = velocity.value();
    data.set_rotvel   = rotvel.is_set();
    data.rotvel       = rotvel.value();

    if(data.set_location || data.set_facing || data.set_velocity || data.set_rotvel) {
        IterateLinks("ControlDevice", ObjId(), 0, set_state, this, static_cast<void*>(&data));
    } else if(debug_enabled()) {
        debug_printf(DL_WARNING, "Design note will not update linked objects, skipping.");
    }
}


int TWTrapPhysStateCtrl::set_state(ILinkSrv*, ILinkQuery* link_query, IScript* script, void* data)
{
    TWStateData *state_data = static_cast<TWStateData *>(data);

    // Get the ControlDevice link - dest should be a moving terrain object
    sLink current_link;
    link_query -> Link(&current_link);
    object target_obj = current_link.dest; // For readability

    // Names are only needed for debugging, but meh.
    std::string target_name;
    static_cast<TWTrapPhysStateCtrl *>(script) -> get_object_namestr(target_name, target_obj);

    if(static_cast<TWTrapPhysStateCtrl *>(script) -> debug_enabled())
        static_cast<TWTrapPhysStateCtrl *>(script) -> debug_printf(DL_DEBUG, "Setting state of %s", target_name.c_str());

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
            static_cast<TWTrapPhysStateCtrl *>(script) -> debug_printf(DL_DEBUG, "Setting Location of %s to X: %.3f Y: %.3f Z: %.3f", target_name.c_str(), position.x, position.y, position.z);
    }

    // And the orientation
    if(state_data -> set_facing) {
        facing = state_data -> facing;
        if(static_cast<TWTrapPhysStateCtrl *>(script) -> debug_enabled())
            static_cast<TWTrapPhysStateCtrl *>(script) -> debug_printf(DL_DEBUG, "Setting Facing of %s to H: %.3f P: %.3f B: %.3f", target_name.c_str(), facing.z, facing.y, facing.x);
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
                static_cast<TWTrapPhysStateCtrl *>(script) -> debug_printf(DL_DEBUG, "Setting Velocity of %s to X: %.3f Y: %.3f Z: %.3f", target_name.c_str(), state_data -> velocity.x, state_data -> velocity.y, state_data -> velocity.z);
        }

        if(state_data -> set_rotvel) {
            cMultiParm prop = state_data -> rotvel;
            prop_srv -> Set(target_obj, "PhysState", "Rot Velocity", prop);

            if(static_cast<TWTrapPhysStateCtrl *>(script) -> debug_enabled())
                static_cast<TWTrapPhysStateCtrl *>(script) -> debug_printf(DL_DEBUG, "Setting Rot Velocity of %s to H: %.3f P: %.3f B: %.3f", target_name.c_str(), state_data -> rotvel.z, state_data -> rotvel.y, state_data -> rotvel.x);
        }

    } else if(static_cast<TWTrapPhysStateCtrl *>(script) -> debug_enabled()) {
        static_cast<TWTrapPhysStateCtrl *>(script) -> debug_printf(DL_DEBUG, "%s has no PhysState property. This should not happen!", target_name.c_str());
    }

    return 1;
}
