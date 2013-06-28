
#ifndef TWMESSAGETOOLS_H
#define TWMESSAGETOOLS_H

class TWMessageTools
{

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

};

#endif
