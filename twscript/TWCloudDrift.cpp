#include <cmath>
#include "TWCloudDrift.h"
#include "ScriptLib.h"

/* =============================================================================
 *  TWCloudDrift Implementation - protected members
 */

void TWCloudDrift::init(int time)
{
    TWBaseScript::init(time);

    // Fetch the initial position if needed
    if(!start_position.Valid()) {
        fetch_initial_location();

        if(debug_enabled()) {
            const mxs_vector *location = start_position;
            debug_printf(DL_WARNING, "Initial location: %f, %f, %f", location -> x, location -> y, location -> z);
        }
    }

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());
    if(!design_note) {
        debug_printf(DL_WARNING, "No Editor -> Design Note. Doing nothing.");

    } else {

        // Nothing can be done if there's no drift setting
        if(get_scriptparam_floatvec(design_note, "Range", driftrange)) {
            // Drifts must be positive. Divide by two so the range is relative to the start pos.
            driftrange.x = fabs(driftrange.x) / 2.0;
            driftrange.y = fabs(driftrange.y) / 2.0;
            driftrange.z = fabs(driftrange.z) / 2.0;

            // And there needs to be actual drift
            if(driftrange) {
                const mxs_vector *location = start_position;

                get_scriptparam_floatvec(design_note, "MaxRate", maxrates, 0.5, 0.5, 0.5);
                get_scriptparam_floatvec(design_note, "MinRate", minrates, 0.05, 0.05, 0.05);

                std::string dummy;
                refresh = get_scriptparam_time(design_note, "Refresh", 1000, dummy);

                char *facmode = get_scriptparam_string(design_note, "Mode");
                if(facmode) {
                    if(!::_stricmp(facmode, "LINEAR")) {
                        factormode = LINEAR;
                    } else if(!::_stricmp(facmode, "LOG")) {
                        factormode = LOGARITHMIC;
                    }

                    g_pMalloc -> Free(facmode);
                }

                // Dump the settings for reference
                if(debug_enabled()) {
                    debug_printf(DL_DEBUG, "Initialised on object. Settings:");
                    debug_printf(DL_DEBUG, "Initial location: (%.3f, %.3f, %.3f)", location -> x, location -> y, location -> z);
                    debug_printf(DL_DEBUG, "Drift amount: (%.3f, %.3f, %.3f)"    , driftrange.x * 2.0, driftrange.y * 2.0, driftrange.z * 2.0);
                    debug_printf(DL_DEBUG, "Minimum rate: (%.3f, %.3f, %.3f)"    , minrates.x , minrates.y , minrates.z);
                    debug_printf(DL_DEBUG, "Maximum rate: (%.3f, %.3f, %.3f)"    , maxrates.x , maxrates.y , maxrates.z);
                    debug_printf(DL_DEBUG, "Update mode: %d", factormode);
                    debug_printf(DL_DEBUG, "Update rate: %d", refresh);
                }

                // Check the velocities and start the refresh timer.
                check_velocities(time);

            // All drift values are zero, there's no point in doing anything
            } else {
                debug_printf(DL_WARNING, "All Drift values are zero. Doing nothing.");
            }

        // Can't do anything if there is no drift information
        } else {
            debug_printf(DL_WARNING, "No Drift specified. Doing nothing.");
        }

        g_pMalloc -> Free(design_note);
    }
}


TWBaseScript::MsgStatus TWCloudDrift::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseScript::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    if(!::_stricmp(msg -> message, "Timer")) {
        return on_timer(static_cast<sScrTimerMsg*>(msg), reply);
    }

    return result;
}


TWBaseScript::MsgStatus TWCloudDrift::on_timer(sScrTimerMsg *msg, cMultiParm& reply)
{
    // Only bother doing anything if the timer name is correct.
    if(!::_stricmp(msg -> name, "CheckVelocity")) {
        check_velocities(msg -> time);
    }

    return MS_CONTINUE;
}


/* =============================================================================
 *  TWCloudDrift Implementation - private members
 */

void TWCloudDrift::fetch_initial_location(void)
{
    SService<IObjectSrv> obj_srv(g_pScriptManager);
    cScrVec position;
    obj_srv -> Position(position, ObjId());

    start_position = &position;
}


float TWCloudDrift::calculate_velocity(float centre, float range, float minrate, float maxrate, float position, float velocity)
{
    if(position < centre - range) {
        // If the velocity is negative, make it positive
        if(velocity < 0) velocity = fabs(minrate);
    } else if(position > centre + range) {
        // If the velocity is positive, make it negative
        if(velocity > 0) velocity = -1 * fabs(minrate);
    } else {
        // location is between min and max, work out the rate
        float offset = fabs(centre - position);
        if(offset > range) offset = range;

        float factor = 0.0;
        switch(factormode) {
            case LINEAR: factor = 1 - (offset / range);
                break;
            case LOGARITHMIC: factor = cosf((offset / range) * M_PI_2);
                break;
            case FIXEDMIN: // fixedmin doesn't need to adjust the factor
                break;
        }

        // Recalculate the magnitude, while retaining its direction
        velocity = copysign(minrate + ((maxrate - minrate) * factor), velocity);
    }

    return velocity;
}


void TWCloudDrift::check_velocities(int time)
{
    if(debug_enabled()) debug_printf(DL_DEBUG, "Updating velocity");

    // Obtain the current location and velocity
    SService<IObjectSrv> obj_srv(g_pScriptManager);
    cScrVec position;
    obj_srv -> Position(position, ObjId());

	SService<IPhysSrv> phys_srv(g_pScriptManager);
    cScrVec velocity;
    phys_srv -> GetVelocity(ObjId(), velocity);

    // And the initial location
    const mxs_vector *location = start_position;

    // Recalculate any velocities that need changing
    if(driftrange.x && minrates.x && maxrates.x)
        velocity.x = calculate_velocity(location -> x, driftrange.x, minrates.x, maxrates.x, position.x, velocity.x);

    if(driftrange.y && minrates.y && maxrates.y)
        velocity.y = calculate_velocity(location -> y, driftrange.y, minrates.y, maxrates.y, position.y, velocity.y);

    if(driftrange.z && minrates.z && maxrates.z)
        velocity.z = calculate_velocity(location -> z, driftrange.z, minrates.z, maxrates.z, position.z, velocity.z);

    phys_srv -> SetVelocity(ObjId(), velocity);

    if(debug_enabled())
        debug_printf(DL_DEBUG, "Pos/Vel,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f", time, position.x, position.y, position.z, velocity.x, velocity.y, velocity.z);

    // This shouldn't be needed, but check anyway
    if(update_timer) {
        cancel_timed_message(update_timer);
    }

    // And schedule the next update.
    update_timer = set_timed_message("CheckVelocity", refresh, kSTM_OneShot);
}
