/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1992, 1993
 *    The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the University of
 *    California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *    @(#)null_vfsops.c    8.7 (Berkeley) 5/14/95
 *
 * @(#)lofs_vfsops.c    1.2 (Berkeley) 6/18/92
 */

/*
 * Null Layer
 * (See null_vnops.c for a description of what this does.)
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc_internal.h>
#include <sys/errno.h>
#include <sys/kauth.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vnode_internal.h>
#include <sys/mount_internal.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include "null.h"
#include <sys/mount.h>
#include <sys/vm.h>
struct proc *current_proc(void);

struct vfs_attr;

extern int VFS_MOUNT(mount_t, vnode_t, user_addr_t, vfs_context_t);
extern int VFS_START(mount_t, int, vfs_context_t);
extern int VFS_UNMOUNT(mount_t, int, vfs_context_t);
extern int VFS_ROOT(mount_t, vnode_t *, vfs_context_t);
extern int VFS_QUOTACTL(mount_t, int, uid_t, caddr_t, vfs_context_t);
extern int VFS_GETATTR(mount_t, struct vfs_attr *, vfs_context_t);
extern int VFS_SETATTR(mount_t, struct vfs_attr *, vfs_context_t);
extern int VFS_SYNC(mount_t, int, vfs_context_t);
extern int VFS_VGET(mount_t, ino64_t, vnode_t *, vfs_context_t);
extern int VFS_FHTOVP(mount_t, int, unsigned char *, vnode_t *, vfs_context_t);
extern int VFS_VPTOFH(vnode_t, int *, unsigned char *, vfs_context_t);


#define VFSATTR_INIT(s)         ((s)->f_supported = (s)->f_active = 0LL)
#define VFSATTR_SET_SUPPORTED(s, a) ((s)->f_supported |= VFSATTR_ ## a)
#define VFSATTR_IS_SUPPORTED(s, a)  ((s)->f_supported & VFSATTR_ ## a)
#define VFSATTR_CLEAR_ACTIVE(s, a)  ((s)->f_active &= ~VFSATTR_ ## a)
#define VFSATTR_IS_ACTIVE(s, a)     ((s)->f_active & VFSATTR_ ## a)
#define VFSATTR_ALL_SUPPORTED(s)    (((s)->f_active & (s)->f_supported) == (s)->f_active)
#define VFSATTR_WANTED(s, a)        ((s)->f_active |= VFSATTR_ ## a)
#define VFSATTR_RETURN(s, a, x)     do { (s)-> a = (x); VFSATTR_SET_SUPPORTED(s, a);} while(0)

#define VFSATTR_f_objcount      (1LL<<  0)
#define VFSATTR_f_filecount     (1LL<<  1)
#define VFSATTR_f_dircount      (1LL<<  2)
#define VFSATTR_f_maxobjcount       (1LL<<  3)
#define VFSATTR_f_bsize         (1LL<< 4)
#define VFSATTR_f_iosize        (1LL<<  5)
#define VFSATTR_f_blocks        (1LL<<  6)
#define VFSATTR_f_bfree         (1LL<<  7)
#define VFSATTR_f_bavail        (1LL<<  8)
#define VFSATTR_f_bused         (1LL<<  9)
#define VFSATTR_f_files         (1LL<< 10)
#define VFSATTR_f_ffree         (1LL<< 11)
#define VFSATTR_f_fsid          (1LL<< 12)
#define VFSATTR_f_owner         (1LL<< 13)
#define VFSATTR_f_capabilities      (1LL<< 14)
#define VFSATTR_f_attributes        (1LL<< 15)
#define VFSATTR_f_create_time       (1LL<< 16)
#define VFSATTR_f_modify_time       (1LL<< 17)
#define VFSATTR_f_access_time       (1LL<< 18)
#define VFSATTR_f_backup_time       (1LL<< 19)
#define VFSATTR_f_fssubtype     (1LL<< 20)
#define VFSATTR_f_vol_name      (1LL<< 21)
#define VFSATTR_f_signature     (1LL<< 22)
#define VFSATTR_f_carbon_fsid       (1LL<< 23)
#define VFSATTR_f_uuid          (1LL<< 24)

struct vfs_attr {
    uint64_t    f_supported;
    uint64_t    f_active;

    uint64_t    f_objcount; /* number of filesystem objects in volume */
    uint64_t    f_filecount;    /* ... files */
    uint64_t    f_dircount; /* ... directories */
    uint64_t    f_maxobjcount;  /* maximum number of filesystem objects */

    uint32_t    f_bsize;    /* block size for the below size values */
    size_t      f_iosize;   /* optimal transfer block size */
    uint64_t    f_blocks;   /* total data blocks in file system */
    uint64_t    f_bfree;    /* free blocks in fs */
    uint64_t    f_bavail;   /* free blocks avail to non-superuser */
    uint64_t    f_bused;    /* blocks in use */
    uint64_t    f_files;    /* total file nodes in file system */
    uint64_t    f_ffree;    /* free file nodes in fs */
    fsid_t      f_fsid;     /* file system id */
    uid_t       f_owner;    /* user that mounted the filesystem */

    vol_capabilities_attr_t f_capabilities;
    vol_attributes_attr_t f_attributes;

    struct timespec f_create_time;  /* creation time */
    struct timespec f_modify_time;  /* last modification time */
    struct timespec f_access_time;  /* time of last access */
    struct timespec f_backup_time;  /* last backup time */

    uint32_t    f_fssubtype;    /* filesystem subtype */

    char        *f_vol_name;    /* volume name */

    uint16_t    f_signature;    /* used for ATTR_VOL_SIGNATURE, Carbon's FSVolumeInfo.signature */
    uint16_t    f_carbon_fsid;  /* same as Carbon's FSVolumeInfo.filesystemID */
};

void    vfs_getnewfsid(struct mount *);

/*
 * Mount null layer
 */
static int
nullfs_mount(mp, devvp, data, context)
    struct mount *mp;
    vnode_t devvp;
    user_addr_t data;
    vfs_context_t context;
{
    int error = 0;
    struct user_null_args args;
    struct vnode *lowerrootvp, *vp;
    struct vnode *nullm_rootvp;
    struct null_mount *xmp;
    size_t size;
    struct nameidata nd;

#ifdef NULLFS_DIAGNOSTIC
    printf("nullfs_mount(mp = %x)\n", mp);
#endif

    /*
     * Update is a no-op
     */
    if (mp->mnt_flag & MNT_UPDATE) {
        return (ENOTSUP);
        /* return VFS_MOUNT(MOUNTTONULLMOUNT(mp)->nullm_vfs, devvp, data,  p);*/
    }

    /*
     * Get argument
     */
    if (vfs_context_is64bit(context)) {
        error = copyin(data, (caddr_t)&args, sizeof (args));
    }
    else {
        struct null_args temp;
        error = copyin(data, (caddr_t)&temp, sizeof (temp));
        args.target = CAST_USER_ADDR_T(temp.target);
    }
    if (error)
        return (error);

    /*
     * Find lower node
     */
    NDINIT(&nd, LOOKUP, FOLLOW|WANTPARENT,
        UIO_USERSPACE, args.target, context);
    if (error = namei(&nd))
        return (error);
    nameidone(&nd);
    /*
     * Sanity check on lower vnode
     */
    lowerrootvp = nd.ni_vp;
    vnode_get(lowerrootvp);
    vnode_ref(lowerrootvp);

    //vnode_put(nd.ni_dvp);
    nd.ni_dvp = NULL;
    
    printf("lowerrootvp->v_type = %d\n", lowerrootvp->v_type);
    printf("lowerrootvp->v_usecount = %d\n", lowerrootvp->v_usecount);
    printf("lowerrootvp->v_iocount = %d\n", lowerrootvp->v_iocount);

    xmp = (struct null_mount *) _MALLOC(sizeof(struct null_mount),
                M_UFSMNT, M_WAITOK);    /* XXX */

    /*
     * Save reference to underlying FS
     */
    xmp->nullm_vfs = lowerrootvp->v_mount;

    /*
     * Save reference.  Each mount also holds
     * a reference on the root vnode.
     */
    error = null_node_create(mp, lowerrootvp, &vp, 1);
    /*
     * Make sure the node alias worked
     */
    if (error) {
        vnode_put(lowerrootvp);
        FREE(xmp, M_UFSMNT);    /* XXX */
        return (error);
    }

    /*
     * Keep a held reference to the root vnode.
     * It is vnode_put'd in nullfs_unmount.
     */
    nullm_rootvp = vp;
    vnode_get(nullm_rootvp);
    nullm_rootvp->v_flag |= VROOT;
    xmp->nullm_rootvp = nullm_rootvp;
    //if (NULLVPTOLOWERVP(nullm_rootvp)->v_mount->mnt_flag & MNT_LOCAL)
        mp->mnt_flag |= MNT_LOCAL;
    mp->mnt_data = (qaddr_t) xmp;
    vfs_getnewfsid(mp);

    (void) copyinstr(args.target, mp->mnt_vfsstat.f_mntfromname, MAXPATHLEN - 1, 
        &size);
    bzero(mp->mnt_vfsstat.f_mntfromname + size, MNAMELEN - size);
#ifdef NULLFS_DIAGNOSTIC
    printf("nullfs_mount: lower %s, alias at %s\n",
        mp->mnt_vfsstat.f_mntfromname, mp->mnt_vfsstat.f_mntonname);
#endif
    return (0);
}

/*
 * VFS start.  Nothing needed here - the start routine
 * on the underlying filesystem will have been called
 * when that filesystem was mounted.
 */
static int
nullfs_start(mp, flags, context)
    struct mount *mp;
    int flags;
    vfs_context_t context;
{
    return (0);
    /* return VFS_START(MOUNTTONULLMOUNT(mp)->nullm_vfs, flags, context); */
}

/*
 * Free reference to null layer
 */
static int
nullfs_unmount(mp, mntflags, context)
    struct mount *mp;
    int mntflags;
    vfs_context_t context;
{
    *((int *) 0) = 0x33333333;
    return;
#if 0
    struct vnode *nullm_rootvp = MOUNTTONULLMOUNT(mp)->nullm_rootvp;
    int error;
    int flags = 0;
    int force = 0;

#ifdef NULLFS_DIAGNOSTIC
    printf("nullfs_unmount(mp = %x)\n", mp);
#endif

    if (mntflags & MNT_FORCE) {
        flags |= FORCECLOSE;
        force = 1;
    }

    if ( (nullm_rootvp->v_usecount > 1) && !force )
        return (EBUSY);
    if ( (error = vflush(mp, nullm_rootvp, flags)) && !force )
        return (error);

#ifdef NULLFS_DIAGNOSTIC
    vprint("alias root of lower", nullm_rootvp);
#endif     
    /*
     * Release reference on underlying root vnode
     */
    vnode_put(nullm_rootvp);
    /*
     * And blow it away for future re-use
     */
    vnode_reclaim(nullm_rootvp);
    /*
     * Finally, throw away the null_mount structure
     */
    FREE(mp->mnt_data, M_UFSMNT);    /* XXX */
    mp->mnt_data = 0;
    return 0;
#endif
}

static int
nullfs_root(mp, vpp, context)
    struct mount *mp;
    struct vnode **vpp;
    vfs_context_t context;
{
    struct proc *p = current_proc();//curproc;    /* XXX */
    struct vnode *vp;

#ifdef NULLFS_DIAGNOSTIC
    printf("nullfs_root(mp = %x, vp = %x->%x)\n", mp,
            MOUNTTONULLMOUNT(mp)->nullm_rootvp,
            NULLVPTOLOWERVP(MOUNTTONULLMOUNT(mp)->nullm_rootvp)
            );
#endif
    //*((unsigned int *) 0xdead1234) = 0x1234;
    /*
     * Return locked reference to root.
     */
    vp = MOUNTTONULLMOUNT(mp)->nullm_rootvp;
    vnode_get(vp);
    *vpp = vp;
    return 0;
}

static int
nullfs_quotactl(mp, cmd, uid, datap, context)
    struct mount *mp;
    int cmd;
    uid_t uid;
    caddr_t datap;
    vfs_context_t context;
{
    return -1;//return VFS_QUOTACTL(MOUNTTONULLMOUNT(mp)->nullm_vfs, cmd, uid, datap, context);
}

int vfs_getattr(mount_t mp, struct vfs_attr *vfa, vfs_context_t ctx);
static int
nullfs_getattr(mount_t mp, struct vfs_attr *fsap, vfs_context_t context)
{
    int error;
    struct vfs_attr attr;

#ifdef NULLFS_DIAGNOSTIC
    printf("nullfs_statfs(mp = %x, vp = %x->%x)\n", mp,
            MOUNTTONULLMOUNT(mp)->nullm_rootvp,
            NULLVPTOLOWERVP(MOUNTTONULLMOUNT(mp)->nullm_rootvp)
            );
#endif

    if(!MOUNTTONULLMOUNT(mp)->nullm_vfs) {
        return -1;
    }

    VFSATTR_INIT(&attr);
    VFSATTR_WANTED(&attr, f_bsize);
    VFSATTR_WANTED(&attr, f_blocks);
    VFSATTR_WANTED(&attr, f_bfree);
    VFSATTR_WANTED(&attr, f_bavail);
    VFSATTR_WANTED(&attr, f_files);
    VFSATTR_WANTED(&attr, f_ffree);
    
    error = vfs_getattr(MOUNTTONULLMOUNT(mp)->nullm_vfs, &attr, context);
    if (error)
        return (error);

    /* now copy across the "interesting" information and fake the rest */
    //sbp->f_type = mstat.f_type;
    
    fsap->f_bsize = VFSATTR_IS_SUPPORTED(&attr, f_bsize) ? attr.f_bsize : 0;
    fsap->f_blocks = VFSATTR_IS_SUPPORTED(&attr, f_blocks) ? attr.f_blocks : 0;
    fsap->f_bfree = VFSATTR_IS_SUPPORTED(&attr, f_bfree) ? attr.f_bfree : 0;
    fsap->f_bavail = VFSATTR_IS_SUPPORTED(&attr, f_bavail) ? attr.f_bavail : 0;
    fsap->f_files = VFSATTR_IS_SUPPORTED(&attr, f_files) ? attr.f_files : 0;
    fsap->f_ffree = VFSATTR_IS_SUPPORTED(&attr, f_ffree) ? attr.f_ffree : 0;       

    VFSATTR_SET_SUPPORTED(fsap, f_bsize);
    VFSATTR_SET_SUPPORTED(fsap, f_blocks);
    VFSATTR_SET_SUPPORTED(fsap, f_bfree);
    VFSATTR_SET_SUPPORTED(fsap, f_bavail);
    VFSATTR_SET_SUPPORTED(fsap, f_files);
    VFSATTR_SET_SUPPORTED(fsap, f_ffree);       
    return (0);

}

static int
nullfs_sync(__unused struct mount *mp, __unused int waitfor,
    __unused vfs_context_t context)
{
    /*
     * XXX - Assumes no data cached at null layer.
     */
    return (0);
}

static int
nullfs_vget(mp, ino, vpp, context)
    struct mount *mp;
    ino64_t ino;
    struct vnode **vpp;
    vfs_context_t context;
{
    
    return VFS_VGET(MOUNTTONULLMOUNT(mp)->nullm_vfs, ino, vpp, context);
}

static int
nullfs_fhtovp(mp, fhlen, fhp, vpp, context)
    struct mount *mp;
    int fhlen;
    unsigned char *fhp;
    struct vnode **vpp;
    vfs_context_t context;
{

    return VFS_FHTOVP(MOUNTTONULLMOUNT(mp)->nullm_vfs, fhlen, fhp, vpp, context);
}

static int
nullfs_vptofh(vp, fhlenp, fhp, context)
    struct vnode *vp;
    int *fhlenp;
    unsigned char *fhp;
    vfs_context_t context;
{
    return VFS_VPTOFH(NULLVPTOLOWERVP(vp), fhlenp, fhp, context);
}

int nullfs_init (struct vfsconf *);

// copypaste

struct vfsops {
    /*!
     @function vfs_mount
     @abstract Perform filesystem-specific operations required for mounting.
     @discussion Typical operations include setting the mount-specific data with vfs_setfsprivate().
     Note that if a mount call fails, the filesystem must clean up any state it has constructed, because
     vfs-level mount code will not clean it up.  
     @param mp Mount structure for the newly mounted filesystem.
     @param devvp Device that the filesystem is mounted from.
     @param data Filesystem-specific data passed down from userspace.
     @param context Context to authenticate for mount.
     @return 0 for success, else an error code.  Once success is returned, the filesystem should be ready to go active;
     VFS will not ask again.
     */
    int  (*vfs_mount)(struct mount *mp, vnode_t devvp, user_addr_t data, vfs_context_t context);
    
    /*!
     @function vfs_start
     @abstract Mark a mount as ready to be used.
     @discussion After receiving this calldown, a filesystem will be hooked into the mount list and should expect 
     calls down from the VFS layer.  
     @param mp Mount structure being activated.
     @param flags Unused.
     @param context Context to authenticate for mount.
     @return Return value is ignored.
     */
    int  (*vfs_start)(struct mount *mp, int flags, vfs_context_t context);
    
    /*!
     @function vfs_unmount
     @abstract Perform filesystem-specific cleanup as part of unmount.
     @discussion If the unmount downcall succeeds, VFS considers itself authorized to destroy all
     state related to the mount.
     @param mp Mount structure to unmount.
     @param mntflags MNT_FORCE indicates that we wish to unmount even if there are active vnodes.
     @param context Context to authenticate for unmount.
     @return 0 for success, else an error code.  
     */
    int  (*vfs_unmount)(struct mount *mp, int mntflags, vfs_context_t context);
    
    /*!
     @function vfs_root
     @abstract Get the root vnode of a filesystem.
     @discussion Upon success, should return with an iocount held on the root vnode which the caller will
     drop with vnode_put().
     @param mp Mount for which to get the root.
     @param vpp Destination for root vnode.
     @param context Context to authenticate for getting the root.
     @return 0 for success, else an error code.  
     */
    int  (*vfs_root)(struct mount *mp, struct vnode **vpp, vfs_context_t context);
    
    /*!
     @function vfs_quotactl
     @abstract Manipulate quotas for a volume.
     @param mp Mount for which to manipulate quotas.
     @param cmds Detailed in "quotactl" manual page.      
     @param uid Detailed in "quotactl" manual page.
     @param arg Detailed in "quotactl" manual page.
     @param context Context to authenticate for changing quotas.
     @return 0 for success, else an error code.  
     */
    int  (*vfs_quotactl)(struct mount *mp, int cmds, uid_t uid, caddr_t arg, vfs_context_t context);

    /*!
     @function vfs_getattr
     @abstract Get filesystem attributes.
     @discussion See VFSATTR_RETURN, VFSATTR_ACTIVE, VFSATTR_SET_SUPPORTED, VFSATTR_WANTED macros.
     @param mp Mount for which to get parameters.
     @param vfa Container for specifying which attributes are desired and which attributes the filesystem
     supports, as well as for returning results.
     @param ctx Context to authenticate for getting filesystem attributes.
     @return 0 for success, else an error code.  
     */
    int  (*vfs_getattr)(struct mount *mp, struct vfs_attr *, vfs_context_t context);
/*    int  (*vfs_statfs)(struct mount *mp, struct vfsstatfs *sbp, vfs_context_t context);*/

    /*!
     @function vfs_sync
     @abstract Flush all filesystem data to backing store.
     @discussion vfs_sync will be called as part of the sync() system call and during unmount.
     @param mp Mountpoint to sync.
     @param waitfor MNT_WAIT: flush synchronously, waiting for all data to be written before returning. MNT_NOWAIT: start I/O but do not wait for it.
     @param ctx Context to authenticate for the sync.
     @return 0 for success, else an error code.  
     */
    int  (*vfs_sync)(struct mount *mp, int waitfor, vfs_context_t context);
    
    /*!
     @function vfs_vget
     @abstract Get a vnode by file id (inode number).
     @discussion This routine is chiefly used to build paths to vnodes.  Result should be turned with an iocount that the 
     caller will drop with vnode_put().
     @param mp Mount against which to look up inode number.
     @param ino File ID for desired file, as found through a readdir.
     @param vpp Destination for vnode.
     @return 0 for success, else an error code.  
     */
    int  (*vfs_vget)(struct mount *mp, ino64_t ino, struct vnode **vpp, vfs_context_t context);
    
    /*!
     @function vfs_fhtovp
     @abstract Get the vnode corresponding to a file handle.
     @discussion Filesystems can return handles to files which are independent of their (transient) vnode identities.
     vfs_thtovp converts that persistent handle back to a vnode.  The vnode should be returned with an iocount which 
     the caller will drop with vnode_put().
     @param mp Mount against which to look up file handle.
     @param fhlen Size of file handle structure, as returned by vfs_vptofh.
     @param fhp Pointer to handle.
     @param vpp Destination for vnode.
     @param ctx Context against which to authenticate the file-handle conversion.
     @return 0 for success, else an error code.  
     */
    int  (*vfs_fhtovp)(struct mount *mp, int fhlen, unsigned char *fhp, struct vnode **vpp,
                       vfs_context_t context);

    /*!
     @function vfs_vptofh
     @abstract Get a persistent handle corresponding to a vnode.
     @param mp Mount against which to convert the vnode to a handle.
     @param fhlen Size of buffer provided for handle; set to size of actual handle returned.
     @param fhp Pointer to buffer in which to place handle data.
     @param ctx Context against which to authenticate the file-handle request.
     @return 0 for success, else an error code.  
     */
    int  (*vfs_vptofh)(struct vnode *vp, int *fhlen, unsigned char *fhp, vfs_context_t context);

    /*!
     @function vfs_init
     @abstract Prepare a filesystem for having instances mounted.
     @discussion This routine is called once, before any particular instance of a filesystem
     is mounted; it allows the filesystem to initialize whatever global data structures
     are shared across all mounts.  If this returns successfully, a filesystem should be ready to have
     instances mounted.
     @param vfsconf Configuration information.  Currently, the only useful data are the filesystem name,
     typenum, and flags.  The flags field will be either 0 or MNT_LOCAL.  Many filesystems ignore this
     parameter.
     @return 0 for success, else an error code.  
     */
    int  (*vfs_init)(struct vfsconf *);
    
    /*!
     @function vfs_sysctl
     @abstract Broad interface for querying and controlling filesystem.
     @discussion VFS defines VFS_CTL_QUERY as a generic status request which is answered
     with the VQ_* macros in a "struct vfsquery."
     A filesystem may also define implementation-specific commands.  See "man 3 sysctl" 
     for the meaning of sysctl parameters.
     @param context Context against which to authenticate command.
     @return 0 for success, else an error code.  
     */
    int  (*vfs_sysctl)(int *, u_int, user_addr_t, size_t *, user_addr_t, size_t, vfs_context_t context);

    /*!
     @function vfs_setattr
     @abstract Set filesystem attributes.
     @discussion The other side of the vfs_getattr coin.  Currently only called to set volume name.
     @param mp Mount on which to set attributes.
     @param vfa VFS attribute structure containing requested attributes to set and their values.  Currently
     will only be called with f_vol_name set.
     @param context Context against which to authenticate attribute change.
     @return 0 for success, else an error code.  
     */
    int  (*vfs_setattr)(struct mount *mp, struct vfs_attr *, vfs_context_t context);
    void *vfs_reserved[7];
};


#define nullfs_sysctl (int (*) (int *, u_int, user_addr_t, size_t *, user_addr_t, size_t, vfs_context_t))eopnotsupp
#define nullfs_setattr (int  (*)(struct mount *mp, struct vfs_attr *, vfs_context_t context)) eopnotsupp


struct vfsops null_vfsops = {
    nullfs_mount,
    nullfs_start,
    nullfs_unmount,
    nullfs_root,
    nullfs_quotactl,
    nullfs_getattr,
    nullfs_sync,
    nullfs_vget,
    nullfs_fhtovp,
    nullfs_vptofh,
    nullfs_init,
    nullfs_sysctl,
    nullfs_setattr,
};

struct vfs_fsentry {
    struct vfsops * vfe_vfsops; /* vfs operations */
    int     vfe_vopcnt; /* # of vnodeopv_desc being registered (reg, spec, fifo ...) */
    struct vnodeopv_desc ** vfe_opvdescs; /* null terminated;  */
    int         vfe_fstypenum;  /* historic filesystem type number */
    char        vfe_fsname[MFSNAMELEN]; /* filesystem type name */
    uint32_t    vfe_flags;      /* defines the FS capabilities */
    void *      vfe_reserv[2];  /* reserved for future use; set this to zero*/
 };


extern struct vnodeopv_desc null_vnodeop_opv_desc;
struct vnodeopv_desc *descs[] = { &null_vnodeop_opv_desc, NULL };
struct vfs_fsentry fe = {
    &null_vfsops,
    1,
    descs,
    9,
    "loopback",
    VFC_VFSGENERICARGS,
    NULL,
    NULL
};

vfstable_t ft;

void *magic[3] = {(void *) 0xdeadf00d, &fe, &ft};

