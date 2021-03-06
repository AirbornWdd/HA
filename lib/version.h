/* lib/version.h.  Generated from version.h.in by configure.
 *
 * Bane version
 * Copyright (C) 1997, 1999 Kunihiro Ishiguro
 * 
 * This file is part of GNU Kroute.
 *
 * GNU Kroute is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Kroute is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Kroute; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#ifndef _KROUTE_VERSION_H
#define _KROUTE_VERSION_H

#define BANE_PROGNAME   "Bane"

#define BANE_VERSION     "3.6.0.1"

#define KROUTE_BUG_ADDRESS "https://bugzilla.bane.net"

#define BANE_URL "http://www.bane.net"

#define BANE_COPYRIGHT "Copyright 2000-2020 BANE, et al."

pid_t pid_output (const char *);

#ifndef HAVE_DAEMON
int daemon(int, int);
#endif

#endif /* _KROUTE_VERSION_H */
