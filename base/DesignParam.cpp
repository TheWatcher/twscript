
#include "DesignParam.h"

bool DesignParam::get_param_string(std::string& design_note, std::string& parameter)
{
    std::string namestr = script_name + param_name;

    // Use Telliamed's code to fetch the string. If it isn't set, just return false.
    // FIXME: Can this be replaced with something cleaner?
    const char *value = GetParamString(design_note.c_str(), namestr.c_str, NULL);
    if(!value) {
        is_set(false);

    } else {
        is_set(true);

        // Update the parameter value store; there's no need to retain the source string
        parameter = value;
        g_pMalloc -> Free(value);
    }

    return is_set();
}



bool DesignParamString::init(const std::string& design_note, const std::string& default_value)
{

}
