
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

using namespace std;

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


void cScr_TWTweqSmooth::init_rotate(const char *design_note)
{
    SService<IPropertySrv> pPS(g_pScriptManager);

    // Does the object have a rotate tweq, configuration, and the matching rotate state?
    // This also allows the user to suppress smoothing on an object's rotate if needed.
    do_tweq_rotate = (pPS -> Possessed(ObjId(), "CfgTweqRotate") &&
                      pPS -> Possessed(ObjId(), "StTweqRotate") &&
                      GetParamBool(design_note, "TWTweqSmoothRotate", true));
    if(do_tweq_rotate) {
        // Obtain maximum and minimum rates for each axis. They may not be needed, but
        // parsing them is not a significant overhead.
        rotate_rates[RotXAxis][MinRate] = get_rate(design_note, "TWTweqSmoothRotateXMin", min_rate);
        rotate_rates[RotXAxis][MaxRate] = get_rate(design_note, "TWTweqSmoothRotateXMax", max_rate, rotate_rates[RotXAxis][MinRate]);
        rotate_rates[RotYAxis][MinRate] = get_rate(design_note, "TWTweqSmoothRotateYMin", min_rate);
        rotate_rates[RotYAxis][MaxRate] = get_rate(design_note, "TWTweqSmoothRotateYMax", max_rate, rotate_rates[RotYAxis][MinRate]);
        rotate_rates[RotZAxis][MinRate] = get_rate(design_note, "TWTweqSmoothRotateZMin", min_rate);
        rotate_rates[RotZAxis][MaxRate] = get_rate(design_note, "TWTweqSmoothRotateZMax", max_rate, rotate_rates[RotZAxis][MinRate]);
    }
}


void cScr_TWTweqSmooth::init_joints(const char *design_note)
{
    SService<IPropertySrv> pPS(g_pScriptManager);

    do_tweq_joints = (pPS -> Possessed(ObjId(), "CfgTweqJoints") &&
                      pPS -> Possessed(ObjId(), "StTweqJoints") &&
                      GetParamBool(design_note, "TWTweqSmoothJoints", true));
    if(do_tweq_joints) {
        char cfg_buffer[CFGBUFFER_SIZE];



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
    const char *design_note = GetObjectParams(ObjId());

    // How frequently should the timer update the tweq rate?
    timer_rate = GetParamInt(design_note, "TWTweqSmoothTimerRate", 250);

    // Has the editor specified defaults for the min and max rates?
    min_rate = get_rate(design_note, "TWTweqSmoothMinRate", 0.1f);
    max_rate = get_rate(design_note, "TWTweqSmoothMaxRate", 10.0f, min_rate); // note: force max >= min.

    // Now we need to determine whether the object has a rotate tweq, and joint
    // tweq, or both, and set up the smoothing facility accordingly.
    init_rotate(design_note);
    init_joints(design_note);

    start_timer();
}


void cScr_TWTweqSmooth::set_axis_rate(const char* propname, ePhysAxes axis)
{
    cMultiParm prop;
    float rate, low, high;

    SService<IPropertySrv> pPS(g_pScriptManager);


    if(pPS -> Possessed(ObjId(), "CfgTweqRotate")) {
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
            case XAxis: current = facing.x; break;
            case YAxis: current = facing.y; break;
            case ZAxis: current = facing.z; break;
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

        SService<IPropertySrv> pPS(g_pScriptManager);

        // Only do anything if the object has a RotateState property.
        if(pPS -> Possessed(ObjId(), "StTweqRotate")) {
            cMultiParm mpProp;
            int AnimS;

            // Get the anim state for the tweq
            pPS -> Get(mpProp, ObjId(), "StTweqRotate", "AnimS");
            AnimS = static_cast<int>(mpProp);

            // Only do anything if the rotate is actually on
            if(AnimS & kTweqFlagOn) {
                set_axis_rate("x rate-low-high", XAxis);
                set_axis_rate("y rate-low-high", YAxis);
                set_axis_rate("z rate-low-high", ZAxis);
            }
        }
        clear_timer();
        start_timer();
    }
    return cBaseScript::OnTimer(pTimerMsg, mpReply);

}
