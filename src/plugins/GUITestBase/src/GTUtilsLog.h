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

#ifndef _U2_GUI_LOG_UTILS_H_
#define _U2_GUI_LOG_UTILS_H_

#include <QStringList>

#include <U2Core/Log.h>

#include "GTGlobals.h"

namespace U2 {

class GTLogTracer : public QObject, public LogListener {
    Q_OBJECT
public:
    GTLogTracer(const QString &expectedMessage = QString());
    ~GTLogTracer();

    void onMessage(const LogMessage &msg);

    bool hasErrors() const {
        return !errorsList.isEmpty();
    }

    QString getJoinedErrorString() const {
        return errorsList.isEmpty() ? "" : errorsList.join("\n");
    }

    static QList<LogMessage *> getMessages();

    static bool checkMessage(QString s);

    bool isExpectedMessageFound;
    QStringList errorsList;
    QString expectedMessage;
};

class GTUtilsLog {
public:
    static void check(HI::GUITestOpStatus &os, const GTLogTracer &logTracer);
    static void checkContainsError(HI::GUITestOpStatus &os, const GTLogTracer &logTracer, const QString &messagePart);
    static void checkContainsMessage(HI::GUITestOpStatus &os, const GTLogTracer &logTracer, bool expected = true);
    static QStringList getErrors(HI::GUITestOpStatus &os, const GTLogTracer &logTracer);
};

}    // namespace U2

#endif
