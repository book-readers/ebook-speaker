/* header file for eBook-speaker
 *  Copyright (C) 2014 J. Lemmens
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ncursesw/curses.h>
#include <fcntl.h>
#include <libgen.h>
#include <dirent.h>
#include <pwd.h>
#include <locale.h>
#include <libintl.h>
#include <sox.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <cdio/cdio.h>
#include <cdio/cdda.h>
#include <cdio/paranoia.h>
#include <cdio/disc.h>
#include <magic.h>

#define max_phrase_len 50000
#define MAX_CMD 512
#define MAX_STR 256
#define MAX_TAG 1024

typedef struct daisy
{
   int playorder, x, y, screen, n_phrases;
   float begin, duration;
   char smil_file[MAX_STR], anchor[MAX_STR], class[MAX_STR];
   char label[max_phrase_len];
   int level, page_number;
   char daisy_mp[MAX_STR]; // discinfo
   char filename[MAX_STR]; // Audio-CD
   lsn_t first_lsn, last_lsn;
} daisy_t;

typedef struct my_attribute
{
   char class[MAX_STR],
        clip_begin[MAX_STR],
        clip_end[MAX_STR],
        content[MAX_STR],
        dc_format[MAX_STR],
        dc_title[MAX_STR],
        dtb_depth[MAX_STR],
        dtb_totalPageCount[MAX_STR],
        href[MAX_STR],
        http_equiv[MAX_STR],
        id[MAX_STR],
        idref[MAX_STR],
        media_type[MAX_STR],
        name[MAX_STR],
        ncc_depth[MAX_STR],
        ncc_maxPageNormal[MAX_STR],
        ncc_totalTime[MAX_STR],
        number[MAX_STR],
        playorder[MAX_STR],
        src[MAX_STR],
        smilref[MAX_STR],
        toc[MAX_STR],
        value[MAX_STR];
} my_attribute_t;

typedef struct misc
{
   int discinfo, playing, just_this_item, audiocd, current_page_number; 
   int current, max_y, max_x, total_items, level, displaying;
   int phrase_nr, tts_no, depth, total_phrases, total_pages;
   int pipefd[2], tmp_wav_fd, tts_flag, scan_flag;
   float speed, total_time;
   float clip_begin, clip_end, start_time;
   xmlTextReaderPtr reader;
   pid_t daisy_player_pid, player_pid, eBook_speaker_pid;
   char NCC_HTML[MAX_STR], ncc_totalTime[MAX_STR];
   char daisy_version[MAX_STR], daisy_title[MAX_STR], daisy_language[MAX_STR];
   char daisy_mp[MAX_STR], *tmp_dir;
   char tag[MAX_TAG], label[max_phrase_len], ocr_language[5];
   char bookmark_title[MAX_STR], search_str[MAX_STR];
   char *wd, cd_dev[MAX_STR], sound_dev[MAX_STR], tts[10][MAX_STR];
   char cddb_flag, opf_name[MAX_STR], ncx_name[MAX_STR], copyright[MAX_STR];
   char current_audio_file[MAX_STR], tmp_wav[MAX_STR + 1], mcn[MAX_STR];
   time_t seconds;
   WINDOW *screenwin, *titlewin;
   lsn_t lsn_cursor;
} misc_t;
