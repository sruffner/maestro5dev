/**
 * Copyright (c) 1996-2023, SR Research Ltd., All Rights Reserved
 *
 * For use by SR Research licencees only. Redistribution and use in source
 * and binary forms, with or without modification, are NOT permitted.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 *
 * Neither name of SR Research Ltd nor the name of contributors may be used
 * to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**********************************************************************************
 * EYELINK GDI EXPT SUPPORT                                                       *
 * Header file for standard functions                                             *
 *                                                                                *
 *                                                                                *
 ******************************************* WARNING ******************************
 *                                                                                *
 * UNDER NO CIRCUMSTANCES SHOULD PARTS OF THESE FILES BE COPIED OR COMBINED.      *
 * This will make your code impossible to upgrade to new releases in the future,  *
 * and SR Research will not give tech support for reorganized code.               *
 *                                                                                *
 * This file should not be modified. If you must modify it, copy the entire file  *
 * with a new name, and change the the new file.                                  *
 *                                                                                *
 **********************************************************************************/

/*!
    \file gdi_expt.h
    \brief Declarations of eyelink_exptkit functions and types.

    This file will also reference the other EyeLink header files.
 */

#ifndef __SRRESEARCH__GDI_EXPT_H__
#define __SRRESEARCH__GDI_EXPT_H__

