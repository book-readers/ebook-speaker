/* header file for daisy-player and eBook-speaker
 *  Copyright (C) 2013 J. Lemmens
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
#include <sys/mount.h>

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

void put_bookmark ();
void get_bookmark ();
void get_tag ();
void get_page_number ();
void view_screen ();
void player_ended ();
void play_now ();
void open_text_file (char *, char *);
void pause_resume ();
void help ();
void previous_item ();
void next_item ();
void skip_left ();
void skip_right ();
void change_level (char);
void read_rc ();
void get_label (int, int);
void save_rc ();
void search (int , char);
void kill_player ();
void go_to_page_number ();
void select_next_output_device ();
void browse ();
char *sort_by_playorder ();
void read_out_eBook (const char *);
const char *read_eBook (char *);
void get_eBook_struct (int);
void parse_smil ();
void start_element (void *, const char *, const char **);
void end_element (void *, const char *);
