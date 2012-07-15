
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

/* =============================================================================
 *  TWTweqSmooth Impmementation
 */

const char *cScr_TWTweqSmooth::axis_names[] = {
    "x rate-low-high",
    "y rate-low-high",
    "z rate-low-high"
};

const char *cScr_TWTweqSmooth::joint_names[] = {
    "    rate-low-high",
    "    rate-low-high2",
    "    rate-low-high3",
    "    rate-low-high4",
    "    rate-low-high5",
    "    rate-low-high6"
};

/* -----------------------------------------------------------------------------
 *  Utility functions
 */

cAnsiStr get_object_namestr(object obj_id)
{
    cAnsiStr str = "0";
    if(obj_id) {
        SInterface<IObjectSystem> pOS(g_pScriptManager);
        const char* pszName = pOS->GetName(obj_id);

        if (pszName) {
            str.FmtStr("%s (%d)", pszName, int(obj_id));

        } else {
            SInterface<ITraitManager> pTM(g_pScriptManager);
            object iArc = pTM->GetArchetype(obj_id);
            pszName = pOS->GetName(iArc);

            if (pszName)
                str.FmtStr("A %s (%d)", pszName, int(obj_id));
            else
                str.FmtStr("%d", int(obj_id));
        }
    }
    return str;
}


std::vector<object>* get_target_objects(sScrMsg *pMsg, const char *dest)
{
    std::vector<object>* matches = new std::vector<object>;
    SInterface<IObjectSystem> pOS(g_pScriptManager);
    SInterface<ITraitManager> pTM(g_pScriptManager);

    // Simple dest/source selection.
    if(!_stricmp(dest, "[me]")) {
        matches -> push_back(pMsg -> to);
    } else if(!_stricmp(dest, "[source]")) {
        matches -> push_back(pMsg -> from);

    // Archetype search
    } else if(*dest == '*' || *dest == '@') {

        // Find the archetype named if possible
        object arch = pOS -> GetObjectNamed(&dest[1]);
        if(int(arch) <= 0) {

            // Build the query flags - '*' just searches direct descendats, '@' does all
            ulong flags = kTraitQueryChildren;
            if(*dest == '@') flags |= kTraitQueryFull;

            // Ask for the list of matching objects
            SInterface<IObjectQuery> query = pTM -> Query(arch, flags);
            if(query) {

                // Process each object, adding it to the match list if it's concrete.
                for(; !query -> Done(); query -> Next()) {
                    object obj = query -> Object();
                    if(int(obj) > 0) matches -> push_back(obj);
                }
            }
        }

    // Named destination object
    } else {
        object obj = pOS -> GetObjectNamed(dest);
        if(obj) matches -> push_back(obj);
    }

    return matches;
}


/* -----------------------------------------------------------------------------
 *  Timer related
 */

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


/* -----------------------------------------------------------------------------
 *  Rate and bounds utility functions
 */

float cScr_TWTweqSmooth::get_rotate_rate(RotAxis axis)
{
    SService<IPropertySrv> pPS(g_pScriptManager);
    cMultiParm prop;

    pPS -> Get(prop, ObjId(), "CfgTweqRotate", axis_names[axis]);

    // The float cast here is probably redundant, but meh.
    return (float)(static_cast<const mxs_vector*>(prop) -> x);
}


bool cScr_TWTweqSmooth::valid_rotate_bounds(RotAxis axis)
{
    SService<IPropertySrv> pPS(g_pScriptManager);
    cMultiParm prop;

    pPS -> Get(prop, ObjId(), "CfgTweqRotate", axis_names[axis]);

    float low  = static_cast<const mxs_vector*>(prop) -> y;
    float high = static_cast<const mxs_vector*>(prop) -> z;

    return(low >= 0.0f && high > low);
}


