#include <sys/cdefs.h>
#include "namespace.h"
#include <lib.h>
#include <string.h>
#include <unistd.h>

int inodewalker()
{
  message m;
  memset(&m, 0, sizeof(m));
  int ret = (_syscall(VFS_PROC_NR, VFS_INODEWALKER, &m));
  if (ret < 0)
 	  return m.m1_i1; //return error code
   return ret;
}

int zonewalker()
{
  message m;
  memset(&m, 0, sizeof(m));
  int ret = (_syscall(VFS_PROC_NR, VFS_ZONEWALKER, &m));
  if (ret < 0)
	  return m.m1_i1; //return error code
  return ret;
}

/*
int recovery(int mode)
{
  message m;
  memset(&m, 0, sizeof(m));
  m.m1_i1 = (int) mode;
  int ret = (_syscall(VFS_PROC_NR, VFS_RECOVERY, &m));
  if (ret < 0)
	  return m.m1_i1; //return error code
  return ret;
}

int damage_inode(int inode_nb)
{
  message m;
  memset(&m, 0, sizeof(m));
  m.m1_i1 = (int) inode_nb;
  int ret = (_syscall(VFS_PROC_NR, VFS_DAMAGEINODE, &m));
  if (ret < 0)
	  return m.m1_i1; //return error code
  return ret;
}

int damage_dir(char* path)
{
  message m;
  memset(&m, 0, sizeof(m));
  m.m1_p1 = path;
  int ret = (_syscall(VFS_PROC_NR, VFS_DAMAGEDIR, &m));
  if (ret < 0)
	  return m.m1_i1; //return error code
  return ret;
}
*/
