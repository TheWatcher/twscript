

bool QVarEquation::init(const std::string& equation, const bool add_listeners)
{
    bool parsed = parse_equation(equation);

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



float QVarEquation::get_qvar(const char* qvar, float def_val)
{
    SService<IQuestSrv> QuestSrv(g_pScriptManager);
    if(QuestSrv -> Exists(qvar))
        return static_cast<float>(QuestSrv -> Get(qvar));

    return def_val;
}



bool QVarEquation::parse_equation(const std::string& equation)
{
    char* buffer = new char[equation.length() + 1];
    if(!buffer) return false;

    // Copy the equation over, and get a pointer to the end of the string
    char* endptr = remove_whitespace(equation.c_str(), buffer, equation.length() + 1);

    // Search for an operator
    char* op = find_operator(buffer, endptr);



}


char* remove_whitespace(const char* src, char* dst, size_t len)
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


bool is_operator(const char ch)
{


}


char* find_operator(const char* buffer, const char* endptr)
{

    while(endptr > buffer) {
        if(is_operator(*endptr)
