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

#ifndef TWTRAPSETSPEED_H
#define TWTRAPSETSPEED_H

#if !SCR_GENSCRIPTS
#include <lg/interface.h>
#include <lg/scrmanagers.h>
#include <lg/scrservices.h>
#include <lg/objects.h>
#include <lg/links.h>
#include <lg/properties.h>
#include <lg/propdefs.h>
#include <string>
#include "TWBaseScript.h"
#include "TWBaseTrap.h"

/** @class TWTrapSetSpeed
 *
 * TWTrapSetSpeed allows the game-time modification of TPath speed settings.
 * This script lets you control how fast a vator moves between TerrPts on the
 * fly - add it to an object, set the TWTrapSetSpeed and TWTrapSetSpeedDest params
 * documented below, and then send a TurnOn message to the object when you
 * want it to apply the speed to the destination.
 *
 * By default, the speed changes made by this script will not be picked up by
 * and moving terrain objects moving between TerrPts until they reach their next
 * waypoint. However, if you want the speed of any moving terrain object to be
 * updated by this script before it reaches the next TerrPt, link the object
 * this script is placed on to the moving terrain object with a ScriptParams link,
 * and set the data for the link to "SetSpeed". This link is needed to get the moving
 * terrain to start moving from a stop (speed = 0).
 *
 * Configuration
 * -------------
 * Parameters are specified using the Editor -> Design Note, please see the
 * main documentation for more about this.  Parameters supported by TWTrapSetSpeed
 * are listed below. If a parameter is not specified, the default value shown is
 * used instead. Note that all the parameters are optional, and if you do not
 * specify a parameter, the script will attempt to use a 'sane' default.
 *
 * Parameter: TWTrapSetSpeedSpeed   (Yes, that is 'Speed' twice.)
 *      Type: float
 *   Default: 0.0
 * The speed to set the target objects' TPath speed values to when triggered. All
 * TPath links on the target object are updated to reflect the speed given here.
 * The value provided for this parameter may be taken from a QVar by placing a $
 * before the QVar name, eg: `TWTrapSetSpeed='$speed_var'`. If you set a QVar
 * as the speed source in this way, each time the script receives a TurnOn, it
 * will read the value out of the QVar and then copy it to the destination object(s).
 * Using a simple QVar as in the previous example will restrict your speeds to
 * integer values; if you need fractional speeds, you can include a simple
 * calculation after the QVar name to scale it, for example,
 * `TWTrapSetSpeed='$speed_var / 10'` will divide the value in speed_var by 10,
 * so if `speed_var` contains 55, the speed set by the script will be 5.5. You
 * can even specify a QVar as the second operand if needed, again by prefixing
 * the name with '$', eg: `TWTrapSetSpeed='$speed_var / $speed_div'`.
 *
 * Parameter: TWTrapSetSpeedWatchQVar
 *      Type: boolean
 *   Default: false
 * If TWTrapSetSpeed is set to read the speed from a QVar, you can make the
 * script trigger whenever the QVar is changed by setting this to true. Note
 * that this will only watch changes to the first QVar specified in TWTrapSetSpeed:
 * if you set `TWTrapSetSpeed='$speed_var / $speed_div'` then changes to speed_var
 * will be picked up, but any changes to speed_div will not trigger this script.
 *
 * Parameter: TWTrapSetSpeedDest
 *      Type: string
 *   Default: [me]
 * Specify the target object(s) to update when triggered. This can either be
 * an object name, [me] to update the object the script is on, [source] to update
 * the object that triggered the change (if you need that, for some odd reason),
 * or you may specify an archetype name preceeded by * or @ to update all objects
 * that inherit from the specified archetype. If you use *Archetype then only
 * concrete objects that directly inherit from that archetype are updated, if you
 * use @Archetype then all concrete objects that inherit from the archetype
 * directly or indirectly are updated.
 *
 * Parameter: TWTrapSetSpeedImmediate
 *      Type: boolean
 *   Default: false
 * If this is set to true, the speed of any linked moving terrain objects is immediately
 * set to the speed value applied to the TerrPts. If it is false, the moving terrain
 * object will smoothly change its speed to the new speed (essentially, setting this
 * to true breaks the appearance of momentum and inertia on the moving object. It is
 * very rare that you will want to set this to true.)
 */
class TWTrapSetSpeed : public TWBaseTrap
{
public:
    TWTrapSetSpeed(const char* name, int object) : TWBaseTrap(name, object),
                                                   speed(object, name, "Speed"),
                                                   intensity(object, name, "Intensity"),
                                                   subscribe(object, name, "WatchQVar"),
                                                   immediate(object, name, "Immediate"),
                                                   set_target(object, name, "Dest"),
                                                   set_speed(0.0f)
        { /* fnord */ }

protected:
    /* ------------------------------------------------------------------------
     *  Initialisation related
     */

    /** Initialise the TWTrapSetSpeed instance. This parses the various
     *  parameters from the design note, and sets up the script so that
     *  it can be used correctly.
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


    /** On message handler, called whenever the script receives an on message.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_onmsg(sScrMsg* msg, cMultiParm& reply);


    /** QuestChange message handler, called whenever the script receives a QuestChange message.
     *
     * @param msg   A pointer to the message received by the object.
     * @param reply A reference to a multiparm variable in which a reply can
     *              be stored.
     * @return A status value indicating whether the caller should continue
     *         processing the message
     */
    MsgStatus on_questchange(sQuestMsg* msg, cMultiParm& reply);

private:
    /** Update the speed set on any selected destination object(s) and linked
     *  moving terrain object(s). This is the function that does most of the
     *  work of actually updating TerrPts and so on to reflect the currently
     *  set speed. It will update the speed setting if the TWTrapSetSpeed
     *  design note parameter contains a QVar.
     *
     * @param msg A pointer to the message that triggered the update.
     */
    void update_speed(sScrMsg* msg);


    /** Update the speed set on an individual TerrPt's TPath links.
     *
     * @param obj_id The TerrPt object to update the TPath links on.
     */
    void set_tpath_speed(object obj_id);


    /** Link iterator callback used to set the speed of moving terrain objects.
     *  This allows the speed of moving terrain objects to be set on the fly,
     *  either with immediate effect or allowing the physics system to change
     *  the speed smoothly.
     *
     * @param link_query A pointer to the link query for the current call.
     * @param data       A pointer to a structure containing the speed and other settings.
     * @return Always returns 1.
     */
    static int set_mterr_speed(ILinkSrv*, ILinkQuery* link_query, IScript* script, void*);


    /* ------------------------------------------------------------------------
     *  Variables
     */

    DesignParamFloat  speed;      //!< User-defined speed to set on targets and linked vators.
    DesignParamBool   intensity;  //!< If true, the speed is derived from the intensity of a stim message.
    DesignParamBool   subscribe;  //!< Subscribe to qvar changes?
    DesignParamBool   immediate;  //!< If true, vator speed changes are instant.
    DesignParamTarget set_target; //!< The target string set by the user.

    float set_speed; //!< Speed cache

};

#else // SCR_GENSCRIPTS

GEN_FACTORY("TWTrapSetSpeed"     , "TWBaseTrap", TWTrapSetSpeed)

#endif // SCR_GENSCRIPTS

#endif // TWTRAPSETSPEED_H
