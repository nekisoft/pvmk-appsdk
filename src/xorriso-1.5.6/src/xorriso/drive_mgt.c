
/* xorriso - creates, loads, manipulates and burns ISO 9660 filesystem images.

   Copyright 2007-2023 Thomas Schmitt, <scdbackup@gmx.net>

   Provided under GPL version 2 or later.

   This file contains functions which operate on drives and media.
*/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "xorriso.h"
#include "xorriso_private.h"

#include "lib_mgt.h"
#include "drive_mgt.h"
#include "iso_img.h"
#include "sort_cmp.h"
#include "write_run.h"
#include "read_run.h"


static const char *un0(const char *text)
{
 if(text == NULL)
   return("");
 return(text);
}




int Xorriso_auto_driveadr(struct XorrisO *xorriso, char *adr, char *result,
                          int flag)
{
 int ret, is_known_mmc= 0, does_exist= 0;
 char *path_pt, *libburn_adr= NULL;
 char *abs_pt, *abs_adr= NULL;
 struct stat stbuf;

 Xorriso_alloc_meM(libburn_adr, char, BURN_DRIVE_ADR_LEN + SfileadrL);
 Xorriso_alloc_meM(abs_adr, char, SfileadrL);
 path_pt= adr;
 if(strncmp(adr, "stdio:", 6) == 0)
   path_pt= adr + 6;
 else if(strncmp(adr, "mmc:", 4) == 0)
   path_pt= adr + 4;


 /* <<< replace by Xorriso_normalize_img_path() ? */;

 if(path_pt[0] != '/') {
   abs_pt= getcwd(abs_adr, SfileadrL - 1);
   if(abs_pt == NULL) {
     Xorriso_msgs_submit(xorriso, 0,
              "Relative drive path given. Cannot determine working directory.",
               errno, "FAILURE", 0);
     {ret= -1; goto ex;}
   }
   ret= Sfile_add_to_path(abs_adr, path_pt, 0);
   if(ret <= 0)
     {ret= -1; goto ex;}
 }

 is_known_mmc= burn_drive_convert_fs_adr(path_pt, libburn_adr);
 does_exist= (stat(path_pt, &stbuf) != -1);
 Xorriso_process_msg_queues(xorriso,0);

 ret= Xorriso_is_in_patternlist(xorriso, xorriso->drive_whitelist, path_pt, 0);
 if(ret > 0)
   goto ok;
 ret= Xorriso_is_in_patternlist(xorriso, xorriso->drive_blacklist, path_pt, 0);
 if(ret < 0)
   goto ex;
 if(ret) {
   strcpy(xorriso->info_text, "Drive address ");
   Text_shellsafe(adr, xorriso->info_text, 1);
   strcat(xorriso->info_text,
          " rejected because: -drive_class 'banned' ");
   Text_shellsafe(Xorriso_get_pattern(xorriso, xorriso->drive_blacklist,
                                      ret - 1, 0),
                  xorriso->info_text, 1);
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
   {ret= 0; goto ex;}
 }
 /* if in greylist and not MMC and not stdio prefix: reject */
 if(is_known_mmc < 0)
   goto ex;
 if(adr == path_pt && !is_known_mmc) { /* no prefix, no MMC */
   ret= Xorriso_is_in_patternlist(xorriso, xorriso->drive_greylist, path_pt,0);
   if(ret < 0)
     goto ex;
   if(ret) {
     strcpy(xorriso->info_text, "Drive address ");
     Text_shellsafe(adr, xorriso->info_text, 1);
     strcat(xorriso->info_text, " rejected because: ");
     if(does_exist)
       strcat(xorriso->info_text, "not MMC");
     else
       strcat(xorriso->info_text, "not existing");
     strcat(xorriso->info_text, " and -drive_class 'caution' ");
     Text_shellsafe(Xorriso_get_pattern(xorriso,xorriso->drive_greylist,
                                        ret - 1, 0),
                    xorriso->info_text, 1);
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
     sprintf(xorriso->info_text,
             "If the address is a legitimate %s, prepend \"stdio:\"",
             does_exist ? "target" : "address for a new regular file");
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "HINT", 0);
     {ret= 0; goto ex;}
   }
 }
ok:;
 if(strncmp(adr, "mmc:", 4) == 0) {
   if(Sfile_str(result, path_pt, 0) <= 0)
     {ret= 0; goto ex;}
 } else if(adr == path_pt && is_known_mmc <= 0) {
   Sfile_str(result, "stdio:", 0);
   if(Sfile_str(result, adr, 1) <= 0)
     {ret= 0; goto ex;}
 } else {
   if(Sfile_str(result, adr, 0) <= 0)
     {ret= 0; goto ex;}
 }
 if(strncmp(result, "stdio:", 6)==0) {
   if(xorriso->ban_stdio_write) {
     strcpy(xorriso->info_text, "Drive address banned by -ban_stdio_write : ");
     Text_shellsafe(result, xorriso->info_text, 1);
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
     {ret= 0; goto ex;}
   }
 }
 ret= 1;
ex:;
 Xorriso_free_meM(libburn_adr);
 Xorriso_free_meM(abs_adr);
 return(ret);
}


static int Xorriso_grasp_loaded_aaip(struct XorrisO *xorriso, IsoImage *volset,
                                     int flag)
{
 int ret, change_pending_rec;
 IsoNode *root_node;
 size_t value_length;
 char *value= NULL;
 double num;
 struct FindjoB *job= NULL;
 struct stat dir_stbuf;

 /* To be re-instated at function end */ 
 change_pending_rec= xorriso->volset_change_pending;

 /* Look for isofs.st and put it into xorriso->isofs_st_in */
 root_node= (IsoNode *) iso_image_get_root(volset);
 ret= iso_node_lookup_attr(root_node, "isofs.st", &value_length, &value, 0);
 if(ret > 0) {
   if(value_length > 0) {
     sscanf(value, "%lf", &num);
     if(num > 0)
       xorriso->isofs_st_in= num;
   }
   free(value);
 }

 if(xorriso->do_hfsplus) {
   /* Bring isofs.hx to iso_hfsplus_xinfo_func,
      isofs.hb to IsoImage blessings
   */
   ret= Findjob_new(&job, "/", 0);
   if(ret<=0) {
     Xorriso_no_findjob(xorriso, "xorriso", 0);
     {ret= -1; goto ex;}
   }
   Findjob_set_action_target(job, 49, NULL, 0);
   ret= Xorriso_findi(xorriso, job, NULL, (off_t) 0, NULL, "/",
                      &dir_stbuf, 0, 0);
   if(ret <= 0)
     goto ex;
 }

 ret= 1;
ex:;
 xorriso->volset_change_pending= change_pending_rec;
 Findjob_destroy(&job, 0);
 return(ret);
}


/* @param flag: bit0= set read speed
                bit1= set write speed
*/
int Xorriso_set_speed(struct XorrisO *xorriso, struct burn_drive *drive,
                      int read_speed, int write_speed, int flag)
{
 int r_speed = 0, w_speed = 0, ret = 0, profile_no= 0;
 char profile_name[80];

 if((flag & 3) == 0)
   return(0);
 if(xorriso->read_speed == -2) {
   if(!(flag & 2))
     return(0);
 }
 if(flag & 1)
   r_speed= read_speed;
 if(flag & 2)
   w_speed= write_speed;

 ret= burn_disc_get_profile(drive, &profile_no, profile_name);
 if(ret <= 0)
   profile_no= 0;
 if((r_speed > 0 || w_speed > 0) && profile_no >= 0x10) {
   ret= burn_drive_set_speed_exact(drive, r_speed, w_speed);
   if(ret > 0)
     goto ex;
 }
 burn_drive_set_speed(drive, r_speed, w_speed);
 ret= 2;
ex:;
 Xorriso_process_msg_queues(xorriso,0);
 return(ret);
}


/* @param flag bit0= no info message "Loading ISO image tree"
*/
int Xorriso_make_read_options(struct XorrisO *xorriso,
                              struct burn_drive *drive,
                              struct isoburn_read_opts **ropts,
                              int flag)
{
 int ret, ext, load_lba;
 enum burn_disc_status state;

 /* fill read opts */ 
 ret= isoburn_ropt_new(ropts, 0);
 if(ret<=0)
   goto ex;

 ret= Xorriso_set_data_cache(xorriso, *ropts, xorriso->cache_num_tiles,
                             xorriso->cache_tile_blocks,
                             xorriso->cache_default);
 if(ret<=0)
   goto ex;

 ext= isoburn_ropt_noiso1999;
 if(xorriso->read_fs & 1)
   ext|= isoburn_ropt_norock;
 if(xorriso->read_fs & 2)
   ext|= isoburn_ropt_nojoliet;
 if((xorriso->ino_behavior & (1 | 2)) && !(xorriso->do_aaip & (1 | 4 | 32))
    && !(xorriso->do_md5 & 1) && !(xorriso->do_hfsplus)) 
   ext|= isoburn_ropt_noaaip;
 if(!(xorriso->do_aaip & 1)) 
   ext|= isoburn_ropt_noacl; 
 if(!(xorriso->do_aaip & 4))
   ext|= isoburn_ropt_noea; 
 if(xorriso->ino_behavior & 1)
   ext|= isoburn_ropt_noino;
 if(!(xorriso->do_md5 & 1))
   ext|= isoburn_ropt_nomd5;
 if(xorriso->do_md5 & 32)
   ext|= isoburn_ropt_nomd5tag;
 if(xorriso->ecma119_map == 0)
   ext|= isoburn_ropt_map_unmapped;
 else if(xorriso->ecma119_map == 2)
   ext|= isoburn_ropt_map_uppercase;
 else if(xorriso->ecma119_map == 3)
   ext|= isoburn_ropt_map_lowercase;
 else
   ext|= isoburn_ropt_map_stripped;
 if(xorriso->joliet_map == 0)
   ext|= isoburn_ropt_joliet_unmapped;
 else
   ext|= isoburn_ropt_joliet_stripped;

 isoburn_ropt_set_extensions(*ropts, ext);

 isoburn_ropt_set_default_perms(*ropts, (uid_t) 0, (gid_t) 0, (mode_t) 0555);
 isoburn_ropt_set_input_charset(*ropts, xorriso->in_charset);
 isoburn_ropt_set_auto_incharset(*ropts, !!(xorriso->do_aaip & 512));
 isoburn_ropt_set_displacement(*ropts, xorriso->displacement,
                                       xorriso->displacement_sign);
 isoburn_ropt_set_truncate_mode(*ropts, 1, xorriso->file_name_limit);
 
 Xorriso_set_image_severities(xorriso, 1); /* No DEBUG messages */

 /* <<< Trying to work around too much tolerance on bad image trees.
        Better would be a chance to instruct libisofs what to do in
        case of image read errors. There is a risk to mistake other SORRYs.
 */
 if(xorriso->img_read_error_mode>0)
   iso_set_abort_severity("SORRY");

 state= isoburn_disc_get_status(drive);
 if(state != BURN_DISC_BLANK) {
   ret= isoburn_disc_get_msc1(drive, &load_lba);
   if(ret > 0 && !(flag & 1)) {
     sprintf(xorriso->info_text,
             "Loading ISO image tree from LBA %d", load_lba);
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE", 0); 
   }
   ret= Xorriso_assert_volid(xorriso, load_lba, 0);
   if(ret <= 0)
     goto ex;
 }
 Xorriso_set_speed(xorriso, drive, xorriso->read_speed, 0, 1);

 ret= 1;
ex:
 return(ret);
}


