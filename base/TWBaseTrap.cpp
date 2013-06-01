
#include "TWBaseTrap.h"
#include "ScriptLib.h"

void TWBaseTrap::init(int time)
{
    cAnsiStr workstr;
    char *msg;
    char *design_note = GetObjectParams(ObjId());

    // Work out what the turnon and turnoff messages should be
    workstr.FmtStr("%sOn", Name());
    if((msg = GetParamString(design_note, static_cast<const char *>(workstr), "TurnOn")) != NULL) {
        turnon_msg = msg;
        g_pMalloc -> Free(msg);
    }

    workstr.FmtStr("%sOff", Name());
    if((msg = GetParamString(design_note, static_cast<const char *>(workstr), "TurnOff")) != NULL) {
        turnoff_msg = msg;
        g_pMalloc -> Free(msg);
    }

    // Now for use limiting.
    int value, falloff;
    get_param_valuefalloff(design_note, "Count", &value, &falloff);
    count.init(time, 0, value, falloff);

    // Handle modes
    count_mode = get_param_countmode(design_note);

    // Now deal with capacitors
    get_param_valuefalloff(design_note, "OnCapacitor", &value, &falloff);
    on_capacitor.init(time, value, 0, falloff);

    get_param_valuefalloff(design_note, "OffCapacitor", &value, &falloff);
    off_capacitor.init(time, value, 0, falloff);
}
