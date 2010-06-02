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

#ifndef __FH_ITCH_H__
#define __FH_ITCH_H__

/*! \brief "real" main function for ITCH feed handler
 *
 *  \param argc number of command line arguments passed along from main function
 *  \param argv array of command line arguments passed along from main function
 *  \param version ITCH feed handler version
 *  \return return value which will in turn be returned by main function (and become the
 *          the application's exit code)
 */
int fh_itch_main(int argc, char **argv, int version);

#endif  /* __FH_ITCH_H__ */
