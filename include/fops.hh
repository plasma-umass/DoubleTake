#if !defined(DOUBLETAKE_FOPS_H)
#define DOUBLETAKE_FOPS_H

/*
 * @file   fops.h
 * @brief  Managing the system calls about files.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hashfuncs.hh"
#include "hashmap.hh"
#include "internalheap.hh"
#include "log.hh"
#include "mm.hh"
#include "real.hh"
#include "sysrecord.hh"
#include "spinlock.hh"
#include "threadstruct.hh"
#include "xdefines.hh"

using namespace std;
class fops {

  class fileInfo {
  public:
    int 	fd;
    off_t pos;          // Initial position of a file.
    FILE* backupStream; // Saved file stream
    FILE* origStream;   // Where is the actual file stream
    bool  isNew;				// Whether the file is opened in this epoch
  };

  class dirInfo {
  public:
    off_t 	pos;       // Initial position of a file.
    DIR* 		backupDir; // Saved DIR stream
    DIR* 		dir;       // Where is the actual dir stream
    bool  	isNew;		 // Whether the directory is opened in this epoch
  };

public:
  fops() {}

  // How to initalize
  void initialize() {
    // PRINF("HASH MAP INITIALIZATION\n");
    // Initialialize the hashmap.
    _filesMap.initialize(HashFuncs::hashInt, HashFuncs::compareInt, xdefines::FILES_MAP_SIZE);
    _dirsMap.initialize(HashFuncs::hashAddr, HashFuncs::compareAddr, xdefines::DIRS_MAP_SIZE);
  }

  // This is called after we are sure that there is no need to rollback.
	// Also, we call this after all closed files has been cleaned up.
  void updateOpenedFiles() {
    // Get file position for all files in the global hash table.
    filesHashMap::iterator i;
    fileInfo* thisFile;
    for(i = _filesMap.begin(); i != _filesMap.end(); i++) {
      thisFile = (fileInfo*)i.getData();

			if(thisFile->isNew) {
        thisFile->isNew = false;
			}
			
			// Fetch the current file position	
			off_t pos = Real::lseek(thisFile->fd, 0, SEEK_CUR);
      thisFile->pos = pos;

			// Backup the stream
      if(thisFile->origStream) {
        assert(thisFile->backupStream != NULL);
        	
				// Backup the file stream.
        memcpy(thisFile->backupStream, thisFile->origStream, xdefines::FOPEN_ALLOC_SIZE);
      }
    }

    // Update those opened dirs.
    dirsHashMap::iterator j;
    dirInfo* thisDir;

    for(j = _dirsMap.begin(); j != _dirsMap.end(); j++) {
      thisDir = (dirInfo*)j.getData();
			
			if(thisDir->isNew) {
        thisDir->isNew = false;
			}
        
			off_t pos = telldir(thisDir->dir);
      thisDir->pos = pos;

      // Backup the file stream.
      memcpy(thisDir->backupDir, thisDir->dir, xdefines::DIROPEN_ALLOC_SIZE);
    }
  }

  bool isNormalFile(int fd) {
    // Check whether the oldfd is a normal file
    fileInfo* thisFile;
    if(_filesMap.find(fd, sizeof(fd), &thisFile)) {
      return true;
    } else {
      return false;
    }
  }

  void saveDupFd(int oldfd, int newfd) {
    // Save the fd to list and hashmap.
    _sysrecord.recordFileOps(E_SYS_FILE_DUP, newfd);

    if(newfd != -1) {
      fileInfo* thisFile;

      // check the file fd.
      if(_filesMap.find(oldfd, sizeof(oldfd), &thisFile)) {
        fileInfo* newFile = (fileInfo*)InternalHeap::getInstance().malloc(sizeof(fileInfo));
        memcpy(newFile, thisFile, sizeof(fileInfo));
        newFile->fd = newfd;

        // Create a new copy of stream in case the old one has been closed.
        if(newFile->backupStream != NULL) {
          void* ptr = InternalHeap::getInstance().malloc(xdefines::FOPEN_ALLOC_SIZE);
          // Set to new file stream. We don't want to have some relationship
          // with old file stream since the old one can be closed in any time.
          memcpy(ptr, thisFile->backupStream, xdefines::FOPEN_ALLOC_SIZE);
          newFile->backupStream = (FILE*)ptr;
        }

        // Insert this file
        _filesMap.insert(newfd, sizeof(newfd), newFile);
      } else {
        // This should not happen, since we checked the fd in the beginning.
        assert(0);
      }
    }
  }

	int getDupFd(void) {
    int fd = -1;

    if(!_sysrecord.getFileOps(E_SYS_FILE_DUP, &fd)) {
      assert(0);
    }

		// No need to do anything for dup or dup2
		return fd;
	}

  void saveDir(DIR* dir) {
    // Trying to get fd about this dir.
    _sysrecord.recordDirOps(E_SYS_DIR_OPEN, dir);

    // Only save to the dirmap when a dir is valid.
    if(dir != NULL) {
      dirInfo* thisDir = (dirInfo*)InternalHeap::getInstance().malloc(sizeof(dirInfo));
      assert(thisDir != NULL);
      thisDir->pos = telldir(dir);
      thisDir->dir = dir;
			thisDir->isNew = true;

      // Allocate a block of memory
      void* ptr = InternalHeap::getInstance().malloc(xdefines::DIROPEN_ALLOC_SIZE);
      memcpy(ptr, dir, xdefines::DIROPEN_ALLOC_SIZE);
      thisDir->backupDir = (DIR*)ptr;

      // Insert this file into the hash map.
      _dirsMap.insert(dir, sizeof(dir), thisDir);
    }
  }

	// Only called in the rollback phase.
	// We want to get the dir.
  DIR* getDirAtOpen() {
    dirInfo* dinfo;
    DIR* dir = NULL;

    if(!_sysrecord.getDirOps(E_SYS_DIR_OPEN, &dir)) {
      assert(0);
    }

    if(dir != NULL) {
			// We should be able to find this dir in the map
      if(!_dirsMap.find(dir, sizeof(dir), &dinfo)) {
        assert(0);
      }

      // Simulate the fopen, since opendir will always call malloc().
      // It is possible that different library will have different size...
      void* ptr = malloc(xdefines::DIROPEN_ALLOC_SIZE);

      // check whether the pointer is the same as before.
      assert(dir == dinfo->dir);
      assert(dir == ptr);

      memcpy(ptr, dinfo->backupDir, xdefines::DIROPEN_ALLOC_SIZE);

			// Reset to the orignal place
			Real::rewinddir(dir);
    }

    return dir;
  }

  int closeDir(DIR* dir) {
    dirInfo* thisDir = NULL;
    int ret = 0;

    // The file should be in the map.
    if((dir == NULL) || !_dirsMap.find(dir, sizeof(dir), &thisDir)) {
      errno = EBADF;
      ret = -1;
    }

    // Check whether the fp is equal to the saved one
    if(dir != thisDir->dir) {
      errno = EBADF;
      ret = -1;
    }

    // Add to the close list
    _sysrecord.recordDirOps(E_SYS_DIR_CLOSE, dir);

    return ret;
  }

	int getCloseDir(DIR * dir) {
    dirInfo* thisDir = NULL;
    int ret = 0;

    // The file should be in the map.
		if(dir == NULL) {
      errno = EBADF;
      ret = -1;
		}

    if(!_dirsMap.find(dir, sizeof(dir), &thisDir)) {
      errno = EBADF;
      ret = -1;
    }

    // Check whether the fp is equal to the saved one
    if(dir != thisDir->dir) {
      errno = EBADF;
      ret = -1;
    }

    // Add to the close list
    _sysrecord.getDirOps(E_SYS_DIR_CLOSE, &dir);

		return ret;
	}

  // Called in the execution phase when fopen or open
  void saveFd(int fd, FILE* file) {
    // Save it even when fopen/open is not successful.
    _sysrecord.recordFileOps(E_SYS_FILE_OPEN, fd);

		//PRINT("saveFD %d origStream %p\n", fd, file);

    if(fd != -1) {
      fileInfo* thisFile = (fileInfo*)InternalHeap::getInstance().malloc(sizeof(fileInfo));
      thisFile->fd = fd;
      thisFile->pos = 0;
      thisFile->origStream = file;

      if(file) {
        // Allocate a block of memory
        void* ptr = InternalHeap::getInstance().malloc(xdefines::FOPEN_ALLOC_SIZE);
        memcpy(ptr, file, xdefines::FOPEN_ALLOC_SIZE);
        thisFile->backupStream = (FILE*)ptr;
      } else {
        thisFile->backupStream = NULL;
      }

      // Insert this file into the hash map.
      _filesMap.insert(fd, sizeof(int), thisFile);
    }
  }

  void saveFopen(FILE* file) {
    if(file) {
      saveFd(file->_fileno, (FILE*)file);
    } else {
      saveFd(-1, NULL);
    }
  }

  // Now we have to rollback current transaction
	// by traversing every entry in _filesMap and _dirsMap.
  void prepareRollback() {

    // We must recove all file offsets of all files now.
    filesHashMap::iterator i;
    for(i = _filesMap.begin(); i != _filesMap.end(); i++) {
      fileInfo* thisFile;
      thisFile = (fileInfo*)i.getData();

			// We only care about those opened files in the previous epoches.
			// Newly opened files will be handled when they are actually opened.
			if(!thisFile->isNew) {
      	// The file has already existed before current epoch
      	if(thisFile->origStream) {
        	memcpy(thisFile->origStream, thisFile->backupStream, xdefines::FOPEN_ALLOC_SIZE);
      	}
      	Real::lseek(thisFile->fd, thisFile->pos, SEEK_SET);
			}
    }

    dirsHashMap::iterator j;

    for(j = _dirsMap.begin(); j != _dirsMap.end();j++) {
    	dirInfo* thisDir = (dirInfo*)j.getData();
			if(!thisDir->isNew) {
      	// PRINF("thisdir is %p dir %p\n", thisDir, thisDir->dir);
				assert(thisDir->backupDir != NULL);
				assert(thisDir->dir != NULL);

        memcpy(thisDir->dir, thisDir->backupDir, xdefines::DIROPEN_ALLOC_SIZE);
      	Real::seekdir(thisDir->dir, thisDir->pos);
      }
    }
  }

  // We are trying to open a file in the rollback phase
  // There is no need to allocate a new block of memory since
  // we already have one.
  FILE* getFopen() {
    int fd = getFdAtOpen();
    FILE* file;

    // Check whether it is a file stream.
    if(fd == -1) {
      // Now this file is already closed before the end of transaction.
      // Or the fopen is not successful.
      file = NULL;
    } else {
      fileInfo* finfo;
      if(!_filesMap.find(fd, sizeof(fd), &finfo)) {
        assert(0);
      }
      // Simulate the fopen, since fopen will always call malloc().
      // It is possible that different library will have different size...
      void* ptr = malloc(xdefines::FOPEN_ALLOC_SIZE);
      // PRINF("getFopen fd %d and file %p current ptr %p\n", fd, finfo.origStream, ptr);

      // check whether the pointer is the same as before.
      assert(ptr == finfo->origStream);

      memcpy(ptr, finfo->backupStream, xdefines::FOPEN_ALLOC_SIZE);
      file = (FILE*)ptr;

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

    if(!_sysrecord.getFileOps(E_SYS_FILE_OPEN, &fd)) {
      assert(0);
    }
    
		// Now let's adjust the file position.
		if(fd != -1) {
			fileInfo* thisFile;
    	if(!_filesMap.find(fd, sizeof(fd), &thisFile)) {
      	assert(0);
    	}
			assert(thisFile != NULL);

			Real::lseek(thisFile->fd, thisFile->pos, SEEK_SET);
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
  int closeFile(int fd, FILE* fp) {
    int ret = 0;
    // Try to get corresponding entry in the hashmap.
    fileInfo* thisFile;

		if(fd == 0 || fd == 1 || fd == 2) {
			ret = 0;
			return ret;
		}

    if(!_filesMap.find(fd, sizeof(fd), &thisFile)) {
      errno = EBADF;
      ret = -1;
    }
		

    // Check whether the fp is equal to the saved one
    if(fp != thisFile->origStream) {
      errno = EBADF;
      ret = -1;
    }
	
    // Add to the closed list
    _sysrecord.recordFileOps(E_SYS_FILE_CLOSE, fd, ret);
    return ret;
  }

	int getCloseFile(int fd, FILE *fp) {
		int ret;
		_sysrecord.getFileOps(E_SYS_FILE_CLOSE, &fd, &ret);
		return ret;
	}
	
	int getClose(int fd) {
		int ret;
		_sysrecord.getFileOps(E_SYS_FILE_CLOSE, &fd, &ret);
		return ret;
  }

	// This function is called when an epoch is finished and 
	// there is no need to rollback
  void cleanClosedFiles() {
    int fd = -1;
    fileInfo* thisFile = NULL;

		//fprintf(stderr, "TONGPING: cleanClosedFiles\n");
		// We will check all closed files in this epoch 
    while((_sysrecord.retrieveFCloseList(&fd))) {

      if(fd == -1) {
        continue;
      }
			
      // Find the entry from the hash map.
      if(_filesMap.find(fd, sizeof(fd), &thisFile)) {
				assert(thisFile != NULL);
 
       // Check whether this file has dup?
        // According to description of dup,
        // http://www.linuxquestions.org/questions/programming-9/close-fd-after-dup-fd-558966/
        // close a newfd won't affect the new fd.
        // check whether this file is a file stream or not.
        if(thisFile->origStream) {
          Real::fclose(thisFile->origStream);

          assert(thisFile->backupStream != NULL);
          // Release temporary file stream now.
          InternalHeap::getInstance().free(thisFile->backupStream);
        } else {
          Real::close(fd);
        }

        // Remove this entry from the filemap.
        _filesMap.erase(fd, sizeof(fd));
        InternalHeap::getInstance().free(thisFile);
      } else {
        // Should not happen.
        assert(0);
      }
    }

    // When there is no need to rollback
    // we can remove the whole list.
    dirInfo* thisDir = NULL;
    DIR* dir = NULL;

    while((_sysrecord.retrieveDIRCloseList(&dir))) {
      if(dir == NULL) {
        continue;
      }

      // Find the entry from the hash map.
      if(_dirsMap.find(dir, sizeof(dir), &thisDir)) {
        assert(dir == thisDir->dir);

        // check whether this file is a file stream or not.
        Real::closedir(dir);

        // Release temporary file stream now.
        InternalHeap::getInstance().free(thisDir->backupDir);

        // Remove this entry from the _dirsMap.
        _dirsMap.erase(dir, sizeof(dir));
        InternalHeap::getInstance().free(thisDir);
      } else {
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
    } else if(fd == 1 || fd == 2 || fd == 3) {
      isAllowed = true;
    } else {
      fileInfo* thisFile = NULL;

      // Find the entry from the hash map.
      if(_filesMap.find(fd, sizeof(fd), &thisFile)) {
        isAllowed = true;
      }
    }

		//PRINT("checkPermission on fd %d isAllowed %d\n", fd, isAllowed);
    return isAllowed;
  }

private:
	SysRecord _sysrecord;

  /*
    typedef std::pair<int, fileInfo> objectPair;

    // We are maintainning a private hash map for each thread.
    typedef hash_map<int, fileInfo, fd_hash, fd_compare, HL::STLAllocator<objectPair, InternalHeap>
    > filesHashMap;
    //typedef hash_map<int, fileInfo, fd_hash, fd_compare, HL::STLAllocator<objectPair,
    InternalHeap> > filesHashMap;
  */
  typedef HashMap<int, fileInfo*, spinlock, InternalHeapAllocator> filesHashMap;
  // A list is to record those newly opened files, which will be added into
  // the global hash map in next transaction.
  filesHashMap _filesMap;

  typedef HashMap<void*, dirInfo*, spinlock, InternalHeapAllocator> dirsHashMap;
  // A list is to record those newly opened files, which will be added into
  // the global hash map in next transaction.
  dirsHashMap _dirsMap;
};

#endif
