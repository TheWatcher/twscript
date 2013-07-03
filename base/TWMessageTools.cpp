
#include "TWMessageTools.h"
#include <cstdio>

//  Longest message type: "sDarkGameModeScrMsg" (19 chars)
// Longest message field: "PrevActionType" (14 chars) in sDoorMsg
//                        "TransitionType" (14 chars) in sRoomMsg
//                        "moving_terrain" (14 chars) in sWaypointMsg
// So, type+field+dot+nul is the minimum space needed, 19+14+1+1 = 35
const int TWMessageTools::MAX_NAMESIZE = 35;

const char* TWMessageTools::get_message_type(sScrMsg* msg)
{
    /* Okay, seriously, what the fuck. Attempting actual RTTI on msg with typeid(*msg)
     * will crash, I guess due to a corrupt vtable. At least, that's the only thing I
     * can think of that will cause that in this case. So, calling msg -> GetName()?
     * That crashes too, at least when using liblg.a compiled in gcc, again I suspect
     * because of vtable corruption or simply :fullofspiders:. However, going into the
     * "hack" vtable seems to work, because madness.
     *
     * Lasciate ogne speranza, voi ch'intrate, damnit.
     */
#ifdef _MSC_VER
    return msg -> GetName();  // No idea if this actually works in MSVC, see scrmsgs.h for more
#else
    // This is the horrible mess that seems to work fine under GCC. Don't ask me,
    // this shit is messed up.
    return msg -> persistent_hack -> thunk_GetName();
#endif
}


bool TWMessageTools::get_message_field(cMultiParm& dest, sScrMsg* msg, const char* field)
{
    // Yes, this is horrible and nasty, but stack allocation is much faster
    // than heap allocation in a std::string or cAnsiStr, and this mess is
    // going to be slower than I'd like anyway, so bleegh
    char namebuffer[MAX_NAMESIZE];

    const char* mtypename = get_message_type(msg);
    if(mtypename) {
        snprintf(namebuffer, MAX_NAMESIZE, "%s.%s", mtypename, field);

        AccessorIter iter = message_access.find(namebuffer);
        if(iter != message_access.end()) {
            (*iter -> second)(dest, msg);
            return true;
        }
    }

    return false;
}


void TWMessageTools::access_msg_from(cMultiParm& dest, sScrMsg* msg)
{
    dest = msg -> from;
}


void TWMessageTools::access_msg_to(cMultiParm& dest, sScrMsg* msg)
{
    dest = msg -> to;
}


AccessorMap TWMessageTools::message_access = {
    { "sScrMsg.from", TWMessageTools::access_msg_from },
    { "sScrMsg.to"  , TWMessageTools::access_msg_to   },
};