/* @param flag bit0= acquire as isoburn input drive
               bit1= acquire as libburn output drive (as isoburn drive if bit0)
               bit2= regard overwritable media as blank
               bit3= if the drive is a regular disk file: truncate it to
                     the write start address
               bit5= do not print toc
               bit6= do not calm down drive after acquiring it
               bit7= re-assess rather than acquire:
                     Do not give up drives,
                     use isoburn_drive_aquire() with re-assessment bit
   @return <=0 failure , 1= ok
           2=success, but not writeable with bit1
           3=success, but not blank and not ISO with bit0
*/
int Xorriso_aquire_drive(struct XorrisO *xorriso, char *adr, char *show_adr,
                         int flag)
{
 int ret, hret, not_writeable= 0, has_what, aquire_flag;
 int lba, track, session, params_flag, adr_mode, read_ret, start_lba;
 int truncate_mode;
 uint32_t size, offst;
 struct burn_drive_info *dinfo= NULL, *out_dinfo= NULL, *in_dinfo= NULL;
 struct burn_drive *drive= NULL, *out_drive= NULL, *in_drive= NULL;
 enum burn_disc_status state;
 IsoImage *volset = NULL;
 struct isoburn_read_opts *ropts= NULL;
 char *libburn_adr= NULL, *off_adr= NULL, *boot_fate, *sev;
 char volid[33], *adr_data= NULL, *adr_pt;

 Xorriso_alloc_meM(libburn_adr, char, SfileadrL);
 Xorriso_alloc_meM(off_adr, char, SfileadrL);
 Xorriso_alloc_meM(adr_data, char, 163);

 if(show_adr == NULL) {
   show_adr= adr;
   ret= burn_drive_convert_fs_adr(adr, off_adr);
   if(ret > 0)
     adr= off_adr;
 }

 if((flag&3)==0) { 
   sprintf(xorriso->info_text, 
         "XORRISOBURN program error : Xorriso_aquire_drive bit0+bit1 not set");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FATAL", 0);
   {ret= -1; goto ex;}
 }
 if(!(flag & 128)) {
   ret= Xorriso_give_up_drive(xorriso, (flag&3)|8);
   if(ret<=0)
     goto ex;
 }
 if(flag & 1) 
   xorriso->isofs_st_out= time(0) - 1;

 ret= Xorriso_auto_driveadr(xorriso, adr, libburn_adr, 0);
 if(ret <= 0)
   goto ex;
 if(strcmp(libburn_adr,"stdio:/dev/fd/1")==0) {
   if(xorriso->dev_fd_1<0) {
     sprintf(xorriso->info_text,
     "-*dev \"stdio:/dev/fd/1\" was not a start argument. Cannot use it now.");
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
     {ret= 0; goto ex;}
   } else {
     sprintf(libburn_adr, "stdio:/dev/fd/%d", xorriso->dev_fd_1);
   }
 }

 if(flag & 128) {
   if(flag & 1)
     Xorriso_get_drive_handles(xorriso, &in_dinfo, &in_drive, "", 16);
   if(flag & 2)
     Xorriso_get_drive_handles(xorriso, &out_dinfo, &out_drive, "", 2 | 16);
   if(in_dinfo != NULL && (out_dinfo == NULL || out_dinfo == in_dinfo)) {
     dinfo= in_dinfo;
     if(flag & 2) {
       xorriso->outdev_is_exclusive= xorriso->indev_is_exclusive;
       xorriso->outdev_access= xorriso->indev_access;
     }
   } else if(out_dinfo != NULL && in_dinfo == NULL) {
     dinfo= out_dinfo;
     if(flag & 1) {
       xorriso->indev_is_exclusive= xorriso->outdev_is_exclusive;
       xorriso->indev_access= xorriso->outdev_access;
     }
   } else if(out_dinfo != NULL || in_dinfo != NULL) {
     sprintf(xorriso->info_text,
             "Two different drives shall be re-assed in one call");
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FATAL", 0);
     {ret= 0; goto ex;}
   } else {
     sprintf(xorriso->info_text, "No drive acquired on re-assessment");
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FATAL", 0);
     {ret= 0; goto ex;}
   }
 } else if((flag&3)==1 && xorriso->out_drive_handle!=NULL) {
   ret= Xorriso_get_drive_handles(xorriso, &out_dinfo, &out_drive,
                         "on attempt to compare new indev with outdev", 2);
   if(ret<=0)
     goto ex;
   ret= burn_drive_equals_adr(out_drive, libburn_adr, 1);
   if(ret==1) {
     dinfo= out_dinfo;
     xorriso->indev_is_exclusive= xorriso->outdev_is_exclusive;
     xorriso->indev_access= xorriso->outdev_access;
   }
 } else if((flag&3)==2 && xorriso->in_drive_handle!=NULL) {
   ret= Xorriso_get_drive_handles(xorriso, &in_dinfo, &in_drive,
                         "on attempt to compare new outdev with indev", 0);
   if(ret<=0)
     goto ex;
   ret= burn_drive_equals_adr(in_drive, libburn_adr, 1);
   if(ret==1) {
     dinfo= in_dinfo;
     xorriso->outdev_is_exclusive= xorriso->indev_is_exclusive;
     xorriso->outdev_access= xorriso->indev_access;
   }
 }

 if(dinfo == NULL || (flag & 128)) {
   aquire_flag= 1 | ((flag&(8|4))>>1) | ((xorriso->toc_emulation_flag & 3)<<3);
   if(xorriso->toc_emulation_flag & 4)
     aquire_flag|= 128;
   if(xorriso->toc_emulation_flag & 8)
     aquire_flag|= 512;
   if(!(xorriso->do_aaip & 1))
     aquire_flag|= 32;
   if((xorriso->ino_behavior & (1 | 2)) && !(xorriso->do_aaip & (4 | 32))) {
     aquire_flag|= 64;
   } else if(xorriso->do_aaip & 1024) {
     aquire_flag|= 1024;
   }
   if(flag & 128)
     aquire_flag|= 256;
   burn_preset_device_open(xorriso->drives_exclusive |
                           (xorriso->linux_scsi_dev_family << 2), 0, 0);
   burn_allow_drive_role_4(1 | (xorriso->early_stdio_test & 14));
   ret= isoburn_drive_aquire(&dinfo, libburn_adr, aquire_flag);
   burn_preset_device_open(1 | (xorriso->linux_scsi_dev_family << 2), 0, 0);
   Xorriso_process_msg_queues(xorriso,0);
   if(ret<=0) {
     if(flag & 128)
       sprintf(xorriso->info_text,"Cannot re-assess drive '%s'", adr); 
     else
       sprintf(xorriso->info_text,"Cannot acquire drive '%s'", adr); 
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
     ret= 0; goto ex;
   }
   xorriso->use_immed_bit_default= burn_drive_get_immed(dinfo[0].drive) > 0 ?
                                   1 : -1;
   if(xorriso->use_immed_bit != 0)
     burn_drive_set_immed(dinfo[0].drive, xorriso->use_immed_bit > 0);
   state= isoburn_disc_get_status(dinfo[0].drive);
   ret= isoburn_get_img_partition_offset(dinfo[0].drive, &offst);
   if((state == BURN_DISC_APPENDABLE || state == BURN_DISC_FULL) && ret == 1) {
     sprintf(xorriso->info_text,
             "ISO image bears MBR with  -boot_image any partition_offset=%lu",
             (unsigned long) offst);
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE", 0);
   }

   if(flag&1)
     if(xorriso->image_start_mode&(1u<<31)) /* used up setting */
       xorriso->image_start_mode= 0; /* no need to perform auto setting */
   if(flag & 1) {
     xorriso->indev_is_exclusive= xorriso->drives_exclusive;
     xorriso->indev_access= xorriso->drives_access;
   }
   if(flag & 2) {
     xorriso->outdev_is_exclusive= xorriso->drives_exclusive;
     xorriso->outdev_access= xorriso->drives_access;
   }
 }
 drive= dinfo[0].drive;
 volset= isoburn_get_attached_image(drive);
 if(volset != NULL) {
   ret= iso_image_set_truncate_mode(volset, 1, xorriso->file_name_limit);
   iso_image_unref(volset);
   volset= NULL;
   Xorriso_process_msg_queues(xorriso,0);
   if(ret < 0)
     {ret= 0; goto ex;}
 }
 state= isoburn_disc_get_status(drive);
 Xorriso_process_msg_queues(xorriso,0);
 if(flag&1) {
   if(xorriso->image_start_mode&(1u<<31)) /* used up setting */
     xorriso->image_start_mode&= ~0xffff; /* perform auto setting */
   if((xorriso->image_start_mode&(1<<30))) { /* if enabled at all */
     adr_pt= xorriso->image_start_value;
     adr_mode= xorriso->image_start_mode & 0xffff;
     if(adr_mode == 4 && strlen(adr_pt) <= 80) {
       /* Convert volid search expression into lba */
       params_flag= 0;
       ret= Xorriso__bourne_to_reg(xorriso->image_start_value, adr_data, 0);
       if(ret == 1)
         params_flag|= 4;
       ret= isoburn_get_mount_params(drive, 4, adr_data, &lba, &track,
                                     &session, volid, params_flag);
       Xorriso_process_msg_queues(xorriso,0);
       if(ret <= 0)
         goto ex;
       if(session <= 0 || track <= 0 || ret == 2) {
         Xorriso_msgs_submit(xorriso, 0,
                "-load : Given address does not point to an ISO 9660 session",
                0, "FAILURE", 0);
         ret= 0; goto ex;
       }
       sprintf(volid, "%d", lba);
       adr_pt= volid;
       adr_mode= 3;
     }
     ret= isoburn_set_msc1(drive, adr_mode, adr_pt,
                           !!(xorriso->image_start_mode & (1<<16)));
     if(ret<=0)
       goto ex;
     if(xorriso->image_start_mode&(1u<<31))
       xorriso->image_start_mode= 0; /* disable msc1 setting completely */
     else
       xorriso->image_start_mode|= (1u<<31); /* mark as used up */
   }
 }
 if(flag&1) {
   volset= isoburn_get_attached_image(drive);
   if(volset != NULL) { /* The image object is already created */
     iso_image_unref(volset);
     volset= NULL;
   }
 }

 if(flag&2) {
   xorriso->out_drive_handle= dinfo;
   if(Sfile_str(xorriso->outdev, show_adr, 0)<=0)
     {ret= -1; goto ex;}
   ret= burn_drive_convert_fs_adr(adr, xorriso->outdev_off_adr);
   if(ret <= 0)
     xorriso->outdev_off_adr[0]= 0;
   if(state != BURN_DISC_BLANK && state != BURN_DISC_APPENDABLE) {
     sprintf(xorriso->info_text, "Disc status unsuitable for writing");
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE", 0);
     not_writeable= 1;
   }
 }
 if(flag&1) {
   xorriso->in_drive_handle= dinfo;
   if(Sfile_str(xorriso->indev, show_adr, 0)<=0)
     {ret= -1; goto ex;}
   ret= burn_drive_convert_fs_adr(adr, xorriso->indev_off_adr);
   if(ret <= 0)
     xorriso->indev_off_adr[0]= 0;
 } else if(flag&2) {
   if(xorriso->in_volset_handle==NULL) {
     /* No volume loaded: create empty one */
     ret= Xorriso_create_empty_iso(xorriso, 0);
     if(ret<=0)
       goto ex;
   } else {
     iso_image_ref((IsoImage *) xorriso->in_volset_handle);
     start_lba= -1;
     ret= Xorriso_get_drive_handles(xorriso, &in_dinfo, &in_drive,
                        "on attempt to attach ISO image object to outdev", 16);
     if(ret > 0)
       start_lba= isoburn_get_attached_start_lba(in_drive);
     ret= isoburn_attach_image(drive, (IsoImage *) xorriso->in_volset_handle);
     if(ret<=0) {
       sprintf(xorriso->info_text,
               "Failed to attach ISO image object to outdev");
       Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FATAL", 0);
       {ret= -1; goto ex;}
     }
     if(start_lba >= 0)
       isoburn_attach_start_lba(drive, lba, 0);
   }
   if(!(flag&32))
     Xorriso_toc(xorriso, 1 | 2 | 8);
   {ret= 1+not_writeable; goto ex;}
 }

 if(xorriso->in_volset_handle!=NULL)
   iso_image_unref((IsoImage *) xorriso->in_volset_handle);
 xorriso->in_volset_handle= NULL;
 Sectorbitmap_destroy(&(xorriso->in_sector_map), 0);
 Xorriso_destroy_hln_array(xorriso, 0);
 Xorriso_destroy_di_array(xorriso, 0);
 xorriso->boot_count= 0;
 xorriso->system_area_clear_loaded=  
                    (strcmp(xorriso->system_area_disk_path, "/dev/zero") == 0);

 /* check for invalid state */
 if(state != BURN_DISC_BLANK && state != BURN_DISC_APPENDABLE &&
    state != BURN_DISC_FULL) {
   sprintf(xorriso->info_text,
           "Disc status not blank and unsuitable for reading");
   sev= "FAILURE";
   if(xorriso->img_read_error_mode==2)
     sev= "FATAL";
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, sev, 0);
   Xorriso_give_up_drive(xorriso, 1|((flag&32)>>2));
   ret= 3; goto ex;
 }

 ret= Xorriso_make_read_options(xorriso, drive, &ropts, 0);
 if(ret <= 0)
   goto ex;
 Xorriso_pacifier_reset(xorriso, 0);
 isoburn_set_read_pacifier(drive, Xorriso__read_pacifier, (void *) xorriso);

 read_ret= ret= isoburn_read_image(drive, ropts, &volset);

 /* <<< Resetting to normal thresholds, after Xorriso_make_read_options */
 if(xorriso->img_read_error_mode>0)
   Xorriso_set_abort_severity(xorriso, 0);

 if(ret<=0) {
   Xorriso_process_msg_queues(xorriso,0);
   Xorriso_set_image_severities(xorriso, 0);
   Xorriso_give_up_drive(xorriso, 1|((flag&32)>>2));
   sprintf(xorriso->info_text,"Cannot read ISO image tree");
   sev= "FAILURE";
   if(xorriso->img_read_error_mode==2)
     sev= "FATAL";
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, sev, 0);
   if(read_ret == (int) ISO_SB_TREE_CORRUPTED && (xorriso->do_md5 & 1)) {
     Xorriso_msgs_submit(xorriso, 0,
"This might be false MD5 alarm if an add-on session was written by growisofs.",
        0, "HINT", 0);
     Xorriso_msgs_submit(xorriso, 0,
      "In this case you get an ISO image tree by option -md5 'load_check_off'",
        0, "HINT", 0);
   } else if(xorriso->img_read_error_mode!=0) {
     Xorriso_msgs_submit(xorriso, 0, "You might get a partial or altered ISO image tree by option -error_behavior 'image_loading' 'best_effort' if -abort_on is set to be tolerant enough.",
                         0, "HINT", 0);
   }


   ret= 3; goto ex;
 }
 Xorriso_pacifier_callback(xorriso, "nodes read", xorriso->pacifier_count, 0,
                           "", 1); /* report end count */
 xorriso->in_volset_handle= (void *) volset;
 xorriso->in_sector_map= NULL;
 Xorriso_set_image_severities(xorriso, 0);

 /* Might have changed due to isofs.nt */
 iso_image_get_truncate_mode(volset, &truncate_mode,
                             &(xorriso->file_name_limit));
 Xorriso_process_msg_queues(xorriso,0);

 Xorriso_update_volid(xorriso, 0);
 strncpy(xorriso->application_id,
         un0(iso_image_get_application_id(volset)), 128);
 xorriso->application_id[128]= 0;
 strncpy(xorriso->publisher, un0(iso_image_get_publisher_id(volset)), 128);
 xorriso->publisher[128]= 0;
 strncpy(xorriso->system_id, un0(iso_image_get_system_id(volset)), 32);
 xorriso->system_id[32]= 0;
 strncpy(xorriso->volset_id, un0(iso_image_get_volset_id(volset)), 128);
 xorriso->volset_id[128]= 0;
 strncpy(xorriso->copyright_file,
         un0(iso_image_get_copyright_file_id(volset)), 37);
 xorriso->copyright_file[37]= 0;
 strncpy(xorriso->biblio_file, un0(iso_image_get_biblio_file_id(volset)), 37);
 xorriso->biblio_file[37]= 0;
 strncpy(xorriso->abstract_file,
         un0(iso_image_get_abstract_file_id(volset)), 37);
 xorriso->abstract_file[37]= 0;

 if(xorriso->out_drive_handle != NULL &&
    xorriso->out_drive_handle != xorriso->in_drive_handle) {
   start_lba= -1;
   ret= Xorriso_get_drive_handles(xorriso, &in_dinfo, &in_drive,
                        "on attempt to attach ISO image volset to outdev", 16);
   if(ret > 0)
     start_lba= isoburn_get_attached_start_lba(in_drive);
   ret= Xorriso_get_drive_handles(xorriso, &out_dinfo, &out_drive,
                         "on attempt to attach ISO image volset to outdev", 2);
   if(ret<=0)
     goto ex;
   iso_image_ref((IsoImage *) xorriso->in_volset_handle);
   isoburn_attach_image(out_drive, xorriso->in_volset_handle);
   if(start_lba >= 0)
     isoburn_attach_start_lba(out_drive, ret, 0);
 }
 Xorriso_process_msg_queues(xorriso,0);
 isoburn_ropt_get_size_what(ropts, &size, &has_what);
 xorriso->isofs_size= size;
 xorriso->isofs_has_what= has_what;
 isoburn_ropt_get_tree_loaded(ropts, &(xorriso->tree_loaded),
                              &(xorriso->rr_loaded));

 if(has_what & isoburn_ropt_has_el_torito) {
   if(xorriso->boot_image_bin_path[0])
     boot_fate= "replaced by new boot image";
   else if(xorriso->patch_isolinux_image & 1)
     boot_fate= "patched at boot info table";
   else if(xorriso->keep_boot_image)
     boot_fate= "kept unchanged";
   else
     boot_fate= "discarded";
   sprintf(xorriso->info_text,
         "Detected El-Torito boot information which currently is set to be %s",
         boot_fate);
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE", 0); 
   Xorriso_record_boot_info(xorriso, 0);
 }

 if(flag & 1) {
   ret= Xorriso_grasp_loaded_aaip(xorriso, volset, 0);
   if(ret <= 0)
     goto ex;
 }

 if(!(flag&32)) {
   Xorriso_toc(xorriso, 1 | 8);
   if(xorriso->loaded_volid[0] != 0 &&
      (state == BURN_DISC_APPENDABLE || state == BURN_DISC_FULL)) {
       sprintf(xorriso->info_text,"Volume id    : '%s'\n",
               xorriso->loaded_volid);
       Xorriso_info(xorriso, 0);
   }
   if(strcmp(xorriso->loaded_volid, xorriso->volid) != 0 &&
      !xorriso->volid_default) {
     sprintf(xorriso->info_text, "New volume id: '%s'\n", xorriso->volid);
     Xorriso_info(xorriso, 0);
   }
 }

 ret= 1+not_writeable;
ex:
 Xorriso_process_msg_queues(xorriso,0);
 if(ret<=0) {
   hret= Xorriso_give_up_drive(xorriso, (flag&3)|((flag&32)>>2));
   if(hret<ret)
     ret= hret;
 } else {
   if(drive != NULL && (xorriso->do_calm_drive & 1) && !(flag & 64))
     burn_drive_snooze(drive, 0); /* No need to make noise from start */
 }
 if(ropts!=NULL)
   isoburn_ropt_destroy(&ropts, 0);
 Xorriso_free_meM(off_adr);
 Xorriso_free_meM(libburn_adr);
 Xorriso_free_meM(adr_data);
 return(ret);
}


/* @param flag bit0=input drive
               bit1=output drive
               bit2=eject
               bit3=no info message or toc
*/
int Xorriso_give_up_drive(struct XorrisO *xorriso, int flag)
{
 int in_is_out_too, ret, do_eject;
 struct burn_drive_info *dinfo;
 struct burn_drive *drive;
 
 in_is_out_too= (xorriso->in_drive_handle == xorriso->out_drive_handle);
 if((flag&4) && in_is_out_too && (flag&(1|2))) {
   if((flag&3)!=3) {
     sprintf(xorriso->info_text,"Giving up for -eject whole -dev ");
     Text_shellsafe(xorriso->indev, xorriso->info_text, 1);
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE", 0);
   }
   flag|= 3; /* give up in/out drive to eject it */
 }
   
 if((flag&1) && xorriso->in_drive_handle != NULL) {
   Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                             "on attempt to give up drive", 0);

   if(!in_is_out_too) {
     do_eject= !!(flag&4);
     if((flag & 4) && xorriso->indev_access == 0) {
       sprintf(xorriso->info_text,
             "Will not eject medium in readonly acquired input drive.");
       Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "WARNING", 0);
       do_eject= 0;
     }
     if(drive!=NULL)
       isoburn_drive_release(drive, do_eject);
     if(dinfo!=NULL)
       burn_drive_info_free(dinfo);
   }
   xorriso->in_drive_handle= NULL;
   xorriso->indev[0]= 0;

   if(xorriso->in_volset_handle!=NULL)
     iso_image_unref((IsoImage *) xorriso->in_volset_handle);
   xorriso->in_volset_handle= NULL;
   Sectorbitmap_destroy(&(xorriso->in_sector_map), 0);
   Xorriso_destroy_di_array(xorriso, 0);
   Xorriso_destroy_hln_array(xorriso, 0);
   xorriso->loaded_volid[0]= 0;
   xorriso->isofs_st_out= time(0) - 1;
   xorriso->isofs_st_in= 0;
   xorriso->volset_change_pending= 0;
   xorriso->no_volset_present= 0; 
   xorriso->loaded_boot_bin_lba= 0;
   xorriso->loaded_boot_cat_path[0]= 0;
   xorriso->boot_count= 0;
   in_is_out_too= 0;
 }
 if((flag&2) && xorriso->out_drive_handle!=NULL) {
   do_eject= !!(flag&4);
   if((flag & 4) && xorriso->outdev_access == 0) {
     sprintf(xorriso->info_text,
             "Will not eject medium in readonly acquired drive.");
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "WARNING", 0);
     do_eject= 0;
   }
   ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                                  "on attempt to give up drive", 2);
   if(ret >= 0 && !in_is_out_too) {
     if(drive!=NULL)
       isoburn_drive_release(drive, do_eject);
     if(dinfo!=NULL)
       burn_drive_info_free(dinfo);
   }
   xorriso->out_drive_handle= NULL;
   xorriso->outdev[0]= 0;
   xorriso->outdev_off_adr[0]= 0;
 } else if((flag&1) && xorriso->out_drive_handle!=NULL) {
   ret= Xorriso_create_empty_iso(xorriso, 0);
   if(ret<=0)
     return(ret);
   if(!(flag&8)) {
     sprintf(xorriso->info_text,
             "Only the output drive remains. Created empty ISO image.\n");
     Xorriso_info(xorriso, 0);
     Xorriso_toc(xorriso, 1 | 2 | 8);
   }
 }
 Xorriso_process_msg_queues(xorriso,0);
 return(1);
}


int Xorriso_may_burn(struct XorrisO *xorriso, int flag)
{

 if(xorriso->outdev_access == 1)
   return(1);
 sprintf(xorriso->info_text, "The output drive was acquired readonly.");
 Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
 sprintf(xorriso->info_text, "Possible remedy: -drive_access \"exclusive:unrestricted\".");
 strcat(xorriso->info_text," Then give up and re-acquire the drive.");
 Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "HINT", 0);
 if(!xorriso->outdev_is_exclusive) {
   sprintf(xorriso->info_text, "If you insist in -drive_access \"shared:unrestricted\", first read man xorriso about the risks.");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "HINT", 0);
 }
 return(0);
}


/* @param flag bit1=report about output drive rather than input drive
               bit2=do not try to read ISO heads
*/
int Xorriso_toc_to_string(struct XorrisO *xorriso, char **toc_text, int flag)
{
 int ret, stack_handle, toc_ret, l;
 struct Xorriso_lsT *results= NULL, *infos= NULL, *lpt;

 *toc_text= NULL;
 ret= Xorriso_push_outlists(xorriso, &stack_handle, 1);
 if(ret <= 0)
   goto ex;
 toc_ret= Xorriso_toc(xorriso, flag & (2 | 4));
 ret= Xorriso_pull_outlists(xorriso, stack_handle, &results, &infos, 0);
 if(ret <= 0)
   goto ex;
 if(toc_ret <= 0)
   {ret= toc_ret; goto ex;}
 l= 0;
 for(lpt= results; lpt != NULL; lpt= Xorriso_lst_get_next(lpt, 0))
   l+= strlen(Xorriso_lst_get_text(lpt, 0));
 *toc_text= calloc(l + 1, 1);
 l= 0;
 for(lpt= results; lpt != NULL; lpt= Xorriso_lst_get_next(lpt, 0)) {
   strcpy((*toc_text) + l, Xorriso_lst_get_text(lpt, 0));
   l+= strlen(Xorriso_lst_get_text(lpt, 0));
 }
ex:;
 Xorriso_lst_destroy_all(&results, 0);
 Xorriso_lst_destroy_all(&infos, 0);
 return(ret);
}


