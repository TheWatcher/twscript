
/** @file
 * This file is the header for the Quest Variable Calculation class.
 *
 * @author Chris Page &lt;chris@starforge.co.uk&gt;
 *
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

#include <string>

/** This class provides a way to encapsulate quest variable calculations
 *  as given in the design note spcification.
 */
class QVarCalculation
{

public:
    /** The supported calculation types
     */
    enum CalcType {
        CALCOP_NONE = '\0', //!< No operation, only LHS set
        CALCOP_ADD  = '+',  //!< Add LHS and RHS
        CALCOP_SUB  = '-',  //!< Subtract RHS from LHS
        CALCOP_MULT = '*',  //!< Multiply the LHS and RHS
        CALCOP_DIV  = '/'   //!< Divide the LHS by the RHS.
    };


    /** Create a new QVarCalculation. This creates an empty, uninitialised
     *  calculation that must be initialised before it can produce useful
     *  values.
     */
    QVarCalculation() : host(0),
                        lhs_qvar("") , rhs_qvar(""),
                        lhs_val(0.0f), rhs_val(0.0f),
                        calc_op(CALCOP_NONE)
        { /* fnord */ }


    /** Initialise the QVarCalculation. This will attempt to parse the specified
     *  calculation string into a left hand side, with possibly an operator and
     *  a right-hand sidde. This implements the qvar-eq rule in the design
     *  note specification.
     *
     * @param hostid The ID of the object this calculation is attached to.
     * @param calculation A reference to a string containing the qvar calculation
     *                 to parse.
     * @param add_listeners If true, add qvar change listeners for any qvars
     *                 found in the specified calculation. Note that the client
     *                 *MUST* call unsubscribe() on EndScript if this is set
     *                 to true.
     * @return true if the QVarCalculation has been initialised successfully,
     *         false if it has not.
     */
    bool init(int hostid, const std::string& calculation, const bool add_listeners = false);


    /** Remove any subscriptions created during init. If no subscriptions
     *  were created, this does not need to be called, but it should be
     *  safe to call regardless.
     */
    void unsubscribe() const;


    /** Retrieve the current value of the QVarCalculation. This will fetch the
     *  current value of any qvars used in the calculation, apply the
     *  calculation to the value, and return the result.
     *
     * @return The result of the calculation. If init() has not been called, or
     *         the right side of a calculation using the divide operator is
     *         zero, this returns 0.
     */
    float value(); // can't be const, as qvars may update values


protected:
    /** Given a parameter string, attempt to parse it as a qvar_eq.
     *  This attempts to parse the specified string based on the rules
     *  defined for the qvar_eq rule in the design_note.abnf file.
     *
     * @param parameter A reference to a string containing the qvar_eq to parse.
     * @return true if parsing completed successfully, false on error.
     */
    bool parse_calculation(const std::string& calculation);


private:
    int         host;     //!< The ID of the host object this calculation is attached to
    std::string lhs_qvar; //!< The name of the qvar on the left side of any calculation, if any
    std::string rhs_qvar; //!< The name of the qvar on the right side of any calculation
    float       lhs_val;  //!< The literal value on the left side if no qvar specified
    float       rhs_val;  //!< The literal value on the right side if no qvar specified
    CalcType    calc_op;  //!< The operation to apply. If this is CALCOP_NONE, only the LHS is considered.
};
