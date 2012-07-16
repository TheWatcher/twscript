#ifndef TWSCRIPT_H
#define TWSCRIPT_H

#if !SCR_GENSCRIPTS
#include "BaseScript.h"
#include "BaseTrap.h"

#include "scriptvars.h"
#include "darkhook.h"

#include <lg/scrservices.h>
#include <lg/links.h>
#include <vector>

/** AnimC bit field flags. No idea why this isn't in lg/defs.h, but hey...
 */
enum eAnimCFlags {
    kNoLimit   = 0x01, // ignore the limits (so I dont have to set them)
    kSim       = 0x02, // update me continually - else just update when I was on screen
    kWrap      = 0x04, // Wrap from low to high, else bounce
    k1Bounce   = 0x08, // Bounce off the top, then down and stop
    kSimSmall  = 0x10, // update if within a small radius
    kSimLarge  = 0x20, // update if within a large radius
    kOffScreen = 0x40  // only run if I'm offscreen
};

typedef struct
{
   float speed;
   int   pause;
   bool  path_limit;

   int   cur_paused;
} sTerrainPath;


class TWScript
{
protected:
    /** Obtain a string containing the specified object's name (or archetype name),
     *  and its ID number. This has been lifted pretty much verbatim from Telliamed's
     *  Spy script - it is used to generate the object name and ID when writing
     *  debug messages.
     *
     * @param obj_id The ID of the object to obtain the name and number of.
     * @return A string containing the object name
     */
    cAnsiStr get_object_namestr(object obj_id);


    /** Given a destination string, generate a list of object ids the destination
     *  corresponds to. If dest is '[me]', the current object is returned, if dest
     *  is '[source]' the source object is returned, if the dest is an object
     *  id or name, the id of that object is returned. If dest starts with * then
     *  the remainder of the string is used as an archetype name and all direct
     *  concrete descendents of that archetype are returned. If dest starts with
     *  @ then all concrete descendants (direct and indirect) are returned.
     *
     * @param pMsg A pointer to a script message containing the to and from objects.
     * @param dest The destination string
     * @return A vector of object ids the destination matches.
     */
    std::vector<object>* get_target_objects(char *dest, sScrMsg *pMsg = NULL);

private:
    /** Determine whether the specified dest string is a radius search, and if so
     *  pull out its components. This will take a string like 5.00<Chest and set
     *  the radius to 5.0 and set the archetype string pointer to the start of the
     *  archetype name.
     *
     * @param dest      The dest string to check
     * @param radius    A pointer to a float to store the radius value in.
     * @param lessthan  A pointer to a bool. If the radius search is a < search
     *                  this is set to true, otherwise it is set to false.
     * @param archetype A pointer to a char pointer to set to the start of the
     *                  archetype name.
     * @return true if the dest string is a radius search, false otherwise.
     */
    bool radius_search(char *dest, float *radius, bool *lessthan, char **archetype);


    /** Search for concrete objects that are descendants of the specified archetype,
     *  either direct only (if do_full is false), or directly and indirectly. This
     *  can also filter the results based on the distance the concrete objects are
     *  from the specified object.
     *
     * @param matches   A pointer to the vector to store object ids in.
     * @param archetype The name of the archetype to search for. *Must not* include
     *                  and filtering (* or @) directives.
     * @param do_full   If false, only concrete objects that are direct descendants of
     *                  the archetype are matched. If true, all concrete objects that
     *                  are descendants of the archetype, or any descendant of that
     *                  archetype, are matched.
     * @param do_radius If false, concrete objects are matched regardless of distance
     *                  from the `from_obj`. If true, objects must be either inside
     *                  the specified radius from the `from_obj`, or outside out depending
     *                  on the `lessthan` flag.
     * @param from_obj  When filtering objects based on their distance, this is the
     *                  object that distance is measured from.
     * @param radius    The radius of the sphere that matched objects must fall inside
     *                  or outside.
     * @param lessthan  If true, objects must fall within the sphere around from_obj,
     *                  if false they must be outside it.
     */
    void archetype_search(std::vector<object> *matches, char *archetype, bool do_full, bool do_radius = false, object from_obj = 0, float radius = 0.0f, bool lessthan = false);

};