/* @param flag bit0+1= what to give up and/or re-assess in what role
                       0=give up outdev
                       1=give up indev if not outdev, re-assess outdev as indev
                       2=re-assess outdev as outdev
                       3=give up indev if not outdev, re-assess outdev as dev
*/
int Xorriso_reaquire_outdev(struct XorrisO *xorriso, int flag)
{
 int ret, aq_flag;
 char *drive_name= NULL, *off_name= NULL;

 Xorriso_alloc_meM(drive_name, char, SfileadrL);
 Xorriso_alloc_meM(off_name, char, SfileadrL);
 aq_flag= flag&3;
 strcpy(drive_name, xorriso->outdev);
 if(xorriso->outdev_off_adr[0])
   strcpy(off_name, xorriso->outdev_off_adr);
 else
   strcpy(off_name, drive_name);

 if(aq_flag == 0) {
   Xorriso_give_up_drive(xorriso, 2);
   sprintf(xorriso->info_text,"Gave up -outdev ");
   Text_shellsafe(xorriso->indev, xorriso->info_text, 1);
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE", 0);
   {ret= 1; goto ex;}
 }

 /* Only give up indev, and only if it is not outdev */;
 if(xorriso->in_drive_handle != xorriso->out_drive_handle &&
    xorriso->in_drive_handle != NULL && (aq_flag & 1))
   Xorriso_give_up_drive(xorriso, 1 | 8);
 sprintf(xorriso->info_text,"Re-assessing -outdev ");
 Text_shellsafe(drive_name, xorriso->info_text, 1);
 if(strcmp(drive_name, off_name) != 0) {
   strcat(xorriso->info_text, "  (");
   Text_shellsafe(off_name, xorriso->info_text, 1);
   strcat(xorriso->info_text, ")");
 }
 Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE", 0);

 /* Re-assess outdev */
 ret= Xorriso_aquire_drive(xorriso, off_name, drive_name, aq_flag | 128); 
 if(ret<=0) {
   sprintf(xorriso->info_text,"Could not re-assess -outdev ");
   Text_shellsafe(drive_name, xorriso->info_text, 1);
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
   goto ex;
 }

 ret= 1;
ex:;
 Xorriso_free_meM(drive_name);
 Xorriso_free_meM(off_name);
 return(ret);
}


/* @param flag
               bit3=report to info channel (else to result channel)
*/
int Xorriso_toc_line(struct XorrisO *xorriso, int flag)
{
 if(!(flag & 8)) {
   Xorriso_result(xorriso,0);
   return(1);
 }
 strcpy(xorriso->info_text, xorriso->result_line);
 Xorriso_info(xorriso, 0);
 return(1);
}


/* @param flag
               bit1=report about output drive
               bit3=report to info channel (else to result channel)
               bit4=do no report failure if no drive acquired
*/
int Xorriso_media_product(struct XorrisO *xorriso, int flag)
{
 int ret, profile_no;
 struct burn_drive_info *dinfo;
 struct burn_drive *drive;
 char *product_id= NULL, *media_code1= NULL, *media_code2= NULL;
 char *book_type= NULL, *manuf= NULL, profile_name[80], *respt;

 respt= xorriso->result_line;

 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                                "on attempt to print media product info",
                                 flag & (2 | 16));
 if(ret <= 0)
   return(ret);
 ret= burn_disc_get_media_id(drive, &product_id, &media_code1, &media_code2,
                             &book_type, 0);
 if(ret > 0) {
   ret= burn_disc_get_profile(drive, &profile_no, profile_name);
   if(ret <= 0)
     return(ret);
   sprintf(respt, "Media product: %s , ", product_id);
   manuf= burn_guess_manufacturer(profile_no, media_code1, media_code2, 0);
   if(manuf != NULL) {
     if(strncmp(manuf, "Unknown ", 8) == 0)
       sprintf(respt + strlen(respt), "(not found in manufacturer list)\n");
     else
       sprintf(respt + strlen(respt), "%s\n", manuf);
   } else
     sprintf(respt + strlen(respt), "(error during manufacturer lookup)\n");
   free(product_id);
   free(media_code1);
   free(media_code2);
   if(book_type != NULL)
     free(book_type);
   if(manuf != NULL)
     free(manuf);
   Xorriso_toc_line(xorriso, flag & 8);
 }
 Xorriso_process_msg_queues(xorriso,0);

 return(1);
}


/* @param flag bit0=short report form
               bit1=report about output drive
               bit2=do not try to read ISO heads
               bit3=report to info channel (else to result channel)
               bit4=do no report failure if no drive acquired
               bit5=only report "Drive current" and "Drive type"
               bit6=report "Media product" with bit0
               bit7=only report "Drive current"
*/
int Xorriso_toc(struct XorrisO *xorriso, int flag)
{
 int num_sessions= 0, num_tracks= 0, lba= 0, nwa= -1, ret;
 int track_count= 0, session_no, track_no, profile_no= -1, track_size;
 int session_size, first_track_start= 0;
 int num_session_data, num_session_other;
 int num_data= 0, other_data= 0, is_data= 0;
 int is_inout_drive= 0, drive_role, status, num_formats, emul_lba;
 int not_recognizable= 0, start_lba, end_lba;
 int sessions_seen, open_sessions= 0, have_real_open_session= 0;
 char profile_name[80],*respt,*devadr, *typetext= "";
 struct burn_toc_entry toc_entry;
 struct burn_drive_info *dinfo;
 struct burn_multi_caps *caps= NULL;
 struct burn_drive *drive;
 enum burn_disc_status s;
 char mem_text[80], *num_free_text, *num_data_text;
 off_t start_byte= 0, num_free= 0, size;
 unsigned dummy;
 struct isoburn_toc_disc *disc= NULL;
 struct isoburn_toc_session **sessions;
 struct isoburn_toc_track **tracks;
 int image_blocks= 0;
 char volume_id[33];
 struct burn_toc_entry next_toc_entry;
 int disk_category, part_version, num_layers, num_blocks;
 char *book_name;
 int num_data_from_format= 0;
 char *sno = NULL;
 int sno_len, i, is_bdr_pow= 0, overburn_blocks= 0;

 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                                "on attempt to print Table Of Content",
                                 flag & (2 | 16));
 if(ret<=0)
   {ret= 0; goto ex;}

 respt= xorriso->result_line;

 if(strcmp(xorriso->indev, xorriso->outdev)==0)
   is_inout_drive= 1;
 if(flag&2)
   devadr= xorriso->outdev;
 else
   devadr= xorriso->indev;
 sprintf(respt, "Drive current: %s '%s'\n",
         (is_inout_drive ? "-dev" : (flag&2 ? "-outdev" : "-indev")),
         devadr);
 Xorriso_toc_line(xorriso, flag & 8);
 if(flag & 128)
   {ret= 1; goto ex;}
 /* Report -drive_access if non-default or if long form */
 respt[0]= 0;
 if(flag & 2) {
   if(xorriso->outdev_is_exclusive == 0 || xorriso->outdev_access == 0 ||
      !(flag & 33)) {
     sprintf(respt, "Drive access : %s:%s\n",
             xorriso->outdev_is_exclusive ? "exclusive" : "shared",
             xorriso->outdev_access == 0 ? "readonly" : "unrestricted");
     }
 } else {
   if(xorriso->indev_is_exclusive == 0 || xorriso->indev_access == 0 ||
      !(flag & 33)) {
     sprintf(respt, "Drive access : %s:%s\n",
             xorriso->indev_is_exclusive ? "exclusive" : "shared",
             xorriso->indev_access == 0 ? "readonly" : "unrestricted");
   }
 }
 if(respt[0])
   Xorriso_toc_line(xorriso, flag & 8);
 sprintf(respt, "Drive type   : vendor '%s' product '%s' revision '%s'\n",
        dinfo[0].vendor, dinfo[0].product, dinfo[0].revision);
 if((flag & 32) || !(flag & 1))
   Xorriso_toc_line(xorriso, flag & 8);
 if(flag & 32)
   {ret= 1; goto ex;}

 if(!(flag & 1)) {
   burn_drive_get_serial_no(drive, &sno, &sno_len);
   if(sno_len > 0) {
     sprintf(respt, "Drive id     : '%s'\n", sno);
     Xorriso_toc_line(xorriso, flag & 8);
   }
   if(sno != NULL)
     free(sno);
   sno= NULL;
 }
 
 ret= burn_disc_get_profile(drive, &profile_no, profile_name);
 s= isoburn_disc_get_status(drive);
 if(profile_no == 0x0002 && s == BURN_DISC_EMPTY)
   profile_no= 0;
 is_bdr_pow= burn_drive_get_bd_r_pow(drive);
 sprintf(respt, "Media current: ");
 drive_role= burn_drive_get_drive_role(drive);
 if (profile_no > 0 && ret > 0) {
   if (profile_name[0])
     sprintf(respt+strlen(respt), "%s", profile_name);
   else
     sprintf(respt+strlen(respt), "%4.4Xh", profile_no);
   if(is_bdr_pow)
     sprintf(respt+strlen(respt), ", Pseudo Overwrite formatted");
   else if(drive_role==2)
     sprintf(respt+strlen(respt), ", overwriteable");
   else if(drive_role == 4)
     sprintf(respt+strlen(respt), ", random read-only");
   else if(drive_role == 5)
     sprintf(respt+strlen(respt), ", random write-only");
   else if(drive_role==0 || drive_role==3)
     sprintf(respt+strlen(respt), ", sequential");
   strcat(respt, "\n");
 } else {
    sprintf(respt+strlen(respt), "is not recognizable\n");
    not_recognizable= 1;
 }
 Xorriso_toc_line(xorriso, flag & 8);

 if((flag & 64) || !(flag & 1)) {
   Xorriso_media_product(xorriso, flag & (2 | 8 | 16));
   if(xorriso->request_to_abort)
     {ret= 1; goto ex;}
 }

 sprintf(respt, "Media status : ");
 if(is_bdr_pow) {
   sprintf(respt+strlen(respt), "is unsuitable , is POW formatted");
 } else if (s == BURN_DISC_FULL) {
   if(not_recognizable)
     sprintf(respt+strlen(respt), "is not recognizable\n");
   else
     sprintf(respt+strlen(respt), "is written , is closed");
 } else if (s == BURN_DISC_APPENDABLE) {
   sprintf(respt+strlen(respt), "is written , is appendable");
 } else if (s == BURN_DISC_BLANK) {
   sprintf(respt+strlen(respt), "is blank");
 } else if (s == BURN_DISC_EMPTY)
   sprintf(respt+strlen(respt), "is not present");
 else
   sprintf(respt+strlen(respt), "is not recognizable");
 if(s == BURN_DISC_APPENDABLE || s == BURN_DISC_BLANK) {
   ret= burn_disc_next_track_is_damaged(drive, 0);
   if(ret & 1)
     sprintf(respt+strlen(respt), " , but next track is damaged");
   else if(ret & 2)
     sprintf(respt+strlen(respt), " , but no writable address");
   else if(profile_no == 0x14) { /* DVD-RW sequential */
     ret= burn_disc_get_multi_caps(drive, BURN_WRITE_TAO, &caps, 0);
     if(ret == 0)
       sprintf(respt+strlen(respt), " , but will need -close on");
     if(caps != NULL)
       burn_disc_free_multi_caps(&caps);
   } else if(profile_no == 0x15) { /* DVD-RW DL */
     sprintf(respt+strlen(respt), " , but will need -close \"on\"");
   }
 }
 strcat(respt, "\n");
 Xorriso_toc_line(xorriso, flag & 8);

 if((s == BURN_DISC_FULL || s == BURN_DISC_APPENDABLE ||
     s == BURN_DISC_BLANK) && !(flag & 1)) {
   burn_drive_get_media_sno(drive, &sno, &sno_len);
   if(sno_len > 0) {
     sprintf(respt, "Media id     : ");
     respt+= strlen(respt);
     for(i= 0; i < sno_len && i < 1024; i++) {
       sprintf(respt, "%2.2X", (unsigned int) ((unsigned char *) sno)[i]);
       respt+= 2;
     }
     if(i < sno_len)
       strcat(respt, "...");
     strcat(respt, "\n");
     Xorriso_toc_line(xorriso, flag & 8);
   }
   if(sno != NULL)
     free(sno);
   sno= NULL;
   respt= xorriso->result_line;

   ret= burn_get_read_capacity(drive, &num_data, 0);
   if(ret != 1 || s == BURN_DISC_BLANK)
     num_data= 0;
   num_free= isoburn_disc_available_space(drive, NULL) / 2048; 
   nwa= -1;
   if (s == BURN_DISC_APPENDABLE) {
     ret= isoburn_disc_track_lba_nwa(drive, NULL, 0, &lba, &nwa);
     if(ret <= 0)
       nwa= -1;
   } else if(s == BURN_DISC_BLANK) {
     ret= isoburn_disc_track_lba_nwa(drive, NULL, 0, &lba, &nwa);
     if(ret == 1) {
       num_free+= nwa;
       nwa= 0;
     }
   }
   lba= num_data + num_free;
   if(nwa >= 0) {
     lba= nwa + num_free;
     if(nwa < num_data)
       num_data= nwa;
   }

   /* If closed CD-RW : obtain ATIP lead out address */
   if(profile_no == 0x0a) {
     ret= burn_disc_read_atip(drive);
     if(ret < 0)
       goto ex;
     ret= burn_drive_get_start_end_lba(drive, &start_lba, &end_lba, 0);
     if(s == BURN_DISC_FULL && ret == 1) {
       if(lba > end_lba - 2) {
         overburn_blocks= lba - end_lba + 2;
       } else {
         lba= end_lba - 2;
       }
     } else {
       if(ret == 1 && end_lba  - 2 > lba) {
         sprintf(xorriso->info_text,
                 "ATIP end_lba %d > overall %d", end_lba, lba);
         Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "WARNING", 0);
       }
     }
   } else if(profile_no == 0x14) {
     ret= burn_disc_get_phys_format_info(drive, &disk_category,
                                         &book_name, &part_version,
                                         &num_layers, &num_blocks, 0);
     if(ret == 1 && num_blocks > lba)
       lba= num_blocks;
   }

   if(drive_role == 5)
     num_data_text= "occupied";
   else
     num_data_text= "readable";
   if(drive_role == 4 || is_bdr_pow)
     num_free_text = "unused";
   else
     num_free_text = "writable";
   sprintf(respt, "Media blocks : %d %s , %d %s , %d overall\n",
                  num_data, num_data_text, (int) num_free, num_free_text, lba);
   Xorriso_toc_line(xorriso, flag & 8);
   if(overburn_blocks > 0) {
     sprintf(respt, "Overburnt by : %d blocks\n", overburn_blocks);
     Xorriso_toc_line(xorriso, flag & 8);
   }
 }

 if(s == BURN_DISC_BLANK) {
   sprintf(respt, "Media summary: 0 sessions, 0 data blocks, 0 data");
   num_free= isoburn_disc_available_space(drive, NULL); 
   Sfile_scale((double) num_free, mem_text,5,1e4,1);
   sprintf(respt+strlen(respt), ", %s free\n", mem_text);
   Xorriso_toc_line(xorriso, flag & 8);
 }
 if(s != BURN_DISC_FULL && s != BURN_DISC_APPENDABLE)
   {ret= 1; goto ex;}
 if(xorriso->request_to_abort)
   {ret= 1; goto ex;}
 if(is_bdr_pow) {
   sprintf(respt,
           "Media summary: unsuitable Pseudo Overwrite formatted BD-R\n");
   Xorriso_toc_line(xorriso, flag & 8);
   {ret= 1; goto ex;}
 }

 if(!(flag & 2))
   Xorriso_show_boot_info(xorriso, 1 | (flag & 8) | ((flag & 1) << 1));

 if(xorriso->isofs_size > 0 && !(flag & 3)) {
   sprintf(respt, "ISO offers   :%s%s%s%s\n",
                  xorriso->isofs_has_what & 1 ? " Rock_Ridge" : "",
                  xorriso->isofs_has_what & 2 ? " Joliet" : "",
                  xorriso->isofs_has_what & 4 ? " ISO_9660_1999" : "",
                  xorriso->isofs_has_what & 7 ? "" : " Only_ECMA_119");
   Xorriso_toc_line(xorriso, flag & 8);
   sprintf(respt, "ISO loaded   : %s\n",
                  xorriso->tree_loaded == 1 ? "Joliet" :
                  xorriso->tree_loaded == 2 ? "ISO_9660_1999" :
                  xorriso->rr_loaded > 0 ? "Rock_Ridge" : "Only_ECMA_119");
   Xorriso_toc_line(xorriso, flag & 8);
 }

 disc= isoburn_toc_drive_get_disc(drive);
 if(flag & 4)
   sprintf(respt, "TOC layout   : %3s , %9s , %10s\n",
           "Idx", "sbsector", "Size");
 else
   sprintf(respt, "TOC layout   : %3s , %9s , %10s , %s\n",
           "Idx", "sbsector", "Size", "Volume Id");
 if(!(flag&1))
   Xorriso_toc_line(xorriso, flag & 8);

 if (disc==NULL) {
   Xorriso_process_msg_queues(xorriso,0);
   nwa= 0;
   if(drive_role == 5 && s == BURN_DISC_APPENDABLE) {
     ret= burn_disc_track_lba_nwa(drive, NULL, 0, &lba, &nwa);
     if(ret != 1)
       nwa= 0;
   } else {
     ret= isoburn_get_min_start_byte(drive, &start_byte, 0);
     nwa= start_byte / 2048;
     if(ret<=0) {
       Xorriso_process_msg_queues(xorriso,0);
       if(flag&1)
         {ret= 1; goto ex;}
       sprintf(xorriso->info_text, "Cannot obtain Table Of Content");
       Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "SORRY", 0);
       {ret= 0; goto ex;}
     }
   }

   /* fabricate TOC */
   typetext= "Other session";
   if(flag & 4) {
     ret= 0;
     typetext= "Session      ";
   } else
     ret= isoburn_read_iso_head(drive, 0, &image_blocks, volume_id, 1);
   if(ret>0) {
     sprintf(respt, "ISO session  : %3d , %9d , %9ds , %s\n",
             1, 0, image_blocks, volume_id);
     nwa= image_blocks;
   } else {
     ret= burn_disc_get_formats(drive, &status, &size, &dummy,
                                &num_formats);
     if(ret <= 0 || status != BURN_FORMAT_IS_FORMATTED)
       size= 0;
     if(size <= 0) {
       ret= burn_get_read_capacity(drive, &num_data, 0);
       if(ret == 1)
         size= ((off_t) num_data) * (off_t) 2048; 
     } else {
       num_data_from_format= 1;
     }
     num_data= size / 2048;
     if(num_data == 0 && drive_role == 5 && s == BURN_DISC_APPENDABLE)
       num_data= nwa;
     sprintf(respt, "%13s: %3d , %9d , %9ds , \n",
             typetext, 1, 0, num_data);
   } 
   if(!(flag&1))
     Xorriso_toc_line(xorriso, flag & 8);
   num_sessions= 1;
 } else {
   num_data= other_data= 0;
   sessions= isoburn_toc_disc_get_sessions(disc, &num_sessions);
   open_sessions= isoburn_toc_disc_get_incmpl_sess(disc);
   for (session_no= 0;
        session_no < num_sessions + open_sessions &&
        !(xorriso->request_to_abort);
        session_no++) {
     num_session_data= num_session_other= 0;
     tracks= isoburn_toc_session_get_tracks(sessions[session_no], &num_tracks);
     if (tracks == NULL || num_tracks <= 0)
   continue;
     if(session_no == num_sessions + open_sessions - 1 && open_sessions > 0)
       have_real_open_session= 1;
     for(track_no= 0; track_no<num_tracks && !(xorriso->request_to_abort);
         track_no++) {
       track_count++;
       is_data= 0;
       isoburn_toc_track_get_entry(tracks[track_no], &toc_entry);
       if((toc_entry.control & 7) >= 4) /* data track */
         is_data= 1;
       if (toc_entry.extensions_valid & 1) {
         /* DVD extension valid */
         lba= toc_entry.start_lba;
         track_size= toc_entry.track_blocks;
       } else {
         lba= burn_msf_to_lba(toc_entry.pmin, toc_entry.psec,
                              toc_entry.pframe);
         if(track_no==num_tracks-1) {
           isoburn_toc_session_get_leadout_entry(sessions[session_no],
                                                 &next_toc_entry);
         } else {
           isoburn_toc_track_get_entry(tracks[track_no+1], &next_toc_entry);
         }
         track_size= burn_msf_to_lba(next_toc_entry.pmin, next_toc_entry.psec,
                                     next_toc_entry.pframe) - lba;
       }
       if((flag & (1 | 4)) || !is_data) {
         ret= 0;
       } else {
         ret= isoburn_toc_track_get_emul(tracks[track_no], &emul_lba,
                                         &image_blocks, volume_id, 0);
         if(ret <= 0)
           ret= isoburn_read_iso_head(drive, lba, &image_blocks, volume_id, 1);
         if(image_blocks > track_size) {
           sprintf(xorriso->info_text,
              "Session %d bears ISO image size %ds larger than track size %ds",
              session_no + 1, image_blocks, track_size);
           Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "WARNING",
                               0);
           image_blocks= track_size;
         }
       }
       if(session_no >= num_sessions && track_no == 0) {
         if(ret <= 0)
           volume_id[0]= 0;
         sprintf(respt, "Incmp session: %3d , %9d , %9ds , %s\n",
                 session_no+1, lba, image_blocks, volume_id);
       } else if(ret>0 && track_no==0) {
         sprintf(respt, "ISO session  : %3d , %9d , %9ds , %s\n",
                 session_no+1, lba, image_blocks , volume_id);
       } else if(ret>0) {
         sprintf(respt, "ISO track    : %3d , %9d , %9ds , %s\n",
                 track_count, lba, image_blocks , volume_id);
       } else if(track_no==0) {
         typetext= "Other session";
         if(flag & 4)
           typetext= "Session      ";
         sprintf(respt, "%13s: %3d , %9d , %9ds , \n",
                 typetext, session_no+1, lba, track_size);
       } else {
         typetext= "Other track  ";
         if(flag & 4)
           typetext= "Track        ";
         sprintf(respt, "%13s: %3d , %9d , %9ds , \n",
                 typetext, track_count, lba, track_size);
       } 
       if(!(flag&1))
         Xorriso_toc_line(xorriso, flag & 8);
       if(is_data)
         num_session_data+= track_size;
       else
         num_session_other+= track_size;
       if(track_no == 0)
         first_track_start= lba;
     }
     isoburn_toc_session_get_leadout_entry(sessions[session_no], &toc_entry);
     if (toc_entry.extensions_valid & 1) {
       lba= toc_entry.start_lba;
     } else {
       lba= burn_msf_to_lba(toc_entry.pmin, toc_entry.psec, toc_entry.pframe);
     }
     session_size= lba - first_track_start;
     if(num_session_data > 0 && num_session_other > 0) {
       num_data+= num_session_data;
       other_data+= num_session_other;
     } else if(is_data) {
       num_data+= session_size;
     } else {
       other_data+= session_size;
     }
   }
 }
 if(xorriso->request_to_abort)
   {ret= 1; goto ex;}

 Sfile_scale(((double) num_data) * 2048.0, mem_text,5,1e4,1);
 sessions_seen= num_sessions + open_sessions;
 if(open_sessions > 0 && !have_real_open_session)
   sessions_seen--;
 sprintf(respt, "Media summary: %d session%s, %d data blocks, %s data",
         sessions_seen, (sessions_seen == 1 ? "" : "s"), num_data, mem_text);
 if(num_data_from_format)
   num_free= 0;
 else
   num_free= isoburn_disc_available_space(drive, NULL); 
 Sfile_scale((double) num_free, mem_text,5,1e4,1);
 sprintf(respt+strlen(respt), ", %s free", mem_text);

 sprintf(respt+strlen(respt), "\n");
 Xorriso_toc_line(xorriso, flag & 8);

 if(other_data > 0) {
   sprintf(respt, "Non-data blks: %d\n", other_data);
   Xorriso_toc_line(xorriso, flag & 8);
 }

 if (s==BURN_DISC_APPENDABLE && nwa!=0) {
   ret= isoburn_disc_track_lba_nwa(drive, NULL, 0, &lba, &nwa);
   if(ret>0) {
     sprintf(respt, "Media nwa    : %ds\n", nwa);
     if(!(flag&1))
       Xorriso_toc_line(xorriso, flag & 8);
   }
 }
 if(profile_no == 0x41 && sessions_seen >= 300) { 
   sprintf(xorriso->info_text,
           "Sequential BD-R medium now contains %d sessions. It is likely to soon fail writing.",
           sessions_seen);
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "WARNING", 0);
 }

 if(have_real_open_session) {
   sprintf(xorriso->info_text, "Incomplete session encountered !");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "WARNING", 0);
 }

 ret= 1;
