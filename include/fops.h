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
 * @file   fops.h
 * @brief  Managing the system calls about files.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef _SYS_FOPS_H_
#define _SYS_FOPS_H_

#include <map>
#include <errno.h>
#include <stdlib.h>
#include <list>

#include "xdefines.h"
#include "record.h"
#include "internalheap.h"

using namespace std;
class fops {

  class fileInfo {
  public:
    int fd;
    off_t pos; // Initial position of a file.
    FILE * backupStream; // Saved file stream
    FILE * origStream; // Where is the actual file stream
#ifdef REPRODUCIBLE_FDS
    bool  isClosed; // Save some time to update 
#else
    bool  isNew;
#endif
  };
  
  class dirInfo {
  public:
    off_t pos; // Initial position of a file.
    DIR * backupDir; // Saved DIR stream
    DIR * dir; // Where is the actual dir stream
#ifdef REPRODUCIBLE_FDS
    bool  isClosed; // Marked when closed, so we can save some time to update
#endif
  };


public:
  
  fops()
  {
  }

  // How to initalize 
  void initialize() {
    //PRINF("HASH MAP INITIALIZATION\n");
    // Initialialize the hashmap.
    _filesMap.initialize(HashFuncs::hashInt, HashFuncs::compareInt, xdefines::FILES_MAP_SIZE);
    _dirsMap.initialize(HashFuncs::hashAddr, HashFuncs::compareAddr, xdefines::DIRS_MAP_SIZE);
  }

  // This is called after we are sure that there is no need to rollback.
  void updateOpenedFiles() {
    // Get file position for all files in the global hash table.
    filesHashMap::iterator i;
    fileInfo * thisFile;

    for(i = _filesMap.begin(); i != _filesMap.end(); i++) {
      thisFile = (fileInfo *)i.getData();
      PRINF("thisfile fd %d pos %ld", thisFile->fd, thisFile->pos); 
#ifdef REPRODUCIBLE_FDS
      if(!thisFile->isClosed) {
        if(thisFile->origStream) {
          assert(thisFile->backupStream != NULL);
        }
#endif
        off_t pos = Real::lseek()(thisFile->fd, 0, SEEK_CUR);
        thisFile->pos = pos;

#ifndef REPRODUCIBLE_FDS
        // Those closed files should not appear here.
        // Check whether current is a file stream or not.
        if(thisFile->isNew) {
          thisFile->isNew = false;
          if(thisFile->origStream) {
            assert(thisFile->backupStream == NULL);
            thisFile->backupStream = (FILE *)InternalHeap::getInstance().malloc(xdefines::FOPEN_ALLOC_SIZE);
          }
        }
#endif
        if(thisFile->origStream) {   
          assert(thisFile->backupStream != NULL);
          // Backup the file stream.
          memcpy(thisFile->backupStream, thisFile->origStream, xdefines::FOPEN_ALLOC_SIZE);
        }
#ifdef REPRODUCIBLE_FDS
      }
#endif
    }

    // Update those opened dirs.
    dirsHashMap::iterator j;
    dirInfo * thisDir;

    for(j = _dirsMap.begin(); j != _dirsMap.end(); j++) {
      thisDir = (dirInfo *)j.getData();
#ifdef REPRODUCIBLE_FDS
      if(!thisDir->isClosed) {
        assert(thisDir->backupDir != NULL);
#endif     
        off_t pos = telldir(thisDir->dir);
        thisDir->pos = pos;

#ifndef REPRODUCIBLE_FDS
        if(!thisDir->backupDir) {
          thisDir->backupDir = (DIR *)InternalHeap::getInstance().malloc(xdefines::DIROPEN_ALLOC_SIZE);
          assert(thisDir->backupDir != NULL);
        }
#endif
 
        // Backup the file stream.
        memcpy(thisDir->backupDir, thisDir->dir, xdefines::DIROPEN_ALLOC_SIZE);
#ifdef REPRODUCIBLE_FDS
      }
#endif
    }
  }

