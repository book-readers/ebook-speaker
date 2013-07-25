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

void ls (int n, int total, struct dirent **namelist,
         WINDOW *titlewin, WINDOW *screenwin)
{
   magic_t myt;
   char name[45];
   int y, page;

   page = (n - 1) / 23 + 1;
   mvwprintw (titlewin, 0, 73, "%3d/%3d", page, (total - 1) / 23 + 1);
   wrefresh (titlewin);
   wclear (screenwin);
   myt = magic_open (MAGIC_CONTINUE | MAGIC_SYMLINK);
   magic_load (myt, NULL);
   for (y = 0; y < 23; y++)
   {
      int index;

      index = (page - 1) * 23 + y + 1;
      if (index > total)
         break;
      snprintf (name, 40, "%s", namelist[index]->d_name);
      mvwprintw (screenwin, y, 1, "%s", name);
      snprintf (name, 40, "%s", magic_file (myt, namelist[index]->d_name));
      mvwprintw (screenwin, y, 41, "%s", name);
   } // for
   magic_close (myt);
   wmove (screenwin, n - (page - 1) * 23 - 1, 0);
} // ls

char *get_input_file (char *src_dir, char *copyright,
                      WINDOW *titlewin, WINDOW *screenwin)
{
   struct dirent **namelist;
   int n, tot, page;
   DIR *dir;
   static char name[MAX_STR];

   nodelay (screenwin, FALSE);
   wclear (titlewin);
   mvwprintw (titlewin, 0, 0, "%s - Choose an input-file", copyright);
   wrefresh (titlewin);
   if (! (dir = opendir (src_dir)))
   {
      int e;

      e = errno;
      endwin ();
      beep ();
      printf ("\n%s\n", strerror (e));
      fflush (stdout);
      _exit (1);
   } // if
   tot = scandir (src_dir, &namelist, NULL, alphasort) - 1;
   closedir (dir);
   n = 1;
   for (;;)
   {
      mvwprintw (titlewin, 1,  0, "----------------------------------------");
      waddstr (titlewin, "----------------------------------------");
      mvwprintw (titlewin, 1, 0, "%s ", get_current_dir_name ());
      wrefresh (titlewin);
      ls (n, tot, namelist, titlewin, screenwin);
      switch (wgetch (screenwin))
      {
      case 13: // ENTER
      case KEY_RIGHT:
      case '6':
      {
         magic_t myt;

         myt = magic_open (MAGIC_CONTINUE | MAGIC_SYMLINK);
         magic_load (myt, NULL);
         if (strncmp (magic_file (myt, namelist[n]->d_name), "directory",
             9) == 0)
         {
            magic_close (myt);
            switch (chdir (namelist[n]->d_name))
            {
            default:
            // discart return-code.
               break;
            } // switch
            if (! (dir = opendir (".")))
            {
               int e;

               e = errno;
               endwin ();
               beep ();
               printf ("\n%s\n", strerror (e));
               fflush (stdout);
               _exit (1);
            } // if
            tot = scandir (src_dir, &namelist, NULL, alphasort) - 1;
            closedir (dir);
            n = 1;
            break;
         } // if
         magic_close (myt);
         snprintf (name, MAX_STR - 1, "%s/%s",
                   get_current_dir_name (), namelist[n]->d_name);
         free (namelist);
         return name;
      }
      case KEY_LEFT:
      case '4':
         switch (chdir (".."))
         {
         default:
         // discart return-code.
            break;
         } // switch
         if (! (dir = opendir (".")))
         {
            int e;

            e = errno;
            endwin ();
            beep ();
            printf ("\n%s\n", strerror (e));
            fflush (stdout);
            _exit (1);
         } // if
         tot = scandir (src_dir, &namelist, NULL, alphasort) - 1;
         closedir (dir);
         n = 1;
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
         if (n < 1)
         {
            n = 1;
            beep ();
         } // if
         break;
      case KEY_NPAGE:
      case '3':
         page = (n - 1) / 23 + 1;
         if (page * 23 >= tot)
            beep ();
         else
            n = page * 23 + 1;
         break;
      case KEY_PPAGE:
      case '9':
         page = (n - 1) / 23 + 1;
         if (page == 1)
            beep ();
         else
            n = (page - 2) * 23 + 1;
         break;
      case 'q':
         free (namelist);
         return "";
      case '/':
      {
         int s, found = 0;
         char search_str[MAX_STR];

         mvwaddstr (titlewin, 1, 0, "----------------------------------------");
         waddstr (titlewin, "----------------------------------------");
         mvwprintw (titlewin, 1, 0, gettext ("What do you search?        "));
         echo ();
         wmove (titlewin, 1, 20);
         wgetnstr (titlewin, search_str, 25);
         noecho ();
         for (s = n + 1; s <= tot; s++)
         {
            if (strcasestr (namelist[s]->d_name, search_str))
            {
               found = 1;
               break;
            } // if
         } // for
         if (found)
            n = s;
         else
            beep ();
         break;
      }
      default:
         beep ();
         break;
      } // switch
   } // for
} // get_input_file
