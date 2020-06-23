
#include <lg/objects.h>
#include <chrono>       // std::chrono::system_clock

#include "TWBaseTrigger.h"
#include "ScriptLib.h"


/* ------------------------------------------------------------------------
 *  Message handling
 */

TWBaseScript::MsgStatus TWBaseTrigger::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseScript::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    if(!::_stricmp(msg -> message, "ResetTriggerCount")) {
        count.reset(msg -> time);

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Trigger count reset to 0");
    }

    return MS_CONTINUE;
}


/* ------------------------------------------------------------------------
 *  Initialisation related
 */

void TWBaseTrigger::init(int time)
{
    TWBaseScript::init(time);

    char *design_note = GetObjectParams(ObjId());

    if(design_note) {
        process_designnote(design_note, time);
        g_pMalloc -> Free(design_note);

    } else {
        process_designnote("", time);
    }

    uint seed = std::chrono::system_clock::now().time_since_epoch().count();
    generator.seed(seed);

    if(debug_enabled()) {
        debug_printf(DL_DEBUG, "Trigger initialised with on = '%s', off = '%s', dest = '%s'.\nChosen links will%s be deleted.", turnon_msg.c_str(), turnoff_msg.c_str(), dest.c_str(), (remove_links ? "" : " not"));
        debug_printf(DL_DEBUG, "On is%s a stimulus", (isstim[SEND_ON] ? "" : " not"));
        if(isstim[SEND_ON]) {
            if(intensity_max[SEND_ON] < 0.0f) {
                debug_printf(DL_DEBUG, "    Stim object: %d, Intensity: %.3f", stimob[SEND_ON], intensity_min[SEND_ON]);
            } else {
                debug_printf(DL_DEBUG, "    Stim object: %d, Intensity range: %.3f to %.3f", stimob[SEND_ON], intensity_min[SEND_ON], intensity_max[SEND_ON]);
            }
        }

        debug_printf(DL_DEBUG, "Off is%s a stimulus", (isstim[SEND_OFF] ? "" : " not"));
        if(isstim[SEND_OFF])  {
            if(intensity_max[SEND_OFF] < 0.0f) {
                debug_printf(DL_DEBUG, "    Stim object: %d, Intensity: %.3f", stimob[SEND_OFF], intensity_min[SEND_OFF]);
            } else {
                debug_printf(DL_DEBUG, "    Stim object: %d, Intensity range: %.3f to %.3f", stimob[SEND_OFF], intensity_min[SEND_OFF], intensity_max[SEND_OFF]);
            }
        }

        debug_printf(DL_DEBUG, "Chance of failure is %d%%%s", static_cast<int>(fail_chance), (static_cast<int>(fail_chance) ? "" : " (will always trigger)"));
        debug_printf(DL_DEBUG, "Count is %d%s with a falloff of %d milliseconds, count mode is %d, limit is %s",
                     count_dp.get_count(), (count_dp.get_count() ? "" : " (no use limit)"),
                     count_dp.get_falloff(),
                     static_cast<int>(count_mode), (count_dp.get_limit() ? "on" : "off"));
    }
}


/* ------------------------------------------------------------------------
 *  Miscellaneous - private functions
 */

void TWBaseTrigger::process_designnote(const std::string& design_note, const int time)
{
    // Work out what the turnon and turnoff messages should be
    turnon_msg.init(design_note, "TurnOn");
    isstim[SEND_ON] = check_stimulus_message(turnon_msg.c_str(),
                                             &stimob[SEND_ON],
                                             &intensity_min[SEND_ON], &intensity_max[SEND_ON]);

    turnoff_msg.init(design_note, "TurnOff");
    isstim[SEND_OFF] = check_stimulus_message(turnoff_msg.c_str(),
                                              &stimob[SEND_OFF],
                                              &intensity_min[SEND_OFF], &intensity_max[SEND_OFF]);

    // And where the messages should go
    dest.init(design_note, "&ControlDevice");

    // Remove links after sending?
    remove_links.init(design_note);

    // Allow triggers to fail
    fail_chance.init(design_note);

    // Now for use limiting.
    count_dp.init(design_note);
    count.init(time, 0, count_dp.get_count(), count_dp.get_falloff(), false, count_dp.get_limit());

    // Handle modes
    count_mode.init(design_note);
}


