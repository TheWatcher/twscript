
#include "TWMsgTools.h"


const char* TWMsgTools::get_message_type(sScrMsg* msg)
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
