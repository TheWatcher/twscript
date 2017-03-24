
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
#include <string>
#include "QVarWrapper.h"

/* ------------------------------------------------------------------------
 *  QVar convenience functions
 */

float get_qvar(const std::string& qvar, float def_val)
{
    SService<IQuestSrv> quest_srv(g_pScriptManager);
    if(quest_srv -> Exists(qvar.c_str()))
        return static_cast<float>(quest_srv -> Get(qvar.c_str()));

    return def_val;
}


int get_qvar(const std::string& qvar, int def_val)
{
    SService<IQuestSrv> quest_srv(g_pScriptManager);
    if(quest_srv -> Exists(qvar.c_str()))
        return quest_srv -> Get(qvar.c_str());

    return def_val;
}
