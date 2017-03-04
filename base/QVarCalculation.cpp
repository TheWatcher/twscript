
#include <cctype>
#include <cstring>
#include <cstdlib>
#include "QVarCalculation.h"
#include <iostream>
#include <cstdio>

/* Anonymous namespace for horrible internal implementation functions that have
 * no business existing on a good and wholesome Earth.
 *
 * All of these are needed to perform character-level operations on a c-style
 * string. In theory they could work on a c++ string, but I've no idea what
 * the overheads of that might be in the context of the dark engine, and no
 * way to profile it. So, I'm going to get it working this way first, then
 * worry about making it less of an offering to Nyarlathotep later.
 */
namespace {
    char* remove_whitespace(const char* src, char* dst, size_t len)
    {
        --len; // account for nul terminator
        while(*src && len) {
            if(!isspace(*src)) {
                *dst = *src;
                ++dst;
                --len;
            }
            ++src;
        }
        *dst = '\0';

        return dst;
    }


    char char_at(const char* bstart, const char* bend, const char* current, int offset)
    {
        current += offset;

        if(current < bstart || current > bend) {
            return 0;
        } else {
            return *current;
        }
    }


    QVarCalculation::CalcType is_operator(const char ch)
    {
        switch(ch) {
            case('+'): return QVarCalculation::CALCOP_ADD;  break;
            case('-'): return QVarCalculation::CALCOP_SUB;  break;
            case('*'): return QVarCalculation::CALCOP_MULT; break;
            case('/'): return QVarCalculation::CALCOP_DIV;  break;
        }

        return QVarCalculation::CALCOP_NONE;
    }


    char* find_operator(char* buffer, const char* endptr)
    {
        char* current = buffer;

        // Starting at the end and working backwards may not may any real
        // difference in practice,
        while(current < endptr) {

            // Does the current character look like an operator?
            if(is_operator(*current) != QVarCalculation::CALCOP_NONE) {

                // It can only really be one if it's followed by $, a digit, or - and a digit
                // and preceeded by a digit, or a letter and the first char is $
                if((char_at(buffer, endptr, current, 1) == '$' ||
                    isdigit(char_at(buffer, endptr, current, 1)) ||
                    (char_at(buffer, endptr, current, 1) == '-' && isdigit(char_at(buffer, endptr, current, 2))))
                   &&
                   (isdigit(char_at(buffer, endptr, current, -1)) ||
                    (isalpha(char_at(buffer, endptr, current, -1)) && *buffer == '$'))) {
                    return current;
                }
            }

            ++current;
        }

        return NULL;
    }


    bool parse_value(const char* str, float& store)
    {
        char* end = NULL;

        store = strtof(str, &end);

        // All we can really do is tell if /something/ has been parsed. We could
        // required that *end == '\0' too, but that may be excessive
        return (end != str);
    }
}



bool QVarCalculation::init(const std::string& calculation, const bool add_listeners)
{
    bool parsed = parse_calculation(calculation);

    // If listeners need to be added, sort that now
    if(parsed && add_listeners) {
        // TODO
    }

    return parsed;
}





/* ------------------------------------------------------------------------
 *  QVar convenience functions
 */


/*float TWBaseScript::get_qvar_value(std::string& qvar, float def_val)
{
    float value = def_val;
    char  op;
    char* lhs_qvar, *rhs_data, *endstr = NULL;
    char* buffer = parse_qvar(qvar.c_str(), &lhs_qvar, &op, &rhs_data);

    if(buffer) {
        value = get_qvar(lhs_qvar, def_val);

        // If an operation and right hand side value/qvar were found, use them
        if(op && rhs_data) {
            float adjval = 0;

            // Is the RHS a qvar itself? If so, fetch it, otherwise treat it as an int
            if(*rhs_data == '$') {
                adjval = get_qvar(&rhs_data[1], 0.0f);
            } else {
                adjval = strtof(rhs_data, &endstr);
            }

            // If a value was parsed in some way, apply it (this also avoids
            // division-by-zero problems for / )
            if(endstr != rhs_data && adjval) {
                switch(op) {
                    case('+'): value += adjval; break;
                    case('-'): value -= adjval; break;
                    case('*'): value *= adjval; break;
                    case('/'): value /= adjval; break;
                }
            }
        }

        delete[] buffer;
    }

    return value;
}



float QVarCalculation::get_qvar(const std::string& qvar, float def_val)
{
    SService<IQuestSrv> QuestSrv(g_pScriptManager);
    if(QuestSrv -> Exists(qvar.c_str()))
        return static_cast<float>(QuestSrv -> Get(qvar.c_str()));

    return def_val;
}*/


