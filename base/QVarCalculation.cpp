
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

#include <lg/interface.h>
#include <lg/scrmanagers.h>
#include <lg/scrservices.h>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <ScriptLib.h>
#include "QVarCalculation.h"

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
    /** Copy from a source string into a destination, discarding any
     *  whitespace in the source. This is guaranteed to include the
     *  nul at the end of the destination.
     *
     * @param src A pointer to the start of the source string.
     * @param dst A pointer to the start of the destination. This must
     *            be at least `len` characters long.
     * @param len The length of the destination string. If the source
     *            string minus any whitespace is longer than this
     *            length, the copy in the destination will be truncated.
     * @return A pointer to the end of the destination string (the
     *         nul at the end of the string).
     */
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


    /** Obtain the character at an offset from a position in a string.
     *  This does a bouinded character loopup on a string, applying an
     *  offset to a pointer into the string, and ensuring that it can
     *  not go out of bounds. Essentially, this is a lookaround function
     *  needed by find_operator to do lookahead/behind.
     *
     * @param sstart  A pointer to the start of the string.
     * @param send    A pointer to the end of the string.
     * @param current A pointer to a location within the string.
     * @param offset  An offset relative to the current position to fetch
     *                the character at.
     * @return The character at the offset from the specified current
     *         location, if it is within the bounds of the string. 0
     *         if it falls outside the string.
     */
    char char_at(const char* sstart, const char* send, const char* current, int offset)
    {
        current += offset;

        if(current < sstart || current > send) {
            return 0;
        } else {
            return *current;
        }
    }


    /** Determine whether the specified character is an operation character.
     *  This will inspect the character provided and if it represents a
     *  known calculation this will return the CalcType of the calculation.
     *
     * @param ch The character to check for operationhood.
     * @return A CalcType for the character; if the character is not an
     *         operation character, this returns CALCOP_NONE.
     */
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


    /** Attempt to locate an operator within the specified buffer. This
     *  will scan the buffer for a character that appears to be an
     *  operator, and then checks that it meets the rules of operatorness.
     *  If it does, a pointer to the operator is returned.
     *
     * @param buffer The buffer to scan for an operator
     * @param endptr A pointer to the end of the buffer.
     * @return A pointer to the operator on success, NULL if no valid
     *         operator is found in the string.
     */
    char* find_operator(char* buffer, const char* endptr)
    {
        char* current = buffer;

        // Starting at the end and working backwards may not may any real
        // difference in practice,
        while(current < endptr) {

            // Does the current character look like an operator?
            if(is_operator(*current) != QVarCalculation::CALCOP_NONE) {

                // It can only really be an op if it's followed by '$', a digit,
                // or '-' and a digit and preceeded by a digit, or a letter and
                // the first char is $
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


    /** Attempt to parse the value in the specified string, and store it in
     *  the provided variable.
     *
     * @param str   A pointer to a string containing the value to parse.
     * @param store A reference to a float to store the parsed value in.
     *              This will not be updated if the function returns false.
     * @return true if the store has been updated with a parsed value, false
     *         if it has not. Note that this will return true for partial
     *         success (some part of `str` is not a valid number, but a value
     *         was parsed from it).
     */
    bool parse_value(const char* str, float& store)
    {
        char* end = NULL;

        float parsed = strtof(str, &end);
        // Only update the store if something was parsed.
        if(end != str) {
            store = parsed;
        }

        // All we can really do is tell if /something/ has been parsed. We could
        // required that *end == '\0' too, but that may be excessive
        return (end != str);
    }


    /* ------------------------------------------------------------------------
     *  QVar convenience functions
     */

    /** Fetch the value in the specified QVar if it exists, return the default
     *  if it does not.
     *
     * @param qvar    The name of the QVar to return the value of.
     * @param def_val The default value to return if the qvar does not exist.
     * @return The QVar value, or the default specified.
     */
    float get_qvar(const std::string& qvar, float def_val)
    {
        SService<IQuestSrv> quest_srv(g_pScriptManager);
        if(quest_srv -> Exists(qvar.c_str()))
            return static_cast<float>(quest_srv -> Get(qvar.c_str()));

        return def_val;
    }

}


/* ------------------------------------------------------------------------
 *  Public interface functions
 */

bool QVarCalculation::init(const std::string& calculation, const float default_value, const bool add_listeners)
{
    bool parsed = parse_calculation(calculation, default_value);

    // If listeners need to be added, sort that now
    if(parsed && add_listeners) {
        SService<IQuestSrv> quest_srv(g_pScriptManager);

        if(!lhs_qvar.empty()) {
            quest_srv -> SubscribeMsg(host, lhs_qvar.c_str(), kQuestDataAny);
        }

        if(!rhs_qvar.empty()) {
            quest_srv -> SubscribeMsg(host, rhs_qvar.c_str(), kQuestDataAny);
        }
    }

    return parsed;
}


void QVarCalculation::unsubscribe()
{
    SService<IQuestSrv> quest_srv(g_pScriptManager);

    if(!lhs_qvar.empty()) {
        quest_srv -> UnsubscribeMsg(host, lhs_qvar.c_str());
    }

    if(!rhs_qvar.empty()) {
        quest_srv -> UnsubscribeMsg(host, rhs_qvar.c_str());
    }
}


float QVarCalculation::value()
{
    // Update the values from the qvars
    if(!lhs_qvar.empty()) {
        lhs_val = get_qvar(lhs_qvar, 0.0f);
    }

    if(!rhs_qvar.empty()) {
        rhs_val = get_qvar(rhs_qvar, 0.0f);
    }

    // Apply calculation, if any
    switch(calc_op) {
        case(CALCOP_ADD):  return (lhs_val + rhs_val); break;
        case(CALCOP_SUB):  return (lhs_val - rhs_val); break;
        case(CALCOP_MULT): return (lhs_val * rhs_val); break;
        case(CALCOP_DIV):  if(rhs_val == 0.0f) return 0.0f; // prevent divide by zero
                           return (lhs_val / rhs_val);
            break;
        default: return lhs_val;
    }
}


/* ------------------------------------------------------------------------
 *  Calculation parser
 */

bool QVarCalculation::parse_calculation(const std::string& calculation, const float default_value)
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
     *   - if an operator appears in a qvar name, it MUST be followed by a
     *     alphabet character (upper or lower case). So "Q-foobar" is valid,
     *     but "Qfoobar-1" is not.
     *
     * From this we can state that operator is always:
     *   - preceeded by either a digit, or an alphabet character if a '$'
     *     has introduced a qvar.
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

        // optimise the situation where both sides are constants: we can
        // do the calculation once, store it on the lhs, and kill the op
        if(lhs_qvar.empty() && rhs_qvar.empty() && calc_op != CALCOP_NONE) {
            lhs_val = value();
            calc_op = CALCOP_NONE;
        }
    }

    // No need to retain the temporary buffer anymore
    delete buffer;

    // Handle default
    if(!parsed) {
        lhs_val = default_value;
    }

    return parsed;
}
