/* header file for eBook-speaker
 *  Copyright (C) 2016 J. Lemmens
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

#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_URL
#undef PACKAGE_VERSION
#undef VERSION
#include "config.h"

#define MAX_CMD 512
#define MAX_STR 256
#define MAX_TAG 1024

#ifndef EBOOK_SPEAKER
#   define EBOOK_SPEAKER
#endif

typedef struct My_attribute
{
   char class[MAX_STR],
        my_class[MAX_STR],
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
        smilref[MAX_STR],
        src[MAX_STR],
        toc[MAX_STR],
        value[MAX_STR];
} my_attribute_t;

typedef struct Misc
{
   int discinfo, playing, just_this_item, current_page_number;
   int current, max_y, max_x, total_items, level, displaying, ignore_bookmark;
   int phrase_nr, tts_no, option_t, depth, total_phrases, total_pages;
   int option_b, pipefd[2], tmp_wav_fd, scan_flag, label_len;
   int items_in_opf, items_in_ncx, show_hidden_files, list_total;
   int phrases_in_opf, phrases_in_ncx;
   float speed, volume;
   xmlTextReaderPtr reader;
   pid_t daisy_player_pid, player_pid, eBook_speaker_pid;
   char NCC_HTML[MAX_STR], ncc_totalTime[MAX_STR];
   char daisy_version[MAX_STR], daisy_title[MAX_STR], daisy_language[MAX_STR];
   char tag[MAX_TAG], *label, ocr_language[5];
   char bookmark_title[MAX_STR], search_str[MAX_STR];
   char cd_dev[MAX_STR], sound_dev[MAX_STR], tts[10][MAX_STR];
   char cddb_flag, opf_name[MAX_STR], ncx_name[MAX_STR], copyright[MAX_STR];
   char eBook_speaker_txt[MAX_STR + 1], eBook_speaker_wav[MAX_STR + 1];
   char first_eBook_speaker_wav[MAX_STR + 1];
   char cmd[MAX_CMD], src_dir[MAX_STR + 1], *tmp_dir, daisy_mp[MAX_STR + 1];
   char orig_file[MAX_STR + 1];
   char orig_epub[MAX_STR + 1];
   char locale[MAX_STR + 1];
   char xmlversion[MAX_STR + 1], encoding[MAX_STR + 1];
   char standalone[MAX_STR + 1];
   char break_on_EOL;
   WINDOW *screenwin, *titlewin;
} misc_t;

typedef struct Daisy
{
   int playorder, x, y, screen, n_phrases;
   char smil_file[MAX_STR + 1], anchor[MAX_STR], my_class[MAX_STR];
   char orig_smil[MAX_STR], label[100];
   int level, page_number;
} daisy_t;

extern void quit_eBook_speaker (misc_t *);
extern void parse_ncx (misc_t *, my_attribute_t *, daisy_t *);
extern void parse_text (misc_t *, my_attribute_t *, daisy_t *);
extern void get_label (misc_t *, my_attribute_t *, daisy_t *, int,
                       xmlTextReaderPtr);
extern void view_screen (misc_t *, daisy_t *);                   
extern void check_phrases (misc_t *, my_attribute_t *, daisy_t *);
extern void go_to_phrase (misc_t *, my_attribute_t *, daisy_t *, int);
extern void kill_player (misc_t *);
extern void pandoc_to_epub (misc_t *, char *, char *);
extern void save_xml (misc_t *);
extern void select_tts (misc_t *, daisy_t *);
extern void playfile (misc_t *);
extern void read_daisy_3 (misc_t *, my_attribute_t *, daisy_t *);
extern int get_tag_or_label (misc_t *, my_attribute_t *, xmlTextReaderPtr);
extern void save_bookmark_and_xml (misc_t *);
extern void open_text_file (misc_t *, my_attribute_t *,
                            char *, char *);
extern char *get_input_file (misc_t *, char *);
extern void put_bookmark (misc_t *);
extern void skip_left (misc_t *, my_attribute_t *, daisy_t *);
extern void skip_right (misc_t *);
extern daisy_t *create_daisy_struct (misc_t *, my_attribute_t *);
extern void store_as_WAV_file (misc_t *, my_attribute_t *, daisy_t *);
extern void store_as_ASCII_file (misc_t *, my_attribute_t *, daisy_t *);
extern void failure (char *, int);
extern char *lowriter_to (char *);
extern void remove_double_tts_entries (misc_t *);
extern void remove_tmp_dir (misc_t *);
extern char *get_file_type (char *);
extern int hidden_files (const struct dirent *);
extern void save_original_XMLs (misc_t *);
extern void play_epub (misc_t *, my_attribute_t *, daisy_t *daisy, char *);
extern void create_epub (misc_t *, my_attribute_t *, daisy_t *, char *, int);
extern void count_phrases (misc_t *, my_attribute_t *, daisy_t *);