bool QVarCalculation::parse_calculation(const std::string& calculation)
{
    char* buffer = new char[calculation.length() + 1];
    if(!buffer) return false;

    // working pointers for left and right sides of the calculation
    char* leftside  = buffer;
    char* rightside = NULL;

    // Copy the calculation over, and get a pointer to the end of the string
    char* endptr = remove_whitespace(calculation.c_str(), leftside, calculation.length() + 1);

    /* So, what we have now is one of the following:
     *
     * 0. the empty string, endptr == leftside, no calculation, nothing to do
     *
     * 1. a number (potentially with sign and fractional part)
     * 2. a qvar name (string of characters starting with $)
     *
     * 3. a number, an operator, a number (which we can always simplify to 1)
     * 4. a number, an operator, a qvar
     * 5. a qvar, an operator, a number
     * 6. a qvar, an operator, a qvar
     *
     * Ignoring 0 - because it's boring - we need to establish some rules to
     * prevent this becoming a cavern of woe and sorrow spiders. In particular,
     * as far as qvars go, we have a problem: the dark engine doesn't seem to
     * have *any restrictions* on quest variable names: literally ANY
     * NUL-terminated ascii character string appears to be usable as a qvar
     * name. In the pathological case, somthing like '$foobar-4.5*bob-3'
     * /IS A VALID QUEST VAR NAME/. Needless to say, allowing such names would
     * involve  of code to support calculations. So, rules:
     *
     * - for numbers:
     *   - numbers must be fully specified; so .5 is not valid while 0.5 is.
     *   - a leading - indicates a negative number.
     *   - a leading + as an explicit positive is not supported.
     *
     * - for qvars:
     *   - qvars must always be introduced with '$'
     *   - qvar names may only contain alphanumerics, '_', or '-'
     *   - qvar names must start with an alphabet character, not a number
     *   - if '-' appears in a qvar name, it MUST be followed by a alphabet
     *     character (upper or lower case). So "Q-foobar" is valid, but
     *     "Qfoobar-1" is not.
     *
     * From this we can state that operator is always:
     *   - preceeded by either a digit, or $ followed by at least one char.
     *   - followed by either '$' (a qvar), a - and a digit, or a digit.
     *
     */

    bool parsed = false;

    // So, that situation 0 above....
    if(leftside != endptr) {

        // Search for an operator
        char* op = find_operator(leftside, endptr);

        // If we have an operator, store and replace it with nul - that gives us a
        // LHS at leftside, and a RHS at op + 1
        if(op) {
            calc_op = is_operator(*op);
            *op = '\0';
            rightside = op + 1;
        }

        // If we get here, regardless of whether the above found an operator, we
        // have a LHS. This may be either a qvar (so, leading '$' followed by
        // a letter), or a number. Note that malformed qvars will be treated
        // as numbers, which should inevitably fail the parse_value() call
        // and result in false from this function.
        if(*leftside == '$' && strlen(leftside) > 1 && isalpha(*(leftside + 1))) {
            lhs_qvar = ++leftside;
            parsed = true;
        } else {
            parsed = parse_value(leftside, lhs_val);
        }

        // rightside only does anything if there is one
        if(rightside) {
            if(*rightside == '$' && strlen(rightside) > 1 && isalpha(*(rightside + 1))) {
                rhs_qvar = ++rightside;
                parsed = true;
            } else {
                parsed = parse_value(rightside, rhs_val);
            }
        }
    }

    // No need to retain the temporary buffer anymore
    delete buffer;

    return parsed;
}


void QVarCalculation::dump()
{
    std::cout << "left side qvar: '" << lhs_qvar << "'\n";
    std::cout << "left side value: '" << lhs_val << "'\n";
    std::cout << "right side qvar: '" << rhs_qvar << "'\n";
    std::cout << "right side value: '" << rhs_val << "'\n";
    if(calc_op) {
        std::cout << "Operation: " << calc_op << "\n";
    } else {
        std::cout << "No operation defined\n";
    }
}

int main(int argc, char** argv)
{
    QVarCalculation calc;

    bool status = calc.init(argv[1]);
    std::cout << "Parsing " << (status ? "success" : "failed") << "\n";
    calc.dump();

    return 0;
}