/** @class cScr_TWTweqSmooth
 *
 * TWTweqSmooth allows the oscillating rotation of objects or joints to be
 * 'smoothed' over time, removing the hard, obvious direction changes otherwise
 * encountered. This can be used to create a number of different effects, but
 * is especially useful when simulating pendulum-like movement of objects or
 * subobjects.
 *
 * - the minimum tweq rate for each object axis or joint is determined by
 *   optional parameters set in the Editor -> Design Note. If minimum rates
 *   are not specified, a global minimum is used. If no global minimum has
 *   been specified, a fall-back default is used instead.
 * - the maximum tweq rate for each axis or joint is taken from the rate set
 *   in the Tweq -> Rotate or Tweq -> Joints settings. Essentially, the rate
 *   you would normally set to control a tweq in Dromed is used as the maximum
 *   rate for a smoothed tweq.
 * - if an axis or joint has a rate set that is less than or equal to the
 *   minimum rate, its movement will not be smoothed (ie: the maximum rate must
 *   be greater than the minimum rate).
 * - if an axis or joint has the same value for its low and high, or the low
 *   is greater than the high, its movement will not be smoothed.
 * - if the AnimC for a Tweq -> Rotate, or for any joint, has 'NoLimit' or
 *   'Wrap' set, no smoothing can be done.
 *
 * If warnings are enabled (see TWTweqSmoothWarn below), warnings will be written
 * to the monolog when the script has to disable smoothing on an axis or joint.
 *
 * Configuration
 * -------------
 * Parameters are specified using the Editor -> Design Note, please see the
 * main documentation for more about this.  Parameters supported by TWTweqSmooth
 * are listed below. If a parameter is not specified, the default value shown is
 * used instead. Note that all the parameters are optional, and if you do not
 * specify a parameter, the script will attempt to use a 'sane' default.
 *
 * Paramater: TWTweqSmoothTimerRate
 *      Type: integer
 *   Default: 250
 * The delay in milliseconds between tweq rate updates. This setting involves a
 * trade-off between performance and appearance: reducing this value (making the
 * delay between updates shorter) will make the rate adjustment smoother, but
 * it will also place more load on the engine. The default value is simply provided
 * as a starting point, and you will need to tweak it to suit the situation in
 * which the script is being used. Note that very small values should only be used
 * with Extreme Care.
 *
 * Parameter: TWTweqSmoothMinRate
 *      Type: real
 *   Default: 0.1
 * This allows you to set the default minimum rate for all the other rate controls.
 * For example, if you do not specify a value for TWTweqSmoothRotateXMin, the
 * script will use the value you set for TWTweqSmoothMinRate, falling back on the
 * built-in default of 0.1 if neither TWTweqSmoothMinRate or TWTweqSmoothRotateXMin
 * are set.
 *
 * Parameter: TWTweqSmoothRotate
 *      Type: string (comma separated values)
 *   Default: all
 * Provides control over the smoothing of rotation on different axes for objects that
 * have Tweq -> Rotate set. If not specified, all the axes are selected for smoothing.
 * If you provide a string, it should either be TWTweqSmoothRotate='none' to completely
 * turn off smoothing of rotation, or a comma separated list of axes to smooth rotation
 * on, for example TWTweqSmoothRotate='X,Z' will select the X and Z axes for smoothing.
 *
 * Parameter: TWTweqSmoothRotateXMin, TWTweqSmoothRotateYMin, TWTweqSmoothRotateZMin
 *      Type: real
 *   Default: TWTweqSmoothMinRate
 * Lets you individually set the minimum rates for each rotation axis. If an axis is not
 * set, the default minimum rate is used instead. If a minimum rate is specified for
 * an axis that is not selected for rotation smoothing by TWTweqSmoothRotate it will be
 * ignored.
 *
 * Parameter: TWTweqSmoothJoints
 *      Type: string (comma separated values)
 *   Default: all
 * Allows for control over the smoothing of individual joint movement on objects that
 * have Tweq -> Joints set. If not set, all joints
 */
class cScr_TWTweqSmooth : public cBaseScript, public TWScript
{
public:

    enum RotAxis {
        RotXAxis,
        RotYAxis,
        RotZAxis,
        RotAxisMax
    };

    enum RateMode {
        MinRate,
        MaxRate,
        RateModeMax
    };

	cScr_TWTweqSmooth(const char* pszName, int iHostObjId)
		: cBaseScript(pszName, iHostObjId)
    { }

protected:
	virtual long OnSim(sSimMsg*, cMultiParm&);
	virtual long OnTimer(sScrTimerMsg* pTimerMsg, cMultiParm& mpReply);

private:
    /** Determine whether the AnimC flags enabled in the specified value would
     *  prevent the correct smoothing of the tweq. This will check whether either
     *  the NoLimit or Wrap flags are set, and if they are it will return false
     *  (ie: incompatible).
     *
     * @param animc The integer containing the AnimC flags to check.
     * @return true if the flags are compatible with tweq smoothing, false otherwise.
     */
    inline bool compatible_animc(const int animc) {
        return !( animc & kNoLimit || animc & kWrap);
    };

    /** Fetch the rate set for a given axis on an object with Tweq -> Rotate set.
     *
     * @param axis The axis to fetch the rate for.
     * @return The tweq rate.
     */
    float get_rotate_rate(RotAxis axis);

    /** Determine whether the low and high values set for the specified axis on
     *  and object with Tweq -> Rotate set are valid. This will check that
     *  low and high are both positive, and that high is greater than low.
     *
     * @param axis The axis to check for bounds validity.
     * @return true if the axis bounds are valid, false otherwise.
     */
    bool valid_rotate_bounds(RotAxis axis);


