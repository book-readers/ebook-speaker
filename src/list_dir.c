/* eBook-speaker - read aloud an eBook using a speech synthesizer
 *  Copyright (C) 2015 J. Lemmens
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

struct dirent **get_dir (misc_t *misc, struct dirent **namelist)
{
   if (misc->show_hidden_files)
      misc->list_total = scandir (misc->src_dir, &namelist, NULL, alphasort);
   else
      misc->list_total =
                 scandir (misc->src_dir, &namelist, hidden_files, alphasort);
   if (misc->list_total == -1)
   {
      int e;

      e = errno;
      beep ();
      endwin ();
      printf ("%s\n", gettext (strerror (e)));
      _exit (0);
   } // if
   return namelist;
} // get_dir

char *get_file_type (char *name)
{
   magic_t myt;

   myt = magic_open (MAGIC_FLAGS);
   magic_load (myt, NULL);
   if (magic_file (myt, name) == NULL)
      return "Unknown format";
   else
      return (char *) magic_file (myt, name);
} // get_file_type

void ls (misc_t *misc, size_t n, struct dirent **namelist)
{
   size_t y, page;
   char str[MAX_STR + 1];
   magic_t myt;

   myt = magic_open (MAGIC_FLAGS);
   magic_load (myt, NULL);
   page = n / 23 + 1;
   wclear (misc->titlewin);
   wrefresh (misc->titlewin);
   mvwprintw (misc->titlewin, 0, 0,
              gettext ("%s - Choose an input-file"), misc->copyright);
   mvwprintw (misc->titlewin, 1,  0,
              "----------------------------------------");
   wprintw (misc->titlewin, "----------------------------------------");
   mvwprintw (misc->titlewin, 1, 0, gettext ("'h' for help "));
   wprintw (misc->titlewin, "- %s ", misc->src_dir);
   snprintf (str, MAX_STR, "%d/%d",
             (int) page, (int) (misc->list_total - 1) / 23 + 1);
   mvwprintw (misc->titlewin, 1, 77 - strlen (str), " %s -", str);
   wrefresh (misc->titlewin);
   wclear (misc->screenwin);
   for (y = 0; y < 23; y++)
   {
      size_t index;

      index = (page - 1) * 23 + y;
      if ((int) index >= misc->list_total)
         break;
      mvwprintw (misc->screenwin, y, 1, "%.39s", namelist[index]->d_name);
      if ((int) n >= misc->list_total)
         break;
      if (magic_file (myt, namelist[index]->d_name) == NULL)
         mvwprintw (misc->screenwin, y, 41, "%.39s", "Unknown format");
      else
         mvwprintw (misc->screenwin, y, 41, "%.39s",
                    magic_file (myt, namelist[index]->d_name));
      wmove (misc->screenwin, n - (page - 1) * 23, 0);
      wrefresh (misc->screenwin);
   } // for
   magic_close (myt);
   wmove (misc->screenwin, n - (page - 1) * 23, 0);
   wrefresh (misc->screenwin);
} // ls

int hidden_files (const struct dirent *entry)
{
   if (*entry->d_name == '.')
      return 0;
   return 1;
} // hidden)files

int search_in_dir (misc_t *misc, int start, int tot, char mode,
                   char *search_str, struct dirent **namelist)
{
   static int c;
   int found = 0;

   if (*search_str == 0)
   {
      mvwaddstr (misc->titlewin, 1, 0, "----------------------------------------");
      waddstr (misc->titlewin, "----------------------------------------");
      mvwprintw (misc->titlewin, 1, 0, gettext ("What do you search? "));
      echo ();
      wgetnstr (misc->titlewin, search_str, 25);
      noecho ();
   } // if
   if (mode == '/' || mode == 'n')
   {
      for (c = start; c < tot; c++)
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
         for (c = tot - 1; c > start; c--)
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

void help_list (misc_t *misc)
{
   wclear (misc->screenwin);
   wprintw (misc->screenwin, "\n%s\n", gettext
            ("These commands are available in this version:"));
   wprintw (misc->screenwin, "========================================");
   wprintw (misc->screenwin, "========================================\n");
   wprintw (misc->screenwin, "%s\n", gettext
            ("cursor down,2   - move cursor to the next item"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("cursor up,8     - move cursor to the previous item"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("cursor right,6  - open this directory or file"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("cursor left,4   - open previous directory"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("page-down,3     - view next page"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("page-up,9       - view previous page"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("enter,6         - open this directory or file"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("/               - search for a label"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("end,B           - move cursor to the last item"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("h,?             - give this help"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("H,0             - display \"hidden\" files on/off"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("n               - search forwards"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("N               - search backwards"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("q               - quit eBook-speaker"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("home,T          - move cursor to the first item"));
   wprintw (misc->screenwin, "\n%s", gettext
            ("Press any key to leave help..."));
   nodelay (misc->screenwin, FALSE);
   wgetch (misc->screenwin);
   nodelay (misc->screenwin, TRUE);
} // help_list

char *get_input_file (misc_t *misc, char *src)
{
   struct dirent **namelist;
   static char file[MAX_STR + 1];
   char *file_type = 0, search_str[MAX_STR], str[MAX_STR + 1];
   int n, page;

   if (strcasecmp (get_file_type (src), "directory") == 0)
   {
      strncpy (misc->src_dir, src, MAX_STR);
      if (*(misc->src_dir + strlen (misc->src_dir) - 1) != '/')
         strcat (misc->src_dir, "/");
      switch (chdir (misc->src_dir))
      {
      default:
         break;
      } // switch
   }
   else
   {
      strncpy (file, basename (src), MAX_STR);
      strncpy (misc->src_dir, dirname (src), MAX_STR);
      if (*(misc->src_dir + strlen (misc->src_dir) - 1) != '/')
         strcat (misc->src_dir, "/");
      switch (chdir (misc->src_dir))
      {
      default:
         break;
      } // switch
   } // if
   nodelay (misc->screenwin, FALSE);
   namelist = NULL;
   misc->list_total = 0;
   namelist = get_dir (misc, namelist);
   n = 0;
   for (n = misc->list_total - 1; n > 0; n--)
      if (strcmp (namelist[n]->d_name, file) == 0)
         break;
   page = n / 23 + 1;
   mvwprintw (misc->titlewin, 0, 0,
              gettext ("%s - Choose an input-file"), misc->copyright);
   mvwprintw (misc->titlewin, 1,  0, "----------------------------------------");
   wprintw (misc->titlewin, "----------------------------------------");
   mvwprintw (misc->titlewin, 1, 0, gettext ("'h' for help "));
   wprintw (misc->titlewin, "- %s ", misc->src_dir);
   snprintf (str, MAX_STR, "%d/%d",
             (int) page, (int) (misc->list_total - 1) / 23 + 1);
   mvwprintw (misc->titlewin, 1, 77 - strlen (str), " %s -", str);
   wrefresh (misc->titlewin);
   *search_str = 0;
   *misc->daisy_title = 0;
   while (1)
   {
      int search_flag;

      namelist = get_dir (misc, namelist);
      ls (misc, n, namelist);
      switch (wgetch (misc->screenwin))
      {
      case 13: // ENTER
      case KEY_RIGHT:
      case '6':
         if (misc->list_total == 0)
         {
            beep ();
            break;
         } // if
         if (strcmp (namelist[n]->d_name, ".") == 0)
            break;
         if (strcmp (namelist[n]->d_name, "..") == 0)
         {
            if (strcmp (misc->src_dir, "/") == 0)
            {
               beep ();
               break;
            } // if
            strncpy (misc->src_dir, dirname (misc->src_dir), MAX_STR);
            switch (chdir (misc->src_dir))
            {
            default:
               break;
            } // switch
            if (*(misc->src_dir + strlen (misc->src_dir) - 1) != '/')
               strcat (misc->src_dir, "/");
            n = 0;
            page = n / 23 + 1;
            break;
         } // if ".."
         file_type = get_file_type (namelist[n]->d_name);
         if (strstr (file_type, "directory") != NULL)
         {
            strncat (misc->src_dir, namelist[n]->d_name, MAX_STR);
            switch (chdir (misc->src_dir))
            {
            default:
               break;
            } // switch
            if (*(misc->src_dir + strlen (misc->src_dir) - 1) != '/')
               strcat (misc->src_dir, "/");
            n = 0;
            page = n / 23 + 1;
            break;
         } // if "directory"
         snprintf (file, MAX_STR, "%s%s", misc->src_dir, namelist[n]->d_name);
         return file;
      case KEY_LEFT:
      case '4':
      {
         char name[MAX_STR + 1];

         if (strcmp (misc->src_dir, "/") == 0)
// topdir
         {
            beep ();
            break;
         } // if
         strncpy (name, basename (misc->src_dir), MAX_STR);
         strncpy (misc->src_dir, dirname (misc->src_dir), MAX_STR);
         if (misc->src_dir[strlen (misc->src_dir) - 1] != '/')
            strncat (misc->src_dir, "/", MAX_STR);
         switch (chdir (misc->src_dir))
         {
         default:
            break;
         } // switch
         namelist = get_dir (misc, namelist);
         for (n = misc->list_total - 1; n > 0; n--)
            if (strcmp (namelist[n]->d_name, name) == 0)
               break;
         break;
      }
      case KEY_DOWN:
      case '2':
         if (misc->list_total == 0 || n == misc->list_total - 1)
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
         if (page * 23 >= misc->list_total)
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
         if ((search_flag = search_in_dir (misc, n + 1,
                      misc->list_total, '/', search_str, namelist)) != -1)
            n = search_flag;
         break;
      case 'B':
      case KEY_END:
         n = misc->list_total - 1;
         break;
      case 'h':
      case '?':
         help_list (misc);
         nodelay (misc->screenwin, FALSE);
         namelist = get_dir (misc, namelist);
         free (file_type);
         file_type = get_file_type (namelist[n]->d_name);
         break;
      case 'H':
      case '0':
      {
         char name[55];

         misc->show_hidden_files = 1 - misc->show_hidden_files;
         strncpy (name, namelist[n]->d_name, 50);
         free (namelist);
         namelist = get_dir (misc, namelist);
         for (n = misc->list_total - 1; n > 0; n--)
            if (strncmp (namelist[n]->d_name, name, MAX_STR) == 0)
               break;
         break;
      }
      case 'n':
         if ((search_flag = search_in_dir (misc, n + 1, misc->list_total, 'n',
                                           search_str, namelist)) != -1)
            n = search_flag;
         break;
      case 'N':
         if ((search_flag = search_in_dir (misc,n - 1, misc->list_total, 'N',
                                           search_str, namelist)) != -1)
            n = search_flag;
         break;
      case 'q':
         free (namelist);
         quit_eBook_speaker (misc);
         _exit (0);
      case 'T':
      case KEY_HOME:
         n = 0;
         break;
      default:
         beep ();
         break;
      } // switch
   } // while
} // get_input_file                                                     
