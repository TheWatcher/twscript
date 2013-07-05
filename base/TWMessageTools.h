
#ifndef TWMESSAGETOOLS_H
#define TWMESSAGETOOLS_H

#include <lg/scrmsgs.h>
#include <map>
#include <cstring>

typedef void (*MessageAccessProc)(cMultiParm&, sScrMsg*);

struct char_icmp
{
    bool operator () (const char* a ,const char* b) const {
        return ::_stricmp(a, b) < 0;
    }
};

/** A map type for fast lookup of accessor functions for named message types.
 *  I'd *much* rather use std::string as the key, but this will need to be
 *  find()able with a char *, and while there is an implicit converstion, it
 *  will require allocation/deallocation overhead. Without a way to profile
 *  the code properly, I can't tell if that overhead is prohibitive... and this
 *  will work, so needs must as the devil drives, and all that.
 */
typedef std::map<const char *, MessageAccessProc, char_icmp> AccessorMap;
typedef AccessorMap::iterator   AccessorIter; //!< Convenience type for AccessorMap iterators
typedef AccessorMap::value_type AccessorPair; //!< Convenience type for AccessorPair key/value pairs


class TWMessageTools
{
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