  bool isNormalFile(int fd) {
    // Check whether the oldfd is a normal file
    fileInfo * thisFile;
    if(_filesMap.find(fd, sizeof(fd), &thisFile)) {
      return true;
    }
    else {
      return false;
    }
  }

  void saveDupFd(int oldfd, int newfd) {
    // Save the fd to list and hashmap.
    getRecord()->recordFileOps(Record::E_OP_FILE_OPEN, newfd);    

    if(newfd != -1) {
      fileInfo * thisFile; 

      // check the file fd.
      if(_filesMap.find(oldfd, sizeof(oldfd), &thisFile)) {
        fileInfo * newFile = (fileInfo *)InternalHeap::getInstance().malloc(sizeof(fileInfo));
        memcpy(newFile, thisFile, sizeof(fileInfo));
        newFile->fd = newfd;

        // Check whether this is a file stream.
        if(newFile->backupStream != NULL) {
          void * ptr = InternalHeap::getInstance().malloc(xdefines::FOPEN_ALLOC_SIZE);
          // Set to new file stream. We don't want to have some relationship
          // with old file stream since the old one can be closed in any time. 
          memcpy(ptr, thisFile->backupStream, xdefines::FOPEN_ALLOC_SIZE);
          newFile->backupStream = (FILE *)ptr;
        }

        // Insert this file
        _filesMap.insert(newfd, sizeof(newfd), newFile);
      }
      else {
        // This should not happen, since we checked the fd in the beginning.
        assert(0);
      }
    }
  }
 
  void saveDir(DIR * dir) {
#ifdef REPRODUCIBLE_FDS
    // Trying to get fd about this dir.
    getRecord()->recordDirOps(Record::E_OP_DIR_OPEN, dir);   
#endif

//    PRINF("saveDir %p\n", dir);
    // Only save to the dirmap when a dir is valid. 
    if(dir != NULL) {
      dirInfo * thisDir = (dirInfo *)InternalHeap::getInstance().malloc(sizeof(dirInfo));
      assert(thisDir != NULL);
      thisDir->pos = telldir(dir); 
      thisDir->dir = dir;
    
      //PRINF("thisDir in the insertion %p dir %p\n", thisDir, thisDir->dir); 
#ifdef REPRODUCIBLE_FDS 
      thisDir->isClosed = false; 
      // Allocate a block of memory
      void * ptr = InternalHeap::getInstance().malloc(xdefines::DIROPEN_ALLOC_SIZE);
      memcpy(ptr, dir, xdefines::DIROPEN_ALLOC_SIZE);
      thisDir->backupDir = (DIR *)ptr;
#else
      thisDir->backupDir = NULL;
#endif
      // Insert this file into the hash map.
      _dirsMap.insert(dir, sizeof(dir), thisDir);
    }
  }

  DIR * getDirAtOpen() {
    dirInfo * dinfo;
    DIR * dir = NULL;

    if(!getRecord()->getDirOps(Record::E_OP_DIR_OPEN, &dir)) {
      assert(0);
    }

    if(dir != NULL) {
      if(!_dirsMap.find(dir, sizeof(dir), &dinfo)) {
        assert(0);
      }

      // Simulate the fopen, since fopen will always call malloc().
      // It is possible that different library will have different size...
      void * ptr = malloc(xdefines::DIROPEN_ALLOC_SIZE);
      //PRINF("getFopen fd %d and file %p current ptr %p\n", fd, finfo.origStream, ptr);

      // check whether the pointer is the same as before.
      assert(dir == dinfo->dir);
      assert(dir == ptr);

      memcpy(ptr, dinfo->backupDir, xdefines::DIROPEN_ALLOC_SIZE);
      dir = (DIR *) ptr;
      // Since we already recover all initial file stream initially
      // there is nothing to do now.
    }

    return dir;
  }

