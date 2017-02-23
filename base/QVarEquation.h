
#include <string>

class QVarEquation
{
protected:
    /** The supported calculation types for qvar_eq parameters.
     */
    enum CalcType {
        CALCOP_NONE = '\0', //!< No operation, only LHS set
        CALCOP_ADD  = '+',  //!< Add LHS and RHS
        CALCOP_SUB  = '-',  //!< Subtract RHS from LHS
        CALCOP_MULT = '*',  //!< Multiply the LHS and RHS
        CALCOP_DIV  = '/'   //!< Divide the LHS by the RHS.
    };


public:
    /** Create a new QVarEquation. This creates an empty, unitialised equation
     *  that must be intialised before it can produce useful values.
     *
     */
    QVarEquation() : lhs_qvar("") , rhs_qvar(""),
                     lhs_val(0.0f), rhs_val(0.0f),
                     calc_op(CALCOP_NONE)
        { /* fnord */ }


    /** Initialise the QVarEquation. This will attempt to parse the specified
     *  equation string into a left hand side, with possibly an operator and
     *  a right-hand sidde. This implements the qvar-eq rule in the design
     *  note specification.
     *
     * @param equation A reference to a string containing the qvar equation
     *                 to parse.
     * @param add_listeners If true, add qvar change listeners for any qvars
     *                 found in the specified equation. Note that the client
     *                 *MUST* call unsubscribe() on EndScript if this is set
     *                 to true.
     * @return true if the QVarEquation has been initialised successfully,
     *         false if it has not.
     */
    bool init(const std::string& equation, const bool add_listeners = false);


    float value();

protected:
    /** Given a parameter string, attempt to parse it as a qvar_eq.
     *  This attempts to parse the specified string based on the rules
     *  defined for the qvar_eq rule in the design_note.abnf file.
     *
     * @param parameter A reference to a string containing the qvar_eq to parse.
     * @return true if parsing completed successfully, false on error.
     */
    bool parse_parameter(const std::string& parameter);


private:
    /** Fetch the value in the specified QVar if it exists, return the default if it
     *  does not.
     *
     * @param qvar    The name of the QVar to return the value of.
     * @param def_val The default value to return if the qvar does not exist.
     * @return The QVar value, or the default specified.
     */
    float get_qvar(const char* name, float def_val);


    /** Parse the pieces of a qvar name string, potentially including a simple calculation.
     *  The takes a string containing a qvar name, and potentially an operator and either
     *  a number or another qvar, and stores pointers to the two sides of the operator, plus
     *  the operator itself, in the provided pointers.
     *
     * @param qvar A pointer to the string containing the qvar name.
     * @param lhs  A pointer to a string pointer in which to store a pointer to the left
     *             hand side operand.
     * @param op   A pointer to a char to store the operator in, if there is one.
     * @param rhs  A pointer to a string pointer in which to store a pointer to the right
     *             hand side operand, if there is one.
     * @return A pointer to a buffer containing a processed version of `qvar`. This should
     *         be freed by the called using `g_pMalloc -> Free()`.
     */
    char* parse_qvar(const char* qvar, char** lhs, char* op, char** rhs);


    std::string lhs_qvar; //!< The name of the qvar on the left side of any calculation, if any
    std::string rhs_qvar; //!< The name of the qvar on the right side of any calculation
    float       lhs_val;  //!< The literal value on the left side if no qvar specified
    float       rhs_val;  //!< The literal value on the right side if no qvar specified
    CalcType    calc_op;  //!< The operation to apply. If this is CALCOP_NONE, only the LHS is considered.
};
