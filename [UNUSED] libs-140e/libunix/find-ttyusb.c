// engler, cs140e: your code to find the tty-usb device on your laptop.
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "libunix.h"

#define _SVID_SOURCE
#include <dirent.h>
static const char *ttyusb_prefixes[] = {
	"ttyUSB",	// linux
    "ttyACM",   // linux
	"cu.SLAB_USB", // mac os
    "cu.usbserial", // mac os
    // if your system uses another name, add it.
	0
};

static int filter(const struct dirent *d) {
    // scan through the prefixes, returning 1 when you find a match.
    // 0 if there is no match.
    for(unsigned i = 0; ttyusb_prefixes[i] != 0; i++) {
        if(strncmp((*d).d_name, ttyusb_prefixes[i], strlen(ttyusb_prefixes[i])) == 0) {
            return 1;  // match found
        }
    }
    return 0;  // no match
}

// find the TTY-usb device (if any) by using <scandir> to search for
// a device with a prefix given by <ttyusb_prefixes> in /dev
// returns:
//  - device name.
// error: panic's if 0 or more than 1 devices.
char *find_ttyusb(void) {
    // use <alphasort> in <scandir>
    // return a malloc'd name so doesn't corrupt.
    struct dirent **nameList; // will be an array of pointers to dirents
    int numNames = scandir("/dev", &nameList, filter, alphasort);
    if(numNames != 1)
        panic("find_ttyusb: did not find exactly 1 tty-usb. found: %d\n", numNames);

    char *prefix = "/dev/";
    char *filename = (**nameList).d_name;

    char *absoluteName = malloc(strlen(prefix) + strlen(filename) + 1);
    if(absoluteName == NULL)
        panic("find_ttyusb: failed to malic() absoluteName\n");
    strcpy(absoluteName, prefix);
    strcpy(absoluteName + strlen(prefix), filename);

    char *name = absoluteName;//strcat(prefix, filename);  // *(nameList[0])
    char *returnName = strdupf(name);
    if (returnName == NULL)
        panic("find_ttyusb: could not malloc() filename\n");

    return returnName;
}

// return the most recently mounted ttyusb (the one
// mounted last).  use the modification time 
// returned by state.
char *find_ttyusb_first(void) {
    struct dirent **nameList; // will be an array of pointers to dirents
    int numNames = scandir("/dev", &nameList, filter, alphasort);
    // if(numNames != 1)
    //     panic("find_ttyusb: did not find exactly 1 tty-usb. found: %d\n", numNames);

    char *prefix = "/dev/";

    time_t shortestTime = 0;
    char *returnName = NULL;

    for(int i = 0; i < numNames; i++) {
        char *filename = (*(nameList[i])).d_name;

        char *absoluteName = malloc(strlen(prefix) + strlen(filename) + 1);
        if(absoluteName == NULL)
            panic("find_ttyusb: failed to malic() absoluteName\n");
        strcpy(absoluteName, prefix);
        strcpy(absoluteName + strlen(prefix), filename);

        struct stat fileData;
        int retValue = stat(absoluteName, &fileData);
        if(retValue == -1)
            panic("find_ttyusb: stat() failed");

        time_t thisTime = fileData.st_mtime;
        if (thisTime < shortestTime || returnName == NULL) {  // first time through too.
            shortestTime = thisTime;
            returnName = strdupf(absoluteName);
        }
        free(absoluteName);
    }

    return returnName;

    // char *name = absoluteName;//strcat(prefix, filename);  // *(nameList[0])
    // char *returnName = strdupf(name);
    // if (returnName == NULL)
    //     panic("find_ttyusb: could not malloc() filename\n");

    // return returnName;
}
// return the oldest mounted ttyusb (the one mounted
// "first") --- use the modification returned by
// stat()
char *find_ttyusb_last(void) {
    struct dirent **nameList; // will be an array of pointers to dirents
    int numNames = scandir("/dev", &nameList, filter, alphasort);
    // if(numNames != 1)
    //     panic("find_ttyusb: did not find exactly 1 tty-usb. found: %d\n", numNames);

    char *prefix = "/dev/";

    time_t longestTime = 0;
    char *returnName = NULL;

    for(int i = 0; i < numNames; i++) {
        char *filename = (*(nameList[i])).d_name;

        char *absoluteName = malloc(strlen(prefix) + strlen(filename) + 1);
        if(absoluteName == NULL)
            panic("find_ttyusb: failed to malic() absoluteName\n");
        strcpy(absoluteName, prefix);
        strcpy(absoluteName + strlen(prefix), filename);

        struct stat fileData;
        int retValue = stat(absoluteName, &fileData);
        if(retValue == -1)
            panic("find_ttyusb: stat() failed");

        time_t thisTime = fileData.st_mtime;
        if (thisTime > longestTime || returnName == NULL) {  // first time through too.
            longestTime = thisTime;
            returnName = strdupf(absoluteName);
        }
        free(absoluteName);
    }

    return returnName;
}


// NOTE ----------------------------------------
// I might want to free all the scandir results using the following code:
//     // Free accordingly after searching through directory
//     for (int i = 0; i < numNames; i++)
//     {
//         free(nameList[i]);
//     }
//     free(nameList);*/