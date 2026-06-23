#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>

#include "xfermon.h"

/* check the GENHD_FL_REMOVABLE flag on a gendisk */
bool xfermon_disk_is_removable(struct gendisk *disk) {
  return (disk->flags & GENHD_FL_REMOVABLE) != 0;
}

/* resolve the gendisk backing a file: file -> inode -> sb -> bdev -> disk */
struct gendisk *xfermon_file_disk(struct file *file) {
  struct super_block *sb;

  if (!file || !file->f_inode) {
    return NULL;
  }

  sb = file->f_inode->i_sb;
  if (!sb || !sb->s_bdev) {
    return NULL;
  }

  return sb->s_bdev->bd_disk;
}