  int closeDir(DIR * dir) {
    int ret = 0;
    dirInfo * thisDir;
 
#ifdef REPRODUCIBLE_FDS
    if(dir == NULL) {
      return 0;
    }

    // The file should be in the map.
    if(!_dirsMap.find(dir, sizeof(dir), &thisDir)) {
      errno = EBADF;
      ret = -1;
      return ret;
    }   

    // Check whether the fp is equal to the saved one
    //assert(fp == thisFile->origStream);
    if(dir != thisDir->dir) {
      errno = EBADF;
      ret = -1;
      return ret;
    }
   
    thisDir->isClosed = true;
 
    // Add to the close list
    getRecord()->recordDirOps(Record::E_OP_DIR_CLOSE, dir);
    
    return ret;
#else
    bool hasFound = false;

    //PRINF("Finding the entry\n");
    hasFound = _dirsMap.find(dir, sizeof(dir), &thisDir);

    if(hasFound) {
      //PRINF("Found the entry thisDir %p\n", thisDir);
      // Remove this entry from the filemap. FIXME
      _dirsMap.erase(dir, sizeof(dir));
      InternalHeap::getInstance().free(thisDir);
    }
    
    // Remove this 
    return Real::closedir()(dir);
#endif

  }
 
  // Called in the execution phase when fopen or open
  void saveFd(int fd, FILE *file) {
  //  PRINF("saveFd %d\n", fd);
#ifdef REPRODUCIBLE_FDS
    // Save it even when fopen/open is not successful. 
    getRecord()->recordFileOps(Record::E_OP_FILE_OPEN, fd);   
#endif 
    if(fd != -1) {
      fileInfo * thisFile = (fileInfo *)InternalHeap::getInstance().malloc(sizeof(fileInfo));
      thisFile->fd = fd;
      thisFile->pos = 0; 
      thisFile->origStream = file; 
      
#ifdef REPRODUCIBLE_FDS
      thisFile->isClosed = false; 
      if(file) {
        // Allocate a block of memory
        void * ptr = InternalHeap::getInstance().malloc(xdefines::FOPEN_ALLOC_SIZE);
        memcpy(ptr, file, xdefines::FOPEN_ALLOC_SIZE);
        thisFile->backupStream = (FILE *)ptr;
      }
      else {
        thisFile->backupStream = NULL;
      }
#else
      thisFile->isNew = true;
      thisFile->backupStream = NULL;
#endif

      // Insert this file into the hash map.
      _filesMap.insert(fd, sizeof(fd), thisFile);
    }
  }

  void saveFopen(FILE * file) {
    if(file) {
      //PRINF("saveFopen fd %d and file %p\n", file->_fileno, file);
      saveFd(file->_fileno, (FILE *)file);
    }
    else {
      saveFd(-1, NULL);
    } 
  }
 

  // Now we have to rollback current transaction
  void prepareRollback() {
    // We must recove all file offsets of all files now.
    filesHashMap::iterator i;

  //  PRINF("***************fops rollback now!**************\n");
    /*  Traverse each entry of the hash table. */
    for(i = _filesMap.begin(); i != _filesMap.end(); i++) {
      fileInfo * thisFile;
      thisFile = (fileInfo *)i.getData();
#ifndef REPRODUCIBLE_FDS
      if(thisFile->isNew) {
        closeFile(thisFile->fd, thisFile->origStream);
      }
#else
      Real::lseek()(thisFile->fd, thisFile->pos, SEEK_SET);

      // The file has already existed before current epoch
      if(thisFile->origStream) {
        memcpy(thisFile->origStream, thisFile->backupStream, xdefines::FOPEN_ALLOC_SIZE);
      }
#endif
    }
    
    dirsHashMap::iterator j;
    dirInfo * thisDir;

    for(j = _dirsMap.begin(); j != _dirsMap.end(); j++) {
      thisDir = (dirInfo *)j.getData();
#ifdef REPRODUCIBLE_FDS 
      seekdir(thisDir->dir, thisDir->pos);
      //PRINF("thisdir is %p dir %p\n", thisDir, thisDir->dir); 
      if(thisDir->backupDir) {
        memcpy(thisDir->dir, thisDir->backupDir, xdefines::DIROPEN_ALLOC_SIZE);
      }
#else
      if(thisDir->backupDir) {
        seekdir(thisDir->dir, thisDir->pos);
        memcpy(thisDir->dir, thisDir->backupDir, xdefines::DIROPEN_ALLOC_SIZE);
      }
      else {
        // Close those newly opened directories.
        closeDir(thisDir->dir);
      }
#endif
      // For those newly created file, maybe we should remove them from the table.
    }
  }
 
