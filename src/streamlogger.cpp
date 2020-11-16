/***************************************************************************
 *   Copyright (C) 2020 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "streamlogger.h"
#include <iostream>
#include <algorithm>

using namespace std;

void LogStreambuf::dump()
{
    cout << log << endl;
}
    
streamsize LogStreambuf::xsputn(const char *s, streamsize n)
{
    cout.write(s, n);
    auto loggable = min<streamsize>(n, size - log.size());
    log.append(s, loggable);
    return n; //Even if the log is full don't report error
}
    
int LogStreambuf::overflow(int c)
{
    if(c != EOF)
    {
        char cc = static_cast<char>(c);
        cout.write(&cc, 1);
        if(log.size() < size) log.append(&cc, 1);
    }
    return c;
}

#ifdef EXAMPLE_CODE
int main()
{
    LogStreambuf log(64*1024);
    ostream os(&log);
    os << "Hello world" << endl;
    os << "Lol\n";
    os.flush();
    
    cout << "----" << endl;
    log.dump();
}
#endif
