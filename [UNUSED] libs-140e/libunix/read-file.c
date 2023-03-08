#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libunix.h"

// allocate buffer, read entire file into it, return it.   
// buffer is zero padded to a multiple of 4.
//
//  - <size> = exact nbytes of file.
//  - for allocation: round up allocated size to 4-byte multiple, pad
//    buffer with 0s. 
//
// fatal error: open/read of <name> fails.
//   - make sure to check all system calls for errors.
//   - make sure to close the file descriptor (this will
//     matter for later labs).
// 
void *read_file(unsigned *size, const char *name) {
    struct stat fileData;
    int retValue = stat(name, &fileData);
    if(retValue == -1)
        panic("read_file: stat() failed");

    *size = fileData.st_size;

    int fd = open(name, O_RDONLY);
    if(fd == -1)
        panic("read_file: failed to open file");
    unsigned padToAdd = *size % 4;

    void *contents = calloc(1, *size + padToAdd);
    if(contents == NULL)
        panic("read_file: calloc() failed");

    unsigned numRead = read(fd, contents, *size);
    if(numRead != *size)
        panic("read_file: %d of %d bytes read", numRead, *size);

    close(fd);

    return contents;
    // How: 
    //    - use stat to get the size of the file.
    //    - round up to a multiple of 4.
    //    - allocate a buffer
    //    - zero pads to a multiple of 4.
    //    - read entire file into buffer.  
    //    - make sure any padding bytes have zeros.
    //    - return it.   
}