float cScr_TWTweqSmooth::get_joint_rate(int joint)
{
    SService<IPropertySrv> pPS(g_pScriptManager);
    cMultiParm prop;

    pPS -> Get(prop, ObjId(), "CfgTweqJoints", joint_names[joint]);

    // The float cast here is probably redundant, but meh.
    return (float)(static_cast<const mxs_vector*>(prop) -> x);
}


float cScr_TWTweqSmooth::get_rate_param(const char *design_note, const char *cfgname, float default_value, float minimum)
{
    float value = GetParamFloat(design_note, cfgname, default_value);

    if(value < minimum) value = minimum;

    return value;
}


/* -----------------------------------------------------------------------------
 *  Initialisation
 */

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
    int      set_count  = 0;
    bool     can_smooth = false;
    cAnsiStr obj_name   = get_object_namestr(ObjId());

    // Find out whether any axes have been selected for smoothing
    char *axis = GetParamString(design_note, "TWTweqSmoothRotate", "all");
    if(axis) {
        set_count = init_rotate_onoffctrl(axis);
        g_pMalloc -> Free(axis);
    }

    // If the object has a TweqRotate configuration, check that the AnimC flags are okay
    if(pPS -> Possessed(ObjId(), "CfgTweqRotate")) {
        cMultiParm flags;
        pPS -> Get(flags, ObjId(), "CfgTweqRotate", "AnimC");

        // If the flags are not compatible, print a warning in the monolog
        can_smooth = compatible_animc(static_cast<int>(flags));
        if(!can_smooth) {

            if(warnings)
                DebugPrintf("WARNING[TWTweqSmooth]: %s has incompatible AnimC flags. Unable to smooth rotation.", static_cast<const char *>(obj_name));
        }
    }

    // Does the object have a rotate tweq, configuration, and the matching rotate state?
    // This also allows the user to suppress smoothing on an object's rotate if needed.
    do_tweq_rotate = (can_smooth &&
                      pPS -> Possessed(ObjId(), "CfgTweqRotate") &&
                      pPS -> Possessed(ObjId(), "StTweqRotate") &&
                      set_count);

    // If the rotate is being smoothed, obtain maximum and minimum rates for each axis.
    // If the user hasn't specified them, the global defaults are used instead.
    if(do_tweq_rotate) {
        rotate_rates[RotXAxis][MinRate] = get_rate_param(design_note, "TWTweqSmoothRotateXMin", min_rate);
        rotate_rates[RotYAxis][MinRate] = get_rate_param(design_note, "TWTweqSmoothRotateYMin", min_rate);
        rotate_rates[RotZAxis][MinRate] = get_rate_param(design_note, "TWTweqSmoothRotateZMin", min_rate);

        // Axis maximum rates are taken from the Tweq -> Rotate coniguration for each axis.
        for(int axis = 0; axis < RotAxisMax; ++axis) {
            if(axis_smooth[axis]) {
                rotate_rates[axis][MaxRate] = get_rotate_rate((RotAxis)axis);

                // If the axis max rate is not set or too low, mark it as not smoothed.
                if(rotate_rates[axis][MaxRate] <= rotate_rates[axis][MinRate]) {
                    axis_smooth[axis] = false;
                    --set_count;

                    if(warnings)
                        DebugPrintf("WARNING[TWTweqSmooth]: %s %s has rate set to the min rate or less, disabling smoothing on this axis.", static_cast<const char *>(obj_name), axis_names[axis]);

                // Similarly, if the low/high bounds on the rotation are not good, disable smoothing
                } else if(!valid_rotate_bounds((RotAxis)axis)) {
                    axis_smooth[axis] = false;
                    --set_count;

                    if(warnings)
                        DebugPrintf("WARNING[TWTweqSmooth]: %s %s has unsupported bounds, disabling smoothing on this axis.", static_cast<const char *>(obj_name), axis_names[axis]);
                }
            }
        }

        // If all the axes have been disabled, disable all smoothing
        if(!set_count) {
            do_tweq_rotate = false;

            if(warnings)
                DebugPrintf("WARNING[TWTweqSmooth]: %s has no smoothable axes after rate checks. Rotation smoothing disabled.", static_cast<const char *>(obj_name));
        }
    } else if(warnings) {
        DebugPrintf("NOTICE[TWTweqSmooth]: %s rotation smoothing disabled.", static_cast<const char *>(obj_name));
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
            if(joint > 0 && joint <= JointCount) {
                // Note -1 here. The UI has the joints 1-indexed, this is 0 indexed!
                joint_smooth[joint - 1] = true;
                ++count;
            }
        }
    }

    // Now go back and check that the joints can actually be smoothed. Easier to
    // do it here after the fact that while checking the string above...
    SService<IPropertySrv> pPS(g_pScriptManager);
    cAnsiStr joint_buffer;

    // Note that, if the object is missing CfgTweqJoints, it's no big problem
    // that we're not going to be able to check the joints - the check in
    // init_joints() will stop *all* smoothing actions there if it doesn't have one.
    if(pPS -> Possessed(ObjId(), "CfgTweqJoints")) {
        // Get the object name, in case it is needed in the loop
        cAnsiStr obj_name = get_object_namestr(ObjId());

        cMultiParm flags;
        for(int joint = 0; joint < JointCount; ++joint) {
            // Only bother checking the config if the joint is selected for smoothing
            if(joint_smooth[joint]) {
                // What's the AnimC we need for this joint?
                joint_buffer.FmtStr("Joint%dAnimC", joint + 1);

                // Get the flags, and check they are okay.
                pPS -> Get(flags, ObjId(), "CfgTweqJoints", static_cast<const char *>(joint_buffer));

                // Disable joints with incompatible animc
                if(!compatible_animc(static_cast<int>(flags))) {
                    joint_smooth[joint] = false;
                    --count;

                    if(warnings)
                        DebugPrintf("NOTICE[TWTweqSmooth]: %s has incompatible AnimC flags on joint %d. Unable to smooth joint %d rotation.", static_cast<const char *>(obj_name), joint + 1, joint + 1);
                }
            }
        }
    }

    return count;
}


