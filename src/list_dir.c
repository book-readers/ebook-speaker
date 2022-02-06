/* eBook-speaker - read aloud an eBook using a speech synthesizer
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

#include "daisy.h"

void fill_ls (int total, struct dirent **namelist, char **file_type)
{
   magic_t myt;
   int n;

   myt = magic_open (MAGIC_CONTINUE | MAGIC_SYMLINK);
   magic_load (myt, NULL);
   for (n = 0; n <= total; n++)
   {
      file_type[n] = malloc (50);
      strncpy (file_type[n], magic_file (myt, namelist[n]->d_name), 40);
   } // for
   magic_close (myt);
} // fill_ls

void ls (int n, int total, struct dirent **namelist, char **file_type,
         WINDOW *titlewin, WINDOW *screenwin)
{
   int y, page;

   page = n / 23 + 1;
   mvwprintw (titlewin, 0, 73, "%3d/%3d", page, (total - 1) / 23 + 1);
   wrefresh (titlewin);
   wclear (screenwin);
   for (y = 0; y < 23; y++)
   {
      int index;

      index = (page - 1) * 23 + y;
      if (index > total)
         break;
      mvwprintw (screenwin, y, 1, "%.39s", namelist[index]->d_name);
      mvwprintw (screenwin, y, 41, "%.39s", file_type[index]);
   } // for
   wmove (screenwin, n - (page - 1) * 23, 0);
} // ls

int hidden_files (const struct dirent *entry)
{
   if (*entry->d_name == '.')
      return 0;
   return 1;
} // hidden)files

int search_in_dir (int start, int total, char mode, WINDOW *titlewin,
             char *search_str, struct dirent **namelist)
{
   static int c;
   int found = 0;

   if (*search_str == 0)
   {
      mvwaddstr (titlewin, 1, 0, "----------------------------------------");
      waddstr (titlewin, "----------------------------------------");
      mvwprintw (titlewin, 1, 0, gettext ("What do you search? "));
      echo ();
      wgetnstr (titlewin, search_str, 25);
      noecho ();
   } // if
   if (mode == '/' || mode == 'n')
   {
      for (c = start; c <= total; c++)
      {
         if (strcasestr (namelist[c]->d_name, search_str))
         {
            found = 1;
            break;
         } // if
      } // for
      if (! found)
      {
         for (c = 0; c < start; c++)
         {
            if (strcasestr (namelist[c]->d_name, search_str))
            {
               found = 1;
               break;
            } // if
         } // for
      } // if
   }
   else
   { // mode == 'N'
      for (c = start; c > 0; c--)
      {
         if (strcasestr (namelist[c]->d_name, search_str))
         {
            found = 1;
            break;
         } // if
      } // for
      if (! found)
      {
         for (c = total - 1; c > start; c--)
         {
            if (strcasestr (namelist[c]->d_name, search_str))
            {
               found = 1;
               break;
            } // if
         } // for
      } // if
   } // if
   if (found)
   {
      return c;
   }
   else
   {
      beep ();
      return -1;
   } // if
} // search_in_dir

void help_list (WINDOW *screenwin)
{
   wclear (screenwin);
   waddstr (screenwin, gettext ("\nThese commands are available in this version:\n"));
   waddstr (screenwin, "========================================");
   waddstr (screenwin, "========================================\n");
   waddstr (screenwin, gettext ("cursor down     - move cursor to the next item\n"));
   waddstr (screenwin, gettext ("cursor up       - move cursor to the previous item\n"));
   waddstr (screenwin, gettext ("cursor right    - open this directory or file\n"));
   waddstr (screenwin, gettext ("cursor left     - open previous directory\n"));
   waddstr (screenwin, gettext ("page-down       - view next page\n"));
   waddstr (screenwin, gettext ("page-up         - view previous page\n"));
   waddstr (screenwin,
            gettext ("enter           - open this directory or file\n"));
   waddstr (screenwin, gettext ("/               - search for a label\n"));
   waddstr (screenwin, gettext ("B               - move cursor to the last item\n"));
   waddstr (screenwin, gettext ("h or ?          - give this help\n"));
   waddstr (screenwin, gettext ("H               - display \"hidden\" files on/off\n"));
   waddstr (screenwin, gettext ("n               - search forwards\n"));
   waddstr (screenwin, gettext ("N               - search backwards\n"));
   waddstr (screenwin, gettext ("q               - quit eBook-speaker\n"));
   waddstr (screenwin, gettext ("T               - move cursor to the first item\n"));
   waddstr (screenwin, gettext ("\nPress any key to leave help..."));
   nodelay (screenwin, FALSE);
   wgetch (screenwin);
   nodelay (screenwin, TRUE);
} // help_list

char *get_input_file (char *src_dir, char *copyright,
                      WINDOW *titlewin, WINDOW *screenwin)
{
   struct dirent **namelist;
   char *file_type[50], search_str[MAX_STR];
   int n, tot, page, show_hidden_files = 0;
   static char name[MAX_STR];

   nodelay (screenwin, FALSE);
   wclear (titlewin);
   mvwprintw (titlewin, 0, 0,
              gettext ("%s - Choose an input-file"), copyright);
   wrefresh (titlewin);
   if (show_hidden_files)
      tot = scandir (src_dir, &namelist, NULL, alphasort) - 1;
   else
      tot = scandir (src_dir, &namelist, &hidden_files, alphasort) - 1;
   fill_ls (tot, namelist, file_type);
   n = 0;
   *search_str = 0;
   while (1)
   {
      int search_flag;

      mvwprintw (titlewin, 1,  0, "----------------------------------------");
      waddstr (titlewin, "----------------------------------------");
      mvwprintw (titlewin, 1, 0, gettext ("'h' for help "));
      wprintw (titlewin, "- %s ", get_current_dir_name ());
      wrefresh (titlewin);
      ls (n, tot, namelist, file_type, titlewin, screenwin);
      switch (wgetch (screenwin))
      {
      case 13: // ENTER
      case KEY_RIGHT:
      case '6':
         if (strncmp (file_type[n], "directory", 9) == 0)
         {
            switch (chdir (namelist[n]->d_name))
            {
            default:
            // discart return-code.
               break;
            } // switch
            free (namelist);
            if (show_hidden_files)
               tot = scandir (src_dir, &namelist, NULL, alphasort) - 1;
            else
               tot = scandir (src_dir, &namelist, &hidden_files, alphasort) - 1;
            fill_ls (tot, namelist, file_type);
            n = 0;
            break;
         } // if
         snprintf (name, MAX_STR - 1, "%s/%s",
                   get_current_dir_name (), namelist[n]->d_name);
         free (namelist);
         return name;
      case KEY_LEFT:
      case '4':
         if (strcmp (get_current_dir_name (), "/") == 0)
         {
            beep ();
            break;
         } // if
         switch (chdir (".."))
         {
         default:
         // discart return-code.
            break;
         } // switch
         if (show_hidden_files)
            tot = scandir (src_dir, &namelist, NULL, alphasort) - 1;
         else
            tot = scandir (src_dir, &namelist, &hidden_files, alphasort) - 1;
         fill_ls (tot, namelist, file_type);
         n = 0;
         break;
      case KEY_DOWN:
      case '2':
         if (n == tot)
            beep ();
         else
            n++;
         break;
      case KEY_UP:
      case '8':
         n--;
         if (n < 0)
         {
            n = 0;
            beep ();
         } // if
         break;
      case KEY_NPAGE:
      case '3':
         page = n / 23 + 1;
         if (page * 23 >= tot)
            beep ();
         else
            n = page * 23;
         break;
      case KEY_PPAGE:
      case '9':
         page = n / 23 - 1;
         if (page < 0)
         {
            page = 0;
            beep ();
         }
         else
            n = page * 23;
         break;
      case '/':
         *search_str = 0;
         if ((search_flag = search_in_dir (n + 1, tot, '/', titlewin,
                                           search_str, namelist)) != -1)
            n = search_flag;
         break;
      case 'B':
         n = tot;
         break;
      case 'h':
      case '?':
         help_list (screenwin);
         nodelay (screenwin, FALSE);
         wclear (titlewin);
         mvwprintw (titlewin, 0, 0,
                    gettext ("%s - Choose an input-file"), copyright);
         wrefresh (titlewin);
         if (show_hidden_files)
            tot = scandir (src_dir, &namelist, NULL, alphasort) - 1;
         else
            tot = scandir (src_dir, &namelist, &hidden_files, alphasort) - 1;
         fill_ls (tot, namelist, file_type);
         break;
      case 'H':
      {
         char name[55];
         int found;

         show_hidden_files = 1 - show_hidden_files;
         strncpy (name, namelist[n]->d_name, 50);
         free (namelist);
         if (show_hidden_files)
            tot = scandir (src_dir, &namelist, NULL, alphasort) - 1;
         else
            tot = scandir (src_dir, &namelist, &hidden_files, alphasort) - 1;
         fill_ls (tot, namelist, file_type);
         found = 0;
         for (n = 0; n <= tot; n++)
         {
            if (strncmp (namelist[n]->d_name, name, MAX_STR) == 0)
            {
               found = 1;
               break;
            } // if
         } // for
         if (! found)
            n = 0;
         break;
      }
      case 'n':
         if ((search_flag = search_in_dir (n + 1, tot, 'n', titlewin,
                                           search_str, namelist)) != -1)
            n = search_flag;
         break;
      case 'N':
         if ((search_flag = search_in_dir (n - 1, tot, 'N', titlewin,
                                           search_str, namelist)) != -1)
            n = search_flag;
         break;
      case 'q':
         free (namelist);
         return "";
      case 'T':
         n = 0;
         break;
      default:
         beep ();
         break;
      } // switch
   } // for
} // get_input_file