#ifdef __SRRESEARCH__SDL_EXPT_H__
#error sdl_expt.h should not be used with gdi_expt.h
#endif
#include <windows.h>
#include <windowsx.h>
#include "core_expt.h"
#ifdef __cplusplus     /* For C++ compilation */
extern "C" {
#endif


/*! @defgroup gdi_graphics GDI Graphics Functions
 *  @addtogroup gdi_graphics
 *  @{
 */

/**************************Convenient Macros *********************************/
#define SCREEN_LEFT   dispinfo.left
#define SCREEN_TOP    dispinfo.top
#define SCREEN_RIGHT  dispinfo.right
#define SCREEN_BOTTOM dispinfo.bottom
#define SCRHEIGHT     dispinfo.height
#define SCRWIDTH      dispinfo.width




/*! Passes the colors of the display background and fixation target to the
	EXPTSPPT library.  During calibration, camera image display, and drift
	correction, the display background should match the brightness of the
	experimental stimuli as closely as possible, in order to maximize tracking
	accuracy.  This function passes the colors of the display background and
	fixation target to the EXPTSPPT library.  This also prevents flickering of
	the display at the beginning and end of drift correction.

 	@param fg Color used for drawing calibration target.
	@param bg Color used for drawing calibration background.

	\b Example: See \c do_tracker_setup()

	\sa \c do_tracker_setup()
 */
void  ELCALLTYPE  set_calibration_colors(COLORREF fg, COLORREF bg);

/*! The standard calibration and drift correction target is a disk (for
	peripheral delectability) with a central "hole" target (for accurate
	fixation).  The sizes of these features may be set with this function.

  	@param diameter Size of outer disk, in pixels.
	@param holesize Size of central feature.  If \c &lt;holesize&gt; is \c 0,
					no central feature will be drawn.

	\b Example: See \c do_tracker_setup()

	\sa \c do_tracker_setup()
 */
void  ELCALLTYPE  set_target_size(UINT16 diameter, UINT16 holesize);

/*! Selects the sounds to be played during \c do_tracker_setup(), including
	calibration, validation and drift correction.  These events are the
	display or movement of the target, successful conclusion of calibration
	or good validation, and failure or interruption of calibration or validation.

	@remarks If no sound card is installed, the sounds are produced as "beeps"
	from the PC speaker.  Otherwise, sounds can be selected by passing a string.
	If the string is "" (empty), the default sounds are played.  If the string
	is "off", no sound will be played for that event.  Otherwise, the string
	should be the name of a .WAV file to play.
	@param ontarget Sets sound to play when target moves.
	@param ongood Sets sound to play on successful operation.
	@param onbad Sets sound to play on failure or interruption.

	\b Example: See \c do_tracker_setup()

	\sa \c do_tracker_setup() and \c set_dcorr_sounds()
 */
void  ELCALLTYPE  set_cal_sounds(char *ontarget, char *ongood, char *onbad);

/*! Selects the sounds to be played during \c do_drift_correct().  These events
	are the display or movement of the target, successful conclusion of drift
	correction, and pressing the 'ESC' key to start the Setup menu.

	@remarks If no sound card is installed, the sounds are produced as "beeps"
	from the PC speaker.  Otherwise, sounds can be selected by passing a string.
	If the string is "" (empty), the default sounds are played.  If the string
	is "off", no sound will be played for that event.  Otherwise, the string
	should be the name of a .WAV file to play.
	@param ontarget Sets sound to play when target moves.
	@param ongood Sets sound to play on successful operation.
	@param onbad Sets sound to play on failure or interruption.

	\b Example: See \c do_tracker_setup()

	\sa \c do_tracker_setup() and \c set_cal_sounds()
 */
void  ELCALLTYPE  set_dcorr_sounds(char *ontarget, char *ongood, char *onbad);

/*! To adjust camera image position. By default the camera is placed at the
 	centre of the screen.

	@param left Left position.
	@param top Top position.
	@param right Right position.
	@param bottom Bottom position.
 */
INT16 ELCALLTYPE  set_camera_image_position(INT16 left, INT16 top,
                                          INT16 right, INT16 bottom);

/*!
	Set calibration monitor index 
	@param index of the monitor Eg. 1 is main monitor
	When the monitor index is set, that index will be used to get the display information.
*/

int ELCALLTYPE set_calibration_monitor_index(int monitor_index);

/*! Measures parameters of the current display mode, and fills a \c DISPLAYINFO
	structure with the data.  This process may take over 100 milliseconds, as
	it measures actual refresh rate.  The returned data can be used to compute
	sizes for drawing, and to check that the current display mode matches the
	requirements of the experiment.  A global \c DISPLAYINFO structure called
	dispinfo should be set up at the start of the program if you wish to use
	the \c SCRWIDTH and \c SCRHEIGHT macros.

	@remarks This is the contents of the \c DISPLAYINFO structure:
	<pre>
	\code
	typedef struct
	{
	     INT32 left;      // left of display
	     INT32 top;       // top of display
	     INT32 right;     // right of display
	     INT32 bottom;    // bottom of display
	     INT32 width;     // width of display
	     INT32 height;    // height of display
	     INT32 bits;      // bits per pixel
	     INT32 palsize;   // total entries in palette (0 if not indexed)
	     INT32 palrsvd;   // number of static entries in palette
	     INT32 pages;     // pages supported
	     float refresh;   // refresh rate in Hz
	     INT32 winnt;     // 0 for 9x/Me, 1 for NT, 2 for 2000, 3 for XP
	} DISPLAYINFO;
	\endcode
	</pre>
	If refresh cannot be measured, the "refresh" field will contain a value less than 40.
	@param di Pointer to \c DISPLAYINFO structure to fill.
 */
void  ELCALLTYPE  get_display_information(DISPLAYINFO *di);

/*! You must always create a borderless, full-screen window for your experiment.
	This function registers the window with EXPTSPPT so it may be used for
	calibration and drift correction.  The window should not be destroyed until
	it is released with \c close_expt_graphics(). This window will be subclassed
	(some messages intercepted) during calibration and drift correction.

	@param hwnd Handle of window that is to be used for calibration and drift
				correction.  This should be a borderless, full-screen window.
				If your language can't give you a window handle, use \c NULL and
				the topmost window will be detected.
	@param info \c NULL or pointer to a \c DISPLAYINFO structure to fill with
				display mode data.
	@return \c 0 if success, \c -1 if error occurred internally.

 */
INT16 ELCALLTYPE  init_expt_graphics(HWND hwnd, DISPLAYINFO *info);

/*! Call this function at the end of the experiment or before destroying the
	window registered with \c init_expt_graphics().  This call will disable
	calibration and drift correction until a new window is registered.
 */
void  ELCALLTYPE  close_expt_graphics(void);


/*! This function will not return until the current refresh of the monitor has
	completed (at the start of vertical retrace).  This can be used to synchronize
	drawing to the scanning out of the display, and to determine when a stimulus
	was first seen by the subject.  The DriverLinx PortIO driver must be installed
	for this function to work.

	@remarks This is function is only applicable for the GDI version.
 */
void ELCALLTYPE wait_for_video_refresh(void);


/*****************************************************************************
 * Note: The old process_key_messages is renamed due to the borland compiler
 *    links this function to some system library that we do not know.
 *****************************************************************************/
/*! Call this function in your window message processing function to handle
	\c WM_CHAR and \c WM_KEYDOWN messages.  These will be translated to an
	EyeLink key code and saved for \c getkey().

	@param hWnd Handle to window.  This message may be \c NULL.
	@param message Windows message.
	@param wParam First message parameter.
	@param lParam Second windows parameter.
	@return \c 0 or \c JUNK_KEY if no key generated.

	\b Example:
	<pre>
	\code
	// The following code illustrates the use of process_key_messages().
	// This would usually be called from a message or event handler (see the
	// w32_demo_window.c module) for a complete example

	#include <eyelink.h>
	switch (message)
	{
	case WM_KEYDOWN:
	case WM_CHAR:
	    // Processes key messages: these can be accessed by getkey()
	    process_key_messages(hwnd, message, wparam, lparam);
	    break;

	    ...
	    // Other windows messages and events
	}
	\endcode
	</pre>
	\sa \c getkey() and \c translate_key_messages()
 */
UINT16 ELCALLTYPE gdi_process_key_messages(HWND hWnd, UINT message,
                                     WPARAM wParam, LPARAM lParam);

/*! See \c gdi_process_key_messages()
 */
#define process_key_messages gdi_process_key_messages


/*!
Forces start of drawing, waits for drawing to finish
hwnd may be NULL for all displays together

@param hwnd Handle to the window.
 */
void ELCALLTYPE wait_for_drawing(HWND hwnd);


/*! This function saves the entire bitmap as a .BMP, .JPG, .PNG, or .TIF file,
	and transfers the image to tracker as backdrop for gaze cursors (See
	\c bitmap_save() and \c bitmap_to_backdrop() for more information).

	@param hbm Handle to the bitmap image.
	@param xs Specifies the x-coordinate of the upper-left corner of the source
				bitmap.
	@param ys Specifies the y-coordinate of the upper-left corner of the source
				bitmap.
	@param width Specify the width of the source image to be copied  (set to \c 0
				to use all).
	@param height Specify the height of the source image to be copied  (set to
				\c 0 to use all).
	@param fname Name of the image file to be saved.  Currently, only .PNG, .BMP,
				.JPG, and .TIF files are saved.
	@param path Directory or drive path in quotes ("." for current directory).
	@param sv_options Use \c SV_NOREPLACE if not to replace an existing file;
				use \c SV_MAKEPATH to create a new path.
	@param xd Specifies the x-coordinate of the upper-left corner of the tracker
				screen.
	@param yd Specifies the y-coordinate of the upper-left corner of the tracker
				screen.
	@param bx_options Set with a bitwise OR of the following constants:
				\arg \c BX_MAXCONTRAST: Maximizes contrast for clearest image.
				\arg \c BX_AVERAGE: averages combined pixels.
				\arg \c BX_DARKEN: chooses darkest (keep thin dark lines).
				\arg \c BX_LIGHTEN: chooses darkest (keep thin white lines).
				\arg \c BX_NODITHER: disables dithering to get clearest text.
				\arg \c BX_GREYSCALE: converts to grayscale.
	@return \c 0 if successful, \c -1 if couldn't save, \c -2 if couldn't transfer.
 */
int ELCALLTYPE gdi_bitmap_save_and_backdrop(HBITMAP hbm, INT16 xs, INT16 ys,
                                   INT16 width, INT16 height,char *fname,
                                   char *path, INT16 sv_options,INT16 xd,
                                   INT16 yd, UINT16 bx_options);

/*! This function transfers the bitmap to the tracker PC as backdrop for gaze cursors.
	The field "bx_options", set with bitwise OR of the following constants, determines
	how bitmap is processed: \c BX_AVERAGE (averaging combined pixels), \c BX_DARKEN
	(choosing darkest and keep thin dark lines), and \c BX_LIGHTEN (choosing darkest
	and keep thin white lines) control how bitmap size is reduced to fit tracker display;
	\c BX_MAXCONTRAST maximizes contrast for clearest image; \c BX_NODITHER disables
	the dithering of the image; \c BX_GREYSCALE converts the image to grayscale
	(grayscale works best for EyeLink I, text, etc.)

	@param hbm Handle to the bitmap image.
	@param xs Specifies the x-coordinate of the upper-left corner of the source bitmap.
	@param ys Specifies the y-coordinate of the upper-left corner of the source bitmap.
	@param width Specify the width of the source image to be copied  (set to \c 0 to
				use all).
	@param height Specify the height of the source image to be copied  (set to \c 0 to
				use all).
	@param xd Specifies the x-coordinate of the upper-left corner of the tracker screen.
	@param yd Specifies the y-coordinate of the upper-left corner of the tracker screen.
	@param bx_options Set with a bitwise OR of the following constants:
				\arg \c BX_MAXCONTRAST: Maximizes contrast for clearest image.
				\arg \c BX_AVERAGE: averages combined pixels.
				\arg \c BX_DARKEN: chooses darkest (keep thin dark lines).
				\arg \c BX_LIGHTEN: chooses darkest (keep thin white lines).
				\arg \c BX_NODITHER: disables dithering to get clearest text.
				\arg \c BX_GREYSCALE: converts to grayscale.
	@return \c 0 if successful, else \c -1 or \c -2.
 */
int ELCALLTYPE gdi_bitmap_to_backdrop(HBITMAP hbm, INT16 xs, INT16 ys,
                                   INT16 width, INT16 height,INT16 xd,
                                   INT16 yd, UINT16 bx_options);

/*! This function saves the entire bitmap or selected part of a bitmap in an image file
	(with an extension of .png, .bmp, .jpg, or .tif). It creates the specified file if
	this file does not exist.  If the file exists, it replaces the file unless
	\c SV_NOREPLACE is specified in the field of "sv_options". The directory to which
	the file will be written is specified in the path field.

	@param hbm Handle to the bitmap image.
	@param xs Specifies the x-coordinate of the upper-left corner of the source bitmap.
	@param ys Specifies the y-coordinate of the upper-left corner of the source bitmap.
	@param width Specify the width of the source image to be copied  (set to \c 0 to use all).
	@param height Specify the height of the source image to be copied  (set to \c 0 to use all).
	@param fname Name of the image file to be saved.  Currently, only .PNG, .BMP, .JPG,
				and .TIF files are saved.
	@param path Directory or drive path in quotes ("." for current directory).
	@param sv_options Use \c SV_NOREPLACE if not to replace an existing file; use \c SV_MAKEPATH to create a new path.
	@return \c 0 if successful, else \c -1.
 */
int ELCALLTYPE gdi_bitmap_save(HBITMAP hbm, INT16 xs, INT16 ys,
                                   INT16 width, INT16 height,char *fname,
                                   char *path, INT16 sv_options);



/* convenient and compatibility macros*/
/*! See \c gdi_bitmap_save_and_backdrop()
 */
#define bitmap_save_and_backdrop gdi_bitmap_save_and_backdrop

/*! See \c gdi_bitmap_to_backdrop()
 */
#define bitmap_to_backdrop       gdi_bitmap_to_backdrop

/*! See \c gdi_bitmap_save()
 */
#define bitmap_save              gdi_bitmap_save

/* the new message pump does not need a parameter */
/*! See \c message_pump()
 */
#define message_pump(x) message_pump()


/* dialog to take in experiment name or file name. */
//INT16 ELCALLTYPE edit_dialog(HWND hwnd, LPSTR title, LPSTR msg, LPSTR txt,
//                           INT16 maxsize);


/********************************************************************************
 * The following are for backwords compatibility.
 * ie. the old eyelink_exptkit20.dll
 ********************************************************************************/
extern DISPLAYINFO dispinfo;
#ifdef CALLTYPE
    #undef CALLTYPE
#endif
#define CALLTYPE ELCALLTYPE




/********************************************************************************
 * The following hook functions are only for backwards compatibility.
 * One should really use the setup_graphic_hook_functions and
 * get_all_hook_functions modify the hook functions.
 ********************************************************************************/
#define HOOK_ERROR    -1  /* if error occurred                                */
#define HOOK_CONTINUE  0  /* if drawing to continue after return from hook    */
#define HOOK_NODRAW    1  /* if drawing should not be done after hook         */



/***********************************************************************************
 * These are the constants in the argument to cal_sound_hook():
 ***********************************************************************************/

#define CAL_TARG_BEEP   1
#define CAL_GOOD_BEEP   0
#define CAL_ERR_BEEP   -1
#define DC_TARG_BEEP    3
#define DC_GOOD_BEEP    2
#define DC_ERR_BEEP    -2

/*! \deprecated See \c setup_graphic_hook_functions() and \c get_all_hook_functions().

To modify the behaviour of \c setup_cal_display() function.

@param hookfn Function to replace \c setup_cal_display().
@param options For future use.

 */

INT16 ELCALLTYPE set_setup_cal_display_hook(
    INT16 (ELCALLBACK * hookfn)(void), INT16 options);

/*! \deprecated See \c setup_graphic_hook_functions() and \c get_all_hook_functions().

To modify the behaviour of \c clear_cal_display() function.

@param hookfn Function to replace \c clear_cal_display().
@param options For future use.
 */
INT16 ELCALLTYPE set_clear_cal_display_hook(
    INT16 (ELCALLBACK * hookfn)(HDC hdc), INT16 options);

/*! \deprecated See \c setup_graphic_hook_functions() and \c get_all_hook_functions().

To modify the behaviour of \c erase_cal_target() function.

@param hookfn Function to replace \c erase_cal_target().
@param options For future use.
 */
INT16 CALLTYPE set_erase_cal_target_hook(
    INT16 (ELCALLBACK * hookfn)(HDC hdc), INT16 options);

/*! \deprecated See \c setup_graphic_hook_functions() and \c get_all_hook_functions().
To modify the behaviour of \c draw_cal_target() function.

@param hookfn Function to replace \c draw_cal_target().
@param options For future use.
 */
INT16 CALLTYPE set_draw_cal_target_hook(
    INT16 (ELCALLBACK * hookfn)(HDC hdc, INT16 * x, INT16 * y), INT16 options);

/*! \deprecated See \c setup_graphic_hook_functions() and \c get_all_hook_functions().

To modify the behaviour of \c exit_cal_display() function.

@param hookfn Function to replace \c exit_cal_display().
@param options For future use.
 */
INT16 ELCALLTYPE set_exit_cal_display_hook(
    INT16 (ELCALLBACK * hookfn)(void), INT16 options);


/*! \deprecated See \c setup_graphic_hook_functions() and \c get_all_hook_functions().

To modify the behaviour of \c cal_sound() function.

@param hookfn Function to replace \c cal_sound().
@param options For future use.
 */
INT16 CALLTYPE set_cal_sound_hook(
	INT16 (ELCALLBACK * hookfn)(INT16 * error), INT16 options);

/*! \deprecated See \c setup_graphic_hook_functions() and \c get_all_hook_functions().

To modify the behaviour of \c record_abort_hide() function.

@param hookfn Function to replace \c record_abort_hide().
@param options For future use.
 */
INT16 ELCALLTYPE set_record_abort_hide_hook(
    INT16 (ELCALLBACK * hookfn)(void), INT16 options);

/*! \deprecated See \c setup_graphic_hook_functions() and \c get_all_hook_functions().

To modify the behaviour of \c setup_image_display() function.

@param hookfn Function to replace \c setup_image_display().
@param options For future use.
 */
INT16 ELCALLTYPE set_setup_image_display_hook(
    INT16 (ELCALLBACK * hookfn)(INT16 width, INT16 height), INT16 options);

/*! \deprecated See \c setup_graphic_hook_functions() and \c get_all_hook_functions().

To modify the behaviour of \c image_title() function.

@param hookfn Function to replace \c image_title().
@param options For future use.
 */
INT16 ELCALLTYPE set_image_title_hook(
    INT16 (ELCALLBACK * hookfn)(INT16 threshold, char *cam_name), INT16 options);

/*! \deprecated See \c setup_graphic_hook_functions() and \c get_all_hook_functions().

To modify the behaviour of \c draw_image_line() function.

@param hookfn Function to replace \c draw_image_line().
@param options For future use.
 */
INT16 ELCALLTYPE set_draw_image_line_hook(
    INT16 (ELCALLBACK * hookfn)(INT16 width, INT16 line, INT16 totlines,
                                byte *pixels), INT16 options);

/*! \deprecated See \c setup_graphic_hook_functions() and \c get_all_hook_functions().

To modify the behaviour of \c set_image_palette() function.

@param hookfn Function to replace \c set_image_palette().
@param options For future use.
 */
INT16 ELCALLTYPE set_set_image_palette_hook(
    INT16 (ELCALLBACK * hookfn)(INT16 ncolors, byte r[], byte g[], byte b[]),
    INT16 options);

/*! \deprecated See \c setup_graphic_hook_functions() and \c get_all_hook_functions().

To modify the behaviour of \c exit_image_display() function.

@param hookfn Function to replace \c exit_image_display().
@param options For future use.
 */
INT16 ELCALLTYPE set_exit_image_display_hook(
    INT16 (ELCALLBACK * hookfn)(void), INT16 options);


/***********************************************************************************
 * GAZE-CONTINGENT WINDOW
 ***********************************************************************************/

/*!
	\arg To initialize a gaze contengent window.
	\arg Initial setup of gaze-contingent window before drawing it.
	\arg Sets size of window, and whether it is a foveal mask.
 	\arg If height or width is \c -1, the window will be a bar covering the display.

	@param wwidth
	@param wheight
	@param window_bitmap
	@param background_bitmap
	@param window The window to display in.
	@param display_rect Area of display in window.
	@param is_mask
	@param deadband Sets number of pixels of anti-jitter applied.
 */
void ELCALLTYPE initialize_gc_window(int wwidth, int wheight,
                          HBITMAP window_bitmap, HBITMAP background_bitmap,
                          HWND window, RECT display_rect, int is_mask, int deadband);

/*!
	Set the location of the gaze window to this new position.  The first time
	window is drawn, the background outside the window will be filled in too.
	If x or y is \c MISSING_DATA (defined in eyelink.h), window is hidden.

 	@param x X location.
 	@param y Y location.
 */
void ELCALLTYPE redraw_gc_window(int x, int y);


/*!
	Gets a display context to draw on the page.  This is a static DC, so all
	settings from your last use are preserved.
 */
HDC ELCALLTYPE get_window_dc(void);


/*!
	Release the page DC
 */
INT16 ELCALLTYPE release_window_dc(HDC hdc);

/*!
  *  @}
  */ // end of gdi_grahics

// ============= Start of Folder Support ============
/*
* Returns the path to the output folder.
* For macos and linux, default output folder is ~/Documents/<exe name>_<date string>
* For Windows default output folder behaviour is as follows.
*take the first pass case. ie. if item #1 passes, take it otherwise move on to the next item.
*1. if set_output_folder is called use that path.
*2. Can the current directory writeable. If yes, use the current directory as output_folder. (This will handle the 32bit application case)
*3. Can we write to the PublicDocuments folder? if yes, use the public documents folder
*4. Can we write to the Documents folder? if yes use the documents folder
*5. If we reach here that means we cannot write anywhere. We can attempt the desktop, but I guess, if documents folder is not writeable, then chances are we cannot write anywhere.

*/
const char * get_output_folder(void);

/*
Allow one to set the output folder.  If output folder needs to be created, call this before calling initialize_sdl_util.
Otherwise, this only sets teh output folder.
*/
int set_output_folder(const char *path);

/*
* Convenient function to copy a resource to output folder.
*/
int copy_resource_to_output_folder(const char *resource);


/*
* Concenient function to show the output folder.
*
*/
int open_output_folder(void);
/*
*initializes the folder util optionally creates the output folder.
*returns non zero on success. 0 if failed.
* see get_output_folder() for more detail about the get_output_folder behaviour.
* exe_path is the path of the executable. eg. argv[0] of the main().
* output_folder if None NULL, use this as the default output folder.
*/
int initialize_folder(const char *exe_path, const char *output_folder, int create_output_folder);

// ================== End of Folder Support ===========

#include "w32_dialogs.h" /* for compatibility */
#ifdef __cplusplus     /* For C++ compilation */
};
#endif
#endif
