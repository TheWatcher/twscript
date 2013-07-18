/** @file
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

#ifndef TWMESSAGETOOLS_H
#define TWMESSAGETOOLS_H

#include <lg/scrmsgs.h>
#include <unordered_map>
#include <map>
#include <cstring>
#include <cctype>
#include "scriptvars.h"

/** Function pointer type for message field access functions. All message
 *  field accessor functions must have this fingerprint.
 */
typedef void (*MessageAccessProc)(cMultiParm&, sScrMsg*);

struct char_icmp
{
    bool operator () (const char* a ,const char* b) const {
        while(*a && *b) {
            if(tolower(*a) != tolower(*b)) return false;
            ++a;
            ++b;
        }
        return(*a == *b);
    }
};


struct char_hash
{
    /** Hashing operator based on the sdbm algorith, see this URL
     *  http://www.cse.yorku.ca/~oz/hash.html for more details.
     */
    size_t operator()(const char* str) const {
        size_t hash = 0;
        int c;

        /* Note the tolower here is not standard: I've added it to make the
         * hash case insensitive.
         */
        while((c = tolower(*str++))) {
            hash = c + (hash << 6) + (hash << 16) - hash;
        }

        return hash;
    }
};


/** A map type for fast lookup of accessor functions for named message types.
 *  I'd *much* rather use std::string as the key, but this will need to be
 *  find()able with a char *, and while there is an implicit converstion, it
 *  will require allocation/deallocation overhead. Without a way to profile
 *  the code properly, I can't tell if that overhead is prohibitive... and this
 *  will work, so needs must as the devil drives, and all that.
 */
typedef std::unordered_map<const char *, MessageAccessProc, char_hash, char_icmp> AccessorMap;
typedef AccessorMap::iterator   AccessorIter; //!< Convenience type for AccessorMap iterators
typedef AccessorMap::value_type AccessorPair; //!< Convenience type for AccessorPair key/value pairs



class ScriptMultiParm : public script_var
{
public:
    ScriptMultiParm() : script_var()
        { /* fnord */ }

    ScriptMultiParm(const char* script_name, const char* var_name) : script_var(script_name, var_name)
        { /* fnord */ }

    ScriptMultiParm(const char* script_name, const char* var_name, int obj) : script_var(script_name, var_name, obj)
        { /* fnord */ }

    void Init() {
        if(!g_pScriptManager -> IsScriptDataSet(&m_tag)) {
            param.type = kMT_Int;
            param.i = 0;
            g_pScriptManager -> SetScriptData(&m_tag, &param);
        }
    }

    operator const sMultiParm* ()
    {
        param.type = kMT_Undef;
        g_pScriptManager->GetScriptData(&m_tag, &param);

        return &param;
    }

    ScriptMultiParm& operator= (const sMultiParm* val)
    {
        param = *val;
        g_pScriptManager->SetScriptData(&m_tag, &param);

        return *this;
    }

private:
    cMultiParm param;

};


typedef std::map<const char *, ScriptMultiParm> PersistentMap;


class TWMessageTools
{



    /* ------------------------------------------------------------------------
     *  static member functions and variables.
     *
     *  In an ideal world, all these functions (or equivalents) would actually
     *  be part of the message classes, rather than stuck in here taking a
     *  message pointer and pulling out data. In fact, an equivalent of
     *  get_message_type() *is* in sScrMsg... it just doesn't work reliably.
     *  The rest aren't there at all, and adding them is not really viable, so
     *  needs must when the Devil vomits into your kettle....
     */
public:
    /** Obtain the type of the specified message, regardless of what its actual
     *  name is. This is mostly useful for identifying stim messages reliably,
     *  but it is applicable across all message types. The string returned by
     *  this should not be freed, and it contains the name of the message type
     *  as defined in lg/scrmsgs.h.
     *
     * @param msg A pointer to the message to obtain the type for.
     * @return A string containing the message's type name.
     */
    static const char* get_message_type(sScrMsg* msg);