  // We are trying to open a file in the rollback phase
  // There is no need to allocate a new block of memory since
  // we already have one.
  FILE * getFopen() {
    int fd = getFdAtOpen();
    FILE * file;

    // Check whether it is a file stream.
    if(fd == -1) {
      // Now this file is already closed before the end of transaction.
      // Or the fopen is not successful.
      file = NULL;
    }
    else {
      fileInfo * finfo; 
      if(!_filesMap.find(fd, sizeof(fd), &finfo)) {
        assert(0);
      }
      // Simulate the fopen, since fopen will always call malloc().
      // It is possible that different library will have different size...
      void * ptr = malloc(xdefines::FOPEN_ALLOC_SIZE);
      //PRINF("getFopen fd %d and file %p current ptr %p\n", fd, finfo.origStream, ptr);

      // check whether the pointer is the same as before.
      assert(ptr == finfo->origStream);

      memcpy(ptr, finfo->backupStream, xdefines::FOPEN_ALLOC_SIZE);
      file = (FILE *) ptr;

     // finfo->origStream = (FILE *)ptr; 

      assert(file != NULL);
      // Since we already recover all initial file stream initially
      // there is nothing to do now.
    } 

    return file;
  }

  // Called in the rollback phase
  // Try to get fd for open operation.
  // The function now can only support single threaded function
  int getFdAtOpen() {
    int fd = -1;

    if(!getRecord()->getFileOps(Record::E_OP_FILE_OPEN, &fd)) {
      assert(0);
    }
    return fd;
  }

  // Call when close or fclose.
  /* 
    In the implementation, we have to delay all close operations until the end of transaction.
    When there is no overflow, we can start to close all files that are supposed to close in 
    current transaction.
    Otherwise, we can repeat all open() without calling actual system calls.

    We have to do this in order to guarantee the repeatness. 
    If we close a file immediately, then we can not repeat the same fd for each file as follows:

    open(a); // fd = 4
    open(b); // fd = 5;
    close(a); // 4 is avaiable.
    open(c); // fd = 4 again;
    ---------- // transaction end

    We can not repeat this execution. 
    Since open(a) will return fd 6 now, not 4 any more.
  */
  int closeFile(int fd, FILE * fp) {
    int ret = 0;
    // Try to get corresponding entry in the hashmap.
    fileInfo * thisFile;

    //PRINF("close file %d\n", fd);
#ifdef REPRODUCIBLE_FDS
      
    // The file should be in
    if(!_filesMap.find(fd, sizeof(fd), &thisFile)) {
      errno = EBADF;
      ret = -1;
      return ret;
    }   

    // Check whether the fp is equal to the saved one
    //assert(fp == thisFile->origStream);
    if(fp != thisFile->origStream) {
      errno = EBADF;
      ret = -1;
      return ret;
    }
   
    thisFile->isClosed = true;
 
    // Add to the close list
    getRecord()->recordFileOps(Record::E_OP_FILE_CLOSE, fd);

    return ret;
#else
    bool hasFound = false;

    hasFound = _filesMap.find(fd, sizeof(fd), &thisFile);

    if(hasFound) {
      // Remove this entry from the filemap. FIXME
      _filesMap.erase(fd, sizeof(fd));

      if(thisFile->backupStream) {
        InternalHeap::getInstance().free(thisFile->backupStream);
      }
      InternalHeap::getInstance().free(thisFile);
   
      PRINT("fd is %d fp %p\n", fd, fp); 
      // Remove this 
      if(fp == NULL) {
        return Real::close()(fd);
      }
      else {
        return Real::fclose()(fp);
      }
    }
#endif
  }


