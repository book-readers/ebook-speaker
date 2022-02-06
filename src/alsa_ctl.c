/*
 * alsa_ctl.c - ALSA sound driver
 *
 * Copyright (C)2003-2021 J. Lemmens
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

#include "daisy.h"

#define VOLUME_BOUND 100

int alsa_ctl (misc_t *misc, int action, int current_sink,
              audio_info_t *sound_devices)
{
   switch (action)
   {
   case ALSA_LIST:
   {
      FILE *r, *p;
      int found;                        
      char *str;
      size_t bytes;

      if (! (r = fopen ("/proc/asound/cards", "r")))
         failure (misc, gettext ("Cannot read /proc/asound/cards"), errno);
      while (1)
      {
         char *orig_language;

         found = 0;
         str = NULL;
         bytes = 0;
         if (getline (&str, &bytes, r) == -1)
            break;
         strcpy (sound_devices[current_sink].type, "alsa");
         while (isspace (*++str));
         *strchr (str, ' ') = 0;
         sprintf (sound_devices[current_sink].device, "hw:%s", str);
         str = NULL;
         bytes = 0;
         if (getline (&str, &bytes, r) == -1)
            break;
         while (isspace (*++str));
         *strrchr (str, ' ') = 0;
         snprintf (sound_devices[current_sink].name, 80, "(alsa) %s", str);

// force english output
         orig_language = strdup (setlocale (LC_ALL, ""));
         setlocale (LC_ALL, "en_GB.UTF-8");
         sprintf (misc->cmd, "/usr/bin/amixer -D %s get Master playback",
                  sound_devices[current_sink].device);
         if (! (p = popen (misc->cmd, "r")))
         {
            beep ();
            endwin ();
            printf ("%s\n",
   "Be sure the package alsa-utils is installed onto your system.");
            remove_tmp_dir (misc);
            setlocale (LC_ALL, orig_language);
            _exit (EXIT_FAILURE);
         } // if
         setlocale (LC_ALL, orig_language);
         while (1)
         {
            str = NULL;
            bytes = 0;
            if (getline (&str, &bytes, p) == -1)
               break;
            if (strcasestr (str, "playback") == NULL)
               continue;
            found = 1;
            if (strchr (str, '[') == NULL)
               continue;
            str = strchr (str, '[') + 1;
            strncpy (sound_devices[current_sink].volume, str, 4);
            if (strchr (sound_devices[current_sink].volume, ']'))
               *strchr (sound_devices[current_sink].volume, ']') = 0;
            strcpy (sound_devices[current_sink].muted,
                    strrchr (str, '[') + 1);
            *strchr (sound_devices[current_sink].muted, ']') = 0;
            if (strcmp (sound_devices[current_sink].muted, "on") == 0)
               strcpy (sound_devices[current_sink].muted, "no");
            if (strcmp (sound_devices[current_sink].muted, "off") == 0)
               strcpy (sound_devices[current_sink].muted, "yes");
         } // while
         pclose (p);
         if (found)
            current_sink++;
      } // while
      fclose (r);
      return current_sink;
   }
   case ALSA_VOLUME_SET:  
   {
      break;
   }
   } // switch
   return 0;
} // alsa_ctl