ex:;
 Xorriso_process_msg_queues(xorriso,0);
 if (disc!=NULL)
   isoburn_toc_disc_free(disc);
 return(ret);
}


/* @param flag bit0= try to find 'meaningful' links for enumerated devices
*/
int Xorriso_show_devices(struct XorrisO *xorriso, int flag)
{
 char *adr= NULL, *link_adr= NULL, *adrpt;
 int i, j, max_dev_len= 1, pad, ret;
 struct burn_drive_info *drive_list= NULL;
 unsigned int drive_count;
 char *respt, perms[8];
 struct stat stbuf;

 Xorriso_alloc_meM(adr, char, BURN_DRIVE_ADR_LEN);
 Xorriso_alloc_meM(link_adr, char, BURN_DRIVE_ADR_LEN);

 sprintf(xorriso->info_text, "Beginning to scan for devices ...\n");
 Xorriso_info(xorriso,0);

 burn_drive_clear_whitelist(); 
 while(!burn_drive_scan(&drive_list, &drive_count)) {
   Xorriso_process_msg_queues(xorriso,0);
   usleep(100000);
 }
 Xorriso_process_msg_queues(xorriso,0);
 if(drive_count == 0) {

   /* >>> was a drive_list created at all ? */
   /* >>> must it be freed ? */

   sprintf(xorriso->info_text, "No drives found");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "SORRY", 0);
   {ret= 0; goto ex;}
 }
 sprintf(xorriso->info_text, "Full drive scan done\n");
 Xorriso_info(xorriso,0);

 sprintf(xorriso->info_text, "-----------------------------------------------------------------------------\n");
 Xorriso_info(xorriso,0);
 respt= xorriso->result_line;
 for(i= 0; i < (int) drive_count && !(xorriso->request_to_abort); i++) {
   if(burn_drive_get_adr(&(drive_list[i]), adr)<=0)
     strcpy(adr, "-get_adr_failed-");
   Xorriso_process_msg_queues(xorriso,0);
   adrpt= adr;
   if(flag & 1) {
     ret= burn_lookup_device_link(adr, link_adr, "/dev", NULL, 0, 0);
     if(ret < 0)
       goto ex;
     if(ret == 1)
       adrpt= link_adr;
   }
   if((int) strlen(adrpt) > max_dev_len)
     max_dev_len= strlen(adrpt);
 }
 for(i= 0; i < (int) drive_count && !(xorriso->request_to_abort); i++) {
   if(burn_drive_get_adr(&(drive_list[i]), adr)<=0)
     strcpy(adr, "-get_adr_failed-");
   Xorriso_process_msg_queues(xorriso,0);
   if(stat(adr,&stbuf)==-1) {
     sprintf(perms,"errno=%d",errno);
   } else { 
     strcpy(perms,"------");
     if(stbuf.st_mode&S_IRUSR) perms[0]= 'r';
     if(stbuf.st_mode&S_IWUSR) perms[1]= 'w';
     if(stbuf.st_mode&S_IRGRP) perms[2]= 'r';
     if(stbuf.st_mode&S_IWGRP) perms[3]= 'w';
     if(stbuf.st_mode&S_IROTH) perms[4]= 'r';
     if(stbuf.st_mode&S_IWOTH) perms[5]= 'w';
   }
   adrpt= adr;
   if(flag & 1) {
     ret= burn_lookup_device_link(adr, link_adr, "/dev", NULL, 0, 0);
     if(ret < 0)
       goto ex;
     if(ret == 1)
       adrpt= link_adr;
   }
   sprintf(respt, "%d  -dev '%s' ", i, adrpt);
   pad= max_dev_len-strlen(adrpt);
   if(pad>0)
     for(j= 0; j<pad; j++)
       strcat(respt, " ");
   sprintf(respt+strlen(respt), "%s :  '%-8.8s' '%s' \n",
           perms, drive_list[i].vendor, drive_list[i].product);
   Xorriso_result(xorriso,0);
 }
 sprintf(xorriso->info_text, "-----------------------------------------------------------------------------\n");
 Xorriso_info(xorriso,0);

 burn_drive_info_free(drive_list);
 ret= 1;
ex:;
 Xorriso_process_msg_queues(xorriso,0);
 Xorriso_free_meM(adr);
 Xorriso_free_meM(link_adr);
 return(ret);
}


int Xorriso_tell_media_space(struct XorrisO *xorriso,
                             int *media_space, int *free_space, int flag)
{
 int ret;
 struct burn_drive_info *dinfo;
 struct burn_drive *drive;
 struct burn_write_opts *burn_options;

 (*free_space)= (*media_space)= 0;

 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                                "on attempt to -tell_media_space", 2);
 if(ret<=0)
   return(0);

 ret= Xorriso_make_write_options(xorriso, drive, &burn_options, 0);
 if(ret<=0)
   return(-1);
 (*free_space)= (*media_space)=
              isoburn_disc_available_space(drive, burn_options) / (off_t) 2048;
 burn_write_opts_free(burn_options);

 if(Xorriso_change_is_pending(xorriso, 0)) {
   ret= Xorriso_write_session(xorriso, 1);
   if(ret>0) {
     (*free_space)-= ret;
   } else {
     Xorriso_process_msg_queues(xorriso,0);
     return(0);
   }
 }
 Xorriso_process_msg_queues(xorriso,0);
 return(1);
}


/* @return <=0 error, 1 success
*/
int Xorriso_list_formats(struct XorrisO *xorriso, int flag)
{
 int ret, i, status, num_formats, profile_no, type, alloc_blocks, free_blocks;
 off_t size;
 unsigned dummy;
 char status_text[80], profile_name[90], *respt;
 struct burn_drive_info *dinfo;
 struct burn_drive *drive;

 respt= xorriso->result_line;

 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                         "on attempt to obtain format descriptor list", 1 | 2);
 if(ret<=0)
   return(0);
 if(ret == 2)
   goto ex;
 ret = burn_disc_get_formats(drive, &status, &size, &dummy,
                             &num_formats);
 if(ret<=0) {
   sprintf(xorriso->info_text, "Cannot obtain format list info");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "SORRY", 0);
   ret= 0; goto ex;
 }
 ret= Xorriso_toc(xorriso, 3);
 if(ret<=0)
   goto ex;
 ret= burn_disc_get_profile(drive, &profile_no, profile_name);
 if(ret<=0)
   goto ex;

 if(status == BURN_FORMAT_IS_UNFORMATTED)
   sprintf(status_text, "unformatted, up to %.1f MiB",
                        ((double) size) / 1024.0 / 1024.0);
 else if(status == BURN_FORMAT_IS_FORMATTED) {
   if(profile_no==0x12 || profile_no==0x13 || profile_no==0x1a ||
      profile_no==0x43)
     sprintf(status_text, "formatted, with %.1f MiB",
                         ((double) size) / 1024.0 / 1024.0);
   else
     sprintf(status_text, "written, with %.1f MiB",
                         ((double) size) / 1024.0 / 1024.0);
 } else if(status == BURN_FORMAT_IS_UNKNOWN) {
   if (profile_no > 0)
     sprintf(status_text, "intermediate or unknown");
   else
     sprintf(status_text, "no media or unknown media");
 } else
   sprintf(status_text, "illegal status according to MMC-5");
 sprintf(respt, "Format status: %s\n", status_text);
 Xorriso_result(xorriso,0);
 ret= burn_disc_get_bd_spare_info(drive, &alloc_blocks, &free_blocks, 0);
 if(ret == 1) {
   sprintf(respt, "BD Spare Area: %d blocks consumed, %d blocks available\n",
           alloc_blocks - free_blocks, free_blocks);
   Xorriso_result(xorriso,0);
 }

 for (i= 0; i < num_formats; i++) {
   ret= burn_disc_get_format_descr(drive, i, &type, &size, &dummy);
   if (ret <= 0)
 continue;
   sprintf(respt, "Format idx %-2d: %2.2Xh , %.fs , %.1f MiB\n",
          i, type, ((double) size) / 2048.0, ((double) size) / 1024.0/1024.0);
   Xorriso_result(xorriso,0);
 }
 ret= 1;
ex:;
 return(ret);
}


/* @param flag bit0= This is write speed. Consider the existence of
                     combo drives (e.g. DVD-ROM/CD-RW or BD-ROM/DVD+-RW)
*/
int Xorriso_choose_speed_factor(struct XorrisO *xorriso,
                                int speed, int profile,
                                struct burn_drive *drive,
                                double *speed_factor, char **speed_unit,
                                int flag)
{
 int i, no_dvd_read= 1, no_dvd_write= 1, no_bd_read= 1, no_bd_write= 1, ret;
 int num_profiles, profiles[64];
 char is_current[64];

 *speed_unit= "D";  
 *speed_factor= 1385000.0;

 if((flag & 1) || profile == 0x00) {
   ret= burn_drive_get_all_profiles(drive, &num_profiles,
                                    profiles, is_current);
   if(ret > 0) {
     for(i= 0; i < num_profiles; i++) {
       if(profiles[i] > 0x10 && profiles[i] < 0x30)
         no_dvd_read= no_dvd_write= 0;
       else if(profiles[i] == 0x10)
         no_dvd_read= 0;
       else if(profiles[i] > 0x40 && profiles[i] <= 0x43)
         no_bd_read= no_bd_write= 0;
       else if(profiles[i] == 0x40)
         no_bd_read= 0;
     }
   }
 }
 if(profile == 0x00) { /* No medium loaded, guess from profile list */
   if(flag & 1) {
     if(no_bd_write && no_dvd_write) 
       *speed_unit= "C";
     else if(!no_bd_write)
       *speed_unit= "B";
   } else {
     if(no_bd_read && no_dvd_read)
       *speed_unit= "C";
     else if(!no_bd_read)
       *speed_unit= "B";
   }
 } else if((profile > 0x00 && profile <= 0x0a) ||
           (((no_dvd_write && no_bd_write) && (flag & 1)))) {
   *speed_unit= "C";
 } else if((profile >= 0x40 && profile <= 0x43) &&
           !(no_bd_write && (flag & 1))) {
   *speed_unit= "B";
 }
 if((*speed_unit)[0] == 'C')
   *speed_factor= 75.0 * 2352.0;
 else if((*speed_unit)[0] == 'B')
   *speed_factor= 4495625.0;
 return(1);
}


/* For sorting the int *speeds array
*/
int Xorriso__reverse_int_cmp(const void *a, const void *b)
{
 int diff;

 diff= *((int *) a) - *((int *) b);
 if(diff < 0)
   return(1);
 else if(diff > 0)
   return(-1);
 return(0);
}


/* @flag bit0= do not issue TOC
         bit1= Report about outdev (else indev)
         bit2= Report about write speed (else read speed)
   @return <=0 error, 1 success
*/
int Xorriso_list_speeds_sub(struct XorrisO *xorriso, int flag)
{
 int ret, high= -1, low= 0x7fffffff, is_cd= 0, i, speed, profile= 0;
 int inout_flag, prev_speed= -1, speed_count= 0;
 int *speeds= NULL;
 char *respt, *speed_unit= "D";
 double speed_factor= 1385000.0, cd_factor= 75.0 * 2352;
 struct burn_drive_info *dinfo;
 struct burn_drive *drive;
 struct burn_speed_descriptor *speed_list= NULL, *item, *other;

 respt= xorriso->result_line;

 inout_flag= (flag & 2);
 if(inout_flag && xorriso->out_drive_handle == NULL)
   inout_flag= 0;
 else if(xorriso->in_drive_handle == NULL)
   inout_flag= 2;
 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                                "on attempt to obtain speed descriptor list",
                                1 | inout_flag);
 if(ret<=0)
   return(0);
 if(ret == 2)
   goto ex;
 ret= burn_drive_get_speedlist(drive, &speed_list);
 if(ret < 0) {
   sprintf(xorriso->info_text, "Cannot obtain speed list info");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "SORRY", 0);
   ret= 0; goto ex;
 }
 if(!(flag & 1)) {
   ret= Xorriso_toc(xorriso, 1 | inout_flag);
   if(ret<=0) {
     sprintf(xorriso->info_text,
             "Cannot obtain overview of drive and media content");
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "SORRY", 0);
     ret= 0; goto ex;
   }
 }

 speed_count= 0;
 for(item= speed_list; item != NULL; item= item->next)
   speed_count++;
 if(speed_count > 0)
   Xorriso_alloc_meM(speeds, int, speed_count);

 speed_count= 0;
 for(item= speed_list; item != NULL; item= item->next) {

   sprintf(xorriso->info_text,
           "read_speed= %5dk , write_speed= %5dk , source= %d",
           item->read_speed, item->write_speed, item->source);
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "DEBUG", 0);

   if(item->source == 1) {
     /* CD mode page 2Ah : report only if not same speed by GET PERFORMANCE */
     if(!(flag & 4))
 continue; /* 2Ah only tells write speed */
     for(other= speed_list; other != NULL; other= other->next)
       if(other->source == 2 && item->write_speed == other->write_speed)
     break;
     if(other != NULL)
 continue;
   }
   if(flag & 4) {
     if(item->write_speed <= 0)
 continue;
     speed= item->write_speed;
   } else {
     if(item->read_speed <= 0)
 continue;
     speed= item->read_speed;
   }
   speeds[speed_count]= speed;
   if(item->profile_loaded > 0)
     profile= item->profile_loaded;
   speed_count++;
 }

 if(speed_count > 0)
   qsort(speeds, (size_t) speed_count, sizeof(int), Xorriso__reverse_int_cmp);

 if(profile >= 0x08 && profile <= 0x0a)
   is_cd= profile;
 for(i= 0; i < speed_count; i++) {

   speed= speeds[i];
   if(speed == prev_speed)
 continue;
   prev_speed= speed;
   if(flag & 4)
     sprintf(respt, "Write speed  : ");
   else
     sprintf(respt, "Read speed   : ");
  
   Xorriso_choose_speed_factor(xorriso, speed, profile, drive,
                               &speed_factor, &speed_unit, !!(flag & 4));
   sprintf(respt + strlen(respt), " %5dk , %4.1fx%s\n",
           speed, ((double) speed) * 1000.0 / speed_factor, speed_unit);
   Xorriso_result(xorriso,0);
   if(speed > high)
     high= speed;
   if(speed < low)
     low= speed;
 }

 /* Maybe there is ATIP info (about write speed only) */
 if(is_cd && (flag & 4)) {
   ret= burn_disc_read_atip(drive);
   if(ret < 0)
     goto ex;
   if(ret > 0) {
     for(i= 0; i < 2; i++) {
       if(i == 0)
         ret= burn_drive_get_min_write_speed(drive);
       else
         ret= burn_drive_get_write_speed(drive);
       if(ret > 0) {
         if(ret < low || (i == 0 && ret != low)) {
           sprintf(respt, "Write speed l: ");
           sprintf(respt + strlen(respt), " %5dk , %4.1fx%s\n",
                   ret, ((double) ret) * 1000.0 / cd_factor, "C");
           Xorriso_result(xorriso,0);
           low= ret;
         }
         if(ret > high || (i == 1 && ret != high)) {
           sprintf(respt, "Write speed h: ");
           sprintf(respt + strlen(respt), " %5dk , %4.1fx%s\n",
                   ret, ((double) ret) * 1000.0 / cd_factor, "C");
           Xorriso_result(xorriso,0);
           high= ret;
         }
       } 
     }
   }
 }
 if(high > -1) {
   Xorriso_choose_speed_factor(xorriso, low, profile, drive,
                               &speed_factor, &speed_unit, !!(flag & 4));
   if(flag & 4)
     sprintf(respt, "Write speed L: ");
   else
     sprintf(respt, "Read speed L : ");
   sprintf(respt + strlen(respt), " %5dk , %4.1fx%s\n",
           low, ((double) low) * 1000.0 / speed_factor, speed_unit);
   Xorriso_result(xorriso,0);
   Xorriso_choose_speed_factor(xorriso, low, profile, drive,
                               &speed_factor, &speed_unit, !!(flag & 4));
   if(flag & 4)
     sprintf(respt, "Write speed H: ");
   else
     sprintf(respt, "Read speed H : ");
   sprintf(respt + strlen(respt), " %5dk , %4.1fx%s\n",
           high, ((double) high) * 1000.0 / speed_factor, speed_unit);
   Xorriso_result(xorriso,0);
   ret= burn_drive_get_best_speed(drive, 0, &item, 2);
   if(ret > 0 && item != NULL && (flag & 4))
     if(item->write_speed != high) {
       sprintf(respt, "Write speed 0:  %5dk , %4.1fx%s\n",
               item->write_speed,
             ((double) item->write_speed) * 1000.0 / speed_factor, speed_unit);
       Xorriso_result(xorriso,0);
     }
 } else {
   sprintf(xorriso->info_text,
           "Could not get any %s speed information from drive",
           (flag & 4) ? "write" : "read");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE", 0);
   ret= 2; goto ex;
 }
 ret= 1;
