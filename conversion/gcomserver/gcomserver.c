/* ----------------------------- MNI Header -----------------------------------
@NAME       : gcomserver.c
@DESCRIPTION: Program to receive images from Philips GYROSCAN.
@GLOBALS    : 
@CREATED    : November 22, 1993 (Peter Neelin)
@MODIFIED   : 
 * $Log: gcomserver.c,v $
 * Revision 6.8  2008-01-17 02:33:02  rotor
 *  * removed all rcsids
 *  * removed a bunch of ^L's that somehow crept in
 *  * removed old (and outdated) BUGS file
 *
 * Revision 6.7  2008/01/12 19:08:14  stever
 * Add __attribute__ ((unused)) to all rcsid variables.
 *
 * Revision 6.6  2001/04/21 13:29:38  neelin
 * Added support for PROJECT_CAPTURE project type.
 *
 * Revision 6.5  2001/04/09 23:02:48  neelin
 * Modified copyright notice, removing permission statement since copying,
 * etc. is probably not permitted by our non-disclosure agreement with
 * Philips.
 *
 * Revision 6.4  2000/06/14 18:24:07  neelin
 * Added UseSafeOrientations keyword to project files to allow forcing of
 * direction cosines to standard (safe) ones, and modified convert_to_dicom
 * so that this is no longer the default behaviour.
 *
 * Revision 6.3  2000/02/21 23:48:14  neelin
 * More changes to improve dicom conformance for MNH PACS system.
 * Allow UID prefix to be defined in project file. Define SOP instance UID in
 * addition to study and series instance UIDs and frame-of-reference UID and
 * make sure that these are all the constant for the same image data.
 * Set series number from acquisition number.
 * Set study description from part of comment field.
 *
 * Revision 6.2  1999/10/29 17:52:02  neelin
 * Fixed Log keyword
 *
 * Revision 6.1  1997/09/12 23:13:28  neelin
 * Added ability to convert gyrocom images to dicom images.
 *
 * Revision 6.0  1997/09/12  13:23:50  neelin
 * Release of minc version 0.6
 *
 * Revision 5.0  1997/08/21  13:24:50  neelin
 * Release of minc version 0.5
 *
 * Revision 4.1  1997/06/13  22:08:13  neelin
 * Modifications to get gcomserver working with modified Acr_nema library.
 *
 * Revision 4.0  1997/05/07  20:01:07  neelin
 * Release of minc version 0.4
 *
 * Revision 3.0  1995/05/15  19:31:44  neelin
 * Release of minc version 0.3
 *
 * Revision 2.3  1995/02/14  18:12:26  neelin
 * Added project names and defaults files (using volume name).
 * Added process id to log file name.
 * Moved temporary files to subdirectory.
 *
 * Revision 2.2  1995/02/09  13:51:26  neelin
 * Mods for irix 5 lint.
 *
 * Revision 2.1  1994/10/20  13:48:02  neelin
 * Write out direction cosines to support rotated volumes.
 * Store single slices as 1-slice volumes (3D instead of 2D).
 * Changed storing of minc history (get args for gyrotominc).
 *
 * Revision 2.0  94/09/28  10:35:21  neelin
 * Release of minc version 0.2
 * 
 * Revision 1.16  94/09/28  10:34:48  neelin
 * Pre-release
 * 
 * Revision 1.15  94/05/18  08:54:50  neelin
 * Changed some ACR_OTHER_ERROR's to ACR_ABNORMAL_END_OF_OUTPUT.
 * 
 * Revision 1.14  94/04/08  10:35:36  neelin
 * Fixed handling of input trace (don't do trace while using the files).
 * 
 * Revision 1.13  94/04/08  09:15:10  neelin
 * Added printing of filename to /dev/log on error.
 * 
 * Revision 1.12  94/04/07  16:18:54  neelin
 * Fixed bug in setting of state when input error or end-of-file occurs.
 * 
 * Revision 1.11  94/04/07  11:02:17  neelin
 * Changed error handling to be more explicit about errors.
 * When the server terminates due to an error, a message is printed to /dev/log.
 * Changed handling of file cleanup.
 * 
 * Revision 1.10  94/04/06  11:12:01  neelin
 * Added switch to increase logging on debug.
 * 
 * Revision 1.9  94/01/14  10:45:04  neelin
 * Fixed handling of multiple reconstructions and image types. Add spiinfo variable with extra info (including window min/max). Changed output
 * file name to include reconstruction number and image type number.
 * 
 * Revision 1.8  93/12/14  16:35:57  neelin
 * Set Keep_files and input tracing according to macros so that 
 * gcomserver-debug can turn them on.
 * 
 * Revision 1.7  93/12/10  15:31:25  neelin
 * Improved file name generation from patient name. No buffering on stderr.
 * Added spi group list to minc header.
 * Optionally read a defaults file to get output minc directory and owner.
 * 
 * Revision 1.6  93/12/08  09:35:06  neelin
 * changed logging level.
 * 
 * Revision 1.5  93/12/08  09:12:53  neelin
 * Delete group list.
 * 
 * Revision 1.4  93/11/30  14:56:18  neelin
 * Changed log level.
 * 
 * Revision 1.3  93/11/30  14:40:42  neelin
 * Copies to minc format.
 * 
 * Revision 1.2  93/11/25  13:26:35  neelin
 * Working version.
 * 
 * Revision 1.1  93/11/23  14:11:36  neelin
 * Initial revision
 * 
@COPYRIGHT  :
              Copyright 1993 Peter Neelin, McConnell Brain Imaging Centre, 
              Montreal Neurological Institute, McGill University.
---------------------------------------------------------------------------- */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gcomserver.h>

