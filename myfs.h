#include "myfs_core.h"

/*** Create filesystem ***/
/* == DESCRYPTION ==
 * It will create a virtual disk file with the filesystem,
 * but not mount it.
 * If the virtual disk file is exist, it will re-format it.
 * 
 * == RETURN ==
 * -1 : Can't open file
 * 0  : Success
 */
int myfs_create(const char *filesystemname, int max_size);

/*** Destroy filesystem ***/
/* == DESCRYPTION ==
 * It will unmount the filesystem if mounted
 * and destroy the virtual disk file with it
 * 
 * == RETURN ==
 * 0  : Success
 * -1 : Remove failed
 * -2 : Unmount failed
 */
int myfs_destroy(const char *filesystemname);

/*** Mount filesystem ***/
/* == DESCRYPTION ==
 * Mount the existing filesystem manually
 *
 * == RETURN ==
 * 0  : Success
 * -1 : Can't open file
 * -2 : Unmount failed
 */
int myfs_mount(const char *filesystemname);

/*** Unount filesystem ***/
/* == DESCRYPTION ==
 * Unmount the existing filesystem manually 
 *
 * == RETURN ==
 * 1  : Already unmount
 * 0  : Success
 * -1 : Close failed
 */
int myfs_umount();

/*** Open file ***/
/* == DESCRYPTION ==
 * Open file in filesystem
 * 
 * == RETURN ==
 * nonegative int : File descriptor
 * -1 : No mounted filesystem
 * -2 : Disk empty
 * -3 : Error opening disk file
 * -4 : File exists
 */
int myfs_file_open(const char *filename);

/*** Close file ***/
/* == DESCRYPTION ==
 * Close file descriptor
 * 
 * == RETURN ==
 * 0  : Success
 * -1 : No mounted filesystem
 * -2 : Disk empty
 * -3 : Error closing file descriptor
 * -4 : File discriptor not found
 */
int myfs_file_close(int fd);

/*** Create file ***/
/* == DESCRYPTION ==
 * Create empty file in filesystem
 * 
 * == RETURN ==
 * 0  : Success
 * -1 : No mounted filesystem
 * -2 : No unused inode
 * -3 : File discriptor not found
 * -4 : File exists
 */
int myfs_file_create(const char *filename);

/*** Delete file ***/
/* == DESCRYPTION ==
 * Delete file from filesystem
 * 
 * == RETURN ==
 * 0  : Success
 * -1 : No mounted filesystem
 * -2 : Disk empty
 * -3 : Error writing disk file
 * -4 : File exists
 */
int myfs_file_delete(const char *filename);
int myfs_file_read(int fd, char *buf, int count);
int myfs_file_write(int fd, char *buf, int count);
