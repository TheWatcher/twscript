
#include "SavedCounter.h"

void SavedCounter::init(int curr_time, int min_count, int max_count, int falloff_ms, bool cap_mode)
{
    // Get the persistent variables initialised if needed
    count.Init(0);
    last_time.Init(curr_time);

    // Negative values for min, max, or falloff make no sense, so zero them
    if(min_count  < 0) min_count  = 0;
    if(max_count  < 0) max_count  = 0;
    if(falloff_ms < 0) falloff_ms = 0;

    // If min and max are specified, max must be less than min or the counter will never work
    if(min_count && max_count && max_count < min_count) {
        int swp = min_count;
        min_count = max_count;
        max_count = swap;
    }

    // Everything should be safe now... probably.
    min = min_count;
    max = max_count;
    capacitor = cap_mode && (min > 1); // capacitor mode is pointless without a min setting over 1.
    falloff = falloff_ms;
}


bool SavedCounter::increase_count(int time)
{
    // Let apply_falloff work out what the count should be before incrementing
    int newcount = apply_falloff(time) + 1;

    // If there is no minimum, it is zero, so there doesn't need to be a special check
    // for it here; count will *always* be > 0 here.
    bool validcount = newcount >= min && (max ? newcount <= max : 1);

    // Capacitor mode resets the counter when the minimum count is reached...
    if(capacitor && newcount >= min) {
        newcount = 0;
    }

    // Update the stored variables as they shouldn't need fiddling with now
    count = newcount;
    last_time = time;

    return validcount;
}


int SavedCounter::apply_falloff(int time)
{
    int last     = last_time; // Cache the script vars to reduce read overhead...
    int tmpcount = count;

    // Only bother working out the falloff if one is set, there is a count to reduce,
    // and a previous update time is available.
    if(falloff && last && tmpcount) {
        int removed = (time - last) / falloff;

        // If one or more ticks have timed out, update the counter
        if(removed) {
            tmpcount -= removed;
            if(tmpcount < 0) tmpcount = 0; // Negative use counts would be be bad!
        }
    }

    return tmpcount;
}