ex:;
 if(speed_list != NULL)
   burn_drive_free_speedlist(&speed_list);
 Xorriso_free_meM(speeds);
 return(ret);
}


int Xorriso_list_speeds(struct XorrisO *xorriso, int flag)
{
 int ret;

 if(xorriso->out_drive_handle == NULL && xorriso->in_drive_handle == NULL) {
   Xorriso_msgs_submit(xorriso, 0,
               "No drive acquired on attempt to list speeds", 0, "FAILURE", 0);
   return(0);
 }
 if(xorriso->in_drive_handle != NULL) {
   ret= Xorriso_list_speeds_sub(xorriso, 0);
   if(ret <= 0)
     return(ret);
 }
 if(xorriso->out_drive_handle != NULL &&
    xorriso->out_drive_handle != xorriso->in_drive_handle) {
   ret= Xorriso_list_speeds_sub(xorriso, 2);
   if(ret <= 0)
     return(ret);
 }
 if(xorriso->out_drive_handle != NULL) {
   ret= Xorriso_list_speeds_sub(xorriso, 1 | 2 | 4);
   if(ret <= 0)
     return(ret);
 }
 return(1);
}


/* @param flag bit0= cdrecord style
               bit1= obtain outdrive, else indrive
   @return <=0 error, 1 success
*/
int Xorriso_list_profiles(struct XorrisO *xorriso, int flag)
{
 int ret, i;
 struct burn_drive_info *dinfo;
 struct burn_drive *drive;
 int num_profiles, profiles[64];
 char is_current[64], profile_name[90], *respt;

 respt= xorriso->result_line;

 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                          "on attempt to obtain profile list", 1 | (flag & 2));
 if(ret<=0)
   return(0);
 burn_drive_get_all_profiles(drive, &num_profiles, profiles, is_current);
 for(i= 0; i < num_profiles; i++) {
   ret= burn_obtain_profile_name(profiles[i], profile_name);
   if(ret <= 0)
     strcpy(profile_name, "unknown");
   sprintf(respt, "%s 0x%4.4X (%s)%s\n",
           flag & 1 ? "Profile:" : "Profile      :",
           (unsigned int) profiles[i],
           profile_name, is_current[i] ? " (current)" : "");
   Xorriso_result(xorriso,0);
 }
 return(1);
}


/* @param flag bit0= -inq
               bit1= -checkdrive
*/
int Xorriso_atip(struct XorrisO *xorriso, int flag)
{
 int ret, profile_number= 0, is_bdr_pow= 0;
 int num_profiles= 0, profiles[64], i, can_write= 0, pf, no_medium= 0;
 char is_current[64];
 char *respt, profile_name[80];
 double x_speed_max, x_speed_min= -1.0;
 struct burn_drive_info *dinfo;
 struct burn_drive *drive;
 enum burn_disc_status s;
 char *manuf= NULL, *media_code1= NULL, *media_code2= NULL;
 char *book_type= NULL, *product_id= NULL;

 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                           "on attempt to print drive and media info", 2);
 if(ret<=0)
   return(0);
 respt= xorriso->result_line;
 sprintf(respt, "Device type    : ");
 ret= burn_drive_get_drive_role(drive);
 if(ret==0)
   sprintf(respt+strlen(respt), "%s\n", "Emulated (null-drive)");
 else if(ret==2)
   sprintf(respt+strlen(respt), "%s\n",
           "Emulated (stdio-drive, 2k random read-write)");
 else if(ret==3)
   sprintf(respt+strlen(respt), "%s\n",
           "Emulated (stdio-drive, sequential write-only)");
 else if(ret == 4)
   sprintf(respt+strlen(respt), "%s\n",
           "Emulated (stdio-drive, 2k random read-only)");
 else if(ret == 5)
   sprintf(respt+strlen(respt), "%s\n",
           "Emulated (stdio-drive, 2k random write-only)");
 else if(ret!=1)
   sprintf(respt+strlen(respt), "%s\n","Emulated (stdio-drive)");
 else
   sprintf(respt+strlen(respt), "%s\n","Removable CD-ROM");
 sprintf(respt+strlen(respt), "Vendor_info    : '%s'\n",dinfo->vendor);
 sprintf(respt+strlen(respt), "Identifikation : '%s'\n",dinfo->product);
 sprintf(respt+strlen(respt), "Revision       : '%s'\n",dinfo->revision);
 Xorriso_result(xorriso,1);
 if(flag&1)
   return(1);

 /* Do not report "Supported modes: SAO TAO" with -ROM drives */
 burn_drive_get_all_profiles(drive, &num_profiles, profiles, is_current);
 if(num_profiles > 0) {
   for(i= 0; i < num_profiles; i++) {
     pf= profiles[i];
     if(pf == 0x09 || pf == 0x0a || pf == 0x11 || pf == 0x12 || pf == 0x13 ||
        pf == 0x14 || pf == 0x15 || pf == 0x1a || pf == 0x1b || pf == 0x2b ||
        pf == 0x41 || pf == 0x43 || pf == 0xffff) {
       can_write= 1;
   break;
     }
   }
 } else
   can_write= 1;
 if(can_write) {
   sprintf(respt, "Driver flags   : BURNFREE\n");
   sprintf(respt+strlen(respt), "Supported modes: SAO TAO\n");
   Xorriso_result(xorriso,1);
 } else if(flag & 2) {
   sprintf(xorriso->info_text, "Not a CD/DVD/BD recorder");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "SORRY", 0);
 }
 if(flag&2)
   return(1);

 is_bdr_pow= burn_drive_get_bd_r_pow(drive);
 s= burn_disc_get_status(drive);
 ret= burn_disc_get_profile(drive,&profile_number,profile_name);
 if(ret<=0) {
   profile_number= 0;
   strcpy(profile_name, "-unidentified-");
 }
 if(s != BURN_DISC_UNSUITABLE) {
   ret= burn_disc_read_atip(drive);
   if(ret>0) {
     ret= burn_drive_get_min_write_speed(drive);
     x_speed_min= ((double) ret)/176.4;
   }
 }
 if(s==BURN_DISC_EMPTY) {
   sprintf(respt, "Current: none\n");
   Xorriso_result(xorriso,1);
   sprintf(xorriso->info_text, "No recognizable medium found in drive");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "SORRY", 0);
   no_medium= 1;
 } else 
   sprintf(respt, "Current: %s%s\n",profile_name,
           is_bdr_pow ? ", Pseudo Overwrite formatted" : "");
 Xorriso_result(xorriso,1);
 Xorriso_list_profiles(xorriso, 1 | 2);
 if(no_medium)
   return(1);
 if(strstr(profile_name,"BD")==profile_name) {
   printf("Mounted Media: %2.2Xh, %s\n", profile_number, profile_name);
 } else if(strstr(profile_name,"DVD")==profile_name) {
   sprintf(respt, "book type:     %s (emulated booktype)\n", profile_name);
   Xorriso_result(xorriso,1);
   if(profile_number == 0x13) {
     sprintf(respt, "xorriso: message for sdvdbackup: \"(growisofs mode Restricted Overwrite)\"\n");
     Xorriso_result(xorriso,1);
   }
 } else {
   sprintf(respt, "ATIP info from disk:\n");
   Xorriso_result(xorriso,1);
   if(burn_disc_erasable(drive))
     sprintf(respt, "  Is erasable\n");
   else
     sprintf(respt, "  Is not erasable\n");
   Xorriso_result(xorriso,1);
   { int start_lba,end_lba,min,sec,fr;
     ret= burn_drive_get_start_end_lba(drive,&start_lba,&end_lba,0);
     if(ret>0) {
       burn_lba_to_msf(start_lba,&min,&sec,&fr);
       sprintf(respt, "  ATIP start of lead in:  %d (%-2.2d:%-2.2d/%-2.2d)\n",
              start_lba,min,sec,fr);
       Xorriso_result(xorriso,1);
       burn_lba_to_msf(end_lba,&min,&sec,&fr);
       sprintf(respt, "  ATIP start of lead out: %d (%-2.2d:%-2.2d/%-2.2d)\n",
              end_lba,min,sec,fr);
       Xorriso_result(xorriso,1);
     }
   }
   ret= burn_drive_get_write_speed(drive);
   x_speed_max= ((double) ret)/176.4;
   if(x_speed_min<0)
     x_speed_min= x_speed_max;
   sprintf(respt,
          "  1T speed low:  %.f 1T speed high: %.f\n",x_speed_min,x_speed_max);
   Xorriso_result(xorriso,1);
 }

 ret= burn_disc_get_media_id(drive, &product_id, &media_code1, &media_code2,
                                &book_type, 0);
 if(ret > 0 && media_code1 != NULL && media_code2 != NULL)
   manuf= burn_guess_manufacturer(profile_number, media_code1, media_code2, 0);
 if(product_id != NULL) {
   sprintf(respt, "Product Id:    %s\n", product_id);
   Xorriso_result(xorriso,1);
 }
 if(manuf != NULL) {
   sprintf(respt, "Producer:      %s\n", manuf);
   Xorriso_result(xorriso, 1);
 }
 if(profile_number == 0x09 || profile_number == 0x0a) {
   sprintf(respt, "Manufacturer: %s\n", manuf);
   Xorriso_result(xorriso, 1);
 } else if(product_id != NULL && media_code1 != NULL && media_code2 != NULL){
   free(product_id);
   free(media_code1);
   free(media_code2);
   if(book_type != NULL)
     free(book_type);
   product_id= media_code1= media_code2= book_type= NULL;
   ret= burn_disc_get_media_id(drive, &product_id, &media_code1, &media_code2,
                               &book_type, 1);
   if(ret > 0) {
     sprintf(respt, "Manufacturer:  '%s'\n", media_code1);
     Xorriso_result(xorriso, 1);
     if(media_code2[0]) {
       sprintf(respt, "Media type:    '%s'\n", media_code2);
       Xorriso_result(xorriso, 1);
     }
   }
 }
 if(manuf != NULL)
   free(manuf);
 if(media_code1 != NULL)
   free(media_code1);
 if(media_code2 != NULL)
   free(media_code2);
 if(book_type != NULL)
   free(book_type);
 if(product_id != NULL)
   free(product_id);
 return(1);
}


/* @param flag bit1= outdev rather than indev
   @return <0 error, 0 = no profile to see , 1= ok , 2= ok, is CD profile
                                                     3= ok, is BD profile
*/
int Xorriso_get_profile(struct XorrisO *xorriso, int *profile_number, 
                        char profile_name[80], int flag)
{
 int ret;
 struct burn_drive_info *dinfo;
 struct burn_drive *drive;

 *profile_number= 0;
 profile_name[0]= 0;
 if(((flag&2) && xorriso->out_drive_handle==NULL) ||
    ((!(flag&2)) && xorriso->in_drive_handle==NULL))
   return(0);
 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                                "on attempt to determine media type", flag&2);
 if(ret<=0)
   return(0);
 ret=burn_disc_get_profile(drive, profile_number, profile_name);
 if(ret<=0)
   return(ret);
 if(*profile_number==0x08 || *profile_number==0x09 || *profile_number==0x0a)
   return(2);
 if(*profile_number == 0x40 || *profile_number == 0x41 ||
    *profile_number == 0x42 || *profile_number == 0x43)
   return(3);
 return(0);
}


/* @param flag bit0= grow_overwriteable_iso
               bit1= obtain info from outdev
               bit2= no need to obtain msc2 (NWA)
*/
int Xorriso_msinfo(struct XorrisO *xorriso, int *msc1, int *msc2, int flag)
{
 int ret, dummy, is_bdr_pow= 0;
 struct burn_drive *drive;
 struct burn_drive_info *dinfo;
 enum burn_disc_status disc_state;

 *msc1= *msc2= -1;
 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                                "on attempt to obtain msinfo", flag&2);
 if(ret<=0)
   return(ret);

 is_bdr_pow=  burn_drive_get_bd_r_pow(drive);
 if(is_bdr_pow) {
   Xorriso_process_msg_queues(xorriso,0);
   sprintf(xorriso->info_text,
          "%s medium is unsuitably POW formatted BD-R. Cannot obtain -msinfo.",
          (flag&2) ? "Output" : "Input");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
   return(0);
 }
 if(flag&1)
   disc_state= isoburn_disc_get_status(drive);
 else
   disc_state= burn_disc_get_status(drive);
 if(disc_state != BURN_DISC_APPENDABLE &&
    !(disc_state == BURN_DISC_FULL && (flag & 4))) {
   Xorriso_process_msg_queues(xorriso,0);
   if(!(flag & 4)) {
     sprintf(xorriso->info_text,
             "%s medium is not appendable. Cannot obtain -msinfo.",
             (flag&2) ? "Output" : "Input");
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
   }
   return(0);
 }
 ret= isoburn_disc_get_msc1(drive, msc1);
 if(ret<=0) {
   Xorriso_process_msg_queues(xorriso,0);
   sprintf(xorriso->info_text, "Cannot obtain address of most recent session");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
   return(0);
 }
 if(flag & 4)
   return(1);
 ret= isoburn_disc_track_lba_nwa(drive, NULL, 0, &dummy, msc2);
 if(ret<0) {
   Xorriso_process_msg_queues(xorriso,0);
   sprintf(xorriso->info_text, "Cannot obtain next writeable address on media");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
   return(0);
 }
 return(1);
}


/* @param flag bit0=input drive
               bit1=output drive
               bit2= wake up rather than calm down
*/
int Xorriso_drive_snooze(struct XorrisO *xorriso, int flag)
{
 int in_is_out_too, ret;
 struct burn_drive_info *dinfo;
 struct burn_drive *drive;
 
 in_is_out_too= (xorriso->in_drive_handle == xorriso->out_drive_handle);
 if((flag & 1) && xorriso->in_drive_handle != NULL) {
   Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                             "on attempt to calm drive", 0);
   burn_drive_snooze(drive, !!(flag & 4));
   if(in_is_out_too)
     {ret= 1; goto ex;}
 }
 if((flag&2) && xorriso->out_drive_handle!=NULL) {
   Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                             "on attempt to calm drive", 2);
   burn_drive_snooze(drive, !!(flag & 4));
 }
 ret= 1;
ex:;
 Xorriso_process_msg_queues(xorriso,0);
 return(ret);
}


/* @param flag bit0= enable SCSI command logging to stderr */
int Xorriso_scsi_log(struct XorrisO *xorriso, int flag)
{
 if(flag == 0)
   burn_set_scsi_logging(0);
 else
   burn_set_scsi_logging(2|4);
 return(1);
}


int Xorriso_check_md5_range(struct XorrisO *xorriso, off_t start_lba,
                            off_t end_lba, char md5[16], int flag)
{
 int ret, us_corr = 0;
 struct burn_drive_info *dinfo= NULL;
 struct burn_drive *drive= NULL;
 off_t pos, data_count, to_read, slowdown_count= 0;
 char *data= NULL, data_md5[16];
 void *ctx = NULL;
 struct timeval prev_time;

 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                                "on attempt to check session MD5 checksum", 0);
 if(ret <= 0)
   goto ex;

 Xorriso_alloc_meM(data, char, 64 * 1024);

 ret= iso_md5_start(&ctx);
 if(ret <= 0) {
   Xorriso_no_malloc_memory(xorriso, NULL, 0);
   goto ex;
 }
 if(xorriso->read_speed_force > 0) /* initialize forced speed limit */
   burn_nominal_slowdown(xorriso->read_speed_force, xorriso->read_speed_corr,
                         &prev_time, &us_corr, (off_t) 0, 1);
 Xorriso_set_speed(xorriso, drive, xorriso->read_speed, 0, 1);
 Xorriso_process_msg_queues(xorriso,0);
 for(pos= start_lba; pos < end_lba; pos+= 32) {
   to_read= 32;
   if(pos + to_read > end_lba)
     to_read= end_lba - pos; 
   ret= burn_read_data(drive, pos * (off_t) 2048, data,
                       to_read * (off_t) 2048, &data_count, 0);
   if(ret <= 0)
     goto ex;
   iso_md5_compute(ctx, data, (int) data_count);
   if(xorriso->read_speed_force > 0 && pos + to_read <= end_lba) {
     slowdown_count+= data_count;
     if(slowdown_count >= 128 * 1024) {
       burn_nominal_slowdown(xorriso->read_speed_force,
                             xorriso->read_speed_corr,
                             &prev_time, &us_corr, slowdown_count, 0);
       slowdown_count= 0;
     }
   }
   xorriso->pacifier_count+= data_count; 
   xorriso->pacifier_byte_count+= data_count;
   Xorriso_pacifier_callback(xorriso, "content bytes read",
                             xorriso->pacifier_count, 0, "", 8);
 }
 iso_md5_end(&ctx, data_md5);
 ret= 1;
 if(! iso_md5_match(md5, data_md5))
   ret= 0;
