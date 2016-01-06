#include "myfs_core.h"

/*** Create filesystem ***/
/* It will create a virtual disk file with the filesystem,
 * and mount it automatically
 * If the virtual disk file is exist, it will re-format it.
 */
int myfs_create(const char *filesystemname, int max_size);

/*** Destroy filesystem ***/
/* It will unmount the filesystem automatically
 * and destroy the virtual disk file with it
 */
int myfs_destroy(const char *filesystemname);

/*** Mount filesystem ***/
/* Mount the existing filesystem manually 
 */
int myfs_mount(const char *filesystemname);

/*** Unount filesystem ***/
/* Unmount the existing filesystem manually 
 */
int myfs_umount();
