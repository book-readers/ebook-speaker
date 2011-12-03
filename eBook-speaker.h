/* header file for eBook-speaker
 *  Copyright (C) 2011 J. Lemmens
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
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
#include "entities.h"
#include <stringprep.h>
#include <magic.h>
#include <sox.h>
#include <errno.h>
#include <zip.h>

void playfile (char *, char *);
char *realname (char *);
double read_time (char *);
void get_clips ();
void put_bookmark ();
void get_bookmark ();
void get_attributes (char *);
int get_tag_or_label (struct zip_file *);
void get_tag ();
void get_page_number ();
void read_eBook_struct ();
int get_next_clips (struct zip_file *);
void view_screen ();
void get_label (int, int);
void player_ended ();
void play_now ();
void open_text_file (char *, char *);
void pause_resume ();
void write_wav (char *, char *);
void help ();
void previous_item ();
void next_item ();
void play_clip (struct zip_file *);
void skip_left ();
void skip_right ();
void change_level (char);
void read_rc ();
void save_rc ();
void quit_eBook_reader ();
void search (int , char);
void kill_player ();
void go_to_page_number ();
void select_next_output_device ();
void browse ();
void usage (char *);
char *sort_by_playorder ();
void read_out_eBook (const char *);
const char *read_eBook (char *);
void get_eBook_struct (int);
int get_element_tag_or_label (struct zip_file *);

struct
{                    
   int x, y, screen, n_phrases;
   char text_file[255], anchor[255], label[255];
   char class[100];
   int level, page_number;
} eBook_struct[2000];
