#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>

#include "xfermon.h"

/**
 * check the GENHD_FL_REMOVABLE flag on a gendisk.
 */
bool xfermon_disk_is_removable(struct gendisk *disk) {
#ifdef GENHD_FL_REMOVABLE
  return (disk->flags & GENHD_FL_REMOVABLE) != 0;
#else
  return false;
#endif
}

/**
 * resolve the gendisk backing a file.
 */
struct gendisk *xfermon_file_disk(struct file *file) {
  struct super_block *sb;

  /* struct file -> struct inode -> struct super_block -> block_device */
  if (!file || !file->f_inode) {
    return NULL;
  }

  sb = file->f_inode->i_sb;
  if (!sb || !sb->s_bdev) {
    return NULL;
  }

  return sb->s_bdev->bd_disk;
}
