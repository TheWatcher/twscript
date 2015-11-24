/** @file
 * This file contains the QVar variable handling base code.
 *
 * @author Chris Page &lt;chris@starforge.co.uk&gt;
 *
 */
/* Okay, let's try to get this right this time. The problem is as follows:
 *
 * design_note = parameter *( SEPARATOR parameter )
 *   parameter = name EQUALS target / string / object / float-vec / qvar-eq
 *        name = script-name param-name
 * script-name = 1*( ALPHA DIGIT )
 *  param-name = 1*( ALPHA DIGIT )
 *      target = FIXME: ohgods
 *      string = [ QUOTE ] noquotestr [ QUOTE ]
 *      object = 1*DIGIT / qvar
 *   float-vec = qvar-eq COMMA qvar-eq COMMA qvar-eq
 *     qvar-eq = qvar-val [ operator qvar-val ]
 *    qvar-val = ( integer / float / qvar )
 *        qvar = "$" 1*identifier
 *  identifier = ALPHA *( ALPHA / DIGIT / "_" / "-" )
 *    operator = "+" / "*" / "/"
 *   SEPARATOR = ";"
 *      EQUALS = "="
 *       ALPHA = FIXME
 *       DIGIT = FIXME
 *       QUOTE = '"' / "'"
 *       COMMA = ","
 *
 * - design notes contain parameters. Each parameter consists of a name
 *   and a value.
 * - Each parameter may have a type:
 *   - `integer`
 *   - `boolean`
 *   - `float`
 *   - `float vector` (three floats)
 *   - `time`
 *   - `object`
 *   - `string`
 *   - `target`
 * - All but `string` and `target` are potentially complex types, where
 *   the design note does not include a simple value, and instead it

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