ex:;
 Xorriso_process_msg_queues(xorriso,0);
 if(ctx != NULL)
   iso_md5_end(&ctx, data_md5);
 Xorriso_free_meM(data);
 return(ret);
}


int Xorriso_check_session_md5(struct XorrisO *xorriso, char *severity,
                              int flag)
{
 int ret, i;
 IsoImage *image;
 uint32_t start_lba, end_lba;
 char md5[16], md5_text[33];

 ret= Xorriso_get_volume(xorriso, &image, 0);
 if(ret<=0)
   return(ret);
 ret= iso_image_get_session_md5(image, &start_lba, &end_lba, md5, 0);
 Xorriso_process_msg_queues(xorriso,0);
 if(ret < 0)
   return(ret);
 if(ret == 0) {
   sprintf(xorriso->info_text,
           "No session MD5 is recorded with the loaded session");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "WARNING", 0);
   return(0);
 }

 sprintf(xorriso->info_text, "Checking loaded session by its recorded MD5.\n"); 
 Xorriso_info(xorriso, 0);
 for(i= 0; i < 16; i++)
   sprintf(md5_text + 2 * i, "%2.2x", ((unsigned char *) md5)[i]);
 sprintf(xorriso->result_line,
         "Session MD5 %s , LBA %.f , %.f blocks\n",
         md5_text, (double) start_lba, (double) end_lba - start_lba);
 Xorriso_result(xorriso,0);
 ret= Xorriso_check_md5_range(xorriso, (off_t) start_lba, (off_t) end_lba,
                              md5, 0);
 return(ret);
}

 
int Xorriso_check_for_abort(struct XorrisO *xorriso,
                            char *abort_file_path,
                            double post_read_time,
                            double *last_abort_file_time, int flag)
{
 struct stat stbuf;

 if(abort_file_path[0] == 0)
   return(0);
 if(post_read_time - *last_abort_file_time >= 0.1) {
   if(stat(abort_file_path, &stbuf) != -1) {
     if(stbuf.st_mtime >= xorriso->start_time) {
       sprintf(xorriso->info_text,
               "-check_media: Found fresh abort_file=%s", abort_file_path);
       Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE", 0);
       return(1);
     }
   }
   *last_abort_file_time= post_read_time;
 }
 return(0);
}


struct xorriso_md5_state {

  /* Resources */
  struct XorrisO *xorriso;
  void *ctx;
  struct SpotlisT *spotlist;
//  pthread_mutex_t spot_mutex;

  /* Checksum tag cursor */
  uint32_t md5_start;
  uint32_t next_tag;
  int chain_broken;
  int in_track_gap;
  int was_sb_tag;
  int md5_spot_value;
  uint32_t md5_spot_lba;

  /* Asynchronous operation */

  int slave_state;   /* Operated by slave 
                        0= not yet started
                        1= slave is started
                        2= slave has reached its end
                        3= slave failed to start
                      */
  int chunk_size;
  int num_chunks;
  char **chunk;
  int *chunk_state;  /*  0= content invalid (set by boss at creation time),
                         1= content readable (set by boss),
                         2= content was read (set by MD5 slave),
                         3= end-of-processing (set by boss when done)
                      */
  int *chunk_fill;    /* Actual number of valid bytes in chunk */
  uint32_t *chunk_lba;
  int chunk_w_idx;    /* Write index. Operated by boss */
  int chunk_r_idx;    /* Read index. Operated by MD5 slave */

  off_t w_sleeps;
  off_t r_sleeps;

};


int Xorriso__add_spot(struct xorriso_md5_state *state,
                      int start_lba, int blocks, int quality, int flag)
{
 int ret, uret;

 if(state->chunk != NULL) {
   //ret= pthread_mutex_lock(&(state->spot_mutex));
	 ret = 0;
   if(ret != 0)
     return(0);
 }
 ret= Spotlist_add_item(state->spotlist, start_lba, blocks, quality, 0);
 if(state->chunk != NULL) {
   //uret= pthread_mutex_unlock(&(state->spot_mutex));
	 uret = 0;
   if(uret != 0 && ret > 0)
     ret= 0;
 }
 return(ret);
}


int Xorriso_chunk_md5(struct XorrisO *xorriso, char *data, int to_read,
                  uint32_t from_lba, struct xorriso_md5_state *state, int flag)
{
 int j, k, ret= 0, valid, tag_type, decode_ret= 0;
 uint32_t lba, pos, range_start, range_size;
 char md5[16], tag_md5[16], *tag_type_name= "", *comparison, *sev_text;
 char md5_text[33];
 void *cloned_ctx= NULL;

 for(j= 0; j < to_read; j++) {
   lba=  j + from_lba;
   if(lba < state->md5_start)
 continue;
   ret= decode_ret= 0;
   if(lba > state->md5_start + 16 &&
      (state->next_tag == 0 || state->chain_broken || lba == state->next_tag)){
     ret= iso_util_decode_md5_tag(data + j * 2048, &tag_type,
                                  &pos, &range_start, &range_size,
                                  &(state->next_tag), tag_md5,
                                  !!state->chain_broken);
     decode_ret= ret;
   }
   valid= (ret == 1 || ret == (int) ISO_MD5_AREA_CORRUPTED) && pos == lba;
   if(valid && tag_type == 2 && (lba < state->md5_start + 32 ||
      state->in_track_gap)){
     tag_type_name= "superblock";
     state->was_sb_tag= 1;
     if(state->in_track_gap && range_start != state->md5_start &&
        range_start < lba && lba - range_start <= (uint32_t) j) {
       /* Looking for next session : start computing in hindsight.
          Session start and superblock tag are supposed to be in the
          same 64 kB chunk.
       */
       iso_md5_end(&(state->ctx), md5);
       ret= iso_md5_start(&(state->ctx));
       if(ret < 0) {
         Xorriso_no_malloc_memory(xorriso, NULL, 0);
         ret= -1; goto ex;
       }
       iso_md5_compute(&(state->ctx), data + (j - (lba - range_start)) * 2048,
                       (lba - range_start) * 2048);
       state->md5_start= range_start;
       state->in_track_gap= 0;
     }
   } else if(valid && tag_type == 4 && lba < 32) {
     tag_type_name= "relocated 64kB superblock";
   } else if(valid && tag_type == 3 && state->was_sb_tag) {
     tag_type_name= "tree";
   } else if(valid && tag_type == 1) {
     /* Allow this without superblock and tree tag */
     tag_type_name= "session";
   } else {
     tag_type_name= "";
   }
   if (tag_type_name[0]) {
     if(range_start != state->md5_start) {
       sprintf(xorriso->info_text,
          "Found MD5 %s tag which covers different data range", tag_type_name);
       Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE",0);
       sprintf(xorriso->info_text, "     Expected start: %u  Found: %u",
               (unsigned int) state->md5_start, range_start);
       Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE",0);
       for(k= 0; k < 16; k++)
         sprintf(md5_text + 2 * k, "%2.2x", ((unsigned char *) tag_md5)[k]);
       sprintf(xorriso->info_text, "     Size: %u  MD5: %s",
               range_size, md5_text);
       Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE",0);
       state->chain_broken= 1;
       valid= 0;
     } else {
       ret= iso_md5_clone(state->ctx, &cloned_ctx);
       if(ret <= 0) {
         Xorriso_no_malloc_memory(xorriso, NULL, 0);
         ret= -1; goto ex;
       }
       iso_md5_end(&cloned_ctx, md5);

       if(decode_ret == (int) ISO_MD5_AREA_CORRUPTED) {
         comparison= "CORRUPTED";
         sev_text= "WARNING";
         state->md5_spot_value= Xorriso_read_quality_md5_mismatcH;
         state->chain_broken= 1;
       } else if(! iso_md5_match(tag_md5, md5)) {
         comparison= "NON-MATCHING";
         sev_text= "WARNING";
         state->md5_spot_value= Xorriso_read_quality_md5_mismatcH;
         state->chain_broken= 1;
       } else {
         comparison= "matching";
         sev_text= "UPDATE";
         state->md5_spot_value= Xorriso_read_quality_md5_matcH;
       }
       state->md5_spot_lba= lba;
       sprintf(xorriso->info_text,
               "Found %s MD5 %s tag: start=%d size=%d",
               comparison, tag_type_name, state->md5_start,
               lba - state->md5_start);
       Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, sev_text, 0);
     }
     if(valid && (tag_type == 1 || (tag_type == 4 && pos == lba && lba < 32))){
       if(state->md5_spot_value != Xorriso_read_quality_untesteD) {
         ret= Xorriso__add_spot(state, state->md5_start,
             state->md5_spot_lba - state->md5_start, state->md5_spot_value, 0);
         if(ret <= 0)
         goto ex;
       }
       state->md5_spot_value= Xorriso_read_quality_untesteD;
       state->md5_start= lba + 1;
       if(state->md5_start % 32)
         state->md5_start= state->md5_start + (32 - (state->md5_start % 32));
       state->next_tag= 0;

       iso_md5_end(&(state->ctx), md5);
       ret= iso_md5_start(&(state->ctx));
       if(ret < 0) {
         Xorriso_no_malloc_memory(xorriso, NULL, 0);
         ret= -1; goto ex;
       }
       if(tag_type == 1)
         state->in_track_gap= 1;
 continue;
     }
   }
   iso_md5_compute(state->ctx, data + j * 2048, 2048);
 }
 ret= 1;
ex:;
 return(ret);
}


static void *Xorriso__md5_slave(void *state_pt)
{
 struct xorriso_md5_state *state;
 int ret, c_state, c_idx;
 static int u_wait= 1;

 state= state_pt;
 state->slave_state= 1;

 while(1) {
   c_idx= state->chunk_r_idx;
   c_state= state->chunk_state[c_idx];
   if(c_state == 1) {
     ret= Xorriso_chunk_md5(state->xorriso, state->chunk[c_idx],
                            state->chunk_fill[c_idx], state->chunk_lba[c_idx],
                            state, 0);
     if(ret <= 0)
       goto ex;
     state->chunk_state[c_idx]= 2;
     state->chunk_r_idx= (c_idx + 1) % state->num_chunks;
   } else if(c_state == 3) {
     goto ex;
   } else {

     /* >>> have a timeout ? */;

     if(u_wait > 0)
       usleep(u_wait);
     state->r_sleeps++;
   }
 }

ex:;
 state->slave_state= 2;
 return NULL;
}


int Xorriso_start_chunk_md5(struct XorrisO *xorriso,
                            struct xorriso_md5_state *state, int flag)
{
	
	Xorriso__md5_slave(state); //pvmk - no pthreads
	return 1;
#if 0	
 int ret, u_wait= 1000;
 pthread_attr_t attr;
 pthread_attr_t *attr_pt = NULL;
 pthread_t thread;

 pthread_attr_init(&attr);
 pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
 attr_pt= &attr;

 ret= pthread_create(&thread, attr_pt, Xorriso__md5_slave, state);
 if(ret != 0) {
   sprintf(xorriso->info_text,
           "-check_media: Cannot create thread for MD5 computation");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, errno, "FAILURE", 0);
   ret= 0; goto ex;
 }

 while(state->slave_state != 1) {

   /* >>> have a timeout ? */;
     /* >>> if this fails set  state->slave_state= 3 */

   usleep(u_wait);
 }
 ret= 1;
ex:;
 return(ret);
 #endif //0
}


int Xorriso__wait_chunk_md5(struct xorriso_md5_state *state,
                            int u_wait, int flag)
{
 if(state->chunk_state == NULL)
   return(1);
 while(state->chunk_state[state->chunk_w_idx] == 1) {

     /* >>> have a timeout ? */;

     usleep(u_wait);
     state->w_sleeps++;
  }
  return(1);
}


int Xorriso__wait_slave_md5_end(struct xorriso_md5_state *state,
                                int u_wait, int flag)
{
 while(state->slave_state == 1) {

     /* >>> have a timeout ? */;

     usleep(u_wait);
  }
  return(1);
}


