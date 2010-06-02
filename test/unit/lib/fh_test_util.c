/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 * 
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

// System headers
#include <string.h>


/*! \brief Strip any path information that may exist from a filename
 *
 *  \param filename name of a file (that may include path information)
 *  \return pointer to the path-less portion of the filename
 */
const char *fh_test_util_strip_path(const char *filename)
{
    const char *pathless;
    
    // find the last occurance of the '/' character
    pathless = strrchr(filename, '/');
    
    // if a '/' was not found in the binary argument, whole binary argument is the process name
    if (pathless == NULL) {
        pathless = filename;
    }
    // if a '/' was found, move 1 character past the '/' to make sure it is not included
    else {
        pathless += sizeof(char);
    }
    
    // return the pathless version
    return pathless;
}
