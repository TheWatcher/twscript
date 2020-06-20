
#include "TWBaseTrap.h"
#include "ScriptLib.h"


/* ------------------------------------------------------------------------
 *  Message handling
 */

TWBaseScript::MsgStatus TWBaseTrap::on_message(sScrMsg* msg, cMultiParm& reply)
{
    // Call the superclass to let it handle any messages it needs to
    MsgStatus result = TWBaseScript::on_message(msg, reply);
    if(result != MS_CONTINUE) return result;

    if(!::_stricmp(msg -> message, turnon_msg.c_str())) {
        if(debug_enabled())
            debug_printf(DL_DEBUG, "Received TurnOn");

        // Handle the global capacitor
        if(!capacitor.enabled() || capacitor.increment(msg -> time)) {

            // And handle on-specific capacitor
            if(!on_capacitor.enabled() || on_capacitor.increment(msg -> time)) {

                // Passed the capacitors, does this have any firings left?
                if(!count.enabled() || count.increment(msg -> time, (count_mode & DesignParamCountMode::CM_TURNON) ? 1 : 0)) {
                    if(debug_enabled()) {
                        debug_printf(DL_DEBUG, "TurnOn firing. Count is %d", count.get_counts());
                    }

                    return on_onmsg(msg, reply);

                } else if(debug_enabled()) {
                    debug_printf(DL_DEBUG, "TurnOn suppressed - count limit (%d) reached", count.get_counts());
                }

            } else if(debug_enabled()) {
                debug_printf(DL_DEBUG, "TurnOn suppressed by on capacitor");
            }

        } else if(debug_enabled()) {
            debug_printf(DL_DEBUG, "TurnOn suppressed by capacitor");
        }

        // Get here and one of the counters returned false, so halt further processing.
        return MS_HALT;

    } else if(!::_stricmp(msg -> message, turnoff_msg.c_str())) {

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Received TurnOff");

        // Global capacitor first
        if(!capacitor.enabled() || capacitor.increment(msg -> time)) {

            // And now off-specific capacitor
            if(!off_capacitor.enabled() || off_capacitor.increment(msg -> time)) {

                // Counter-based supression
                if(!count.enabled() || count.increment(msg -> time, (count_mode &  DesignParamCountMode::CM_TURNOFF) ? 1 : 0)) {
                    if(debug_enabled()) {
                        debug_printf(DL_DEBUG, "TurnOff firing. Count is %d", count.get_counts());
                    }
                    return on_offmsg(msg, reply);

                } else if(debug_enabled()) {
                    debug_printf(DL_DEBUG, "TurnOff suppressed - count limit (%d) reached", count.get_counts());
                }
            } else if(debug_enabled()) {
                debug_printf(DL_DEBUG, "TurnOff suppressed by off capacitor");
            }

        } else if(debug_enabled()) {
            debug_printf(DL_DEBUG, "TurnOff suppressed by capacitor");
        }

        // Get here and one of the counters returned false, so halt further processing.
        return MS_HALT;

    } else if(!::_stricmp(msg -> message, "ResetCount")) {
        count.reset(msg -> time);

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Use count reset to 0");
    }

    return MS_CONTINUE;
}


/* ------------------------------------------------------------------------
 *  Initialisation related
 */

void TWBaseTrap::init(int time)
{
    TWBaseScript::init(time);
    char *design_note = GetObjectParams(ObjId());

    if(design_note) {
        process_designnote(design_note, time);
        g_pMalloc -> Free(design_note);
    } else {
        process_designnote("", time);
    }
}


void TWBaseTrap::process_designnote(const std::string& design_note, const int time)
{
    // Work out what the turnon and turnoff messages should be
    turnon_msg.init(design_note, "TurnOn");
    turnoff_msg.init(design_note, "TurnOff");

    if(debug_enabled())
        debug_printf(DL_DEBUG, "Trap initialised with on = '%s', off = '%s'", turnon_msg.c_str(), turnoff_msg.c_str());

    // Now for use limiting.
    limit_dp.init(design_note);
    if(limit_dp.is_set()) {
        count.init(time, 0, limit_dp.get_count(), limit_dp.get_falloff(), false, limit_dp.get_limit());

        // Handle modes
        count_mode.init(design_note);

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Count is %d%s with a falloff of %d milliseconds, count mode is %d",
                         limit_dp.get_count(),
                         (limit_dp.get_limit() ? "" : " (no count limit)"),
                         limit_dp.get_falloff(),
                         static_cast<int>(count_mode));
    }

    // Now deal with capacitors
    cap_dp.init(design_note);
    if(cap_dp.is_set()) {
        capacitor.init(time, cap_dp.get_count(), 0, cap_dp.get_falloff(), true);

        if(debug_enabled())
            debug_printf(DL_DEBUG, "Capacitor is %d%s with a falloff of %d milliseconds",
                         cap_dp.get_count(),
                         (cap_dp.get_count() > 1 ? "" : " (every turnon fires)"),
                         cap_dp.get_falloff());
    }

    on_cap_dp.init(design_note);
    if(on_cap_dp.is_set()) {
        on_capacitor.init(time, on_cap_dp.get_count(), 0, on_cap_dp.get_falloff(), true);

        if(debug_enabled())
            debug_printf(DL_DEBUG, "OnCapacitor is %d%s with a falloff of %d milliseconds",
                         on_cap_dp.get_count(),
                         (on_cap_dp.get_count() > 1 ? "" : " (every turnon fires)"),
                         on_cap_dp.get_falloff());
    }

    off_cap_dp.init(design_note);
    if(off_cap_dp.is_set()) {
        off_capacitor.init(time, off_cap_dp.get_count(), 0, off_cap_dp.get_falloff(), true);

        if(debug_enabled())
            debug_printf(DL_DEBUG, "OffCapacitor is %d%s with a falloff of %d milliseconds",
                         off_cap_dp.get_count(),
                         (off_cap_dp.get_count() > 1 ? "" : " (every turnoff fires)"),
                         off_cap_dp.get_falloff());
    }
}