  // Handling all delayed close after the commit.
  // There is no need to rollback any more.
  void cleanClosedFiles() {
    int fd = -1;
    struct fileInfo * thisFile = NULL;

    while((getRecord()->retrieveFCloseList(&fd))) {

      if(fd == -1) {
        continue;
      }

      // Find the entry from the hash map.
      if(_filesMap.find(fd, sizeof(fd), &thisFile)) {
        // Check whether this file has dup?
        // According to description of dup, http://www.linuxquestions.org/questions/programming-9/close-fd-after-dup-fd-558966/
        // close a newfd won't affect the new fd. 
        // check whether this file is a file stream or not.
        if(thisFile->origStream) {
          Real::fclose()(thisFile->origStream);
        
          assert(thisFile->backupStream != NULL); 
          // Release temporary file stream now.
          InternalHeap::getInstance().free(thisFile->backupStream);
        }
        else {
          Real::close()(fd);
        }
      
        // Remove this entry from the filemap. FIXME
        _filesMap.erase(fd, sizeof(fd));
        InternalHeap::getInstance().free(thisFile);
      }
      else {
        // Should not happen.
        assert(0);
      }
    } 

    // When there is no need to rollback
    // we can remove the whole list.
    struct dirInfo * thisDir = NULL;
    DIR * dir;

    while((getRecord()->retrieveDIRCloseList(&dir))) {
      if(dir == NULL) {
        continue;
      }

      // Find the entry from the hash map.
      if(_dirsMap.find(dir, sizeof(dir), &thisDir)) {
        assert(dir == thisDir->dir);

        // check whether this file is a file stream or not.
        Real::closedir()(dir);
         
        // Release temporary file stream now.
        InternalHeap::getInstance().free(thisDir->backupDir);
      
        // Remove this entry from the filemap. FIXME
        _dirsMap.erase(dir, sizeof(dir));
        InternalHeap::getInstance().free(thisDir);
      }
      else {
        // Should not happen.
        assert(0);
      }
    } 
  }

  // Check whether this file is a normal file, not a socket file.
  bool checkPermission(int fd) {  
    bool isAllowed = false;
    
    if(fd <= 0) {
      isAllowed = false;
    }
    else if(fd == 1 || fd == 2 || fd == 3) {
      isAllowed = true;
    } 
    else {
      struct fileInfo * thisFile = NULL;

      // Find the entry from the hash map.
      if(_filesMap.find(fd, sizeof(fd), &thisFile)) {
        isAllowed = true;
      }
    }
  
    return isAllowed;
  }

  
private:

  Record * getRecord() {
    return (Record *)current->record;
  }

/*
  typedef std::pair<int, fileInfo> objectPair;
 
  // We are maintainning a private hash map for each thread.
  typedef hash_map<int, fileInfo, fd_hash, fd_compare, HL::STLAllocator<objectPair, InternalHeap> > filesHashMap;
  //typedef hash_map<int, fileInfo, fd_hash, fd_compare, HL::STLAllocator<objectPair, InternalHeap> > filesHashMap;
*/
  typedef HashMap<int, fileInfo *, spinlock, InternalHeapAllocator> filesHashMap;
  // A list is to record those newly opened files, which will be added into
  // the global hash map in next transaction.
  filesHashMap _filesMap;
  
  typedef HashMap<void *, dirInfo *, spinlock, InternalHeapAllocator> dirsHashMap;
  // A list is to record those newly opened files, which will be added into
  // the global hash map in next transaction.
  dirsHashMap _dirsMap;
};

#endif


