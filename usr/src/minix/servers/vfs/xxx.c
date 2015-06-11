/* Includes */
#include "fs.h"
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <minix/callnr.h>
#include <minix/safecopies.h>
#include <minix/endpoint.h>
#include <minix/com.h>
#include <minix/sysinfo.h>
#include <minix/u64.h>
#include <sys/ptrace.h>
#include <sys/svrctl.h>
#include <sys/resource.h>
#include "file.h"
#include "scratchpad.h"
#include <minix/vfsif.h>
#include "vnode.h"
#include "vmnt.h"
#include "path.h"
#include "lock.h"

/* System call inodewalker */
int do_inodewalker(){
	printf("HEY I AM DO_INODEWALKER\n");
	int r;
	struct vmnt *vmp;
	struct vnode *vp;
	struct lookup resolve;
	char fullpath[PATH_MAX] = "/";

	/* Get a virtual inode and virtual mount corresponding to the path */
	lookup_init(&resolve, fullpath, PATH_NOFLAGS, &vmp, &vp);
	resolve.l_vmnt_lock = VMNT_READ;
	resolve.l_vnode_lock = VNODE_READ;
	if ((vp = last_dir(&resolve, fp)) == NULL) return(err_code);

	/* Emit a request to FS */
	r = req_inodewalker(vmp->m_fs_e);

	/* Unlock virtual inode and virtual mount */
	unlock_vnode(vp);
	printf("Virtual node unlocked.\n");
	unlock_vmnt(vmp);
	printf("Virtual mount unlocked.\n");
	put_vnode(vp);
	printf("Virtual node released.\n");

	return r;
}

/* System call zonewalker */
int do_zonewalker(){
	printf("HEY I AM DO_ZONEWALKER\n");
	int r;
	struct vmnt *vmp;
	struct vnode *vp;
	struct lookup resolve;
	char fullpath[PATH_MAX] = "/";

	/* Get a virtual inode and virtual mount corresponding to the path */
	lookup_init(&resolve, fullpath, PATH_NOFLAGS, &vmp, &vp);
	resolve.l_vmnt_lock = VMNT_READ;
	resolve.l_vnode_lock = VNODE_READ;
	if ((vp = last_dir(&resolve, fp)) == NULL) return(err_code);

	/* Emit a request to FS */
	r = req_zonewalker(vmp->m_fs_e);

	/* Unlock virtual inode and virtual mount */
	unlock_vnode(vp);
	printf("Virtual node unlocked.\n");
	unlock_vmnt(vmp);
	printf("Virtual mount unlocked.\n");
	put_vnode(vp);
	printf("Virtual node released.\n");

	return r;
}

/* System call recovery */
int do_recovery(){
	printf("FUCK YOU EVEN MORE\n");
	printf("MODE: %d", m_in.m1_i1);
	return 0;
}

/* System call damage inode */
int do_damage_inode(){
	printf("FUCK YOU EVEN EVEN MORE\n");
	printf("INODE NR: %d", m_in.m1_i1);
	return 0;
}

/* System call damage directory file */
int do_damage_dir(){
	char* msg = m_in.m1_p1;
	printf("FUCK YOU EVEN EVEN MORE\n");
	printf("DIRECTORY PATH: %s", msg);
	return 0;
}
