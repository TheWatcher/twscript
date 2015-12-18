
#include "DesignParam.h"

bool DesignParam::get_param_string(const std::string& design_note, std::string& parameter)
{
    // Use Telliamed's code to fetch the string. If it isn't set, just return false.
    // FIXME: Can this be replaced with something cleaner?
    const char *value = GetParamString(design_note.c_str(), fullname.c_str, NULL);
    if(!value) return false;

    // Update the parameter value store; there's no need to retain the source string
    parameter = value;
    g_pMalloc -> Free(value);

    return true;
}



bool DesignParamString::init(const std::string& design_note, const std::string& default_value)
{
    // For string parameters, all we need to do is fetch the parameter as a string; no
    // validation or processing is needed beyond this.
    bool valid = get_param_string(design_note, data);

    // Allow for fallback if the parameter is not set in the design note
    if(!valid) data = default_value;

    is_set(value);

    // note that init returns true on success
    return true;
}
