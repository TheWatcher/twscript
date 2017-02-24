

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


float TWBaseScript::get_qvar_value(std::string& qvar, float def_val)
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



float QVarCalculation::get_qvar(const char* qvar, float def_val)
{
    SService<IQuestSrv> QuestSrv(g_pScriptManager);
    if(QuestSrv -> Exists(qvar))
        return static_cast<float>(QuestSrv -> Get(qvar));

    return def_val;
}



bool QVarCalculation::parse_calculation(const std::string& calculation)
{
    char* buffer = new char[calculation.length() + 1];
    if(!buffer) return false;

    // Copy the calculation over, and get a pointer to the end of the string
    char* endptr = remove_whitespace(calculation.c_str(), buffer, calculation.length() + 1);

    /* So, what we have now is one of the following:
     *
     * 1. the empty string, endptr == buffer, no calculation, nothing to do
     * 2. a number (treat int as float with implicit .0)
     * 3. a qvar name (string of characters starting with $)
     * 4. a number, an operator, a number (which we can always simplify to a single value)
     * 5. a number, an operator, a qvar
     * 6. a qvar, an operator, a number
     * 7. a qvar, an operator, a qvar
     *
     * Ignoring 1 - because it's boring - we need to establish some rules to prevent
     * this becoming a cavern of woe and sorrow spiders:
     *
     * - for numbers we must insist on fully specifying them, so .5 is not
     *   valid while 0.5 is.
     *
     * - for qvars we have a problem: the dark engine doesn't seem to have
     *   *any restrictions* on quest variable names: ANY non-NUL ascii
     *   character appears to be usable in qvar names. In the pathological
     *   case, somthing like '$foobar-4.5*bob-3' /IS A VALID QUEST VAR NAME/.
     *   Needless to say, allowing such names would make it nearly impossible
     *   to support
     *
     * numbers are either always (\d+|\d*\.\d+)
     * a qvar always starts with $,
     * operator is always followed by either '$' introducing a qvar, or a digit

     */

    // Search for an operator
    char* op = find_operator(buffer, endptr);



}


char* QVarCalculation::remove_whitespace(const char* src, char* dst, size_t len)
{
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


QVarCalculation::CalcType QVarCalculation::is_operator(const char ch)
{
    switch(ch) {
        case('+'): return CALCOP_ADD;  break;
        case('-'): return CALCOP_SUB;  break;
        case('*'): return CALCOP_MULT; break;
        case('/'): return CALCOP_DIV;  break;
    }

    return CALCOP_NONE;
}


char* find_operator(const char* buffer, const char* endptr)
{

    while(endptr > buffer) {
        if(is_operator(*endptr)
