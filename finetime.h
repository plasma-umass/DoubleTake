/*
  Copyright (c) 2012-2013, University of Massachusetts Amherst.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#ifndef __TIME_H__
#define __TIME_H__
/*
 * @file   finetime.h
 * @brief  Fine timing management based on rdtsc.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *  Note:  Code is borrowed from the test cases from GRACE project, original author is Ting Yang.
 */


extern "C" {
struct timeinfo {
	unsigned long low;
	unsigned long high;
};

void start(struct timeinfo *ti);
double stop(struct timeinfo * begin, struct timeinfo * end);
unsigned long elapse2ms(double elapsed);
};
#endif /* __TIME_H__ */
