/* header file for eBook-speaker
 *
 * Copyright (C) 2017 J. Lemmens
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

#define _GNU_SOURCE         /* See feature_test_macros(7) */
#define MAGIC_FLAGS MAGIC_CONTINUE | MAGIC_SYMLINK | MAGIC_NO_CHECK_CDF

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#define __STDC_WANT_IEC_60559_BFP_EXT__
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
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <libxml/HTMLparser.h>
#include <magic.h>
#include <fnmatch.h>
#include <grp.h>
#include <time.h>

#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_URL
#undef PACKAGE_VERSION
#undef VERSION
#include "config.h"

#define MAX_CMD 2000
#define MAX_STR 512
#define MAX_TAG 1024

typedef struct My_attribute
{
   char class[MAX_STR],
        my_class[MAX_STR],
        content[MAX_STR],
        dc_format[MAX_STR],
        dc_title[MAX_STR],
        dtb_depth[MAX_STR],
        dtb_totalPageCount[MAX_STR],
        href[MAX_STR],
        http_equiv[MAX_STR],
        *id,
        *idref,
        media_type[MAX_STR],
        name[MAX_STR],
        ncc_depth[MAX_STR],
        ncc_maxPageNormal[MAX_STR],
        ncc_totalTime[MAX_STR],
        number[MAX_STR],
        playorder[MAX_STR],
        smilref[MAX_STR],
        *src,
        toc[MAX_STR],
        value[MAX_STR];
} my_attribute_t;

typedef struct Misc
{
   int playing, just_this_item, current_page_number;
   int current, max_y, max_x, total_items, level, ignore_bookmark;
   int phrase_nr, attr_tts_no, tts_no, depth, total_phrases, total_pages;
   int option_b, tmp_wav_fd, scan_flag;
   int items_in_opf, items_in_ncx, show_hidden_files, list_total;
   int total_phrases_in_opf, total_phrases_in_ncx;
   int use_cuneiform, pause_resume_playing, break_phrase;
   int ncx_failed, opf_failed, verbose;
   int start_arg_is_a_dir, no_smil_items, update_progress;
   float speed;
   htmlDocPtr doc;
   xmlTextReaderPtr reader;
   pid_t player_pid, main_pid;
   int pulseaudio_device;
   char ncc_html[MAX_STR], ncc_totalTime[MAX_STR];
   char daisy_version[MAX_STR], daisy_title[MAX_STR], daisy_language[MAX_STR];
   char tag[MAX_TAG], *label, ocr_language[5];
   char bookmark_title[MAX_STR];
   char cd_dev[MAX_STR], tts[15][MAX_STR];
   char option_t[MAX_STR];
   char opf_name[MAX_STR], ncx_name[MAX_STR], copyright[MAX_STR], option_d[5];
   char eBook_speaker_txt[MAX_STR + 1], tmp_wav[MAX_STR + 1];
   char cmd[MAX_CMD + 1], str[MAX_STR + 1];
   char *src_dir, *tmp_dir, *daisy_mp;
   char orig_input_file[MAX_STR], search_str[30];
   char locale[25], xmlversion[MAX_STR + 1], encoding[MAX_STR + 1];
   char standalone[MAX_STR + 1], scan_resolution[10];
   WINDOW *screenwin, *titlewin;
   int use_OPF, use_NCX; // for testing
   int term_signaled;
} misc_t;

typedef struct Daisy
{
   int playorder, x, y, screen, n_phrases;
   char *smil_file, *smil_anchor;
   char *xml_file, *xml_anchor, *orig_xml_file;
   char class[MAX_STR], my_class[MAX_STR];
   char label[100];
   char first_id[MAX_STR], last_id[MAX_STR];
   int level, page_number;
} daisy_t;

extern int hidden_files (const struct dirent *);
extern void failure (misc_t *, char *, int);
extern void remove_tmp_dir (misc_t *);
extern int get_tag_or_label (misc_t *, my_attribute_t *, xmlTextReaderPtr);
extern void quit_eBook_speaker (misc_t *, my_attribute_t *, daisy_t *);
extern void view_screen (misc_t *, daisy_t *);
extern void pause_resume (misc_t *, my_attribute_t *, daisy_t *);
extern void open_xml_file (misc_t *, my_attribute_t *, daisy_t *,
                           char *, char *);
extern void get_next_phrase (misc_t *, my_attribute_t *, daisy_t *, int);
extern void get_path_name (char *, char *, char *);
extern char *convert_URL_name (misc_t *, char *);
extern void kill_player (misc_t *);
extern int get_page_number_2 (misc_t *, my_attribute_t *, daisy_t *, char *);
extern int get_page_number_3 (misc_t *, my_attribute_t *);        
extern void playfile (misc_t *, char *, char *, char *, char *, char *);
extern void fill_page_numbers (misc_t *, daisy_t *, my_attribute_t *);
extern void check_pulseaudio_device (misc_t *, daisy_t *);
extern void select_next_output_device (misc_t *, daisy_t *);
extern void go_to_page_number (misc_t *, my_attribute_t *, daisy_t *);
extern daisy_t *create_daisy_struct (misc_t *, my_attribute_t *, daisy_t *);
extern void read_daisy_3 (misc_t *, my_attribute_t *, daisy_t *);
extern void skip_right (misc_t *, daisy_t *);
extern char *pactl (char *, char *, char *);
extern char *get_input_file (misc_t *, my_attribute_t *, daisy_t *, char *);
extern void fill_daisy_struct_2 (misc_t *, my_attribute_t *,daisy_t *);
extern void make_tmp_dir (misc_t *);
extern void player_ended ();
extern void parse_page_number (misc_t *, my_attribute_t *, xmlTextReaderPtr);
extern void parse_ncx (misc_t *, my_attribute_t *, daisy_t *);
extern void save_bookmark (misc_t *);
extern void free_all (misc_t *, my_attribute_t *, daisy_t *);
extern void start_playing (misc_t *, my_attribute_t *, daisy_t *);
extern void select_tts (misc_t *, daisy_t *);
extern void put_bookmark (misc_t *);
extern void go_to_phrase (misc_t *, my_attribute_t *, daisy_t *, int, int);
extern void save_xml (misc_t *);
extern void check_phrases (misc_t *, my_attribute_t *, daisy_t *);
extern void pandoc_to_epub (misc_t *, char *, char *);
extern void store_item_as_WAV_file (misc_t *, my_attribute_t *, daisy_t *);
extern void store_item_as_ASCII_file (misc_t *, my_attribute_t *, daisy_t *);
extern void create_epub (misc_t *, my_attribute_t *, daisy_t *, char *, int);
extern void play_epub (misc_t *, my_attribute_t *, daisy_t *, char *);
extern char *ascii2html (misc_t *, char *);
extern void write_ascii (misc_t *, my_attribute_t *, daisy_t *, int, char *);
extern void get_real_pathname (char *, char *, char *);
extern void parse_opf (misc_t *, my_attribute_t *, daisy_t *);
extern void reset_term_signal_handlers_after_fork (void);
extern void parse_smil_opf (misc_t *, my_attribute_t *, daisy_t *, int);
extern void show_progress (misc_t *, daisy_t *);
