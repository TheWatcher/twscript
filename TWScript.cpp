
#include "TWScript.h"
#include "ScriptModule.h"

#include <lg/interface.h>
#include <lg/scrmanagers.h>
#include <lg/scrservices.h>
#include <lg/objects.h>
#include <lg/links.h>
#include <lg/properties.h>
#include <lg/propdefs.h>

#include "ScriptLib.h"
#include "utils.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cctype>
#include <list>
#include <utility>
#include <algorithm>
#include <stdexcept>
#include <memory>

const unsigned int cScr_TWTweqSmooth::CFGBUFFER_SIZE = 22;

const char *cScr_TWTweqSmooth::axis_names[] = {
    "x rate-low-high",
    "y rate-low-high",
    "z rate-low-high"
};


void cScr_TWTweqSmooth::start_timer()
{
    timer = SetTimedMessage("TWRateUpdate", timer_rate, kSTM_OneShot, "TWScripts");
}


void cScr_TWTweqSmooth::clear_timer()
{
    if(timer) {
        KillTimedMessage(timer);
        timer = NULL;
    }

}


float cScr_TWTweqSmooth::get_rate(const char *design_note, const char *cfgname, float default_value, float minimum)
{
    float value = GetParamFloat(design_note, cfgname, default_value);

    if(value < minimum) value = minimum;

    return value;
}


int cScr_TWTweqSmooth::init_rotate_onoffctrl(char *axes)
{
    // Handle being passed nothing, or an empty string
    if(!axes || !*axes) return 0;

    bool all  = !strcasecmp(axes, "all");
    int count = 0;

    // First initialise the axes either to all on (if all is set), or
    // all off if it is not.
    for(int axis = 0; axis < RotAxisMax; ++axis) {
        axis_smooth[axis] = all;
        if(all) ++count;
    }

    // If all is not set, check which axes have been selected for smoothing, if any
    if(!all) {

        // Check the whole axes string for names, this will ignore any characters
        // other than upper or lower case x, y, or z.
        while(*axes) {

            // Realistically, this could probably update *axes, but the temp won't hurt.
            char current = tolower(*axes);
            if(current >= 'x' && current <= 'z') {

                // Only change the axes, and count the change, the first time.
                if(!axis_smooth[current - 'x']) {
                    axis_smooth[current - 'x'] = true;
                    ++count;
                }
            }
            ++axes;
        }
    }

    return count;
}


void cScr_TWTweqSmooth::init_rotate(char *design_note)
{
    SService<IPropertySrv> pPS(g_pScriptManager);

    // Find out whether any axes have been selected for smoothing
    int set_count = init_rotate_onoffctrl(GetParamString(design_note, "TWTweqSmoothRotate", "all"));

    // Does the object have a rotate tweq, configuration, and the matching rotate state?
    // This also allows the user to suppress smoothing on an object's rotate if needed.
    do_tweq_rotate = (pPS -> Possessed(ObjId(), "CfgTweqRotate") &&
                      pPS -> Possessed(ObjId(), "StTweqRotate") &&
                      set_count);

    // If the rotate is being smoothed, obtain maximum and minimum rates for each axis.
    // If the user hasn't specified them, the global defaults are used instead.
    if(do_tweq_rotate) {
        rotate_rates[RotXAxis][MinRate] = get_rate(design_note, "TWTweqSmoothRotateXMin", min_rate);
        rotate_rates[RotXAxis][MaxRate] = get_rate(design_note, "TWTweqSmoothRotateXMax", max_rate, rotate_rates[RotXAxis][MinRate]);
        rotate_rates[RotYAxis][MinRate] = get_rate(design_note, "TWTweqSmoothRotateYMin", min_rate);
        rotate_rates[RotYAxis][MaxRate] = get_rate(design_note, "TWTweqSmoothRotateYMax", max_rate, rotate_rates[RotYAxis][MinRate]);
        rotate_rates[RotZAxis][MinRate] = get_rate(design_note, "TWTweqSmoothRotateZMin", min_rate);
        rotate_rates[RotZAxis][MaxRate] = get_rate(design_note, "TWTweqSmoothRotateZMax", max_rate, rotate_rates[RotZAxis][MinRate]);
    }
}


int cScr_TWTweqSmooth::init_joints_onoffctrl(char *joints)
{
    // Handle being passed nothing, or an empty string
    if(!joints || !*joints) return 0;

    bool all  = !strcasecmp(joints, "all");
    int count = 0;

    // First initialise the joints either to all on (if all is set), or
    // all off if it is not.
    for(int joint = 0; joint < JointCount; ++joint) {
        joint_smooth[joint] = all;
        if(all) ++count;
    }

    // If all is not set, individual joints have been selected for smoothing.
    if(!all) {
        char *cont = joints;

        // This loops over the joints string, parsing out comma separated joint
        // numbers and enabling the joints encountered.
        while(*cont) {
            int joint = strtol(joints, &cont, 10);
            if(cont == joints) break;    // Halt if parse failed
            if(*cont) joints = cont + 1; // Keep going if not at the end of string

            // If the joint specified by the user is in range, enable it
            if(joint >= 0 && joint < JointCount) {
                joint_smooth[joint] = true;
                ++count;
            }
        }
    }

    return count;
}


