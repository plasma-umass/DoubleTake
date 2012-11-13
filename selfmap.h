// -*- C++ -*-

/*
  Copyright (c) 2012, University of Massachusetts Amherst.

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
 * @file   selfmap.h
 * @brief  Analyze the self mapping. 
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _SELFMAP_H_
#define _SELFMAP_H_

#include <sys/resource.h>
#include <iostream>
#include <fstream>

#include "xdefines.h"
#include "regioninfo.h"

extern "C" {
  extern char __data_start;
};

using namespace std;
class selfmap {

public:
  // We may use a inode attribute to analyze whether we need to do this.
  static void getRegionInfo(std::string & mapentry, void ** start, void ** end) {

    // Now this entry is about globals of libc.so.
    string::size_type pos = 0;
    string::size_type endpos = 0;
    // Get the starting address and end address of this entry.
    // Normally the entry will be like this
    // "00797000-00798000 rw-p ...."
    string beginstr, endstr;

    while(mapentry[pos] != '-') pos++;

    beginstr = mapentry.substr(0, pos);
  
    // Skip the '-' character
    pos++;
    endpos = pos;
    
    // Now pos should point to a space or '\t'.
    while(!isspace(mapentry[pos])) pos++;
    endstr = mapentry.substr(endpos, pos-endpos);

    // Save this entry to the passed regions.
    *start = (void *)strtoul(beginstr.c_str(), NULL, 16);
    *end = (void *)strtoul(endstr.c_str(), NULL, 16);

    return;
  }
 
  // Trying to get information about global regions. 
  static void getGlobalRegions(regioninfo * regions, int * regionNumb) {
    using namespace std;
    ifstream iMapfile;
    string mapentry;

    //#define PAGE_ALIGN_DOWN(x) (((size_t) (x)) & ~xdefines::PAGE_SIZE_MASK)
    //static void * globalstart = (void *)PAGE_ALIGN_DOWN(&__data_start);

    try {
      iMapfile.open("/proc/self/maps");
    } catch(...) {
      fprintf(stderr, "can't open /proc/self/maps, exit now\n");
      abort();
    } 

    // Now we analyze each line of this maps file.
    static bool toCheckNextEntry = false;
    
    void * startaddr, * endaddr;
    while(getline(iMapfile, mapentry)) {
      // Check the globals for libc.so
#if 0      
      if(mapentry.find("libc-", 0) != string::npos) {
        toCheckNextEntry = true;
      }
      else if((toCheckNextEntry == true) && (mapentry.find(" 00:00 ", 0) != string::npos) && (mapentry.find(" rw-p ") != string::npos)) {
#endif
      if((mapentry.find("libc-", 0) != string::npos) && (mapentry.find(" 08:06 ", 0) != string::npos) && (mapentry.find(" rw-p ") != string::npos)) {

        // Save this entry to the passed regions.
        getRegionInfo(mapentry, &startaddr, &endaddr);
      
        regions[*regionNumb].start = startaddr;
        regions[*regionNumb].end = endaddr;
        (*regionNumb)++;
        //fprintf(stderr, "selfmap entry start %p end %p\n", startaddr, endaddr);
      }
      else if((mapentry.find(" 08:0b ", 0) != string::npos) && (mapentry.find(" rw-p ") != string::npos)) {
        getRegionInfo(mapentry, &startaddr, &endaddr);
     
        // Check whether it is the applications' global region.
        //if(globalstart >= startaddr && globalstart <= endaddr) { 
          // When it is done, now we can exit.
          regions[*regionNumb].start = startaddr;
          regions[*regionNumb].end = endaddr;
        //  fprintf(stderr, "selfmap entry start %p end %p\n", startaddr, endaddr);
          (*regionNumb)++;
          break;
        //}
      }
      else {
        toCheckNextEntry = false;
      }
    }
    iMapfile.close();
  } 
  
  // Trying to get stack information. 
  static void getStackInformation(unsigned long* stackBottom, unsigned long * stackTop) {
    using namespace std;
    ifstream iMapfile;
    string mapentry;

    try {
      iMapfile.open("/proc/self/maps");
    } catch(...) {
      fprintf(stderr, "can't open /proc/self/maps, exit now\n");
      abort();
    } 

    // Now we analyze each line of this maps file.
    while(getline(iMapfile, mapentry)) {

      // Find the entry for stack information.
      if(mapentry.find("[stack]", 0) != string::npos) {
        string::size_type pos = 0;
        string::size_type endpos = 0;
        // Get the starting address and end address of this entry.
        // Normally the entry will be like this
        // ffce9000-ffcfe000 rw-p 7ffffffe9000 00:00 0   [stack]
        string beginstr, endstr;

        while(mapentry[pos] != '-') pos++;
          beginstr = mapentry.substr(0, pos);
  
          // Skip the '-' character
          pos++;
          endpos = pos;
        
          // Now pos should point to a space or '\t'.
          while(!isspace(mapentry[pos])) pos++;
          endstr = mapentry.substr(endpos, pos-endpos);

          // Now we get the start address of stack and end address
          *stackTop = strtoul(beginstr.c_str(), NULL, 16);
          *stackBottom = strtoul(endstr.c_str(), NULL, 16);
      }
    }
    iMapfile.close();
  } 
};

#endif
