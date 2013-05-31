
void TWBaseTrap::init(int time)
{
    cAnsiStr workstr;
    char *msg;
    char *design_note = GetObjectParams(ObjId());

    // Work out what the turnon and turnoff messages should be
    workstr.FmtStr("%sOn", Name());
    if(msg = GetParamString(design_note, static_cast<const char *>(workstr), "TurnOn")) {
        turnon_msg = msg;
        g_pMalloc -> Free(msg);
    }

    workstr.FmtStr("%sOff", Name());
    if(msg = GetParamString(design_note, static_cast<const char *>(workstr), "TurnOff")) {
        turnoff_msg = msg;
        g_pMalloc -> Free(msg);
    }

    // Now for use limiting.
    int value, falloff;
    parse_valuefalloff(design_note, "Count", &value &falloff);
    count.init(time, 0, value, falloff);

    // Handle modes
    count_mode = parse_countmode(design_note);

    // Now deal with capacitors
    parse_valuefalloff(design_note, "OnCapacitor", &value, &falloff);
    on_capacitor.init(time, value, 0, falloff);

    parse_valuefalloff(design_note, "OffCapacitor", &value, &falloff);
    off_capacitor.init(time, value, 0, falloff);
}


void TWBaseTrap::parse_valuefalloff(char *design_note, const char *param, int *value, int *falloff)
{
    cAnsiStr workstr;

    // Get the value
    if(value) {
        workstr.FmtStr("%s%s", Name(), param);
        *value = GetParamInt(design_note, static_cast<const char *>(workstr), 0);
    }

    // Allow uses to fall off over time
    if(falloff) {
        workstr.FmtStr("%s%sFalloff", Name(), param);
        *falloff = GetParamInt(design_note, static_cast<const char *>(workstr), 0);
    }
}


CountMode TWBaseTrap::parse_countmode(char *design_note, CountMode default)
{
    cAnsiStr workstr;
    CountMode result = default;

    workstr.FmtStr("%sCountOnly", Name());
    char *mode = GetParamString(design_note, static_cast<const char *>(workstr), "Both");

    // The editor has specified /something/ for CountOnly, so try to work out what
    if(mode) {
        char *end = NULL;

        // First up, art thou an int?
        int parsed = strtol(mode, &end, 10);
        if(mode != end) {
            // Well, something parsed, only update the result if it is in range!
            if(parsed >= CM_NOTHING && parsed <= CM_BOTH) {
                result = parsed;
            }

        // Doesn't appear to be numeric, so search for known modes
        } else if(!::_stricmp(mode, "None")) {
            result = CM_NOTHING;
        } else if(!::_stricmp(mode, "On")) {
            result = CM_TURNON;
        } else if(!::_stricmp(mode, "Off")) {
            result = CM_TURNOFF;
        } else if(!::_stricmp(mode, "Both")) {
            result = CM_BOTH;
        }

        g_pMalloc -> Free(mode);
    }

    return result;
}