bool TWBaseTrigger::check_stimulus_message(const char* message, int* obj, float* int_min, float* int_max)
{
    char *end  = NULL;
    char *from = NULL;

    // If the first character is not a '[', the message is not a stimulus
    if(*message != '[') return false;

    ++message;
    *int_min = strtof(message, &end);

    // If number parsing fails, the message is still not a stimulus.
    if(end == message) return false;

    // Something has been parsed out, so traverse the string looking for |, ] or end
    while(*end && *end != ']' && *end != '|') ++end;

    // Hit a '|'? If so, try parsing another number
    if(*end == '|') {
        from = end;
        *int_max = strtof(from, &end);

        // Parsing failed - restore the value that was in int_max
        if(end == from) {
            *int_max = 0.0f;
        } else {
            // Make sure min and max are the right way around
            if(*int_min > *int_max) {
                float tmp = *int_max;
                *int_max = *int_min;
                *int_min = tmp;
            }

            while(*end && *end != ']') ++end;
        }
    }

    // Hit the end of string? Not something we can use, then
    if(!*end) return false;

    // Skip the ], and then check that we haven't hit the end of the string
    ++end;
    if(!*end) return false;

    // end now contains the name of an object, so try to locate it
    SInterface<IObjectSystem> ObjectSys(g_pScriptManager);
    *obj = ObjectSys -> GetObjectNamed(end);

    // The stimulus must be a negative (ie: a stimulus archetype)
    if(*obj >= 0) return false;

    // Okay, looks like an archetype has been found, so this is a stim message
    return true;
}


float TWBaseTrigger::make_intensity(float min, float max)
{
    // Negative max means no max was set - just pass the stim value as-is
    if(max < 0) {
        return min;
    }

    // Otherwise, generate a random value in the range
    int amount = uni_dist(generator);
    return (min + ((max - min) * (amount / 100.0f)));
}


/* ------------------------------------------------------------------------
 *  Message handling - private functions
 */

bool TWBaseTrigger::send_trigger_message(bool send_on, sScrMsg* msg)
{
    std::vector<TargetObj>* targets = NULL;

    if(debug_enabled())
        debug_printf(DL_DEBUG, "Doing %s trigger", (send_on ? "On" : "Off"));

    // Do failure checking; should be done before count checking as failed
    // firings should not be counted
    if(static_cast<int>(fail_chance) > 0 &&
       (uni_dist(generator) > static_cast<int>(fail_chance))) {

        if(debug_enabled())
            debug_printf(DL_DEBUG, "%s trigger aborted: random failure", (send_on ? "On" : "Off"));

        return false;
    }

    DesignParamCountMode::CountMode mode = (send_on ? DesignParamCountMode::CountMode::CM_TURNON : DesignParamCountMode::CountMode::CM_TURNOFF);

    if(count.increment(msg -> time, (count_mode & mode) ? 1 : 0)) {
        if(debug_enabled()) {
            int max, counted = count.get_counts(NULL, &max);
            debug_printf(DL_WARNING, "Count passed (%d of %d), doing trigger", counted, max);
        }

        targets = dest.values(msg);

        if(!targets -> empty()) {
            std::vector<TargetObj>::iterator it;
            SService<IActReactSrv> ar_srv(g_pScriptManager);

            // Convert the bool to an index into the various arrays
            int send = (send_on ? SEND_ON : SEND_OFF);

            for(it = targets -> begin(); it != targets -> end(); it++) {
                // If sending a stim instead of a message, do that...
                if(isstim[send]) {
                    float intensity = make_intensity(intensity_min[send], intensity_max[send]);

                    if(debug_enabled()) {
                        std::string objname, stimname;
                        get_object_namestr(objname, it -> obj_id);
                        get_object_namestr(stimname, stimob[send]);

                        debug_printf(DL_DEBUG, "Stimulating %s with %s, intensity %.3f", objname.c_str(), stimname.c_str(), intensity);
                    }

                    ar_srv -> Stimulate(it -> obj_id, stimob[send], intensity, ObjId());

                // otherwise, send the message to the target
                } else {

                    // Work out which message to send
                    DesignParamString* message;
                    if(send_on) {
                        message = &turnon_msg;
                    } else {
                        message = &turnoff_msg;
                    }

                    // Report it if needed
                    if(debug_enabled()) {
                        std::string objname;
                        get_object_namestr(objname, it -> obj_id);

                        debug_printf(DL_DEBUG, "Sending %s to %s", message -> c_str(), objname.c_str());
                    }

                    // And send it
                    post_message(it -> obj_id, message -> c_str());

                    // TODO: Handle link delete
                }
            }
        } else if(debug_enabled()) {
            debug_printf(DL_WARNING, "No targets found for trigger");
        }

        // No longer need the target list
        delete targets;

        // Indicate messages have been sent
        return true;
    } else if(debug_enabled()) {
        int max, counted = count.get_counts(NULL, &max);
        debug_printf(DL_WARNING, "Count exceeded (%d of %d), ignoring trigger", counted, max);
    }

    return false;
}
