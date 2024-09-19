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

/*******************************************************************************
 * EYELINK EXPT SUPPORT: SDL PLATFORM (Coppied from WIN32 PLATFORM)			   *
 * EYELINK II: 3 February 2003                                                 *
 * Provides text support for sdl							                   *
 *                                                                             *
 *                                                                             *
 **************************************** WARNING ******************************
 *                                                                             *
 * UNDER NO CIRCUMSTANCES SHOULD PARTS OF THESE FILES BE COPIED OR COMBINED.   *
 * This will make your code impossible to upgrade to new releases in the future*
 * and SR Research will not give tech support for reorganized code.            *
 *                                                                             *
 * This file should not be modified. If you must modify it, copy the entire    *
 * file with a new name, and change the the new file.                          *
 *                                                                             *
 *******************************************************************************/



#ifndef __SR_RESEARCH_SDL_TEXT_SUPPORT_H__
#define __SR_RESEARCH_SDL_TEXT_SUPPORT_H__
#include <SDL2/SDL.h>

#ifdef __cplusplus     /* For C++ compilation */
extern "C" {
#endif

typedef enum
{
	NONE          = 0x000, /* no justification. just place the text a the given point */
	JUSTIFY_RIGHT = 0x001, /* right justify the text */
	CENTER        = 0x010, /* horizontally center the text */
	WRAP          = 0x100  /* wrap the text if it cannot fit in one line. */
} PRINTFORMAT;


/*****************************************************************************
 * Function: set_margin
 * Parameters:
 *		left:	Left margin
 *		right:	Right margin
 *      top:	Top margin
 *      bottom: Bottom margin
 * Purpose:
 *		set margin for the text to print.
 *****************************************************************************/
void set_margin(int left, int right, int top, int bottom);

/*****************************************************************************
 * Function: set_line_spacing
 * Parameters:
 *		linespace:	line spacing - percentage of line height
 * Purpose:
 *		Adjust the line spacing
 *****************************************************************************/
void set_line_spacing(double linespace);

/*****************************************************************************
 * Function: get_new_font
 * Parameters:
 *		fontname:	name of the font. Not the name of the font file name
 *		font_size:	font size
 *      bold:	bold !=0 means bold font otherwise regular font.
 * Purpose:
 *		close the current font and open up a new font.
 *****************************************************************************/
int get_new_font(char *fontname, int font_size, int bold);

/*****************************************************************************
 * Function: sdl2_printf
 * Parameters:
 *		surface: sdl surface to print on
 *		fg: foreground color
 *		format: format type. use any of the following and combined. NONE,
 *				JUSTIFY_RIGHT, CENTER, WRAP
 *		x,y: x,y position
 *		fmt: printf like format
 *		...: any parameter applicable to the fmt.
 * Purpose:
 *		Printf like print utility function
 *****************************************************************************/
//void graphic_printf(SDL_Surface *surface, SDL_Color fg, int format, int x,
//					int y, const char *fmt, ...);
void sdl2_printf(SDL_Window *window, SDL_Color fg, int format, int x, int y, const char *fmt, ...);

/*****************************************************************************
 * Function: get_font_height
 * Parameters:
 *		None
 * Purpose:
 *		Returns the font size.
 *****************************************************************************/
int get_font_height(void);
/*
 * Returns the resources path for the application
 * for macos this will be .app/Contents/Resources
 * for windows and linux this will be the folder of the executable.
 */
const char * get_resources_path(void);

/*
 * Allow to add font search folder. By default fonts are search in
 * get_resources_path() and some system folders.
 * in windows <windir>\Fonts
 * in mac, we should technically search /System/Library/Fonts, /Library/Fonts and ~/Library/Fonts
 * However, in recent macs there does not seem to be any ttf files in these foledrs.  So they are not
 * searched.
 */
void add_font_search_path(const char *path);

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
 *initializes the sdl util and optionally creates the output folder.
 *returns non zero on success. 0 if failed.
 * see get_output_folder() for more detail about the get_output_folder behaviour.
 * exe_path is the path of the executable. eg. argv[0] of the main().
 * output_folder if None NULL, use this as the default output folder.
 */
int initialize_sdl_util(const char *exe_path, const char *output_folder, int create_output_folder);


/*
 * Convenient function to copy a resource to output folder.
 */
int copy_resource_to_output_folder(const char *resource);


/*
 * Concenient function to show the output folder.
 *
 */
int open_output_folder(void);

#ifdef __cplusplus     /* For C++ compilation */
}
#endif

#endif

