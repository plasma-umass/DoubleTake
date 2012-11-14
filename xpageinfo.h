// -*- C++ -*-

/*
  Copyright (C) 2011 University of Massachusetts Amherst.

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

/*
 * @file   xpageinfo.h
 * @brief  Information about each page.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */ 

#ifndef __XPAGEINFO_H__
#define __XPAGEINFO_H__

#define MAX_DIFF_ENTRY 4 

struct pageinfo {
	int pageNo;	

	unsigned long pageVersion;
   
	// Used to save start address for this page. 
	void * pageStart;
	
	// Following two fields are different in functionality.
	void * twinPage;

  void * canaryBitmap; 
	bool shared;
	bool alloced;
};

#endif /* __XPAGEINFO_H__ */
