/** @file
 * This file contains the interface for the persistent counter/capacitor
 * class.
 *
 * @author Chris Page &lt;chris@starforge.co.uk&gt;
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

#ifndef SAVED_COUNTER_H
#define SAVED_COUNTER_H

#include "scriptvars.h"

/** A class providing persistent use count and limiting facilities. This
 *  class simplifies the process of maintaining use counters, limiters,
 *  or capacitors in a save-aware form that allows state to persist
 *  across saves and loads. When using objects of this class, the init
 *  function **must** be called before any other member function,
 *  otherwise the behaviour of the counter is undefined (and probably
 *  exceptionally broken).
 */
class SavedCounter
{
public:
    /** Create a new SavedCounter object, and initialise it. Note that the
     *  SavedCounter is not actually usable until init() has been called,
     *  as this can not safely finish setting up the count and last_time
     *  variables.
     *
     * @param script_name A pointer to a string containing the script's name.
     * @param obj_id      The ID of the object the script is attached to.
     * @return A new SavedCounter object. init() must be called before it is
     *         used!
     */
    SavedCounter(const char *script_name, int obj_id) : min(0), max(0), capacitor(false), falloff(0), count(script_name, "count", obj_id), last_time(script_name, "last_time", obj_id)
        { /* fnord */ }


    /** Initialise the variables in the SavedCounter object. This sets up the
     *  variables in the object so that it can actually be used. The counter
     *  is initialised to zero if it has not already been set in a previous
     *  session.
     *
     * @param curr_time  The current sim time.
     * @param min_count  The number of times the counter must be incremented before
     *                   increase_count() will return true. If this is 0, there is no
     *                   minimum.
     * @param max_count  The maximum number of times the counter can be incremented
     *                   before increase_count() stops returning true. If this is 0,
     *                   there is no maximum.
     * @param falloff_ms The time it takes for one count to time out.
     * @param cap_mode   Operate in capacitor mode. If this is true, and min_count is
     *                   set, as soon as the counter reaches the minimum it will reset.
     */
    void init(int curr_time, int min_count = 0, int max_count = 0, int falloff_ms = 0, bool cap_mode = false);


    /** This will increment the counter and return true if the count - after the
     *  increment - is equal to or greater than the minimum and less than or equal
     *  to the maximum (if they are set).
     *
     * @param time   The current sim time.
     * @param amount The amount to increment the counter by. If this is zero, the
     *               function behaves as normal - applying falloff, etc - but the
     *               counter isn't incremented at all.
     * @return true if the count is in the range min <= count <= max.
     */
    bool increment(int time, uint amount = 1);


    /** Reset the counter to zero. Does exactly what it says on the tin.
     *
     * @param time The current sim time.
     */
    void reset(int time)
        { count = 0; last_time = time; }


    /** Change the minimum number of times the counter must be incremented before
     *  increase_count will return true. Note that this will not change the counter
     *  value; it simply updates the stored minimum.
     *
     * @param newmin The new minimum count to set for the counter. If set to 0,
     *               there is no minimum enforced.
     */
    void set_min(int newmin)
        { min = newmin; }


    /** Change the maximum number of times the counter can be incremented before
     *  increase_count stops returning true. Note that this will not change the
     *  counter value; it simply updates the stored maximum.
     *
     * @param newmax The new maximum count to set for the counter. If set to 0,
     *               there is no maximum enforced.
     */
    void set_max(int newmax)
        { max = newmax; }


    /** Set the time in milliseconds it takes for the count to decrease by one.
     *  This allows counts to decrease over time automatically, useful for
     *  providing features that will work a number of times rapidly, and then
     *  drop to a fixed rate.
     *
     * @param newfalloff The new falloff in milliseconds. If this is 0, no falloff
     *                   is applied to the counter.
     */
    void set_falloff(int newfalloff)
        { falloff = newfalloff; }


    /** Fetch the current counts.
     *
     * @return The current count value.
     */
    int get_counts(int *minval = NULL, int *maxval = NULL)
        {
            if(minval) *minval = min;
            if(maxval) *maxval = max;
            return count;
        }

private:
    /** Apply the falloff to the current count (if it is set) and return the updated
     *  counter value. Note that this does not update the `count` member variable: it
     *  is up to the caller to save the value back into `count` after any further
     *  changes have been made to it.
     *
     * @param time     The current sim time.
     * @param oldcount The count value to apply falloff to.
     * @return The count, with the count falloff applied if needed.
     */
    int apply_falloff(int time, int oldcount);

    int  min;             //!< The minimum number of times increase_count() must be called before it returns true
    int  max;             //!< The maximum number of times increase_count() can be called before it returns false
    bool capacitor;       //!< If true, and min is set, the counter works in capacitor mode.
    int  falloff;         //!< The time in milliseconds it takes for the count to decrease by 1.
    script_int count;     //!< The current count
    script_int last_time; //!< The sim time at which the count was last updated
};

#endif // SAVED_COUNTER_H