    /** Fetch the value stored in the specified field of the provided message.
     *  This will attempt to copy the value in the named field of the message
     *  into the dest variable, if the message actually contains the requested
     *  field, and it can actually be shoved into a MultiPArm structure.
     *
     * @param dest  The MultiParm structure to store the field contents in. If
     *              this function returns false, dest is not modified.
     * @param msg   The message to retrieve the value from.
     * @param field The name of the field in the message to retrieve.
     * @return true if dest has been updated (the requested field is valid),
     *         false if the requested field is not available in the message, or
     *         it is of a type that MultiParm can not store.
     */
    static bool get_message_field(cMultiParm& dest, sScrMsg* msg, const char* field);

private:
    static void access_msg_from(cMultiParm& dest, sScrMsg* msg);
    static void access_msg_to(cMultiParm& dest, sScrMsg* msg);
    static void access_msg_message(cMultiParm& dest, sScrMsg* msg);
    static void access_msg_time(cMultiParm& dest, sScrMsg* msg);
    static void access_msg_flags(cMultiParm& dest, sScrMsg* msg);
    static void access_msg_data(cMultiParm& dest, sScrMsg* msg);
    static void access_msg_data2(cMultiParm& dest, sScrMsg* msg);
    static void access_msg_data3(cMultiParm& dest, sScrMsg* msg);
    static void access_sim_starting(cMultiParm& dest, sScrMsg* msg);
    static void access_dgmc_resuming(cMultiParm& dest, sScrMsg* msg);
    static void access_dgmc_suspending(cMultiParm& dest, sScrMsg* msg);
    static void access_aimc_mode(cMultiParm& dest, sScrMsg* msg);
    static void access_aimc_previousmode(cMultiParm& dest, sScrMsg* msg);
    static void access_aialertness_level(cMultiParm& dest, sScrMsg* msg);
    static void access_aialertness_oldlevel(cMultiParm& dest, sScrMsg* msg);
    static void access_aihighalert_level(cMultiParm& dest, sScrMsg* msg);
    static void access_aihighalert_oldlevel(cMultiParm& dest, sScrMsg* msg);
    static void access_airesult_action(cMultiParm& dest, sScrMsg* msg);
    static void access_airesult_result(cMultiParm& dest, sScrMsg* msg);
    static void access_airesult_resultdata(cMultiParm& dest, sScrMsg* msg);
    static void access_aiobjactres_target(cMultiParm& dest, sScrMsg* msg);
    static void access_aipatrolpoint_obj(cMultiParm& dest, sScrMsg* msg);
    static void access_aisignal_signal(cMultiParm& dest, sScrMsg* msg);
    static void access_attack_weapon(cMultiParm& dest, sScrMsg* msg);
    static void access_combine_combiner(cMultiParm& dest, sScrMsg* msg);
    static void access_contained_event(cMultiParm& dest, sScrMsg* msg);
    static void access_contained_container(cMultiParm& dest, sScrMsg* msg);
    static void access_container_event(cMultiParm& dest, sScrMsg* msg);
    static void access_container_containee(cMultiParm& dest, sScrMsg* msg);
    static void access_damage_kind(cMultiParm& dest, sScrMsg* msg);
    static void access_damage_damage(cMultiParm& dest, sScrMsg* msg);
    static void access_damage_culprit(cMultiParm& dest, sScrMsg* msg);
    static void access_difficulty_difficulty(cMultiParm& dest, sScrMsg* msg);
    static void access_door_actiontype(cMultiParm& dest, sScrMsg* msg);
    static void access_door_prevactiontype(cMultiParm& dest, sScrMsg* msg);
#if (_DARKGAME == 3) || ((_DARKGAME == 2) && (_NETWORKING == 1))
    static void access_door_isproxy(cMultiParm& dest, sScrMsg* msg);
#endif
    static void access_frob_srcobj(cMultiParm& dest, sScrMsg* msg);
    static void access_frob_dstobj(cMultiParm& dest, sScrMsg* msg);
    static void access_frob_frobber(cMultiParm& dest, sScrMsg* msg);
    static void access_frob_srcloc(cMultiParm& dest, sScrMsg* msg);
    static void access_frob_dstloc(cMultiParm& dest, sScrMsg* msg);
    static void access_frob_sec(cMultiParm& dest, sScrMsg* msg);
    static void access_frob_abort(cMultiParm& dest, sScrMsg* msg);
    static void access_body_action(cMultiParm& dest, sScrMsg* msg);
    static void access_body_motion(cMultiParm& dest, sScrMsg* msg);
    static void access_body_flagvalue(cMultiParm& dest, sScrMsg* msg);
    static void access_pick_prevstate(cMultiParm& dest, sScrMsg* msg);
    static void access_pick_newstate(cMultiParm& dest, sScrMsg* msg);
    static void access_phys_submod(cMultiParm& dest, sScrMsg* msg);
    static void access_phys_collobj(cMultiParm& dest, sScrMsg* msg);
    static void access_phys_colltype(cMultiParm& dest, sScrMsg* msg);
    static void access_phys_collsubmod(cMultiParm& dest, sScrMsg* msg);
    static void access_phys_collmomentum(cMultiParm& dest, sScrMsg* msg);
    static void access_phys_collnormal(cMultiParm& dest, sScrMsg* msg);
    static void access_phys_collpt(cMultiParm& dest, sScrMsg* msg);
    static void access_phys_contacttype(cMultiParm& dest, sScrMsg* msg);
    static void access_phys_contactobj(cMultiParm& dest, sScrMsg* msg);
    static void access_phys_contactsubmod(cMultiParm& dest, sScrMsg* msg);
    static void access_phys_transobj(cMultiParm& dest, sScrMsg* msg);
    static void access_phys_transsubmod(cMultiParm& dest, sScrMsg* msg);
    static void access_report_warnlevel(cMultiParm& dest, sScrMsg* msg);
    static void access_report_flags(cMultiParm& dest, sScrMsg* msg);
    static void access_report_type(cMultiParm& dest, sScrMsg* msg);
    static void access_report_textbuffer(cMultiParm& dest, sScrMsg* msg);
    static void access_room_fromobjid(cMultiParm& dest, sScrMsg* msg);
    static void access_room_toobjid(cMultiParm& dest, sScrMsg* msg);
    static void access_room_moveobjid(cMultiParm& dest, sScrMsg* msg);
    static void access_room_objtype(cMultiParm& dest, sScrMsg* msg);
    static void access_room_transitiontype(cMultiParm& dest, sScrMsg* msg);
    static void access_slay_culprit(cMultiParm& dest, sScrMsg* msg);
    static void access_slay_kind(cMultiParm& dest, sScrMsg* msg);
    static void access_schemadone_coords(cMultiParm& dest, sScrMsg* msg);
    static void access_schemadone_target(cMultiParm& dest, sScrMsg* msg);
    static void access_schemadone_name(cMultiParm& dest, sScrMsg* msg);
    static void access_sounddone_coords(cMultiParm& dest, sScrMsg* msg);
    static void access_sounddone_target(cMultiParm& dest, sScrMsg* msg);
    static void access_sounddone_name(cMultiParm& dest, sScrMsg* msg);
    static void access_stim_stimulus(cMultiParm& dest, sScrMsg* msg);
    static void access_stim_intensity(cMultiParm& dest, sScrMsg* msg);
    static void access_stim_sensor(cMultiParm& dest, sScrMsg* msg);
    static void access_stim_source(cMultiParm& dest, sScrMsg* msg);
    static void access_timer_name(cMultiParm& dest, sScrMsg* msg);
    static void access_tweq_type(cMultiParm& dest, sScrMsg* msg);
    static void access_tweq_op(cMultiParm& dest, sScrMsg* msg);
    static void access_tweq_dir(cMultiParm& dest, sScrMsg* msg);
    static void access_waypoint_mterr(cMultiParm& dest, sScrMsg* msg);
    static void access_movingterr_waypoint(cMultiParm& dest, sScrMsg* msg);
    static void access_quest_name(cMultiParm& dest, sScrMsg* msg);
    static void access_quest_oldvalue(cMultiParm& dest, sScrMsg* msg);
    static void access_quest_newvalue(cMultiParm& dest, sScrMsg* msg);
    static void access_mediumtrans_from(cMultiParm& dest, sScrMsg* msg);
    static void access_mediumtrans_to(cMultiParm& dest, sScrMsg* msg);
    static void access_yorno_yorn(cMultiParm& dest, sScrMsg* msg);
    static void access_keypad_code(cMultiParm& dest, sScrMsg* msg);

    static AccessorMap message_access;
    static const int   MAX_NAMESIZE;
};

#endif
