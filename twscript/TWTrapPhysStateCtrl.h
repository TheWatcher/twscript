/** @file
 * This file contains the interfaces for the TWTrapPhysStateCtrl class.
 *
 * @author Chris Page &lt;chris@starforge.co.uk&gt;
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

#ifndef TWTRAPPHYSSTATECTRL_H
#define TWTRAPPHYSSTATECTRL_H

#if !SCR_GENSCRIPTS
#include <lg/interface.h>
#include <lg/scrmanagers.h>
#include <lg/scrservices.h>
#include <lg/objects.h>
#include <lg/links.h>
#include <lg/properties.h>
#include <lg/propdefs.h>

#include "TWBaseScript.h"
#include "TWBaseTrap.h"


/** @class TWTrapPhysStateCtrl
 *
 * TWTrapPhysStateControl provides direct control over the location, orientation,
 * velocity, and rotational velocity of objects in Thief 2. Note that this script
 * provides a means to set the physics state values, but the game may ignore these
 * values in some situations, and any changes you make will be subsequently subject
 * to the normal physics simulation performed by the game (so, for example, changing
 * an object's position may result in it either staying in the new location, or
 * falling to (or through!) the ground, depending on how the object has been set up).
 *
 * Expect to have to experiment with this script!
 *
 * Add this script to a marker, link the marker to the object(s) whose physics
 * state you want to control using ControlDevice links. Whenever the marker is sent
 * a TurnOn message, the script will update the physics state of the objects linked
 * to the marker.
 *
 * Configuration
 * -------------
 * Parameters are specified using the Editor -> Design Note, please see the
 * main documentation for more about this.  Parameters supported by
 * TWTrapPhysStateControl are listed below. If a parameter is not specified,
 * the default value shown is used instead. Note that all the parameters are
 * optional, and if you do not specify a parameter, the script will attempt to use
 * a 'sane' default.
 *
 * Parameter: TWTrapPhysStateCtrlLocation
 *      Type: float vector
 *   Default: none (location is not changed)
 * Set the location of the controlled object(s) the position specified. If this
 * parameter is not specified, the location of the object(s) is not modified. If you
 * specify this parameter, but give it no value (ie: `TWTrapPhysStateCtrlLocation=;`)
 * then the default location of `0, 0, 0` is used.
 *
 * Parameter: TWTrapPhysStateCtrlFacing
 *      Type: float vector
 *   Default: none (orientation is not changed)
 * Set the orientation of the controlled object(s) the values specified. If this
 * parameter is not specified, the orientation of the object(s) is not modified. If you
 * specify this parameter, but give it no value (ie: `TWTrapPhysStateCtrlFacing=;`)
 * then the default orientation of `0, 0, 0` is used. *IMPORTANT NOTE*: the values
 * specified for this parameter match the order found in Physics -> Model -> State,
 * so the first value is bank (B), the second is pitch (P), and the third is
 * heading (H). This is the opposite of the order most people would expect; if you
 * find yourself having problems orienting objects, check that you haven't mixed up
 * the bank and heading!
 *
 * Parameter: TWTrapPhysStateCtrlVelocity
 *      Type: float vector
 *   Default: none (velocity is not changed)
 * Set the velocity of the controlled object(s) the values specified. If this
 * parameter is not specified, the velocity of the object(s) is not modified. If you
 * specify this parameter, but give it no value (ie: `TWTrapPhysStateCtrlVelocity=;`)
 * then the default velocity of `0, 0, 0` is used.
 *
 * Parameter: TWTrapPhysStateCtrlRotVel
 *      Type: float vector
 *   Default: none (rotational velocity is not changed)
 * Set the rotational velocity of the controlled object(s) the values specified. If
 * this parameter is not specified, the rotational velocity of the object(s) is not
 * modified. If you specify this parameter, but give it no value
 * (ie: `TWTrapPhysStateCtrlRotVel=;`) then the default of `0, 0, 0` is used. Note
 * that, as with TWTrapPhysStateCtrlFacing, the first value of the vector is the
 * bank, the second is the pitch, and the third is the heading.
 */
class TWTrapPhysStateCtrl : public TWBaseTrap
{
public:
    TWTrapPhysStateCtrl(const char* name, int object) : TWBaseTrap(name, object),
                                                        location(object, name, "Location"),
                                                        facing  (object, name, "Facing"),
                                                        velocity(object, name, "Velocity"),
                                                        rotvel  (object, name, "RotVel")
        { /* fnord */ }

protected:
    /* ------------------------------------------------------------------------
     *  Initialisation related
     */

    /** Initialise the TWTrapPhysStateCtrl instance. This parses the various
     *  parameters from the design note, and sets up the script so that
     *  it can be used correctly.
     */
    void init(int time);


    /* ------------------------------------------------------------------------
     *  Message handling
     */

    /** TurnOn message handler, called whenever the script receives a TurnOn message.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_onmsg(sScrMsg* msg, cMultiParm& reply);

private:
    /** Update the TWTrapPhysStateControl instance. This parses the various
     *  parameters from the design note, and updates the linked object(s).
     */
    void update();


    /** Link iterator callback used to set the physics state of objects. This
     *  is used to set the location, facing, velocity, and rotational velocity
     *  of objects linked to the script object via ControlDevice links.
     *
     * @param link_query  A pointer to the link query for the current call.
     * @param data        A pointer to a structure containing the physics state settings.
     * @return Always returns 1.
     */
    static int set_state(ILinkSrv*, ILinkQuery* link_query, IScript* script, void* data);

    DesignParamFloatVec location;
    DesignParamFloatVec facing;
    DesignParamFloatVec velocity;
    DesignParamFloatVec rotvel;
};


#else // SCR_GENSCRIPTS

GEN_FACTORY("TWTrapPhysStateCtrl", "TWBaseTrap", TWTrapPhysStateCtrl)

#endif // SCR_GENSCRIPTS

#endif // TWTRAPPHYSSTATECTRL_H
