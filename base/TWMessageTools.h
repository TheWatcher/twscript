
#ifndef TWMESSAGETOOLS_H
#define TWMESSAGETOOLS_H

#include <lg/scrmsgs.h>
#include <map>
#include <cstring>

typedef bool (*MessageAccessProc)(sMultiParm&, sScrMsg*);

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
    static bool get_message_field(sMultiParm& dest, sScrMsg* msg, const char* field);

private:
    static bool access_sim_msg(sMultiParm& dest, sScrMsg* msg, const char* field);

    static AccessorMap message_access;
    static const int   MAX_NAMESIZE;
};

#endif
