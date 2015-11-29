/** @file
 * This file contains the QVar variable handling base code.
 *
 * @author Chris Page &lt;chris@starforge.co.uk&gt;
 *
 */
/* Okay, let's try to get this right this time. The full description of
 * what at design note can include is given in docs/design_note.abnf
 * Some observations from that:
 *
 * - string is the only parameter type that absolutely can never depend on the
 *   qvar rule directly or indirectly.
 *
 * - object is the only design note parameter type that uses qvar
 *   values directly without supporting calculations (as all we want
 *   in the qvar is the object ID to act upon)
 *
 * - target parameter is either a float, or an implicitly float qvar-eq
 *
 * - boolean and time have specific fiddliness involved:
 *   - bool can either be t/f/y/n followed by any arbitrary text, or
 *     an implicitly integer qvar-eq
 *   - time can either be an integer followed by s or m, or an implicitly
 *     integer qvar-eq
 *
 * - the remaining design note parameter types are integer, float, and
 *   floatvec. The integer and float ones are implicit integer or float
 *   qvar-eqs with no special treatment, while the floatvec type is
 *   three comma-separated implicitly float qvar-eqs
 *
 * Now, really we don't want

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
