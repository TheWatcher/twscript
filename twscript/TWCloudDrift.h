/** @file
 * This file contains the interface for the TWTrapSetSpeed classe.
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

#ifndef TWCLOUDDRIFT_H
#define TWCLOUDDRIFT_H

#if !SCR_GENSCRIPTS
#include <lg/interface.h>
#include <lg/scrmanagers.h>
#include <lg/scrservices.h>
#include <lg/objects.h>
#include <lg/links.h>
#include <lg/properties.h>
#include <lg/propdefs.h>
#include <string>
#include "scriptvars.h"
#include "TWBaseScript.h"

/** @class TWCloudDrift
 *
 * Configuration
 * -------------
 *
 * The script params are specified using the Editor -> Design Note, please see the
 * main documentation for more about this.  Parameters supported by TWCloudDrift
 * are listed below. If a parameter is not specified, the default value shown is
 * used instead. Note that most parameters are optional, and if you do not
 * specify a parameter, the script will attempt to use a 'sane' default.
 *
 * Parameter: TWCloudDriftRange
 *      Type: float vector
 *   Default: none (must be supplied)
 * This parameter must be supplied. If it is not given, no drift control will be
 * performed. This specifies the distance the cloud can drift on the x, y, and z
 * axis, centred around the start position of the cloud.
 *
 * Parameter: TWCloudDriftRange
 *      Type: float vector
 *   Default: none (must be supplied)
 *
 */
class TWCloudDrift : public TWBaseScript
{

public:
    /** Possible scaling factor modes for velocity updates
     */
    enum FactorMode {
        FIXEDMIN = 0, //!< Always use the minimum speed.
        LINEAR,       //!< Scale the velocity linearly based on the distance from the start location.
        LOGARITHMIC,  //!< Scalw logarithmically based on distance from the start.
    };

    TWCloudDrift(const char* name, int object) : TWBaseScript(name, object), driftrange(), maxrates(), minrates(), refresh(0), factormode(FIXEDMIN),
                                                 SCRIPT_VAROBJ(TWCloudDrift, start_position, object),
                                                 SCRIPT_VAROBJ(TWCloudDrift, update_timer, object)
        { /* fnord */ }

protected:
    /* ------------------------------------------------------------------------
     *  Initialisation related
     */

    /** Initialise the TWCloudDrift instance. This parses the various
     *  parameters from the design note, and sets up the script so that
     *  it can be used correctly.
     *
     * @param time The current sim time.
     */
    void init(int time);


    /* ------------------------------------------------------------------------
     *  Message handling
     */

    /** Handle messages passed to the script. This is invoked whenever the
     *  script receives a message, and subclasses of this class will generally
     *  override or extend this function to provide script-specific behaviour.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_message(sScrMsg* msg, cMultiParm& reply);


    /** Timer message handler, called whenever the script receives a timer message.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_timer(sScrTimerMsg *msg, cMultiParm& reply);

private:
    /** Store the initial location of the object, required to determine the
     *  velocity based on the distance from initial location.
     */
    void fetch_initial_location(void);


    /** Calculate the speed to use based on the distance from the initial
     *  location and the minimum and maximum rates supported.
     *
     * @param center   The initial location.
     * @param range    The maximum distance from the initial location.
     * @param minrate  The slowest speed supported.
     * @param maxrate  The fastest speed supported.
     * @param position The current location of the object.
     * @param velocity The current speed and direction.
     * @return The new velocity to use.
     */
    float calculate_velocity(float centre, float range, float minrate, float maxrate, float position, float velocity);


    /** Determine whether the velocity of the cloud needs updating, and adjust
     *  it accordingly. Once the velocity has been updated, start the update
     *  timer.
     *
     * @param time The current sim time.
     */
    void check_velocities(int time);

    // DesignNote configured options
    cScrVec    driftrange;         //!< How far should the cloud be able to drift?
    cScrVec    maxrates;           //!< How fast can the cloud travel?
    cScrVec    minrates;           //!< How slow can the cloud travel?
    int        refresh;            //!< How frequently should the speed be updated?
    FactorMode factormode;         //!< How should the speed behave based on position?

    // Persistent variables (current location and velocity are handled by the game)
    script_vec               start_position;  //!< The initial location of the cloud
    script_handle<tScrTimer> update_timer;    //!< A timer used to update the cloud speed
};

#else // SCR_GENSCRIPTS

GEN_FACTORY("TWCloudDrift", "TWBaseScript", TWCloudDrift)

#endif // SCR_GENSCRIPTS

#endif // TWCLOUDDRIFT_H
