#ifndef TWSCRIPT_H
#define TWSCRIPT_H

#if !SCR_GENSCRIPTS
#include "BaseScript.h"
#include "BaseTrap.h"

#include "scriptvars.h"
#include "darkhook.h"

#include <lg/scrservices.h>
#include <lg/links.h>

enum RotAxes {
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

/**
 * Script: TWTweqSmooth
 *
 */
class cScr_TWTweqSmooth : public cBaseScript
{
public:
	cScr_TWTweqSmooth(const char* pszName, int iHostObjId)
		: cBaseScript(pszName, iHostObjId)
    { }

protected:
	virtual long OnSim(sSimMsg*, cMultiParm&);
	virtual long OnTimer(sScrTimerMsg* pTimerMsg, cMultiParm& mpReply);

private:
    float get_rate(const char *design_note, const char *cfgname, float default_value, float minimum = 0.0f);
    void init();
    void init_rotate(const char *design_note);
    void init_joints(const char *design_note);

    void start_timer();
    void clear_timer();

    void set_axis_rate(const char *propname, ePhysAxes axis);

    // Timer-related variables.
    tScrTimer timer;      //!< The currently active timer for this object, or NULL.
    int       timer_rate; //!< The update rate, in milliseconds.

    // Default rates (which themselves have defaults!)
    float     min_rate;   //!< Default min rate for axes/joints that do not specify one
    float     max_rate;   //!< Default max rate for axes/joints that do not specify one

    // TweqRotate settings
    bool      do_tweq_rotate;                        //!< Should the rotate tweq be smoothed?
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

    //! How big is the buffer used to store joint configuration names?
    static const unsigned int CFGBUFFER_SIZE;
};

#else // SCR_GENSCRIPTS
GEN_FACTORY("TWTweqSmooth", "BaseScript", cScr_TWTweqSmooth)
#endif // SCR_GENSCRIPTS

#endif // TWSCRIPT_H
