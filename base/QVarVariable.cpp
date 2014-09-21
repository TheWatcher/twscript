#include <lg/interface.h>
#include <lg/scrmanagers.h>
#include <lg/scrservices.h>
#include <string>
#include <cstdlib>
#include "QVarVariable.h"
#include "ScriptLib.h"


void QVarVariable::init(const char* design_val, float defval, bool listen, VarType typeinfo)
{
    // This will get overwritten if there is a valid value available.
    value_cache = defval;

    // Can only do any parsing if the design_val is set, and the length is non-zero
    if(design_val || !strlen(design_val)) {
        std::string left, right;
        char calc_op;

        // split the value up as much as possible
        parse_qvar_parts(design_val, left, right, &calc_op);

        // Do we even have a left side? If so, process it and a possible right
        if(!left.empty()) {
            process_variable(left, lhs_qvar, value_cache, defval, listen, typeinfo);

            // if there's an operator and a right, deal with them
            if(calc_op && !right.empty()) {
                op = calc_op;
                process_variable(right, rhs_qvar, rhs_val, 0.0f, listen, typeinfo);
            }
        }
    }
}


void QVarVariable::process_variable(std::string& src, std::string& qvar, float& value, float defval, bool listen, VarType typeinfo)
{
    // $ indicates that the source contains a qvar, so store it
    if(src[0] == '$') {
        qvar.assign(src, 1, std::string::npos);

        // add a listener if needed
        if(listen) {
            SService<IQuestSrv> quest_srv(g_pScriptManager);
            quest_srv -> SubscribeMsg(obj_id, qvar.c_str(), kQuestDataAny);
        }

    // source contains something other than a qvar
    } else {

        const char *srcstr = src.c_str();
        char *endstr = NULL;

        // Process the source based on the type of variable this should be.
        switch(typeinfo) {
            case VT_FLOAT:
            case VT_INT:  value = strtof(srcstr, &endstr);
                          if(endstr == srcstr) {
                              value = defval;
                          }
                break;
            case VT_TIME: process_time(src, value, defval);
                break;
            case VT_BOOL: process_bool(src, value, defval);
                break;
            default:
                value = defval;
        }
    }
}


void QVarVariable::process_time(std::string& src, float& value, float defval)
{
    const char *srcstr = src.c_str();
    char *endstr = NULL;

    // The start of the string should be a number.
    value = strtof(srcstr, &endstr);
    if(endstr == srcstr) {
        value = defval;

    } else if(*endstr) {
        // skip spaces, just in case
        while(*endstr && isspace(*endstr)) ++endstr;

        // The next character may be a modifier for seconds and minutes
        switch(*endstr) {
            // 's' indicates the time is in seconds, multiply up to milliseconds
            case 's':
            case 'S': value *= 1000.0f;
                break;

            // 'm' indicates the time is in minutes, multiply up to milliseconds
            case 'm':
            case 'M': value *= 60000.0f;
                break;
        }
    }
}


void QVarVariable::process_bool(std::string& src, float& value, float defval)
{
    const char *srcstr = src.c_str();
    char *endstr = NULL;

    // skip any possible spaces
    while(*srcstr && isspace(*srcstr)) ++srcstr;

    // Default the variable to be safe
    value = defval;
    switch(*srcstr) {
        // There are several possible 'true' values
        case '1':
        case 't':
        case 'T':
        case 'y':
        case 'Y': value = 1.0f;
            break;

        // If it's not a known true, try parsing as a number
        default:  value = strtof(srcstr, &endstr);
                  if(endstr == srcstr) {
                      value = defval;
                  }
    }
}


float QVarVariable::update_value()
{
    // No qvar to pull from? Just return the initial value
    if(lhs_qvar.empty()) return value_cache;

    // Pull the left (or perhaps only) value
    value_cache = get_qvar(lhs_qvar, value_cache);

    // If there is no operator, there's nothing else to do
    if(!op) return value_cache;

    // There's an operator, so work out what's on the rhs
    float right = get_qvar(rhs_qvar, rhs_val);

    switch(op) {
        case('+'): value_cache += right; break;
        case('-'): value_cache -= right; break;
        case('*'): value_cache *= right; break;
        case('/'): value_cache /= right; break;
    }

    return value_cache;
}


float QVarVariable::get_qvar(std::string &qvar, float defval)
{
    // Can't do anything if there's no qvar name
    if(qvar.empty()) return defval;

    // Fetch the value if possible
    SService<IQuestSrv> QuestSrv(g_pScriptManager);
    if(QuestSrv -> Exists(qvar.c_str()))
        return static_cast<float>(QuestSrv -> Get(qvar.c_str()));

    return defval;
}


void QVarVariable::parse_qvar_parts(const char* qvar, std::string& left, std::string& right, char *calc_op)
{
    // Duplicate the string so it can be manipulated. This is rather manky, but it is less brain-hurty.
    char* buffer = new char[strlen(qvar) + 1];
    if(!buffer) return;

    strcpy(buffer, qvar);

    char* workstr = buffer;

    // skip any leading spaces
    while(*workstr && isspace(*workstr)) {
        ++workstr;
    }

    // Anchor the left side
    char* lhs = workstr++;

    // Search for an operator
    char* endstr = NULL;
    *calc_op = '\0';
    while(*workstr) {
        // NOTE: '-' is not included here. LarryG encountered problems with using QVar names containing
        // '-' as this was interpreting it as an operator.
        if(*workstr == '+' || *workstr == '*' || *workstr == '/') {
            *calc_op = *workstr;
            endstr = workstr - 1; // record the character before the operator, for space trimming
            *workstr = '\0';      // terminate so that lhs can potentially be used 'as is'
            ++workstr;
            break;
        }
        ++workstr;
    }

    // Only bother doing any more work if an operator was found
    if(*calc_op && endstr) {
        // Trim spaces before the operator if needed
        while(isspace(*endstr)) {
            *endstr = '\0';
            --endstr;
        }

        // Skip spaces before the second operand
        while(*workstr && isspace(*workstr)) {
            ++workstr;
        }

        // If there is anything left on the right side, store the pointer to it
        if(*workstr) {
            right = workstr;
        }
    }

    left  = lhs;
    delete buffer;
}
