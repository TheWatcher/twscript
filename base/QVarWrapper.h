
#ifndef QVARWRAPPER_H
#define QVARWRAPPER_H

#include <string>

/** Fetch the value in the specified QVar if it exists, return the default
 *  if it does not.
 *
 * @param qvar    The name of the QVar to return the value of.
 * @param def_val The default value to return if the qvar does not exist.
 * @return The QVar value, or the default specified.
 */
float get_qvar(const std::string& qvar, float def_val);


/** Fetch the value in the specified QVar if it exists, return the default
 *  if it does not.
 *
 * @param qvar    The name of the QVar to return the value of.
 * @param def_val The default value to return if the qvar does not exist.
 * @return The QVar value, or the default specified.
 */
int get_qvar(const std::string& qvar, int def_val);


#endif // QVARWRAPPER_H