int Xorriso__end_slave_md5(struct xorriso_md5_state *state,
                                int u_wait, int flag)
{
 int i, ret;

 /* Tell slave thread to end */
 for(i= 0; i < state->num_chunks; i++) {
   ret= Xorriso__wait_chunk_md5(state, 10000, 0);
   if(ret <= 0)
     return(ret);
   state->chunk_state[state->chunk_w_idx]= 3;
   state->chunk_w_idx= (state->chunk_w_idx + 1) % state->num_chunks;
 }
 /* Wait for slave to end */
 ret= Xorriso__wait_slave_md5_end(state, 10000, 0);
 if(ret <= 0)
   return(ret);
 return(1);
}

 
/* @param flag bit0= this is a follow-up session (i.e. on CD: TAO)
               bit1= no pacifier messages
               bit2= compute stream MD5 and look out for checksum tag
   @return <=0 error, 1= done, 2= aborted due to limit
*/
int Xorriso_check_interval(struct XorrisO *xorriso, struct SpotlisT *spotlist,
                           struct CheckmediajoB *job,
                           int from_lba, int block_count, int read_chunk,
                           int md5_start, int flag)
{
 int i, j, ret, total_count= 0, sectors= -1, sector_size= -1, skip_reading;
 int prev_quality= -1, quality= -1, retry= 0, profile_no, is_cd= 0;
 int eccb_size= 16, us_corr = 0, data_skip;
 int start_sec, end_sec, first_value, fret, suspect_tao_end= 0;
 char profile_name[80];
 int start_lba= 0;
 struct burn_drive *drive;
 struct burn_drive_info *dinfo;
 char *data= NULL, *data_pt;
 off_t data_count, to_read, read_count= 0, write_amount, skipped_to_read;
 off_t slowdown_count= 0, seek_adr;
 struct timeval prev_time;
 double pre_read_time, post_read_time, time_diff, total_time_diff= 0;
 double last_abort_file_time= 0;
 void *ctx= NULL;
 char md5[16];
 size_t data_size;
 struct xorriso_md5_state state;
 int num_chunks, async_md5;
 static off_t chunks_limit= 256 * 1024 * 1024;

 memset(&state, 0, sizeof(state));
 state.chunk= NULL;
 state.chunk_state= NULL;
 state.chunk_fill= NULL;
 state.chunk_lba= NULL;
 state.spotlist= spotlist;

 if(read_chunk > 1024)
   read_chunk= 1024;
 else if(read_chunk < 1)
   read_chunk= 1;

 data_skip= job->data_to_skip;
 num_chunks= job->async_chunks;
 if(((off_t) num_chunks) * ((off_t) read_chunk) > chunks_limit)
   num_chunks= chunks_limit / read_chunk;
 async_md5= (num_chunks >= 2);

 if(async_md5)
   data_size= num_chunks * read_chunk * 2048;
 else
   data_size= read_chunk * 2048;
 Xorriso_alloc_meM(data, char, data_size);
 data_pt= data;

 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                                "on attempt to check media readability",
                                2 * !!job->use_dev);
 if(ret<=0)
   goto ex;
 ret= burn_disc_get_profile(drive, &profile_no, profile_name);
 if(ret > 0) {
   if(profile_no >= 0x08 && profile_no <= 0x0a) {
     is_cd= 1;
     eccb_size= 1;
   } else if(profile_no >= 0x40 && profile_no <= 0x43) {
     eccb_size= 32;
   } else if(burn_drive_get_drive_role(drive) != 1) {
     eccb_size= 1;
   }
 }

 if(job->sector_map != NULL) {
   Sectorbitmap_get_layout(job->sector_map, &sectors, &sector_size, 0);
   sector_size/= 2048;
 }

 if(job->retry > 0)
   retry= 1;
 else if(job->retry == 0 && is_cd)
   retry= 1;

 if(flag & 4) {
   ret= iso_md5_start(&ctx);
   if(ret < 0) {
     Xorriso_no_malloc_memory(xorriso, NULL, 0);
     ret= -1; goto ex;
   }
 }

 state.xorriso= xorriso;
 state.ctx= ctx;
 state.spotlist= spotlist;
 state.md5_start= md5_start;
 state.next_tag= 0;
 state.chain_broken= 0;
 state.in_track_gap= 0;
 state.was_sb_tag= 0;
 state.md5_spot_value= Xorriso_read_quality_untesteD;
 state.md5_spot_lba= 0;
 state.slave_state= 0;
 state.chunk_size= read_chunk;
 if(async_md5) {
   state.num_chunks= num_chunks;
   Xorriso_alloc_meM(state.chunk, char *, num_chunks);
   Xorriso_alloc_meM(state.chunk_state, int, num_chunks);
   Xorriso_alloc_meM(state.chunk_fill, int, num_chunks);
   Xorriso_alloc_meM(state.chunk_lba, uint32_t, num_chunks);
   for(i= 0; i < state.num_chunks; i++) {
     state.chunk[i]= data + read_chunk * i * 2048;
     state.chunk_state[i]= 0;
     state.chunk_fill[i]= 0;
     state.chunk_lba[i]= 0;
   }
   //ret= pthread_mutex_init(&(state.spot_mutex), NULL);
   if(ret != 0) {
     sprintf(xorriso->info_text,
             "-check_media: Cannot initialize thread mutex");
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, errno, "FAILURE", 0);
     goto ex;
   }
 } else
   state.num_chunks= 0;
 state.chunk_w_idx= 0;
 state.chunk_r_idx= 0;
 state.w_sleeps= 0;
 state.r_sleeps= 0;

 if(async_md5) {
   ret= Xorriso_start_chunk_md5(xorriso, &state, 0);
   if(ret <= 0)
     goto ex;
 }

 if(xorriso->read_speed_force > 0) /* initialize forced speed limit */
   burn_nominal_slowdown(xorriso->read_speed_force, xorriso->read_speed_corr,
                         &prev_time, &us_corr, (off_t) 0, 1);
 Xorriso_set_speed(xorriso, drive, xorriso->read_speed, 0, 1);
 Xorriso_process_msg_queues(xorriso,0);
 start_lba= from_lba;
 to_read= read_chunk;
 post_read_time= Sfile_microtime(0);
 for(i= 0; i < block_count; i+= to_read) {
   if(i != 0)
     data_skip= 0;
   skip_reading= 0;
   ret= Xorriso_check_for_abort(xorriso, job->abort_file_path, post_read_time,
                                &last_abort_file_time, 0);
   if(ret == 1)
     goto abort_check;
   if(job->item_limit > 0 &&
      Spotlist_count(spotlist, 0) + 2 >= job->item_limit) {
     sprintf(xorriso->info_text, "-check_media: Reached item_limit=%d",
             job->item_limit);
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE", 0);
     goto abort_check;
   }
   pre_read_time= Sfile_microtime(0);
   if(job->time_limit > 0
	 && job->start_time + job->time_limit < pre_read_time) {
     sprintf(xorriso->info_text, "-check_media: Reached time_limit=%d",
             job->time_limit);
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE", 0);
abort_check:;
     if(prev_quality >= 0) {
       ret= Xorriso__add_spot(&state, start_lba, i + from_lba - start_lba,
                              prev_quality, 0);
       if(ret <= 0)
         goto ex;
     }
     ret= Xorriso__add_spot(&state, i + from_lba, block_count - i,
                            Xorriso_read_quality_untesteD, 0);
     if(ret > 0)
       ret= 2;
     goto ex;
   }

   to_read= read_chunk;
   skipped_to_read= 0;
   suspect_tao_end= 0;
   if(i + to_read > block_count)
     to_read= block_count - i;
   if(is_cd && i + to_read + 2 >= block_count) {
     /* Read last 2 blocks of CD track separately, because with TAO tracks
        they are always unreadable but with SAO tracks they contain data.
     */
     if(to_read > 2) {
       to_read-= 2;
     } else {
       if(to_read > 1)
         to_read--;
       suspect_tao_end= 1;
     }
   }

   if(sector_size == read_chunk && from_lba % read_chunk == 0 
      && !skip_reading) {
     if(Sectorbitmap_is_set(job->sector_map, (i + from_lba) / sector_size, 0)){
       quality= Xorriso_read_quality_valiD;
       skip_reading= 1;
     }
   } else if(sector_size > 0 && !skip_reading) {
     start_sec= (i + from_lba) / sector_size;
     end_sec= (i + to_read + from_lba) / sector_size;
     first_value= Sectorbitmap_is_set(job->sector_map, start_sec, 0);
     for(j= start_sec; j < end_sec; j++)
       if(Sectorbitmap_is_set(job->sector_map, j, 0) != first_value)
     break;
     to_read= j * sector_size - i - from_lba;
     skip_reading= !!first_value;
     if(skip_reading)
       quality= Xorriso_read_quality_valiD;
   }

   if(skip_reading) {
     pre_read_time= post_read_time= Sfile_microtime(0);
     skipped_to_read= to_read;
   } else {
     data_count= 0;
     pre_read_time= Sfile_microtime(0);

     if(async_md5) {
       ret= Xorriso__wait_chunk_md5(&state, 1, 0);
       if(ret <= 0)
         goto ex;
       data_pt= state.chunk[state.chunk_w_idx];
     }
     ret= burn_read_data(drive, ((off_t) (i + from_lba)) * (off_t) 2048,
                     data_pt, to_read * (off_t) 2048, &data_count,
                     (4 * !retry) | (16 * !!suspect_tao_end));
     post_read_time= Sfile_microtime(0);
     time_diff= post_read_time - pre_read_time;
     total_time_diff+= time_diff;
     total_count++;
     if(ret <= 0) {
       Xorriso_process_msg_queues(xorriso,0);
       if(data_count / 2048 < to_read) {
         if(data_count > 0 && retry) {
           if(prev_quality >= 0) {
             ret= Xorriso__add_spot(&state, start_lba,
                                    i + from_lba - start_lba, prev_quality, 0);
              if(ret <= 0)
                goto ex;
           }
           ret= Xorriso__add_spot(&state, i + from_lba, data_count / 2048,
                                  Xorriso_read_quality_partiaL, 0);
           if(ret <= 0)
             goto ex;
           start_lba= i + from_lba + data_count / 2048;
           if(suspect_tao_end && ret == -3)
             prev_quality= Xorriso_read_quality_tao_enD;
           else
             prev_quality= Xorriso_read_quality_unreadablE;
         }
         if(suspect_tao_end && ret == -3)
           quality= Xorriso_read_quality_tao_enD;
         else
           quality= Xorriso_read_quality_unreadablE;
         if(retry) /* skip one eccb_size */
           to_read= data_count / 2048 + eccb_size;

       } else { /* (can hardly happen) */
         quality= Xorriso_read_quality_partiaL;
       }
       fret= Xorriso_eval_problem_status(xorriso, ret, 1|2);
       if(fret<0)
         goto ex;
     } else {
       quality= Xorriso_read_quality_gooD;
       if(time_diff > job->slow_threshold_seq && job->slow_threshold_seq > 0 &&
          i > 0)
         quality= Xorriso_read_quality_sloW;
     }

     /* MD5 checksumming */
     if(ctx != NULL) {
       if(async_md5) {
         state.chunk_fill[state.chunk_w_idx]= to_read;
         state.chunk_lba[state.chunk_w_idx]= i + from_lba;
         state.chunk_state[state.chunk_w_idx]= 1;
         /* The MD5 thread will call Xorriso_chunk_md5() */

         state.chunk_w_idx= (state.chunk_w_idx + 1) % state.num_chunks;
       } else {
         ret= Xorriso_chunk_md5(xorriso, data_pt, to_read,
                                (uint32_t) (i + from_lba), &state, 0);
         if(ret <= 0)
           goto ex;
       }
     }

     write_amount= data_count - data_skip;
     if(data_count > 0) {
       read_count+= data_count - data_skip;
       if(job->data_to_limit >= 0 && read_count > job->data_to_limit)
         write_amount-= (read_count - job->data_to_limit);
     }
     if(xorriso->read_speed_force > 0) {
       slowdown_count+= data_count;
       if(slowdown_count >= 128 * 1024) {
         burn_nominal_slowdown(xorriso->read_speed_force,
                               xorriso->read_speed_corr,
                               &prev_time, &us_corr, slowdown_count, 0);
         slowdown_count= 0;
       }
     }
     if(write_amount > 0) {
       if(job->data_to_fd >= 0) {
         seek_adr= ((off_t) (i + from_lba)) * (off_t) 2048 +
                   job->data_to_skip + job->data_to_offset;
         if(strcmp(job->data_to_path, "-") != 0) {
           ret= lseek(job->data_to_fd, seek_adr, SEEK_SET);
           if(ret == -1) {
failed_to_write:;
             sprintf(xorriso->info_text,
                     "Cannot write %d bytes to position %.f in ",
                     (int) data_count, (double) seek_adr);
             Text_shellsafe(job->data_to_path, xorriso->info_text, 1);
             Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, errno,
                                 "FAILURE", 0);
             {ret= 0; goto ex;}
           }
         }
         ret= write(job->data_to_fd, data_pt + data_skip, write_amount);
         if(ret == -1)
           goto failed_to_write;
       }
     }
   }
   if(quality != prev_quality) {
     if(prev_quality >= 0) {
       ret= Xorriso__add_spot(&state, start_lba,
                              i + from_lba - start_lba, prev_quality, 0);
       if(ret <= 0)
         goto ex;
     }
     start_lba= i + from_lba;
     prev_quality= quality;
   }
   if(!(flag & 2)) {
     xorriso->pacifier_count+= to_read - skipped_to_read;
     if(post_read_time - xorriso->last_update_time >=
        xorriso->pacifier_interval)
       Xorriso_pacifier_callback(xorriso, "blocks read",
                 xorriso->pacifier_count, xorriso->pacifier_total, "",
                 8 | 16 | (128 * (job->use_dev == 1)));
   }
 }
 if(prev_quality >= 0) {
   ret= Xorriso__add_spot(&state, start_lba,
                          block_count + from_lba - start_lba, prev_quality, 0);
   if(ret <= 0)
     goto ex;
 }

 /* <<< for calibration of quality */
 if(total_count > 0) {
   sprintf(xorriso->info_text, "Xorriso_check_interval: %.1f s / %d = %f",
           total_time_diff, total_count, total_time_diff / total_count);
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "DEBUG", 0);
 }


 /* MD5 checksumming : register result */
 if(async_md5) {
   ret= Xorriso__end_slave_md5(&state, 10000, 0);
   if(ret <= 0)
     goto ex;
 }

 /* >>> ??? allow chain_broken to be a match ? */

 if(state.next_tag > 0) {
   sprintf(xorriso->info_text, "Missing announced MD5 tag: start=%d pos=%d",
           state.md5_start, state.next_tag);
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "WARNING", 0);
   state.md5_spot_value= Xorriso_read_quality_md5_mismatcH;
   state.md5_spot_lba= state.next_tag;
 }
 if(state.md5_spot_value != Xorriso_read_quality_untesteD) {
   ret= Xorriso__add_spot(&state, state.md5_start,
                state.md5_spot_lba - state.md5_start, state.md5_spot_value, 0);
   if(ret <= 0)
     goto ex;
 }

 ret= 1;
ex:;
 if(async_md5) {
   Xorriso__end_slave_md5(&state, 10000, 0);
   sprintf(xorriso->info_text,
           "async_chunks=%d , chunk_size=%ds , w_sleeps: %.f , r_sleeps: %.f",
           state.num_chunks, read_chunk, (double) state.w_sleeps,
           (double) state.r_sleeps);
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "DEBUG", 0);
   if(state.chunk != NULL)
   {/*pthread_mutex_destroy(&(state.spot_mutex));*/}
   Xorriso_free_meM(state.chunk);
   Xorriso_free_meM(state.chunk_state);
   Xorriso_free_meM(state.chunk_fill);
   Xorriso_free_meM(state.chunk_lba);
 }
 Xorriso_free_meM(data);
 if(state.ctx != NULL)
   iso_md5_end(&(state.ctx), md5);

 return(ret);
}


int Xorriso_check_media(struct XorrisO *xorriso, struct SpotlisT **spotlist,
                        struct CheckmediajoB *job, int flag)
{
 int media_blocks= 0, read_chunk= 32, ret, mode, start_lba= 0;
 int blocks, os_errno, i, j, last_track_end= -1, track_blocks, track_lba;
 int num_sessions, num_tracks, declare_untested= 0, md5_start;
 int read_capacity= -1, end_lba, hret, count, quality, profile_no;
 int track_bad_claim= 0;
 char *toc_info= NULL, profile_name[80], msg[160];
 struct burn_drive *drive;
 struct burn_drive_info *dinfo;
 enum burn_disc_status s;
 struct isoburn_toc_disc *isoburn_disc= NULL;
 struct isoburn_toc_session **isoburn_sessions;
 struct isoburn_toc_track **iso_burn_tracks;
 struct burn_toc_entry isoburn_entry;
 struct stat stbuf;
 struct burn_multi_caps *caps= NULL;

 *spotlist= NULL;

 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                                "on attempt to check media readability",
                                2 * !!job->use_dev);
 if(ret<=0)
   goto ex;
 
 ret = burn_disc_get_profile(drive, &profile_no, profile_name);
 if(ret <= 0)
   profile_no= 0;

 if(job->min_block_size != 0)
   read_chunk= job->min_block_size;

 ret= Spotlist_new(spotlist, 0);
 if(ret <= 0)
   {ret= -1; goto ex;}

 if(job->sector_map_path[0]) {
   Sectorbitmap_destroy(&(job->sector_map), 0);
   if(stat(job->sector_map_path, &stbuf) != -1) {
     ret= Sectorbitmap_from_file(&(job->sector_map), job->sector_map_path,
                                 xorriso->info_text, &os_errno, 0);
     if(ret <= 0) {
       if(xorriso->info_text[0])
         Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, os_errno,
                             "FAILURE", 0);
       goto ex;
     }
   }
   Xorriso_toc_to_string(xorriso, &toc_info,
                         (2 * !!job->use_dev) | (4 * !job->map_with_volid));
 }
 ret= Xorriso_open_job_data_to(xorriso, job, 0);
 if(ret <= 0)
   goto ex;
 Xorriso_pacifier_reset(xorriso, 0);
 job->start_time= time(NULL);
 mode= job->mode;
 if(job->min_lba > 0) {
   start_lba= job->min_lba;
   ret= Spotlist_add_item(*spotlist, 0, job->min_lba, 
                          Xorriso_read_quality_untesteD, 0);
   if(ret <= 0)
     goto ex;
 }

 s= isoburn_disc_get_status(drive);
 if(s != BURN_DISC_APPENDABLE && s != BURN_DISC_FULL) {
   ret= 0;
   if(s == BURN_DISC_BLANK) {
     /* check whether medium is overwritable */
     ret= burn_disc_get_multi_caps(drive, BURN_WRITE_NONE, &caps, 0);
     if(ret > 0)
       if(caps->start_adr == 0)
         ret= 0;
   }
   if(ret <= 0) {
no_readable_medium:;
     Xorriso_msgs_submit(xorriso, 0, "-check_media: No readable medium found",
                         0, "SORRY", 0);
     ret= 0; goto ex;
   }
 }

 ret= burn_get_read_capacity(drive, &read_capacity, 0);
 if(ret <= 0)
   read_capacity= -1;
 if(s == BURN_DISC_BLANK && read_capacity <= 0)
   goto no_readable_medium;

 if(job->max_lba >= 0) {
   blocks= job->max_lba + 1 - start_lba;
   xorriso->pacifier_total= blocks;
   ret= Xorriso_check_interval(xorriso, *spotlist, job, start_lba, blocks,
                               read_chunk, start_lba, 0);
   if(ret <= 0)
     goto ex;
   
 } else if(mode == 0) { /* track by track */
   if(s == BURN_DISC_BLANK) {
no_tracks_found:;
     Xorriso_msgs_submit(xorriso, 0, "-check_media: No tracks found on medium",
                         0, "SORRY", 0);
     ret= 0; goto ex;
   }
   isoburn_disc= isoburn_toc_drive_get_disc(drive);
   if(isoburn_disc == NULL)
     goto libburn_whole_disc;
   isoburn_sessions=
                    isoburn_toc_disc_get_sessions(isoburn_disc, &num_sessions);
   for(i= 0; i < num_sessions; i++) {
     iso_burn_tracks= isoburn_toc_session_get_tracks(isoburn_sessions[i],
                                                     &num_tracks);
     for(j= 0; j < num_tracks; j++) {
       isoburn_toc_track_get_entry(iso_burn_tracks[j], &isoburn_entry);
       if(!(isoburn_entry.extensions_valid & 1)) /* should not happen */
     continue;
       track_lba= isoburn_entry.start_lba;
       track_blocks= isoburn_entry.track_blocks;

       /* The last track of an appendable BD-R reports more blocks than the
          read capacity allows. All BD-R track sizes are multiple of 64 kB.
       */
       if (i == num_sessions - 1 && 
           (track_lba + track_blocks > read_capacity &&
            track_lba + track_blocks < read_capacity + 32 &&
            (profile_no == 0x41 || profile_no == 0x40)))
         track_blocks= read_capacity - track_lba;
       if(track_lba + track_blocks > read_capacity) {
         if(track_bad_claim < track_lba + track_blocks)
           track_bad_claim= track_lba + track_blocks;
         if(track_lba >= read_capacity) {
           sprintf(msg, "-check_media: Track %d of session %d begins after end of readable medium area.",
                   j + 1, i + 1);
           Xorriso_msgs_submit(xorriso, 0, msg, 0, "WARNING", 0);
     continue;
         } else {

           if(profile_no >= 0x08 && profile_no <= 0x0a &&
              track_lba + track_blocks == read_capacity + 2 &&
              i == num_sessions - 1 && j == num_tracks - 1) {
             sprintf(msg, "-check_media: Last CD track exceeds readable area by 2 blocks. Assuming TAO.");
             Xorriso_msgs_submit(xorriso, 0, msg, 0, "DEBUG", 0);
           } else {
             sprintf(msg, "-check_media: Track %d of session %d extends over the end of readable medium area.",
                   j + 1, i + 1);
             Xorriso_msgs_submit(xorriso, 0, msg, 0, "WARNING", 0);
           }
           track_blocks= read_capacity - track_lba;
         }
       }
       md5_start= track_lba;
       if(i == 0 && j == 0) {
         if(track_lba == 32) {
           ret= burn_disc_get_multi_caps(drive, BURN_WRITE_NONE, &caps, 0);
           if(ret > 0) {
             if(caps->start_adr) {
               /* block 0 to 31 are the overall mount entry of overwritable */
               track_lba= 0;
               track_blocks+= 32;
             }
           }
         }
       }
       if(last_track_end >= 0 && last_track_end < track_lba &&
          last_track_end >= start_lba) {
         ret= Spotlist_add_item(*spotlist, last_track_end,
                                track_lba - last_track_end,
                                Xorriso_read_quality_off_tracK, 0);
         if(ret <= 0)
           goto ex;
       }
       last_track_end= track_lba + track_blocks;

       if(track_lba < start_lba) {
         track_blocks-= start_lba - track_lba;
         track_lba= start_lba;
       }
       if(track_blocks <= 0)
     continue;
       if(declare_untested) {
         ret= Spotlist_add_item(*spotlist, track_lba, track_blocks,
                                Xorriso_read_quality_untesteD, 0);
         if(ret <= 0)
           goto ex;
       } else {
         ret= Xorriso_check_interval(xorriso, *spotlist, job, track_lba,
                                     track_blocks, read_chunk, md5_start,
                                     (i > 0) | (4 * (xorriso->do_md5 & 1)));
         if(ret <= 0)
           goto ex;
         if(ret == 2)
           declare_untested= 1;
       }
     }
   }

   if(track_bad_claim > read_capacity) {
     count= Spotlist_count(*spotlist, 0);
     Spotlist_get_item(*spotlist, count - 1, &track_lba, &blocks, &quality, 0);
     if(profile_no >= 0x08 && profile_no <= 0x0a &&
        track_bad_claim - read_capacity == 2 &&
        quality != Xorriso_read_quality_tao_enD)
       quality= Xorriso_read_quality_tao_enD;
     else
       quality= Xorriso_read_quality_unreadablE;
     ret= Spotlist_add_item(*spotlist, read_capacity,
                            track_bad_claim - read_capacity, quality, 0);
     if(ret <= 0)
       goto ex;
   }

 } else if(mode == 1) { /* Image range */
   /* Default is the emulated disc capacity.
   */
   if(s == BURN_DISC_BLANK)
     goto no_tracks_found;
   isoburn_disc= isoburn_toc_drive_get_disc(drive);
   if(isoburn_disc == NULL)
     goto libburn_whole_disc;
   blocks= media_blocks= isoburn_toc_disc_get_sectors(isoburn_disc);

   /* If possible, determine the end address of the loaded ISO image.
   */
   track_lba= isoburn_get_attached_start_lba(drive);
   if(track_lba >= 0) {
     ret= isoburn_read_iso_head(drive, track_lba, &track_blocks, NULL, 0);
     if(ret > 0) {
       blocks= media_blocks= track_lba + track_blocks;
     }
   }

   if(start_lba >= 0)
     blocks-= start_lba;
   if(media_blocks <= 0)
     goto libburn_whole_disc;
   xorriso->pacifier_total= blocks;
   ret= Xorriso_check_interval(xorriso, *spotlist, job, start_lba, blocks,
                           read_chunk, start_lba, (4 * (xorriso->do_md5 & 1)));
   if(ret <= 0)
     goto ex;
 } else if(mode == 2) {
libburn_whole_disc:;
   /* single sweep over libburn medium capacity */
   ret= burn_get_read_capacity(drive, &blocks, 0);
   if(ret <= 0) {
     Xorriso_process_msg_queues(xorriso,0);
     sprintf(xorriso->info_text, "No content detected on media");
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
     {ret= 0; goto ex;}
   }
   blocks-= start_lba;
   xorriso->pacifier_total= blocks;
   ret= Xorriso_check_interval(xorriso, *spotlist, job, start_lba, blocks,
                           read_chunk, start_lba, (4 * (xorriso->do_md5 & 1)));
   if(ret <= 0)
     goto ex;
 }

 Xorriso_pacifier_callback(xorriso, "blocks read",
                      xorriso->pacifier_count, xorriso->pacifier_total, "",
                      1 | 8 | 16 | 32 | (128 * (job->use_dev == 1)));
 ret= 1;
