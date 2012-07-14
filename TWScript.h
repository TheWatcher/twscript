#ifndef TWSCRIPT_H
#define TWSCRIPT_H

#if !SCR_GENSCRIPTS
#include "BaseScript.h"
#include "BaseTrap.h"

#include "scriptvars.h"
#include "darkhook.h"

#include <lg/scrservices.h>
#include <lg/links.h>

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

/**
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
class cScr_TWTweqSmooth : public cBaseScript
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

    /** Obtain a string containing the specified object's name (or archetype name),
     *  and its ID number. This has been lifted pretty much verbatim from Telliamed's
     *  Spy script - it is used to generate the object name and ID when writing
     *  debug messages.
     *
     * @param obj_id The ID of the object to obtain the name and number of.
     * @return A string containing the object name
     */
    cAnsiStr get_object_namestr(object obj_id);

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

#else // SCR_GENSCRIPTS
GEN_FACTORY("TWTweqSmooth", "BaseScript", cScr_TWTweqSmooth)
#endif // SCR_GENSCRIPTS

#endif // TWSCRIPT_H