/* Global for minc history */
extern char *minc_history;

/* State of server. Note that DISCONNECTING is used for a high-level
   protocol error and TERMINATING is used for a low-level error or
   end of input */
typedef enum {
   WAITING_FOR_GROUP, WAITING_FOR_OBJECT, READY_FOR_OBJECT, 
   END_OF_GROUP, DISCONNECTING, TERMINATING
} Server_state;

/* Do we do logging? */
int Do_logging = 
#ifndef DO_HIGH_LOGGING
   LOW_LOGGING;
#else
   HIGH_LOGGING;
#endif

/* Do we keep files or are they temporary? */
static int Keep_files = 
#ifndef KEEP_FILES
   FALSE;
#else
   TRUE;
#endif

int main(int argc, char *argv[])
{
   char *pname;
   Acr_File *afpin, *afpout;
   Acr_Status status;
   Server_state state;
   int acr_command, spi_command;
   Acr_Element element;
   Acr_Group group_list;
   Acr_Message input_message, output_message;
   int exit_status;
   char *exit_string;
   char **file_list;
   Data_Object_Info *file_info_list;
   int num_files;
   int cur_file;
   static char file_prefix_string[L_tmpnam+1] = "gcomserver";
   char *file_prefix = file_prefix_string;
   char *temp_dir;
   int continue_looping;
   FILE *fptemp;
   char last_file_name[256];
   char *project_name = NULL;
   char logfilename[256];
   Project_File_Info project_info;

   /* Create minc history string */
   {
      char *string;
      string = "gcomserver";
      minc_history = time_stamp(1, &string);
   }

   /* Re-open stderr if we are logging */
   if (Do_logging > NO_LOGGING) {
      (void) sprintf(logfilename, "gcomserver-%d.log", (int) getpid());
      (void) freopen(logfilename, "w", stderr);
      setbuf(stderr, NULL);
   }

   /* Print message at start */
   pname = argv[0];
   if (Do_logging >= LOW_LOGGING) {
      (void) fprintf(stderr, "%s: Started gyrocom server.\n", pname);
   }

   /* Make connection */
   open_connection(argc, argv, &afpin, &afpout);

#ifdef DO_INPUT_TRACING
   /* Enable input tracing */
   acr_file_enable_trace(afpin);
#endif

   /* Test endian-ness of input data */
   (void) acr_test_byte_order(afpin);

   /* Create file prefix. Create the temporary file to avoid file name 
      clashes */
   if (! Keep_files) {
      (void) tmpnam(file_prefix);
      if (mkdir(file_prefix, (mode_t) 0777)) {
         (void) fprintf(stderr, 
            "%s: Unable to create directory for temporary files.\n",
                        pname);
         perror(pname);
         exit(EXIT_FAILURE);
      }
      temp_dir = strdup(file_prefix);
      (void) strcat(file_prefix, "/gyro");
   }

   /* Loop while reading messages */
   state = WAITING_FOR_GROUP;
   continue_looping = TRUE;
   num_files = 0;
   while (continue_looping) {

      /* Read in the message */
      status=spi_input_message(afpin, &input_message);

      /* Check for error */
      if (status != ACR_OK) {
         continue_looping = FALSE;
         state = TERMINATING;
         break;
      }

      /* Get group list */
      group_list = acr_get_message_group_list(input_message);

      /* Get ACR-NEMA command */
      element = acr_find_group_element(group_list, ACR_Command);
      if (element == NULL) 
         acr_command = ACR_UNKNOWN_COMMAND;
      else 
         acr_command = acr_get_element_short(element);

      /* Get SPI command */
      element = acr_find_group_element(group_list, SPI_Command);
      if (element == NULL) 
         spi_command = SPI_UNKNOWN_COMMAND;
      else 
         spi_command = acr_get_element_short(element);

      /* Check command */
      if (acr_command == SENDq) {

         /* Deal with command */
         switch (spi_command) {

            /* Begin group copy */
         case GCBEGINq:
            if (state != WAITING_FOR_GROUP) {
               status = ACR_HIGH_LEVEL_ERROR;
               state = DISCONNECTING;
               break;
            }
            output_message = gcbegin_reply(input_message, &num_files,
                                           &project_name, &project_info);
            if (project_name == NULL) {
               num_files = 0;
               break;
            }

            file_list = MALLOC((size_t) num_files * sizeof(*file_list));
            for (cur_file=0; cur_file < num_files; cur_file++) {
               file_list[cur_file] = NULL;
            }
            file_info_list = MALLOC(num_files * sizeof(*file_info_list));
            cur_file = -1;
            if (Do_logging >= LOW_LOGGING) {
               (void) fprintf(stderr, "\nCopying a group of files:\n");
            }

            /* Change to next state */
            state = WAITING_FOR_OBJECT;
            break;

            /* Ready */
         case READYq:
            if (state != WAITING_FOR_OBJECT) {
               status = ACR_HIGH_LEVEL_ERROR;
               state = DISCONNECTING;
               break;
            }
            output_message = ready_reply(input_message);
            state = READY_FOR_OBJECT;
            break;

            /* Send */
         case SENDq:
            if ((state != WAITING_FOR_OBJECT) &&
                 (state != READY_FOR_OBJECT)) {
               status = ACR_HIGH_LEVEL_ERROR;
               state = DISCONNECTING;
               break;
            }
            output_message = send_reply(input_message);
            cur_file++;
            if (cur_file >= num_files-1) {
               state = END_OF_GROUP;
            }
            else {
               state = WAITING_FOR_OBJECT;
            }
            break;

            /* GCENDq */
         case GCENDq:
            if (state != END_OF_GROUP) {
               status = ACR_HIGH_LEVEL_ERROR;
               state = DISCONNECTING;
               break;
            }

            /* Create the output message */
            output_message = gcend_reply(input_message);
            /* Print message */
            if (Do_logging >= LOW_LOGGING) {
               (void) fprintf(stderr, "\nFinished group copy.\n");
            }

            /* Do something with the files */
            if (project_info.type == PROJECT_DIRECTORY) {
#ifdef DO_INPUT_TRACING
               /* Disable input tracing */
               acr_file_disable_trace(afpin);
#endif
               use_the_files(project_name, num_files, file_list, file_info_list);
#ifdef DO_INPUT_TRACING
               /* Enable input tracing */
               acr_file_enable_trace(afpin);
#endif
            }
            else if (project_info.type == PROJECT_DICOM) {
               acr_close_dicom_connection(project_info.info.dicom.afpin,
                                          project_info.info.dicom.afpout);
            }

            /* Remove the temporary files */
            cleanup_files(num_files, file_list);
            free_list(num_files, file_list, file_info_list);
            num_files = 0;
            state = WAITING_FOR_GROUP;
            break;

            /* Unknown command */
         default:
            status = ACR_HIGH_LEVEL_ERROR;
            state = DISCONNECTING;
            break;
         }        /* End of switch on spi_command */

      }        /* End of if acr_command == SENDq */

      /* Handle cancel command */
      else if (acr_command == CANCELq) {
         if ((state == WAITING_FOR_OBJECT) || (state == READY_FOR_OBJECT)) {
            cleanup_files(num_files, file_list);
            free_list(num_files, file_list, file_info_list);
            num_files = 0;
         }
         output_message = cancel_reply(input_message);
         state = WAITING_FOR_GROUP;
      }

      /* Illegal command */
      else {
         status = ACR_HIGH_LEVEL_ERROR;
         state = DISCONNECTING;
      }

      /* Delete input message */
      acr_delete_message(input_message);

      /* Check for disconnection */
      if (state == DISCONNECTING) {
         continue_looping = FALSE;
         break;
      }

      /* Send reply */
      status = spi_output_message(afpout, output_message);

      /* Delete output message */
      acr_delete_message(output_message);

      if (status != ACR_OK) {
         state = TERMINATING;
         break;
      }

      /* Read in groups for SENDq command */
      if ((acr_command == SENDq) && (spi_command == SENDq)) {
         status = spi_input_data_object(afpin, &group_list);
         if (Do_logging >= HIGH_LOGGING) {
            (void) fprintf(stderr, "\n\nReceived data object:\n");
            acr_dump_group_list(stderr, group_list);
         }
         if (status != ACR_OK) {
            state = TERMINATING;
            break;
         }

         /* Do something with the data */
         if (project_info.type == PROJECT_DIRECTORY) {
            save_transferred_object(group_list, 
                                    file_prefix, &file_list[cur_file],
                                    &file_info_list[cur_file]);
            if (Do_logging >= LOW_LOGGING) {
               (void) fprintf(stderr, "   Copied %s\n", file_list[cur_file]);
            }
         }
         else if (project_info.type == PROJECT_CAPTURE) {
            save_transferred_object(group_list,
                                    project_info.info.directory.file_prefix,
                                    &file_list[cur_file],
                                    &file_info_list[cur_file]);
            if (Do_logging >= LOW_LOGGING) {
               (void) fprintf(stderr, "   Saved %s\n", file_list[cur_file]);
            }
            FREE(file_list[cur_file]);
            file_list[cur_file] = NULL;
         }
         else if (project_info.type == PROJECT_DICOM) {

            /* Check that we have image data and send it */
            if (acr_find_group_element(group_list, ACR_Pixel_data)
                != NULL) {
               convert_to_dicom(group_list, 
                                project_info.info.dicom.UIDprefix,
                                project_info.info.dicom.use_safe_orientations);
               if (!acr_send_group_list(project_info.info.dicom.afpin,
                                        project_info.info.dicom.afpout, 
                                        group_list,
                                        ACR_MR_IMAGE_STORAGE_UID)) {
                  (void) fprintf(stderr, "Error sending dicom image\n");
                  state = TERMINATING;
                  break;
               }
               if (Do_logging >= LOW_LOGGING) {
                  (void) fprintf(stderr, ".");
                  (void) fflush(stderr);
               }
            }
         }

         /* Delete the input data */
         acr_delete_group_list(group_list);
      }

   }        /* End of loop over messages */

   /* Free the input and output streams */
   acr_file_free(afpin);
   acr_file_free(afpout);

   /* Save name of first file in last set transferred */
   if ((num_files > 0) && (file_list[0] != NULL)) {
      last_file_name[sizeof(last_file_name) - 1] = '\0';
      (void) strncpy(last_file_name, file_list[0], sizeof(last_file_name)-1);
   }
   else {
      last_file_name[0] = '\0';
   }

   /* Clean up files, if needed */
   if (num_files > 0) {
      cleanup_files(num_files, file_list);
      free_list(num_files, file_list, file_info_list);
      num_files = 0;
   }
   
   /* Remove the file prefix directory */
   if (! Keep_files) {
      cleanup_files(1, &temp_dir);
      FREE(temp_dir);
   }

   /* Print final message */
   switch (status) {
   case ACR_OK:
   case ACR_END_OF_INPUT:
      exit_status = EXIT_SUCCESS;
      exit_string = "Finished transfer.";
      break;
   case ACR_PROTOCOL_ERROR:
      exit_status = EXIT_FAILURE;
      exit_string = "Protocol error. Disconnecting.";
      break;
   case ACR_ABNORMAL_END_OF_INPUT:
      exit_status = EXIT_FAILURE;
      exit_string = "Abnormal end of input. Disconnecting.";
      break;
   case ACR_ABNORMAL_END_OF_OUTPUT:
      exit_status = EXIT_FAILURE;
      exit_string = "Abnormal end of output. Disconnecting.";
      break;
   case ACR_HIGH_LEVEL_ERROR:
      exit_status = EXIT_FAILURE;
      exit_string = "High-level protocol error. Disconnecting";
      break;
   case ACR_OTHER_ERROR:
      exit_status = EXIT_FAILURE;
      exit_string = "I/O error. Disconnecting.";
      break;
   default:
      exit_status = EXIT_FAILURE;
      exit_string = "Unknown error. Disconnecting.";
      break;
   }

   if ((Do_logging >= LOW_LOGGING) || 
       ((status != ACR_END_OF_INPUT) && (status != ACR_OK))) {
      (void) fprintf(stderr, "%s: %s\n", pname, exit_string);
   }

   if ((status != ACR_OK) && (status != ACR_END_OF_INPUT)) {
      if (SYSTEM_LOG != NULL) {
         if ((fptemp = fopen(SYSTEM_LOG, "w")) != NULL) {
            if ((int) strlen(last_file_name) > 0) {
               (void) fprintf(fptemp, "%s: File \"%s\"\n",
                              pname, last_file_name);
            }
            (void) fprintf(fptemp, "%s: %s\n", pname, exit_string);
            (void) fclose(fptemp);
         }
      }
   }

   /* Free the project_name string */
   if (project_name != NULL) FREE(project_name);

   exit(exit_status);

}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : cleanup_files
@INPUT      : num_files - number of files in list
              file_list - array of file names
@OUTPUT     : (none)
@RETURNS    : (nothing)
@DESCRIPTION: Removes files.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : November 22, 1993 (Peter Neelin)
@MODIFIED   : 
---------------------------------------------------------------------------- */
public void cleanup_files(int num_files, char *file_list[])
{
   int i;

   if (Keep_files) return;

   for (i=0; i < num_files; i++) {
      if (file_list[i] != NULL) {
         (void) remove(file_list[i]);
      }
   }

   return;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : free_list
@INPUT      : num_files - number of files in list
              file_list - array of file names
@OUTPUT     : (none)
@RETURNS    : (nothing)
@DESCRIPTION: Frees strings in file list
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : November 22, 1993 (Peter Neelin)
@MODIFIED   : 
---------------------------------------------------------------------------- */
public void free_list(int num_files, char **file_list, 
                      Data_Object_Info *file_info_list)
{
   int i;

   for (i=0; i < num_files; i++) {
      if ( file_list[i] != NULL) {
         FREE(file_list[i]);
      }
   }
   FREE(file_list);
   FREE(file_info_list);

   return;
}