void cScr_TWTweqSmooth::init_joints(char *design_note)
{
    SService<IPropertySrv> pPS(g_pScriptManager);
    int set_count = 0;

    // Find out how many joints have been selected for smoothing, and pass checks on their AnimC
    char *joints = GetParamString(design_note, "TWTweqSmoothJoints", "all");
    if(joints) {
        set_count = init_joints_onoffctrl(joints);
        g_pMalloc -> Free(joints);
    }

    // As near as I can tell, NoLimit and Wrap on the CfgTweqJoints main config (as opposed to on
    // individual joints) does bugger all, and can be ignored here unlike in init_rotate().

    // Only smooth joints if there are joints to smooth, and one or more are set for smoothing
    do_tweq_joints = (pPS -> Possessed(ObjId(), "CfgTweqJoints") &&
                      pPS -> Possessed(ObjId(), "StTweqJoints") &&
                      set_count);

    // If joint smoothing is enabled, the minimum and maximum rates for each joint
    // need to be set.
    if(do_tweq_joints) {
        cAnsiStr cfg_buffer;
        cAnsiStr obj_name = get_object_namestr(ObjId());

        for(int joint = 0; joint < JointCount; ++joint) {
            // Only bother setting the rates if the joint is selected for smoothing
            if(joint_smooth[joint]) {
                // Get the rates (the max comes out of the joint config)
                cfg_buffer.FmtStr("TWTweqSmoothJoint%1dMin", joint + 1);
                joint_rates[joint][MinRate] = get_rate_param(design_note, static_cast<const char *>(cfg_buffer), min_rate);
                joint_rates[joint][MaxRate] = get_joint_rate(joint);

                // Check it is smoothable.


                DebugPrintf("%s joint %d has rate %f", static_cast<const char *>(obj_name), joint, joint_rates[joint][MaxRate]);
            }
        }
    }
}