    float get_joint_rate(int joint);
    float get_rate_param(const char *design_note, const char *cfgname, float default_value, float minimum = 0.0f);

    void init();
    int  init_rotate_onoffctrl(char *axes);
    void init_rotate(char *design_note);
    int  init_joints_onoffctrl(char *joints);
    void init_joints(char *design_note);

    void start_timer();
    void clear_timer();

    void set_axis_rate(const char *propname, RotAxis axis);

    bool      warnings;   //!< Show warning messages in monolog?

    // Timer-related variables.
    tScrTimer timer;      //!< The currently active timer for this object, or NULL.
    int       timer_rate; //!< The update rate, in milliseconds.

    // Default rates (which themselves have defaults!)
    float     min_rate;   //!< Default min rate for axes/joints that do not specify one
    float     max_rate;   //!< Default max rate for axes/joints that do not specify one

    // TweqRotate settings
    bool      do_tweq_rotate;                        //!< Should the rotate tweq be smoothed?
    bool      axis_smooth[RotAxisMax];               //!< Which axes should be smoothed?
    float     rotate_rates[RotAxisMax][RateModeMax]; //!< Store the per-axis rate min/max values

    // TweqJoints settings
    /** Anonymous enum hack, used to avoid the need for another static const int
     *  just to define the joint_* array sizes.
     */
    enum {
        JointCount = 6  //!< How many joints should we support? Dark supports 6.
    };
    bool      do_tweq_joints;                       //!< Should the joint tweq be smoothed?
    bool      joint_smooth[JointCount];             //!< Should individual joints be smoothed?
    float     joint_rates[JointCount][RateModeMax]; //!< Store the per-joint rate min/max values.

    static const char *axis_names[RotAxisMax];
    static const char *joint_names[JointCount];
};


/** @class cScr_TWTrapSetSpeed
 *
 * TWTrapSetSpeed allows the run-time modification of TPath speed settings.
 * This script lets you control how fast a vator moves between TerrPts on the
 * fly - add it to an object, set the TWTrapSetSpeed and TWTrapSetSpeedDest params
 * documented below, and then send a TurnOn message to the object when you
 * want it to apply the speed to the destination.
 *
 * @note This script currently only allows for modification of speeds set on
 * TerrPts, *not* directly controlling the speed of the MovingTerrain object
 * moving between them. What this means is that the speed of the MovingTerrain
 * will not change *until it reaches another TerrPt*, at which point it will pick
 * up the new speed value. You should keep this in mind when distributing your
 * TerrPts, and possibly add in 'redundant' points to help reflect speed changes
 * more rapidly if needed.
 *
 * Configuration
 * -------------
 * Parameters are specified using the Editor -> Design Note, please see the
 * main documentation for more about this.  Parameters supported by TWTweqSmooth
 * are listed below. If a parameter is not specified, the default value shown is
 * used instead. Note that all the parameters are optional, and if you do not
 * specify a parameter, the script will attempt to use a 'sane' default.
 *
 * Parameter: TWTrapSetSpeed
 *      Type: float
 *   Default: 0.0
 * The speed to set the target object's TPath speed value to when triggered. All
 * TPath links on the target object are updated to reflect the speed given here.
 *
 * Parameter: TWTrapSetSpeedQVar
 *      Type: string
 *   Default: none
 * The downside to using the TWTrapSetSpeed parameter is that the value you set is
 * fixed per object. While different objects can have different TWTrapSetSpeed
 * values, if you have a wide variety of speeds needed, you may need a lot of
 * objects and methods to trigger the right one. By using this parameter, you can
 * specify a QVar to read the speed from - each time you send a TurnOn to an
 * object with this script on it, and TWTrapSetSpeedQVar set, it will read the
 * value out of the QVar and then copy it to the destination object(s). The downside
 * to this parameter is that QVars may only store integer values, so you can not
 * use this if you need fractional speed control.
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
 */
class cScr_TWTrapSetSpeed : public cBaseTrap, public TWScript
{
public:
    cScr_TWTrapSetSpeed(const char* pszName, int iHostObjId)
		: cBaseTrap(pszName, iHostObjId)
    { }

protected:
	virtual long OnTurnOn (sScrMsg* pMsg, cMultiParm& mpReply);

private:
    void set_speed(object obj_id, float speed);
    static int link_iter(ILinkSrv* linksrv, ILinkQuery* query, IScript* script, void *data);
};

#else // SCR_GENSCRIPTS

GEN_FACTORY("TWTweqSmooth"  , "BaseScript", cScr_TWTweqSmooth)
GEN_FACTORY("TWTrapSetSpeed", "BaseTrap"  , cScr_TWTrapSetSpeed)

#endif // SCR_GENSCRIPTS

#endif // TWSCRIPT_H
