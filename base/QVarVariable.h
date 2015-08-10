
#ifndef QVARVARIABLES_H
#define QVARVARIABLES_H

#include <string>
#include <cmath>

class QVarVariable
{
private:

    enum VarType {
        VT_FLOAT = 0, //!< Identifies the variable as a float type
        VT_INT,       //!< Identifies the variable as an int type
        VT_TIME,      //!< Identifies the variable as a time variable
        VT_BOOL       //|< Identifies the variable as a bool variable
    };


public:
    /** Create a new QVarVariable. This just initialises everything with the
     *  minimal values - in order to be useful, init() must be called.
     */
    QVarVariable(int obj): obj_id(obj), lhs_qvar(), op(0), rhs_qvar(), rhs_val(0.0f), value_cache(0.0f)
        { /* fnord */ }


    /** Destroy the QVarVariable object.
     */
    virtual ~QVarVariable()
        { /* fnord */ }


    /** Initialise the QVarVariable, using the variable in the
     *  design note to determine what the value should be, or which
     *  qvar(s) to look at to calculate the value.
     *
     * @param design_val The string containing the value set in the design note.
     * @param defval     The default value to use for the variable.
     * @param listen     Subscribe to any changes in the qvars set for the variable.
     */
    void init_float(const char* design_val, float defval = 0.0f, bool listen = false)
        { init(design_val, defval, listen, VT_FLOAT); }


    /** Initialise the QVarVariable, using the variable in the
     *  design note to determine what the value should be, or which
     *  qvar(s) to look at to calculate the value.
     *
     * @param design_val The string containing the value set in the design note.
     * @param defval     The default value to use for the variable.
     * @param listen     Subscribe to any changes in the qvars set for the variable.
     */
    void init_integer(const char* design_val, int defval = 0, bool listen = false)
        { init(design_val, static_cast<float>(defval), listen, VT_INT); }


    /** Initialise the QVarVariable, using the variable in the
     *  design note to determine what the value should be, or which
     *  qvar(s) to look at to calculate the value.
     *
     * @param design_val The string containing the value set in the design note.
     * @param defval     The default value to use for the variable.
     * @param listen     Subscribe to any changes in the qvars set for the variable.
     */
    void init_time(const char* design_val, int defval = 0, bool listen = false)
        { init(design_val, static_cast<float>(defval), listen, VT_TIME); }


    /** Initialise the QVarVariable, using the variable in the
     *  design note to determine what the value should be, or which
     *  qvar(s) to look at to calculate the value.
     *
     * @param design_val The string containing the value set in the design note.
     * @param defval     The default value to use for the variable.
     * @param listen     Subscribe to any changes in the qvars set for the variable.
     */
    void init_boolean(const char* design_val, bool defval = false, bool listen = false)
        { init(design_val, (defval ? 1.0f : 0.0f), listen, VT_BOOL); }


    operator int ()
        { return static_cast<int>(round(update_value())); }

    operator float ()
        { return update_value(); }

    operator bool ()
        { return (floorf(update_value()) != 0); }

private:

    void init(const char* design_val, float defval, bool listen, VarType typeinfo);


    void process_variable(std::string& src, std::string& qvar, float& value, float defval, bool listen, VarType typeinfo);


    void process_time(std::string& src, float& value, float defval);


    void process_bool(std::string& src, float& value, float defval);


    /** Update the value based on the value in a qvar, potentially applying a
     *  simple calculation in the process.
     *
     * @return The new value for the variable.
     */
    float update_value();


    /** Fetch the value in the specified QVar if it exists, return the default if it
     *  does not.
     *
     * @param qvar   The name of the QVar to return the value of.
     * @param defval The default value to return if the qvar does not exist.
     * @return The QVar value, or the default specified.
     */
    float get_qvar(std::string &qvar, float defval);

    void parse_qvar_parts(const char* qvar, std::string& left, std::string& right, char *calc_op);

    int         obj_id;   //!< The ID of the object this is a variable for.
    std::string lhs_qvar; //!< The left side qvar of any simple calculation (or the only one, if there is no op/rhs)
    char        op;       //!< The operator to apply, zero if no operator
    std::string rhs_qvar; //!< Possible right side qvar of the calculation
    float       rhs_val;  //!< Possible right side value of the calculation

    float value_cache;    //!< The last value calculated for the variable.
};

#endif