void cScr_TWTweqSmooth::init_joints(char *design_note)
{
    SService<IPropertySrv> pPS(g_pScriptManager);

    // Find out how many joints have been selected for smoothing
    int set_count = init_joints_onoffctrl(GetParamString(design_note, "TWTweqSmoothJoints", "all"));

    // Only smooth joints if there are joints to smooth, and one or more are set for smoothing
    do_tweq_joints = (pPS -> Possessed(ObjId(), "CfgTweqJoints") &&
                      pPS -> Possessed(ObjId(), "StTweqJoints") &&
                      set_count);

    // If joint smoothing is enabled, the minimum and maximum rates for each joint
    // need to be set.
    if(do_tweq_joints) {
        char cfg_buffer[CFGBUFFER_SIZE];

        // Each joint can have separate minimum and maximum rates, and they default to
        // the global min and max if not set.
        for(int joint = 0; joint < JointCount; ++joint) {
            snprintf(cfg_buffer, CFGBUFFER_SIZE, "TWTweqSmoothJoint%1dMin", joint);
            joint_rates[joint][MinRate] = get_rate(design_note, cfg_buffer, min_rate);

            snprintf(cfg_buffer, CFGBUFFER_SIZE, "TWTweqSmoothJoint%1dMax", joint);
            joint_rates[joint][MaxRate] = get_rate(design_note, cfg_buffer, max_rate, joint_rates[joint][MinRate]);
        }
    }
}


void cScr_TWTweqSmooth::init()
{
    // Ensure that the timer can't fire while re-initialising
    clear_timer();

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    // How frequently should the timer update the tweq rate?
    timer_rate = GetParamInt(design_note, "TWTweqSmoothTimerRate", 250);

    // Has the editor specified defaults for the min and max rates?
    min_rate = get_rate(design_note, "TWTweqSmoothMinRate", 0.1f);
    max_rate = get_rate(design_note, "TWTweqSmoothMaxRate", 10.0f, min_rate); // note: force max >= min.

    // Now we need to determine whether the object has a rotate tweq, and joint
    // tweq, or both, and set up the smoothing facility accordingly.
    init_rotate(design_note);
    init_joints(design_note);

    // Now set up the timer to fire to do the actual tweq smoothing
    start_timer();
}


void cScr_TWTweqSmooth::set_axis_rate(const char* propname, RotAxes axis)
{
    cMultiParm prop;
    float rate, low, high;

    SService<IPropertySrv> pPS(g_pScriptManager);

    pPS -> Get(prop, ObjId(), "CfgTweqRotate", propname);
    low  = static_cast<const mxs_vector*>(prop) -> y; // low
    high = static_cast<const mxs_vector*>(prop) -> z; // high

    // If either the low or high are non-zero, update the rate...
    if((low || high) && (low != high)) {
        // Where is the object facing?
        cScrVec facing;
        SService<IObjectSrv> pOS(g_pScriptManager);

        // Fetch the direction in degrees
        pOS -> Facing(facing, ObjId());

        float current;
        switch(axis) {
            case RotXAxis: current = facing.x; break;
            case RotYAxis: current = facing.y; break;
            case RotZAxis: current = facing.z; break;
            case RotAxisMax:
            default: current = low;
        }

        // Rate is a function of the current angle within the range
        rate = max_rate * sin(((current - low) * M_PI) / (high - low));

            //SService<IDebugScrSrv> pDSS(g_pScriptManager);
            //cAnsiStr outstr;
            //outstr.FmtStr(999, "EaseRotate: min = %0.2f; max = %0.2f; c = %0.2f; rate = %0.2f", low, high, current, rate);
            //pDSS->MPrint(static_cast<const char*>(outstr), cScrStr::Null,cScrStr::Null,cScrStr::Null,cScrStr::Null,cScrStr::Null,cScrStr::Null,cScrStr::Null);

        // Clamp the rate. The upper setting should never be needed, but check anyway
        if(rate < min_rate) rate = min_rate;
        if(rate > max_rate) rate = max_rate;

        // Now update the object
        cScrVec newprop;
        newprop.x = rate;
        newprop.y = low;
        newprop.z = high;
        prop = newprop;
        pPS -> Set(ObjId(), "CfgTweqRotate", propname, prop);
    }
}


long cScr_TWTweqSmooth::OnSim(sSimMsg* pSimMsg, cMultiParm& mpReply)
{
    if(pSimMsg -> fStarting) init();

    return cBaseScript::OnSim(pSimMsg, mpReply);
}


long cScr_TWTweqSmooth::OnTimer(sScrTimerMsg* pTimerMsg, cMultiParm& mpReply)
{
    if(!strcmp(pTimerMsg -> name, "TWRateUpdate") &&
       (pTimerMsg->data.type == kMT_String) &&
       !stricmp(pTimerMsg -> data.psz, "TWScripts")) {
        clear_timer();

        if(do_tweq_rotate) {
            for(int axis = 0; axis < RotAxisMax; ++axis) {
                if(axis_smooth[axis]) {
                    set_axis_rate(axis_names[axis], (RotAxes)axis);
                }
            }
        }

        start_timer();
    }

    return cBaseScript::OnTimer(pTimerMsg, mpReply);

}
