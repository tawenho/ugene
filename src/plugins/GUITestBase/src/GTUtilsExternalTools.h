/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2021 UniPro <ugene@unipro.ru>
 * http://ugene.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef _GTUTILS_EXTERNAL_TOOLS_H_
#define _GTUTILS_EXTERNAL_TOOLS_H_

#include <U2Core/U2OpStatus.h>
#include <core/GUITestOpStatus.h>

namespace U2 {

class GTUtilsExternalTools {
public:
    static void removeTool(HI::GUITestOpStatus &os, const QString &toolName);
    static void checkValidation(HI::GUITestOpStatus &os, const QString &toolName);
    static void setToolUrl(HI::GUITestOpStatus &os, const QString &toolName, const QString &url);
};

}    // namespace U2

#endif    // _GTUTILS_EXTERNAL_TOOLS_H_