void cScr_TWTweqSmooth::init()
{
    // Ensure that the timer can't fire while re-initialising
    clear_timer();

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    if(design_note) {
        // Should warnings be displayed in the monolog?
        warnings = GetParamBool(design_note, "TWTweqSmoothWarn", true);

        // How frequently should the timer update the tweq rate?
        timer_rate = GetParamInt(design_note, "TWTweqSmoothTimerRate", 250);

        // Has the editor specified defaults for the min and max rates?
        min_rate = get_rate_param(design_note, "TWTweqSmoothMinRate", 0.1f);
        max_rate = get_rate_param(design_note, "TWTweqSmoothMaxRate", 10.0f, min_rate); // note: force max >= min.

        // Now we need to determine whether the object has a rotate tweq, and joint
        // tweq, or both, and set up the smoothing facility accordingly.
        init_rotate(design_note);
        init_joints(design_note);

        // If either rotate or joints are going to be smoothed, start the timer to do it.
        if(do_tweq_rotate || do_tweq_joints) {
            start_timer();

            // Otherwise potentially bitch at the user.
        } else if(warnings) {
            cAnsiStr obj_name = get_object_namestr(ObjId());
            DebugPrintf("WARNING[TWTweqSmooth]: %s has TweqRotate and TweqJoints smoothing disabled. Why am I on this object?", static_cast<const char *>(obj_name));
        }

        // No longer need the design note
        g_pMalloc->Free(design_note);
    }
}


/* -----------------------------------------------------------------------------
 *  Smoothing implementations
 */

void cScr_TWTweqSmooth::set_axis_rate(const char *propname, RotAxis axis)
{
    cMultiParm prop;
    float rate, low, high;

/*    SService<IPropertySrv> pPS(g_pScriptManager);

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
        }*/
}


/* -----------------------------------------------------------------------------
 *  Dark Engine message hooks
 */

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
                    set_axis_rate(axis_names[axis], (RotAxis)axis);
                }
            }
        }

        start_timer();
    }

    return cBaseScript::OnTimer(pTimerMsg, mpReply);

}


/* =============================================================================
 *  TWTrapSetSpeed Impmementation
 */

void cScr_TWTrapSetSpeed::set_speed(object obj_id, float speed)
{
    SService<ILinkSrv> pLS(g_pScriptManager);
    SService<ILinkToolsSrv> pLTS(g_pScriptManager);

    // Convert to a multiparm here for ease
    cMultiParm setspeed = speed;

    // Fetch all TPath links from the specified object to any other
    linkset lsLinks;
    pLS -> GetAll(lsLinks, pLTS -> LinkKindNamed("TPath"), obj_id, 0);

    // Set the speed for each link to the set speed.
    for(; lsLinks.AnyLinksLeft(); lsLinks.NextLink()) {
        pLTS -> LinkSetData(lsLinks.Link(), "Speed", setspeed);
    }
}


long cScr_TWTrapSetSpeed::OnTurnOn(sScrMsg* pMsg, cMultiParm& mpReply)
{
    SInterface<IObjectSystem> pOS(g_pScriptManager);

    // Fetch the contents of the object's design note
    char *design_note = GetObjectParams(ObjId());

    if(design_note) {
        // Get the speed the user has set for this object (which could be a QVar)
        float speed = GetParamFloat(design_note, "TWTrapSetSpeed", 0.0f);

        // Who should be updated?
        char *target = GetParamString(design_note, "TWTrapSetSpeedDest", "[me]");

        // If a target has been parsed, fetch all the objects that match it
        if(target) {
            std::vector<object>* targets = get_target_objects(pMsg, target);

            // Process the target list, setting the speeds accordingly
            std::vector<object>::iterator it;
            for(it = targets -> begin() ; it < targets -> end(); it++) {
                set_speed(*it, speed);
            }

            // And clean up
            delete targets;
            g_pMalloc -> Free(target);
        }

        g_pMalloc -> Free(design_note);
    }

	return cBaseTrap::OnTurnOn(pMsg, mpReply);
}
