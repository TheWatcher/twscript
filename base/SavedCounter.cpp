
#include "SavedCounter.h"
#include "ScriptModule.h"
#include "ScriptLib.h"

void SavedCounter::init(int curr_time, int min_count, int max_count, int falloff_ms, bool cap_mode, bool limit_mode)
{
    // Get the persistent variables initialised if needed
    count.Init(0);
    last_time.Init(curr_time);

    // Negative values for min, max, or falloff make no sense, so zero them
    if(min_count  < 0) min_count  = 0;
    if(max_count  < 0) max_count  = 0;
    if(falloff_ms < 0) falloff_ms = 0;

    // If min and max are specified, min must be less than max or the counter will never work
    if(min_count && max_count && max_count < min_count) {
        int swap = min_count;
        min_count = max_count;
        max_count = swap;
    }

    // Everything should be safe now... probably.
    min = min_count;
    max = max_count;
    capacitor = cap_mode && (min > 1); // capacitor mode is pointless without a min setting over 1.
    limit = limit_mode && max && !capacitor; // limit mode is pointless if capacitor mode is set, or there's no max.
    falloff = falloff_ms;

    is_enabled = true;
}


bool SavedCounter::increment(int time, uint amount)
{
    int oldcount = static_cast<int>(count);

	g_pfnMPrintf("SavedCounter[%s].increment, oldcount=%d amount=%d\n", name.c_str(), oldcount, amount);

    // Let apply_falloff work out what the count should be before incrementing
    int newcount = apply_falloff(time, oldcount) + amount;

    g_pfnMPrintf("SavedCounter[%s].increment, newcount=%d\n", name.c_str(), newcount);

    // If limit mode is enabled, force at most max + 1 for the new value.
    if(limit && max && (newcount > max)) newcount = max + 1;

    // If there is no minimum, it is zero, so there doesn't need to be a special check
    // for it here; count will *always* be > 0 here.
    bool validcount = newcount >= min && (max ? newcount <= max : 1);

    if(newcount != oldcount) {
        // Capacitor mode resets the counter when the minimum count is reached...
        if(capacitor && newcount >= min) {
            newcount = 0;
        }
        g_pfnMPrintf("SavedCounter[%s].increment, setting count=newcount=%d last_time=%d\n", name.c_str(), newcount, time);

        // Update the stored variables as they shouldn't need fiddling with now
        count = newcount;
        last_time = time;
    }

	g_pfnMPrintf("SavedCounter[%s].increment, finishing: count=%d, valid=%d\n", name.c_str(), static_cast<int>(count), validcount);

    return validcount;
}


int SavedCounter::apply_falloff(int time, int oldcount)
{
    int last = last_time; // Cache the script var to reduce read overhead...

    // Only bother working out the falloff if one is set, there is a count to reduce,
    // and a previous update time is available.
    if(falloff && last && oldcount) {
        int removed = (time - last) / falloff;

        // If one or more ticks have timed out, update the counter
        if(removed) {
            oldcount -= removed;
            if(oldcount < 0) oldcount = 0; // Negative use counts would be be bad!

            last_time = time; // Made a change, so record that.
        }
    }

    return oldcount;
}
