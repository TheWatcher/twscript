
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

using namespace std;


void cScr_TWTweqSmooth::StartTimer()
{
    timer = SetTimedMessage("TWRateUpdate", timer_rate, kSTM_OneShot, "TWScripts");
}


void cScr_TWTweqSmooth::ClearTimer()
{
    if(timer) {
        KillTimedMessage(timer);
        timer = NULL;
    }
}


void cScr_TWTweqSmooth::Init()
{
    ClearTimer();

    char* design_note = GetObjectParams(ObjId());

    min_rate   = GetParamFloat(design_note, "TWTweqSmoothMinRate", 0.1f);
    max_rate   = GetParamFloat(design_note, "TWTweqSmoothMaxRate", 10.0f);
    timer_rate = GetParamInt  (design_note, "TWTweqSmoothTimerRate", 250);

    StartTimer();
}


void cScr_TWTweqSmooth::SetAxisRate(const char* propname, ePhysAxes axis)
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
    if(pSimMsg -> fStarting) Init();

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
                SetAxisRate("x rate-low-high", XAxis);
                SetAxisRate("y rate-low-high", YAxis);
                SetAxisRate("z rate-low-high", ZAxis);
            }
        }
        ClearTimer();
        StartTimer();
    }
    return cBaseScript::OnTimer(pTimerMsg, mpReply);

}
