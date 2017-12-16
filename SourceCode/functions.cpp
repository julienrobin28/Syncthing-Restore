//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

//    Written by Julien ROBIN (julien.robin@pixconfig.fr) - 2017-12-16

#include "functions.h"

bool getTimestamp (QDateTime &dateTime, QString filepath)
{
    int nBegin = filepath.lastIndexOf('~');
    if (nBegin == -1) return false; //not found

    //~YYYYMMDD-HHMMSS : 16 characters
    if (filepath.size() < nBegin + 16) return false; //timestamp size does not fit

    QChar dash_Check = filepath[nBegin+9];
    if (dash_Check != '-') return false; //the dash is not where it should be

    //here, we are sure that the range is the good one :
    filepath.remove(0, nBegin+1); //Erase everything before YYYYMMDD-HHMMSS
    QString strDateTime = filepath.left(15); //crop the exceeding characters

    if (strDateTime.size() != 15) return false; //something is wrong !

    QString strDate = strDateTime.left(8); //YYYYMMDD
    QString strTime = strDateTime.right(6); //HHMMSS

    QDate fileDate = QDate::fromString(strDate, "yyyyMMdd");
    QTime fileTime = QTime::fromString(strTime, "hhmmss");

    dateTime.setDate(fileDate);
    dateTime.setTime(fileTime);

    return true;
}

bool stripTimestamp (QString &filepath)
{
    int nBegin = filepath.lastIndexOf('~');
    if (nBegin == -1) return false; //not found

    //~YYYYMMDD-HHMMSS : 16 characters
    if (filepath.size() < nBegin + 16) return false; //timestamp size does not fit

    QChar dash_Check = filepath[nBegin+9];
    if (dash_Check != '-') return false; //the dash is not where it should be

    //here, we are sure that the range is the good one :
    filepath.remove(nBegin, 16);
    return true;
}

bool stripStversions (QString &filepath)
{
    QString toDelete = ".stversions/";
    int nPosition = filepath.indexOf(toDelete);

    if (nPosition == -1) return false; //not found

    filepath.remove(nPosition, toDelete.size());
    return true;
}

bool stripFilename (QString &filepath)
{
    //Thanks to Qt, on every OS, seperators are marked as '/'
    int lastSeparatorPos = filepath.lastIndexOf('/');

    if (lastSeparatorPos == -1) return false; //not found

    int removeSize = filepath.size() - lastSeparatorPos;
    filepath.remove(lastSeparatorPos, removeSize);
    return true;
}