ex:;

 if(job->data_to_fd != -1 && strcmp(job->data_to_path, "-") != 0)
   close(job->data_to_fd);
 job->data_to_fd= -1;
 
 if(read_capacity >= 0) {
   count= Spotlist_count(*spotlist, 0);
   end_lba= 0;
   for(i= 0; i < count; i++) {
     Spotlist_get_item(*spotlist, i, &start_lba, &blocks, &quality, 0);
     if(start_lba + blocks > end_lba)
       end_lba= start_lba + blocks;
   }
   if(read_capacity > end_lba) {
     hret= Spotlist_add_item(*spotlist, end_lba, read_capacity - end_lba, 
                             Xorriso_read_quality_untesteD, 0);
     if(hret < ret)
       ret= hret;
   }
 }

 if(ret > 0)
   ret= Xorriso_update_in_sector_map(xorriso, *spotlist, read_chunk, job, 0);
 
 if(ret > 0) {
   ret= Xorriso_spotlist_to_sectormap(xorriso, *spotlist, read_chunk,
                                      &(job->sector_map), 2);
   if(ret > 0 && job->sector_map_path[0]) {
     ret= Sectorbitmap_to_file(job->sector_map, job->sector_map_path, toc_info,
                               xorriso->info_text, &os_errno, 0);
     if(ret <= 0) {
       if(xorriso->info_text[0])
         Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, os_errno,
                             "FAILURE", 0);
     }
   }
 }
 if(toc_info != NULL)
    free(toc_info);
 if(ret <= 0)
   Spotlist_destroy(spotlist, 0);
 if(caps!=NULL)
   burn_disc_free_multi_caps(&caps);
 if(isoburn_disc != NULL)
   isoburn_toc_disc_free(isoburn_disc);
 return(ret);
}


/* @param flag
          bit0= if not MMC drive print NOTE and return 2  
          bit1= obtain outdrive, else indrive
          bit4= do not report failure
*/
int Xorriso_get_drive_handles(struct XorrisO *xorriso,
                              struct burn_drive_info **dinfo,
                              struct burn_drive **drive,
                              char *attempt, int flag)
{
 int ret;

 if(flag&2)
   *dinfo= (struct burn_drive_info *) xorriso->out_drive_handle;
 else
   *dinfo= (struct burn_drive_info *) xorriso->in_drive_handle;
 if(*dinfo==NULL && !(flag & 16)) {
   Xorriso_process_msg_queues(xorriso,0);
   sprintf(xorriso->info_text, "No %s drive acquired %s",
           (flag&2 ? "output" : "input"), attempt);
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
 }
 if(*dinfo==NULL)
   return(0);
 *drive= (*dinfo)[0].drive;
 if(flag & 1) {
   ret= burn_drive_get_drive_role(*drive);
   if(ret != 1) {
     sprintf(xorriso->info_text,
       "Output device is not an MMC drive. Desired operation does not apply.");
     Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "NOTE", 0);
     return(2);
   }
 }
 return((*drive)!=NULL);
}


int Xorriso_pretend_full_disc(struct XorrisO *xorriso, int flag)
{
 int ret;
 struct burn_drive_info *dinfo;
 struct burn_drive *drive;

 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                "on attempt to let libburn pretend having a closed medium", 2);
 if(ret<=0)
   return(ret);

 ret= isoburn_disc_pretend_full_uncond(drive);
 Xorriso_process_msg_queues(xorriso,0);
 if(ret <= 0) {
   sprintf(xorriso->info_text,
           "Failed to let libburn pretend having a closed medium");
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "FAILURE", 0);
   return(0);
 }
 return(1);
}


int Xorriso_scsi_dev_family(struct XorrisO *xorriso, int flag)
{
 burn_preset_device_open(xorriso->drives_exclusive | (xorriso->linux_scsi_dev_family << 2), 0, 0);
 return(1);
}


int Xorriso_use_immed_bit(struct XorrisO *xorriso, int flag)
{
 int enable= 1, ret;
 struct burn_drive_info *dinfo;
 struct burn_drive *drive;

 /* It is not an error if no drive is acquired.
    Xorriso_drive_aquire() will apply use_immed_bit.
 */
 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                "on attempt to control use of Immed bit", 2 | 16);
 if(ret<0)
   return(ret);
 if(ret == 0)
   return(1);

 if(xorriso->use_immed_bit == -1) {
   enable= 0;
 } else if(xorriso->use_immed_bit == 1) {
   enable= 1;
 } else if(xorriso->use_immed_bit == 0) {
   /* obtain default value as determined after drive aquiration */
   if(xorriso->use_immed_bit_default == 0)
     return(1);
   enable= (xorriso->use_immed_bit_default > 0);
 }
 burn_drive_set_immed(drive, enable);
 Xorriso_process_msg_queues(xorriso,0);
 return(1);
}


int Xorriso_obtain_indev_readsize(struct XorrisO *xorriso, uint32_t *blocks,
                                  int flag)
{
 int ret, num_data;
 struct burn_drive_info *dinfo;
 struct burn_drive *drive;
 enum burn_disc_status s;

 *blocks= 0;
 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                                "on attempt to determine readable size", 0);
 if(ret <= 0)
   return(0);
 s= isoburn_disc_get_status(drive);
 if(s == BURN_DISC_BLANK)
   return(1);
 ret= burn_get_read_capacity(drive, &num_data, 0);
 if(ret <= 0)
   return(0);
 *blocks= num_data;
 return(1);
}


/* @param flag bit0= as_mkisofs, else cmd
*/
int Xorriso_feature_to_cmd(struct XorrisO *xorriso, char *name, char *value,
                           int flag)
{
 int i, as_m;
 double val_num= -123456789;
 static char *ignored_names[]= {"size", "eltorito", "tree_loaded",
                                "tree_loaded_text", "rr_loaded", "aaip",
                                "relaxed_vol_atts", "rrip_1_10_px_ino",
                                ""};

 sscanf(value, "%lf", &val_num);
 for(i= 0; ignored_names[i][0] != 0; i++)
   if(strcmp(name, ignored_names[i]) == 0)
     return(0);
 as_m= flag & 1;
 if(strcmp(name, "iso_level") == 0) {
   if(as_m) {
     sprintf(xorriso->result_line, "-iso-level %s", value);
   } else {
     sprintf(xorriso->result_line, "-compliance iso_9660_level=%s", value);
   }
 } else if(strcmp(name, "rockridge") == 0) {
   if(as_m) {
     if(val_num > 0.0)
       sprintf(xorriso->result_line, "-R");
     else
       sprintf(xorriso->result_line, "--norock");
   } else {
     sprintf(xorriso->result_line, "-rockridge %s",
                                   val_num > 0.0 ? "on" : "off");
   }
 } else if(strcmp(name, "joliet") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "-J");
   } else {
     sprintf(xorriso->result_line, "-joliet %s",
                                   val_num > 0.0 ? "on" : "off");
   }
 } else if(strcmp(name, "iso1999") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "-iso-level 4");
   } else {
     sprintf(xorriso->result_line, "-compliance iso_9660_1999%s",
                                   val_num > 0.0 ? "" : "_off");
   }
 } else if(strcmp(name, "untranslated_name_len") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "-untranslated_name_len %s", value);
   } else {
     sprintf(xorriso->result_line, "-compliance untranslated_name_len=%s",
                                   value);
   }
 } else if(strcmp(name, "allow_dir_id_ext") == 0) {
   if(as_m) {
     if(val_num > 0.0)
       return(0);
     sprintf(xorriso->result_line, "-disallow_dir_id_ext");
   } else {
     sprintf(xorriso->result_line, "-compliance allow_dir_id_ext%s",
                                   val_num > 0.0 ? "" : "_off");
   }
 } else if(strcmp(name, "omit_version_numbers") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "-N");
   } else {
     if(val_num <= 0.0)
       sprintf(xorriso->result_line,
               "-compliance omit_version_off:only_iso_version_off");
     else if(val_num == 2.0)
       sprintf(xorriso->result_line,
               "-compliance omit_version_off:only_iso_version");
     else
       sprintf(xorriso->result_line,
               "-compliance omit_version:only_iso_version_off");
   }
 } else if(strcmp(name, "allow_deep_paths") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "-D");
   } else {
     sprintf(xorriso->result_line, "-compliance deep_paths%s",
                                   val_num > 0.0 ? "" : "_off");
   }
 } else if(strcmp(name, "allow_longer_paths") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "-U");
   } else {
     sprintf(xorriso->result_line, "-compliance long_paths%s",
                                   val_num > 0.0 ? "" : "_off");
   }
 } else if(strcmp(name, "allow_full_ascii") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "-U");
   } else {
     sprintf(xorriso->result_line, "-compliance full_ascii%s",
                                   val_num > 0.0 ? "" : "_off");
   }
 } else if(strcmp(name, "max_37_char_filenames") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "-max-iso9660-filenames");
   } else {
     sprintf(xorriso->result_line, "-compliance long_names%s",
                                   val_num > 0.0 ? "" : "_off");
   }
 } else if(strcmp(name, "no_force_dots") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "-d");
   } else {
     sprintf(xorriso->result_line,
             "-compliance no_force_dots%s:no_j_force_dots%s",
             ((int) val_num) & 1 ? "" : "_off",
             ((int) val_num) & 2 ? "" : "_off");
   }
 } else if(strcmp(name, "allow_lowercase") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "-allow-lowercase");
   } else {
     sprintf(xorriso->result_line, "-compliance lowercase%s",
                                   val_num > 0.0 ? "" : "_off");
   }
 } else if(strcmp(name, "joliet_longer_paths") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "-joliet-long");
   } else {
     sprintf(xorriso->result_line, "-compliance joliet_long_paths%s",
                                   val_num > 0.0 ? "" : "_off");
   }
 } else if(strcmp(name, "joliet_long_names") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "-joliet-long");
   } else {
     sprintf(xorriso->result_line, "-compliance joliet_long_names%s",
                                   val_num > 0.0 ? "" : "_off");
   }
 } else if(strcmp(name, "joliet_utf16") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "-joliet-utf16");
   } else {
     sprintf(xorriso->result_line, "-compliance joliet_utf16%s",
                                   val_num > 0.0 ? "" : "_off");
   }
 } else if(strcmp(name, "rrip_version_1_10") == 0) {
   if(as_m) {
     return(0);
   } else {
     sprintf(xorriso->result_line, "-compliance %s_rr",
                                   val_num > 0.0 ? "old" : "new");
   }
 } else if(strcmp(name, "aaip_susp_1_10") == 0) {
   if(as_m) {
     return(0);
   } else {
     sprintf(xorriso->result_line, "-compliance aaip_susp_1_10%s",
                                   val_num > 0.0 ? "" : "_off");
   }
 } else if(strcmp(name, "record_md5_session") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "--md5");
   } else {
     sprintf(xorriso->result_line, "-md5 %s", val_num > 0.0 ? "on" : "off");
   }
 } else if(strcmp(name, "record_md5_files") == 0) {
   if(as_m) {
     if(val_num <= 0.0)
       return(0);
     sprintf(xorriso->result_line, "--md5");
   } else {
     sprintf(xorriso->result_line, "-md5 %s", val_num > 0.0 ? "on" : "off");
   }
 } else {
   sprintf(xorriso->info_text, "Program error: unexpected feature name '%s'",
                               name);
   Xorriso_msgs_submit(xorriso, 0, xorriso->info_text, 0, "DEBUG", 0);

   return(-1);
 }
 return(1);
}


/* @param flag bit0= do not print xorriso->result_line, but accumulate it
*/
int Xorriso_assess_written_features(struct XorrisO *xorriso, char *mode,
                                    int flag)
{
 int ret, l, ftext_l, replay_count= 0, max_line_size= 2 * SfileadrL;
 struct burn_drive_info *dinfo;
 struct burn_drive *drive;
 enum burn_disc_status s;
 struct isoburn_read_opts *ropts= NULL;
 IsoReadImageFeatures *features= NULL;
 struct isoburn_imgen_opts *imgen_opts= NULL; 
 char *ftext= NULL, *cpt, *npt, *ept, *prev_line= NULL, *cmd_line= NULL;
 char *result_acc= NULL;
 static char *tree_loaded_names[3]= {"ISO9660", "Joliet", "ISO9660:1999"};
 int tree_loaded_names_max= 2;

 Xorriso_alloc_meM(prev_line, char, max_line_size);
 Xorriso_alloc_meM(cmd_line, char, max_line_size);
 if(flag & 1) {
   Xorriso_alloc_meM(result_acc, char, 10 * SfileadrL);
   result_acc[0]= 0;
 }
 prev_line[0]= 0;
 ret= Xorriso_get_drive_handles(xorriso, &dinfo, &drive,
                                "when assessing written features", 0);
 if(ret <= 0)
   {ret= 0; goto ex;}
 s= isoburn_disc_get_status(drive);
 if(s != BURN_DISC_APPENDABLE && s != BURN_DISC_FULL) {
   Xorriso_msgs_submit(xorriso, 0,
       "The disc in the input drive offers no readable content", 0, "NOTE", 0);
   ret= 2; goto ex;
 }
 ret= Xorriso_make_read_options(xorriso, drive, &ropts, 1);
 if(ret <= 0)
   goto ex;
 ret = isoburn_assess_written_features(drive, ropts, &features, &imgen_opts,
                                       0);
 Xorriso_process_msg_queues(xorriso,0);
 /* <<< Resetting to normal thresholds, after Xorriso_make_read_options */
 if(xorriso->img_read_error_mode > 0)
   Xorriso_set_abort_severity(xorriso, 0);
 if(ret <= 0)
   {ret= 0; goto ex;}
 ret= iso_read_image_features_text(features, 1, &ftext);
 if(ret < 0)
   {ret= 0; goto ex;}

 /* print results, depending on mode */
 ftext_l= strlen(ftext);
 for(cpt= ftext ; cpt - ftext < ftext_l ; cpt+= l + 1) {
   npt= strchr(cpt, '\n');
   if(npt == NULL)
     l= strlen(cpt);
   else
     l= npt - cpt;
   cpt[l]= 0;
   ept= strchr(cpt, '=');
   if(ept == NULL)
 continue;
   if(strcmp(mode, "cmd") == 0 || strcmp(mode, "replay") == 0 ||
     strcmp(mode, "as_mkisofs") == 0 ) {
     *ept= 0;
     ret= Xorriso_feature_to_cmd(xorriso, cpt, ept + 1,
                                 strcmp(mode, "as_mkisofs") == 0);
     if(ret <= 0)
 continue;
   } else { /* "plain" and any other */
     strcpy(xorriso->result_line, "Indev feature: ");
     if(strncmp(cpt, "tree_loaded=", 12) == 0) {
       sprintf(xorriso->result_line + strlen(xorriso->result_line),
               "tree_loaded=%d", xorriso->tree_loaded);
     } else if(strncmp(cpt, "tree_loaded_text=", 17) == 0) {
       if(xorriso->tree_loaded >= 0 &&
          xorriso->tree_loaded <= tree_loaded_names_max)
         sprintf(xorriso->result_line + strlen(xorriso->result_line),
                 "tree_loaded_text=%s",
                 tree_loaded_names[xorriso->tree_loaded]);
     } else if(strncmp(cpt, "rr_loaded=", 10) == 0) {
       sprintf(xorriso->result_line + strlen(xorriso->result_line),
               "rr_loaded=%d", xorriso->rr_loaded);
     } else {
       strcat(xorriso->result_line, cpt);
     }
   }
   /* Truncate to plausible length */
   xorriso->result_line[max_line_size - 1]= 0;

   if(strcmp(xorriso->result_line, prev_line) == 0)
 continue;
   strcpy(prev_line, xorriso->result_line);
   if(strcmp(mode, "replay") == 0) {
     /* Perform result_line as command */
     strcpy(cmd_line, xorriso->result_line);
     ret= Xorriso_execute_option(xorriso, cmd_line, (1 << 16));
     if(ret <= 0) {

       /* >>> ??? what to do on error */;

     }
     replay_count++;
   } else {
     strcat(xorriso->result_line, "\n");
     if(flag & 1) {
       if(strlen(result_acc) + strlen(xorriso->result_line) < 10 * SfileadrL)
         strcat(result_acc, xorriso->result_line);
     } else {
       Xorriso_result(xorriso, 0);
     }
   }
 }

 if(strcmp(mode, "replay") == 0) {
   sprintf(xorriso->info_text,
          "-assess_indev_features replay : Number of performed commands: %d\n",
                              replay_count);
   Xorriso_info(xorriso, 0);
 }
 if(flag & 1)
   strcpy(xorriso->result_line, result_acc);
 ret= 1;
ex:;
 Xorriso_process_msg_queues(xorriso,0);
 if(ropts != NULL)
   isoburn_ropt_destroy(&ropts, 0);
 if(features != NULL)
   iso_read_image_features_destroy(features);
 if(imgen_opts != NULL)
   isoburn_igopt_destroy(&imgen_opts, 0);
 Xorriso_free_meM(ftext);
 Xorriso_free_meM(result_acc);
 Xorriso_free_meM(cmd_line);
 Xorriso_free_meM(prev_line);
 return(ret);
}

