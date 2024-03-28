/* eBook-speaker - read aloud an eBook using a speech synthesizer
 *
 *  Copyright (C) 2021 J. Lemmens
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

void clear_tmp_dir (misc_t *misc)
{
   if (strstr (misc->tmp_dir, "/tmp/"))
   {
// Be sure not to remove wrong files
      snprintf (misc->cmd, MAX_CMD - 1, "rm -rf %s/*", misc->tmp_dir);
      system (misc->cmd);
   } // if
} // clear_tmp_dir

void quit_eBook_speaker (misc_t *misc, my_attribute_t *my_attribute,
                         daisy_t *daisy)
{
   save_bookmark_and_xml (misc);
   system ("reset");
   close (misc->tmp_wav_fd);
   unlink (misc->tmp_wav);
   remove_tmp_dir (misc);
   free_all (misc, my_attribute, daisy);
   printf ("\n");
   remove_tmp_dir (misc);
   _exit (EXIT_SUCCESS);
} // quit_eBook_speaker

void remove_double_tts_entries (misc_t *misc)
{
   int x, y;
   int entry_already_exists_in_misc, tts_no = 0;
   char temp[12][MAX_STR];

   for (x = 0; x < 12; x++)
      strncpy (temp[x], misc->tts[x], MAX_STR - 1);
   memset ((void *) misc->tts, 0, sizeof (misc->tts));
   for (x = 0; x < 15; x++)
   {
      if (! *temp[x])
         break;
      entry_already_exists_in_misc = 0;
      for (y = 0; y < 15; y++)
      {
         if (*misc->tts[y] == 0)
            break;
         if (strcmp (temp[x], misc->tts[y]) == 0)
         {
            entry_already_exists_in_misc = 1;
            break;
         } // if
      } // for
      if (entry_already_exists_in_misc == 0)
         strncpy (misc->tts[tts_no++], temp[x], MAX_STR);
   } // for
} // remove_double_tts_entries

void skip_left (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   if (misc->playing < 0)
   {
      beep ();
      return;
   } // if
   if (misc->player_pid > -1)
      kill_player (misc);
   if (misc->playing < -1 ||
       (misc->current <= 0 && misc->phrase_nr <= 1))
   {
      beep ();
      return;
   } // if
   if (misc->phrase_nr == 1)
   {
      if (misc->just_this_item > -1 &&
          daisy[misc->playing].level <= misc->level)
      {
         beep ();
         misc->current = misc->playing;
         misc->playing = misc->just_this_item = -1;
         view_screen (misc, daisy);
         wmove (misc->screenwin, daisy[misc->current].y,
                daisy[misc->current].x - 1);
         return;
      } // if misc->just_this_item
   } // if

   if (misc->phrase_nr <= 1)
      go_to_phrase (misc, my_attribute, daisy, misc->playing - 1,
                    daisy[misc->playing - 1].n_phrases);
   else
      go_to_phrase (misc, my_attribute, daisy, misc->playing,
                    misc->phrase_nr - 1);
} // skip_left

void start_playing (misc_t *misc, my_attribute_t *my_attribute,
                    daisy_t *daisy, audio_info_t *sound_devices)
{
   FILE *w;
   struct stat buf;

   if (*misc->tts[misc->tts_no] == 0 && *misc->option_t == 0)
   {
      misc->tts_no = 0;
      select_tts (misc, my_attribute, daisy);
   } // if

   if ((w = fopen (misc->eBook_speaker_txt, "w")) == NULL)
   {
      char *str;

      str = malloc (10 + strlen (misc->eBook_speaker_txt));
      sprintf (str, "fopen (%s)", misc->eBook_speaker_txt);
      failure (misc, str, errno);
   } // if
   if (fwrite (misc->label, strlen (misc->label), 1, w) == 0)
   {
      int e;

      e = errno;
      beep ();
      snprintf (misc->str, MAX_STR, "write (\"%s\"): failed.\n", misc->label);
      failure (misc, misc->str, e);
   } // if
   fclose (w);
   stat (misc->eBook_speaker_txt, &buf);
   if (buf.st_size == 0)
      return;
   setlocale (LC_NUMERIC, "en_GB");
   lseek (misc->tmp_wav_fd, SEEK_SET, 0);
   system (misc->tts[misc->tts_no]);
   switch (misc->player_pid = fork ())
   {
   case -1:
   {
      int e;

      e = errno;
      skip_left (misc, my_attribute, daisy);
      put_bookmark (misc);
      failure (misc, "fork ()", e);
      break;
   }
   case 0: // child
   {
      char tempo_str[10];

      reset_term_signal_handlers_after_fork ();
      snprintf (tempo_str, 10, "%lf", misc->speed);
      playfile (misc->tmp_wav, "wav",
                sound_devices[misc->current_sink].device,
                sound_devices[misc->current_sink].type, tempo_str);
      _exit (EXIT_SUCCESS);
   }
   default: // parent
      return;
   } // switch
} // start_playing

void go_to_next_item (misc_t *misc, my_attribute_t *my_attribute,
                      daisy_t *daisy, audio_info_t *sound_devices)
{
   misc->phrase_nr = 0;
   if (++misc->playing >= misc->total_items)
   {
      char *str;
      struct passwd *pw;;

      pw = getpwuid (geteuid ());
      str = malloc (strlen (pw->pw_dir) + 20 + strlen (misc->bookmark_title));
      sprintf (str, "%s/.eBook-speaker/%s", pw->pw_dir, misc->bookmark_title);
      unlink (str);
      free (str);
      view_screen (misc, daisy);
      close (misc->tmp_wav_fd);
      unlink (misc->tmp_wav);
      if (misc->start_arg_is_a_dir == 1)
      {
         char file[MAX_STR];

         kill_player (misc);
         put_bookmark (misc);
         misc->playing = misc->just_this_item = -1;
         misc->current = 0;
         misc->total_pages = 0;
         clear_tmp_dir (misc);
         wclear (misc->screenwin);
         wrefresh (misc->screenwin);
         strcpy (file, get_input_file (misc, my_attribute,
                                       daisy, misc->src_dir));
         wclear (misc->screenwin);
         wrefresh (misc->screenwin);
         create_epub (misc, my_attribute, daisy, file, 1);
         play_epub (misc, my_attribute, daisy, sound_devices, file);
         return;
      } // if
      remove_tmp_dir (misc);
      endwin ();
      puts ("");
      _exit (EXIT_SUCCESS);
   } // if
   if (daisy[misc->playing].level <= misc->level)
      misc->current = misc->playing;
   if (misc->just_this_item != -1)
      if (daisy[misc->playing].level <= misc->level)
         misc->playing = misc->just_this_item = -1;
   if (misc->playing > -1)
   {
      unlink (misc->tmp_wav);
      unlink ("old.wav");
      open_xml_file (misc, my_attribute, daisy, daisy[misc->playing].xml_file,
                      daisy[misc->playing].xml_anchor);
   } // if
   view_screen (misc, daisy);
} // go_to_next_item

void get_next_phrase (misc_t *misc, my_attribute_t *my_attribute,
                      daisy_t *daisy, audio_info_t *sound_devices,
                      int give_sound)
{
   int eof;

   while (1)
   {
      eof = 1 - get_tag_or_label (misc, my_attribute, misc->reader);
      if (*misc->label && eof == 0)
      {
         if (daisy[misc->playing].screen == daisy[misc->current].screen)
         {
            wattron (misc->screenwin, A_BOLD);
            if (misc->current_page_number && misc->update_progress)
               mvwprintw (misc->screenwin, daisy[misc->playing].y, 63,
                          "(%3d)", misc->current_page_number);
            misc->phrase_nr++;
            wattroff (misc->screenwin, A_BOLD);
            strncpy (misc->str, misc->label, 80);
            mvwprintw (misc->screenwin, 22, 0, misc->str);
            wclrtoeol (misc->screenwin);
            wmove (misc->screenwin, daisy[misc->current].y,
                   daisy[misc->current].x - 1);
            wrefresh (misc->screenwin);
         } // if
         if (give_sound)
            start_playing (misc, my_attribute, daisy, sound_devices);
         break;
      } // if
      if (strcasestr (misc->daisy_version, "2.02") &&
          (strcasecmp (misc->tag, "pagenum") == 0 ||
           strcasecmp (my_attribute->class, "pagenum") == 0 ||
           strcasecmp (my_attribute->class, "page-normal") == 0))
      {
         if (get_page_number_2 (misc, my_attribute, daisy, my_attribute->src))
            break;
      } // if
      if (strcasestr (misc->daisy_version, "3") &&
          (strcasecmp (misc->tag, "pagenum") == 0 ||
           strcasecmp (my_attribute->class, "pagenum") == 0 ||
           strcasecmp (my_attribute->class, "page-normal") == 0))
      {
         if (get_page_number_3 (misc, my_attribute))
            break;
      } // if
      if (eof)
      {
         go_to_next_item (misc, my_attribute, daisy, sound_devices);
         break;
      } // if
      if (misc->playing + 1 < misc->total_items &&
          *daisy[misc->playing + 1].xml_anchor &&
          strcmp (my_attribute->id,
                  daisy[misc->playing + 1].xml_anchor) == 0)
      {
         go_to_next_item (misc, my_attribute, daisy, sound_devices);
         break;
      } // if
   } // while
} // get_next_phrase

void select_tts (misc_t *misc, my_attribute_t *my_attribute,
                 daisy_t *daisy)
{
   int y;

   kill_player (misc);
   wclear (misc->screenwin);
   wprintw (misc->screenwin, "\n%s\n\n", gettext
            ("Select a Text-To-Speech application"));
   wprintw (misc->screenwin, "    1 %s\n", misc->tts[0]);
   load_xml (misc, my_attribute);
   for (y = 1; y < 15; y++)
   {
      char str[MAX_STR];

      if (*misc->tts[y] == 0)
         break;
      strncpy (str, misc->tts[y], MAX_STR - 1);
      str[72] = 0;
      wprintw (misc->screenwin, "   %2d %s\n", y + 1, str);
   } // for
   wmove (misc->screenwin, 14, 0);
   if (y >= 15)
   {
      beep ();
      wprintw (misc->screenwin, "\n%s\n\n", gettext
          ("Maximum number of TTS's is reached."));
   } // if
   nodelay (misc->screenwin, FALSE);
   y = misc->tts_no;
   if (*misc->tts[y])
      wmove (misc->screenwin, y + 3, 2);
   while (1)
   {
      switch (wgetch (misc->screenwin))
      {
      case 13:
         misc->tts_no = y;
         view_screen (misc, daisy);
         nodelay (misc->screenwin, TRUE);
         return;
      case KEY_DOWN:
         y++;
         if (*misc->tts[y] == 0 || y == 15)
            y = 0;
         wmove (misc->screenwin, y + 3, 2);
         break;
      case KEY_UP:
         y--;
         if (y < 0)
         {
            y = 14;
            while (1)
            {
               if (*misc->tts[y])
                  break;
               y--;
            } // while
         } // if
         wmove (misc->screenwin, y + 3, 2);
         break;
      case ERR:
         break;
      default:
         view_screen (misc, daisy);
         nodelay (misc->screenwin, TRUE);
         return;
      } // switch
   } // while
} // select_tts

void count_phrases (misc_t *misc, my_attribute_t *my_attribute,
                    daisy_t *daisy)
{
   xmlTextReaderPtr r;
   htmlDocPtr d;
   int item;

   misc->total_phrases = 0;
   for (item = 0; item < misc->total_items; item++)
   {
      daisy[item].n_phrases = 0;
      if (! *daisy[item].xml_file)
         continue;
      if (! (d = htmlParseFile (daisy[item].xml_file, "UTF-8")))
         failure (misc, "htmlParseFile ()", errno);
      if (! (r = xmlReaderWalker (d)))
         failure (misc, "xmlReaderWalker ()", errno);

// start counting
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, r))
            break;
         if (*misc->label)
            daisy[item].n_phrases++;
         if (item == misc->total_items - 1)
            continue;
         if (strcasecmp (daisy[item].xml_file,
                         daisy[item + 1].xml_file) != 0)
         {
// no need to search t_anchor
            continue;
         } // if
         if (*daisy[item + 1].xml_anchor)
// stop at "to" anchor
            if (strcasecmp (my_attribute->id,
                            daisy[item + 1].xml_anchor) == 0)
               break;
      } // while
      misc->total_phrases += daisy[item].n_phrases;
   } // for
} // count_phrases

void split_phrases (misc_t *misc, my_attribute_t *my_attribute,
                    daisy_t *daisy, int i)
{
   char *new, *p;
   int start_of_new_phrase;
   xmlTextWriterPtr writer;

   open_xml_file (misc, my_attribute, daisy, daisy[i].xml_file,
                   daisy[i].xml_anchor);
   new = malloc (strlen (daisy[i].xml_file) + 20);
   sprintf (new, "%s.%03d.splitted.xhtml", daisy[i].xml_file, i);
   if ((writer = xmlNewTextWriterFilename (new, 0)) == NULL)
   {
      int e;
      char *str;

      e = errno;
      str = malloc (strlen (new) + strlen (strerror (e)) + 20);
      sprintf (str, "fopen (%s): %s\n", new, strerror (e));
      free (new);
      failure (misc, str, e);
   } // if
   if (! *misc->xmlversion)
      strncpy (misc->xmlversion, "1.0", 5);
   if (! *misc->encoding)
      strncpy (misc->encoding, "utf-8", 8);
   xmlTextWriterSetIndent (writer, 1);
   xmlTextWriterSetIndentString (writer, BAD_CAST "   ");
   xmlTextWriterStartDocument
                (writer, misc->xmlversion, misc->encoding, misc->standalone);
   xmlTextWriterStartElement (writer, (const xmlChar *) misc->tag);
   if (*my_attribute->id)
   {
      xmlTextWriterWriteFormatAttribute (writer, BAD_CAST "id", "%s",
                                         my_attribute->id);
   } // if id
   while (1)
   {
      int eof;

      eof = 1 - get_tag_or_label (misc, my_attribute, misc->reader);
      if ((*my_attribute->id && i + 1 < misc->total_items &&
           strcasecmp (my_attribute->id,
                       daisy[i + 1].xml_anchor) == 0) ||
          eof)
      {
         xmlTextWriterEndElement (writer);
         break;
      } // if
      if (*misc->tag == '/')
      {
// close tag
         xmlTextWriterEndElement (writer);
         continue;
      } // if
      if (*misc->tag)
      {
         if (strncasecmp (misc->tag, "br", 2) == 0 ||
             strcasecmp (misc->tag, "meta") == 0)
            continue;
         if (strcasecmp (misc->tag, "style") == 0)
         {
            while (1)
            {
               if (! get_tag_or_label (misc, my_attribute, misc->reader))
                  break;
               if (strcasecmp (misc->tag, "/style") == 0)
                  break;
            } // while
            continue;
         } // if
         xmlTextWriterStartElement (writer, (const xmlChar *) misc->tag);
/*
   The function xmlTextReaderGetAttributeNo (reader, j); gives the
   value of the n-th attribute of the current element.
   How to get the name of the n-th attribute?
*/

         if (*my_attribute->id)
         {
            xmlTextWriterWriteFormatAttribute
                 (writer, BAD_CAST "id", "%s", my_attribute->id);
         } // if id
         if (*my_attribute->class)
         {
            xmlTextWriterWriteFormatAttribute
                 (writer, BAD_CAST "class", "%s", my_attribute->class);
         } // if class
         if (*my_attribute->src)
         {
            xmlTextWriterWriteFormatAttribute
                 (writer, BAD_CAST "src", "%s", my_attribute->src);
         } // if src
         if (*my_attribute->href)
         {
            xmlTextWriterWriteFormatAttribute
                 (writer, BAD_CAST "href", "%s", my_attribute->href);
         } // if href
         continue;
      } // if misc->tag
      if (! *misc->label)
         continue;

// investigate this label
      p = misc->label;
      start_of_new_phrase = 1;
      while (1)
      {
         if (start_of_new_phrase == 1 && isspace (*p))
         {
// remove first spaces
            p++;
            continue;
         } // if
         start_of_new_phrase = 0;
         if ((*p == '.' || *p == ',' || *p == '!' || *p == '?' ||
              *p == '/' || *p == ':' || *p == ';' || *p == '-' ||
              *p == '*' || *p == '\"') &&
             (*(p + 1) == ' ' || *(p + 1) == '\r' || *(p + 1) == '\n' ||
              *(p + 1) == '\'' || *(p + 1) == 0 ||
// ----
              (signed) *(p + 1) == -62))
              // I don't understand it, but it works
              // Some files use -62 -96 (&nbsp;) as space
// ----
         {
            xmlTextWriterWriteFormatString (writer, "%c", *p);
            p++;
            xmlTextWriterWriteElement (writer, BAD_CAST "br", NULL);
            start_of_new_phrase = 1;
            continue;
         } // if
         if (*p != 0)
         {
            xmlTextWriterWriteFormatString (writer, "%c", *p);
            p++;
            continue;
         } // if
         if (*p == 0)
         {
            if (misc->break_phrase == 'y')
            {
               xmlTextWriterWriteElement (writer, BAD_CAST "br", NULL);
               start_of_new_phrase = 1;
            }
            if (misc->break_phrase == 'n')
            {
               xmlTextWriterWriteString (writer, BAD_CAST " ");
            } // if
            break;
         } // if
      } // while
   } // while
   xmlTextWriterEndDocument (writer);
   xmlTextReaderClose (misc->reader);
   xmlTextWriterEndDocument (writer);
   xmlFreeTextWriter (writer);
   daisy[i].xml_file = strdup (new);
   free (new);             
} // split_phrases

void put_bookmark (misc_t *misc)
{
   char *str;
   xmlTextWriterPtr writer;
   struct passwd *pw;;

   pw = getpwuid (geteuid ());
   str = malloc (strlen (pw->pw_dir) + 20);
   sprintf (str, "%s/.eBook-speaker", pw->pw_dir);
   mkdir (str, 0755);
   str = realloc (str, strlen (pw->pw_dir) + 20 +
                  strlen (misc->bookmark_title));
   sprintf (str, "%s/.eBook-speaker/%s", pw->pw_dir, misc->bookmark_title);
   if (! (writer = xmlNewTextWriterFilename (str, 0)))
   {
      free (str);
      return;
   } // if
   xmlTextWriterStartDocument (writer, NULL, NULL, NULL);
   xmlTextWriterStartElement (writer, BAD_CAST "bookmark");
   if (misc->playing < 0)
      xmlTextWriterWriteFormatAttribute
         (writer, BAD_CAST "item", "%d", misc->current);
   else
      xmlTextWriterWriteFormatAttribute
         (writer, BAD_CAST "item", "%d", misc->playing);
   xmlTextWriterWriteFormatAttribute
      (writer, BAD_CAST "phrase", "%d", misc->phrase_nr);
   xmlTextWriterWriteFormatAttribute
      (writer, BAD_CAST "level", "%d", misc->level);
   xmlTextWriterWriteFormatAttribute
      (writer, BAD_CAST "tts", "%d", misc->tts_no);
   xmlTextWriterWriteFormatAttribute
      (writer, BAD_CAST "speed", "%f", misc->speed);
   if (misc->break_phrase == 'y' || misc->break_phrase == 'n')
      xmlTextWriterWriteFormatAttribute
         (writer, BAD_CAST "break_phrase", "%c", misc->break_phrase);
   else
      xmlTextWriterWriteFormatAttribute
         (writer, BAD_CAST "break_phrase", "%d", misc->break_phrase);
   xmlTextWriterWriteFormatAttribute (writer, BAD_CAST
                          "current_sink", "%d", misc->current_sink);
   xmlTextWriterEndElement (writer);
   xmlTextWriterEndDocument (writer);
   xmlFreeTextWriter (writer);
   free (str);
} // put_bookmark

void get_bookmark (misc_t *misc, my_attribute_t *my_attribute,
                   daisy_t *daisy, audio_info_t *sound_devices)
{
   xmlTextReaderPtr local_reader;
   htmlDocPtr local_doc;
   struct passwd *pw;;
   char *str;

   if (! *misc->bookmark_title)
      return;
   pw = getpwuid (geteuid ());
   str = malloc (strlen (pw->pw_dir) + 20 + strlen (misc->bookmark_title));
   sprintf (str, "%s/.eBook-speaker/%s", pw->pw_dir, misc->bookmark_title);
   local_doc = htmlParseFile (str, "UTF-8");
   free (str);
   if (! (local_reader = xmlReaderWalker (local_doc)))
   {
      if (misc->current_sink < 0)
         select_next_output_device (misc, daisy, sound_devices);
      return;
   } // if
   do
   {
      if (! get_tag_or_label (misc, my_attribute, local_reader))
         break;
   } while (strcasecmp (misc->tag, "bookmark") != 0);
   if (misc->current_sink < 0)
      select_next_output_device (misc, daisy, sound_devices);
   xmlTextReaderClose (local_reader);
   if (*misc->option_t)
      misc->tts_no = 0;
   if (misc->current < 0 || misc->current >= misc->total_items)
      misc->current = 0;
   if (misc->phrase_nr < 0)
      misc->phrase_nr = 0;
   if (misc->level < 1 || misc->level > misc->depth)
      misc->level = 1;
   misc->playing = misc->current;
   misc->current_page_number = daisy[misc->playing].page_number;
   misc->just_this_item = -1;
} // get_bookmark

void show_progress (misc_t *misc, daisy_t *daisy)
{
   if (misc->playing == -1 ||
       daisy[misc->current].screen != daisy[misc->playing].screen)
      return;
   wattron (misc->screenwin, A_BOLD);
   mvwprintw (misc->screenwin, daisy[misc->playing].y, 69, "%5d %5d",
              misc->phrase_nr,
              daisy[misc->playing].n_phrases - misc->phrase_nr);
   wattroff (misc->screenwin, A_BOLD);
   wmove (misc->screenwin, daisy[misc->current].y,
                           daisy[misc->current].x - 1);
   wrefresh (misc->screenwin);
} // show_progress

void view_screen (misc_t *misc, daisy_t *daisy)
{
   long i, x, l;

   mvwaddstr (misc->titlewin, 1, 0,
              "----------------------------------------");
   waddstr (misc->titlewin, "----------------------------------------");
   mvwprintw (misc->titlewin, 1, 0, "%s ", gettext ("'h' for help"));
   if (misc->total_pages)
   {
      mvwprintw (misc->titlewin, 1, 15, " ");
      wprintw (misc->titlewin, gettext
                 ("%d pages"), misc->total_pages);
      wprintw (misc->titlewin, " ");
   } // if
   mvwprintw (misc->titlewin, 1, 28, " ");
   wprintw (misc->titlewin, gettext
              ("level: %d of %d"), misc->level, misc->depth);
   wprintw (misc->titlewin, " ");
   mvwprintw (misc->titlewin, 1, 47, " ");
   wprintw (misc->titlewin, gettext
             ("total phrases: %d"), misc->total_phrases);
   wprintw (misc->titlewin, " ");
   mvwprintw (misc->titlewin, 1, 74, " %d/%d ",
              misc->current / misc->max_y + 1,
              (misc->total_items - 1) / misc->max_y + 1);
   wrefresh (misc->titlewin);

   werase (misc->screenwin);
   for (i = 0; daisy[i].screen != daisy[misc->current].screen; i++);
   do
   {
      mvwprintw (misc->screenwin, daisy[i].y, daisy[i].x, "%s",
                 daisy[i].label);
      l = (long) strlen (daisy[i].label) + daisy[i].x;
      if (l / 2 * 2 == l)
         waddstr (misc->screenwin, " ");
      for (x = l; x < 56; x += 2)
         waddstr (misc->screenwin, " .");
      if (daisy[i].page_number)
      {
         mvwprintw (misc->screenwin, daisy[i].y, 63,
                    "(%3d)", daisy[i].page_number);
      } // if
      l = daisy[i].n_phrases;
      x = i + 1;
      while (daisy[x].level > misc->level)
      {
         if (x >= misc->total_items)
            break;
         l += daisy[x++].n_phrases;
      } // while
      if (daisy[i].level <= misc->level)
         mvwprintw (misc->screenwin, daisy[i].y, 75, "%5d", l);
      if (i >= misc->total_items - 1)
         break;
   } while (daisy[++i].screen == daisy[misc->current].screen);
   if (misc->just_this_item != -1 &&
       daisy[misc->current].screen == daisy[misc->playing].screen)
      mvwprintw (misc->screenwin, daisy[misc->playing].y, 0, "J");
   mvwprintw (misc->screenwin, 21, 0,
              "----------------------------------------");
   wprintw (misc->screenwin, "----------------------------------------");
   wmove (misc->screenwin, daisy[misc->current].y,
          daisy[misc->current].x - 1);
   wrefresh (misc->screenwin);
} // view_screen

void view_total_phrases (misc_t *misc, daisy_t *daisy, int nth)
{
// added by Colomban Wendling <cwendling@hypra.fr>
   if (daisy[nth].screen == daisy[misc->current].screen &&     
       (daisy[nth].level <= misc->level || misc->playing == nth))
   {
      int x, total_phrases;

      x = nth;
      total_phrases = 0;
      do
      {
         total_phrases += daisy[x].n_phrases;
         if (++x >= misc->total_items)
            break;
      } while (misc->playing != nth && daisy[x].level > misc->level);
      if (misc->playing == nth)
      {
         mvwprintw (misc->screenwin, daisy[misc->playing].y, 68, "      ");
         wattron (misc->screenwin, A_BOLD);
         mvwprintw (misc->screenwin, daisy[nth].y, 75, "%5d", total_phrases);
         wattroff (misc->screenwin, A_BOLD);
         wmove (misc->screenwin, daisy[misc->current].y,
                daisy[misc->current].x - 1);
      } // if 
   } // if
} // view_total_phrases

void help (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy,
           audio_info_t *sound_devices)
{
   int y, x, playing;

   playing = misc->playing;
   if (playing > -1)
      pause_resume (misc, my_attribute, daisy, sound_devices);
   getyx (misc->screenwin, y, x);
   wclear (misc->screenwin);
   wprintw (misc->screenwin, "\n%s\n", gettext
            ("These commands are available in this version:"));
   waddstr (misc->screenwin, "========================================");
   waddstr (misc->screenwin, "========================================\n\n");
   wprintw (misc->screenwin, "%s\n", gettext
            ("cursor down,2   - move cursor to the next item"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("cursor up,8     - move cursor to the previous item"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("cursor right,6  - skip to next phrase"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("cursor left,4   - skip to previous phrase"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("page-down,3     - view next page"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("page-up,9       - view previous page"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("enter           - start playing"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("space,0         - pause/resume playing"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("home,*          - play on normal speed"));
   wprintw (misc->screenwin, "\n");
   wprintw (misc->screenwin, gettext ("Press any key for next page..."));
   nodelay (misc->screenwin, FALSE);
   wgetch (misc->screenwin);
   nodelay (misc->screenwin, TRUE);
   wclear (misc->screenwin);
   wprintw (misc->screenwin, "\n\n\n");
   wprintw (misc->screenwin, "%s\n", gettext
            ("/               - search for a label"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("A               - store current item to disk in ASCII-format"));
   wprintw (misc->screenwin, "%s\n", gettext
   ("b               - consider the end of an input line as a phrase-break"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("B               - move cursor to the last item"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("d               - store current item to disk in WAV-format"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("D,-             - decrease playing speed"));
   wprintw (misc->screenwin, "%s\n", gettext
 ("f               - find the currently playing item and place the cursor there"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("g               - go to phrase in current item"));
   if (misc->total_pages)
      wprintw (misc->screenwin, "%s\n", gettext
                        ("G               - go to page number"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("h,?             - give this help"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("j,5             - just play current item"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("l               - switch to next level"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("L               - switch to previous level"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("m               - mute sound output on/off"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("n               - search forwards"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("N               - search backwards"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("o               - select next output sound device"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("p               - place a bookmark"));
   wprintw (misc->screenwin, "\n");
   wprintw (misc->screenwin, gettext ("Press any key for next page..."));
   nodelay (misc->screenwin, FALSE);
   wgetch (misc->screenwin);
   nodelay (misc->screenwin, TRUE);
   wclear (misc->screenwin);
   wprintw (misc->screenwin, "\n\n\n\n");
   wprintw (misc->screenwin, "%s\n", gettext
            ("q               - quit eBook-speaker and place a bookmark"));
   if (misc->scan_flag == 1)
      wprintw (misc->screenwin, "%s\n", gettext
               ("r               - rotate scanned document"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("s               - stop playing"));         
   wprintw (misc->screenwin, "%s\n", gettext
            ("S               - show progress on/off"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("t               - select next TTS"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("T               - move cursor to the first item"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("U,+             - increase playing speed"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("v,1             - decrease playback volume"));
   wprintw (misc->screenwin, "%s\n", gettext
        ("V,7             - increase playback volume (beware of Clipping)"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("w               - store whole book to disk in WAV-format"));
   wprintw (misc->screenwin, "%s\n", gettext
            ("x               - go to the file-manager"));
   wprintw (misc->screenwin, "\n%s", gettext
            ("Press any key to leave help..."));
   nodelay (misc->screenwin, FALSE);
   wgetch (misc->screenwin);
   nodelay (misc->screenwin, TRUE);
   view_screen (misc, daisy);
   wmove (misc->screenwin, y, x);
   if (playing > -1)
      pause_resume (misc, my_attribute, daisy, sound_devices);
} // help

void previous_item (misc_t *misc, daisy_t *daisy)
{
   do
   {
      if (--misc->current < 0)
      {
         beep ();
         misc->current = 0;
         break;
      } // if
   } while (daisy[misc->current].level > misc->level);
   view_screen (misc, daisy);
} // previous_item

void next_item (misc_t *misc, daisy_t *daisy)
{
   do
   {
      if (++misc->current >= misc->total_items)
      {
         misc->current = misc->total_items - 1;
         beep ();
         break;
      } // if
   } while (daisy[misc->current].level > misc->level);
   view_screen (misc, daisy);
} // next_item

void change_level (misc_t *misc, daisy_t *daisy, char key)
{      
   if (key == 'l')
      if (++misc->level > misc->depth)
         misc->level = 1;
   if (key == 'L')
      if (--misc->level < 1)
         misc->level = misc->depth;
   if (misc->playing > -1)
      if (daisy[misc->playing].level > misc->level)
         previous_item (misc, daisy);
   view_screen (misc, daisy);
} // change_level

void save_xml (misc_t *misc)
{
   int x;
   char str[MAX_STR];
   xmlTextWriterPtr writer;
   struct passwd *pw;

   pw = getpwuid (geteuid ());
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker.xml", pw->pw_dir);
   if (! (writer = xmlNewTextWriterFilename (str, 0)))
      return;
   xmlTextWriterSetIndent (writer, 1);
   xmlTextWriterSetIndentString (writer, BAD_CAST "   ");
   xmlTextWriterStartDocument (writer, NULL, NULL, NULL);
   xmlTextWriterStartElement (writer, BAD_CAST "eBook-speaker");
   xmlTextWriterStartElement (writer, BAD_CAST "voices");
   xmlTextWriterStartComment (writer);
   xmlTextWriterWriteString (writer, BAD_CAST "\n\
/*\n\
 * This list of TTS is initially populated with a voice, accordinly\n\
 * set to the current locale, of the espeak synthesizer, provided as\n\
 * a fall back, that will be recreated if removed.\n\
 * You can add new lines for other TTS, which should also be surrounded\n\
 * by lines with just <tts> above and </tts> below.\n\
 * The command should read eBook-speaker.txt and write eBook-speaker.wav.\n\
 * Most synthesizers have a man page and a --help option that can help\n\
 * to write the corresponding command lines.\n\
 * Examples using the flite, pico2wave, mbrola, text2wave and Cepstral\n\
 * synthesizers are shown the first time you start the ebook-speaker program.\n\
 * You may remove them. You may modify and delete any added item.\n\
 * If you delete an item, remove also the lines with <tts> and </tts>\n\
 * that surround it.\n\
 * Warning: a character '<' in the command should be replaced &lt; and a\n\
 * character '>' by &gt; else ebook-speaker will fail.\n\
 */\n      ");
   xmlTextWriterEndComment (writer);

   x = 1;
   do
   {
      char str[MAX_STR];

      xmlTextWriterStartElement (writer, BAD_CAST "tts");
      snprintf (str, MAX_STR - 1, "\n         %s\n      ", misc->tts[x]);
      xmlTextWriterWriteString (writer, BAD_CAST str);
      xmlTextWriterEndElement (writer);
      x++;
   } while (*misc->tts[x] && x < 15);
   xmlTextWriterEndElement (writer);
   xmlTextWriterEndElement (writer);
   xmlTextWriterEndDocument (writer);
   xmlFreeTextWriter (writer);
} // save_xml

void load_xml (misc_t *misc, my_attribute_t *my_attribute)
{
   long x, i;
   char str[MAX_STR], *p;
   struct passwd *pw;;
   xmlTextReaderPtr reader;
   htmlDocPtr doc;

// first clear tts list
   for (x = 0; x < 15; x++)
      *misc->tts[x] = 0;

   x = 1;
   pw = getpwuid (geteuid ());

   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker.xml", pw->pw_dir);

// eSpeak NG text-to-speech: 1.49.2  Data
   snprintf (misc->tts[0], MAX_STR - 1,
     "espeak -f eBook-speaker.txt -w eBook-speaker.wav -v %s", misc->locale);
   if ((doc = htmlParseFile (str, "UTF-8")) == NULL)
   {
// If no TTS; give some examples

/* Carnegie Mellon University, Copyright (c) 1999-2016, all rights reserved
 * version: flite-2.1-release Dec 2017 (http://cmuflite.org)
 */
      strcpy (misc->tts[1],
              "flite  eBook-speaker.txt eBook-speaker.wav");

// pico2wave
      strcpy (misc->tts[2],
       "pico2wave -l=en-GB -w=eBook-speaker.wav < eBook-speaker.txt");

/* Cepstral Swift v5.1.0, July 2008
 * Copyright (C) 2000-2006, Cepstral LLC.
 */
      strcpy (misc->tts[3],
   "swift -n Callie -f eBook-speaker.txt -m text -o eBook-speaker.wav");
      strcpy (misc->tts[4],
   "swift -n Lawrence -f eBook-speaker.txt -m text -o eBook-speaker.wav");

/* mbrola -                                                 
 * Copyright (C) 1996-2008  Dr Thierry Dutoit <thierry.dutoit@fpms.ac.be>
 */
      strcpy (misc->tts[5],
   "espeak -f eBook-speaker.txt -w eBook-speaker.wav -v mb-en1");

/* (C) Centre for Speech Technology Research, 1996-1998
 * University of Edinburgh
 * 80 South Bridge
 * Edinburgh EH1 1HN
 * http://www.cstr.ed.ac.uk/projects/festival.html
 */
      strcpy (misc->tts[6],
   "text2wave eBook-speaker.txt -o eBook-speaker.wav");
      save_xml (misc);
      misc->tts_no = 0;                 
      return;
   } // if no tts give examples

   if ((reader = xmlReaderWalker (doc)))
   {
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, reader))
            break;
         if (strcasecmp (misc->tag, "prefs") == 0)
            continue;
         if (strcasecmp (misc->tag, "tts") == 0)
         {
            do
            {
               if (! get_tag_or_label (misc, my_attribute, reader))
                  break;
            } while (! *misc->label);
            for (i = 0; misc->label[i]; i++)
               if (misc->label[i] != ' ' && misc->label[i] != '\n')
                  break;;
            p = misc->label + i;
            for (i = (long) strlen (p) - 1; i > 0; i--)
               if (p[i] != ' ' && p[i] != '\n')
                  break;
            p[i + 1] = 0;
            strncpy (misc->tts[x++], p, MAX_STR - 1);
            if (x == 15)
               break;
         } // if
      } // while
      if (misc->tts_no >= x)
         misc->tts_no = (int) 0;
      xmlTextReaderClose (reader);
      xmlFreeDoc (doc);
   } // if
} // load_xml

void save_bookmark_and_xml (misc_t *misc)
{
   endwin ();
   kill_player (misc);
   wait (NULL);
   put_bookmark (misc);
   save_xml (misc);
   remove_tmp_dir (misc);
} // save_bookmark_and_xml

void pause_resume (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy,
                   audio_info_t *sound_devices)
{
   if (*misc->tts[misc->tts_no] == 0)
   {
      misc->tts_no = 0;
      select_tts (misc, my_attribute, daisy);
   } // if
   if (misc->playing < 0 && misc->pause_resume_playing < 0)
      return;
   if (misc->current_sink < 0)
      select_next_output_device (misc, daisy, sound_devices);
   if (misc->playing > -1)
   {
      misc->pause_resume_playing = misc->playing;
      misc->playing = -1;
      kill_player (misc);
      return;
   } // if

   misc->playing = misc->pause_resume_playing;
   open_xml_file (misc, my_attribute, daisy, daisy[misc->playing].xml_file,
                  daisy[misc->playing].xml_anchor);
   go_to_phrase (misc, my_attribute, daisy, misc->playing , misc->phrase_nr);
} // pause_resume

void search (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy,
             audio_info_t *sound_devices, int start, char mode)
{
   int c, found = 0;

   if (misc->playing != -1)
   {
      kill (misc->player_pid, SIGKILL);
      misc->player_pid = -2;
      misc->playing = misc->just_this_item = -1;
      view_screen (misc, daisy);
   } // if
   if (mode == '/' || *misc->search_str == 0)
   {
      mvwprintw (misc->titlewin, 1, 0,
                 "                                        ");
      wprintw (misc->titlewin, "                                        ");
      mvwprintw (misc->titlewin, 1, 0, "%s ",
                 gettext ("What do you search?"));
      echo ();
      wgetnstr (misc->titlewin, misc->search_str, 25);
      noecho ();
   } // if
   if (mode == '/' || mode == 'n')
   {
      for (c = start; c < misc->total_items; c++)
      {
         if (strcasestr (daisy[c].label, misc->search_str))
         {
            found = 1;
            break;
         } // if
      } // for
      if (! found)
      {
         for (c = 0; c < start; c++)
         {
            if (strcasestr (daisy[c].label, misc->search_str))
            {
               found = 1;
               break;
            } // if
         } // for
      } // if
   } // if
   if (mode == 'N')
   {
      for (c = start; c >= 0; c--)
      {
         if (strcasestr (daisy[c].label, misc->search_str))
         {
            found = 1;
            break;
         } // if
      } // for
      if (! found)
      {
         for (c = misc->total_items + 1; c > start; c--)
         {
            if (strcasestr (daisy[c].label, misc->search_str))
            {
               found = 1;
               break;
            } // if
         } // for
      } // if
   } // if
   if (found)
   {
      misc->current = c;
      misc->current_page_number = daisy[misc->current].page_number;
      misc->phrase_nr = 0;
      misc->just_this_item = -1;
      misc->playing = misc->current;
      misc->player_pid = -2;
      unlink (misc->tmp_wav);
      unlink ("old.wav");
      open_xml_file (misc, my_attribute, daisy, daisy[misc->current].xml_file,
                      daisy[misc->current].xml_anchor);
   }
   else
   {
      beep ();
      pause_resume (misc, my_attribute, daisy, sound_devices);
   } // if
} // search

void break_phrase (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   int level;
   char pos[5];

   xmlTextReaderClose (misc->reader);
   unlink (misc->tmp_wav);
   unlink ("old.wav");
   misc->phrase_nr = misc->total_phrases = 0;
   level = misc->level;
   mvwprintw (misc->titlewin, 1, 0, "%s ", gettext
       ("Break phrases at EOL or at a specific position (y/n/position)"));
   wclrtoeol (misc->titlewin);
   echo ();
   wgetnstr (misc->titlewin, pos, 3);
   noecho ();
   view_screen (misc, daisy);
   if (*pos == 'j')
      *pos = 'y';
   if (*pos == 'n' || *pos == 'y')
      misc->break_phrase = *pos;
   else
      misc->break_phrase = atoi (pos);
   if (misc->break_phrase == 0)
      misc->break_phrase = 'n';

// reset original xml_file
   for (misc->current = 0; misc->current < misc->total_items;
        misc->current++)
   {
      free (daisy[misc->current].xml_file);
      daisy[misc->current].xml_file =
                              strdup (daisy[misc->current].orig_xml_file);
   } // for
   misc->current = 0;
   check_phrases (misc, my_attribute, daisy);
   misc->level = level;
   *misc->search_str = 0;
   misc->playing = misc->just_this_item = -1;
   view_screen (misc, daisy);
} // break_phrase

void go_to_phrase (misc_t *misc, my_attribute_t *my_attribute,
                   daisy_t *daisy, int item, int pn)
{
   if (item <= 0 && pn <= 0)
   {
      beep ();
      return;
   } // if
   open_xml_file (misc, my_attribute, daisy,
                   daisy[item].xml_file, daisy[item].xml_anchor);
   misc->phrase_nr = 0;
   misc->current_page_number = daisy[item].page_number;
   if (pn > 1)
   {
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, misc->reader))
            break;
         if (strcasecmp (misc->tag, "pagenum") == 0 ||
             strcasecmp (my_attribute->class, "pagenum") == 0)
         {
            parse_page_number (misc, my_attribute, misc->reader);
         } // if
         if (! *misc->label)
            continue;
         misc->phrase_nr++;
         if (misc->phrase_nr + 1 == pn)
            break;
      } // while
   } // if
   misc->playing = misc->current = item;
   view_screen (misc, daisy);
   show_progress (misc, daisy);
   unlink (misc->tmp_wav);
   unlink ("old.wav");
} // go_to_phrase                            

void start_OCR (misc_t *misc, char *file)
{
   char str[MAX_STR + 1];

   if (misc->option_o == NULL)
      misc->ocr_language = strdup (misc->locale);
   else
      misc->ocr_language = strdup (misc->option_o);
   if (misc->use_cuneiform == 0)
   {
// tesseract languages
      char codes[][2][3] = {{"bg", "bul"},
                           {"da", "dan"},
                           {"de", "deu"},
                           {"en", "eng"},
                           {"es", "spa"},
                           {"fi", "fin"},
                           {"fr", "fra"},
                           {"hu", "hun"},
                           {"nb", "nor"},
                           {"nl", "nld"},
                           {"po", "pol"},
                           {"pt", "por"},
                           {"sv", "swe"}};
      int i;

      for (i = 0; i < 13; i++)
      {
         if (strcmp (misc->ocr_language, codes[i][0]) == 0)
         {
            misc->ocr_language = strdup (codes[i][1]);
            misc->ocr_language[3] = 0;
            break;
         } // if
      } // for
   }
   else
   {
// cuneiform languages
      char codes[][2][3] = {{"bg", "bul"},
                           {"cs", "cze"},
                           {"da", "dan"},
                           {"de", "ger"},
                           {"en", "eng"},
                           {"es", "spa"},
                           {"et", "est"},
                           {"fi", "fin"},
                           {"fr", "fra"},
                           {"hr", "hrv"},
                           {"hu", "hun"},
                           {"it", "ita"},
                           {"lt", "lit"},
                           {"lv", "lav"},
                           {"nl", "dut"},
                           {"po", "pol"},
                           {"pt", "por"},
                           {"ro", "rum"},
                           {"ru", "rus"},
                           {"sl", "slv"},
                           {"sr", "srp"},
                           {"sv", "swe"},
                           {"tk", "tuk"},
                           {"uk", "ukr"}};
      int i;

      for (i = 0; i < 24; i++)
      {
         if (strcmp (misc->ocr_language, codes[i][0]) == 0)
         {
            misc->ocr_language = strdup (codes[i][1]);
            misc->ocr_language[3] = 0;
            break;
         } // if
      } // for
   } // if

   if (misc->use_cuneiform == 0)
      snprintf (misc->cmd, MAX_CMD - 1,
             "tesseract -l %s \"%s\" \"%s/out\" quiet", misc->ocr_language, 
             file, misc->daisy_mp);
   else
      snprintf (misc->cmd, MAX_CMD - 1,
             "cuneiform \"%s\" -o \"%s/out.txt\" -l %s > /dev/null", file,
              misc->daisy_mp, misc->ocr_language);
   switch (system (misc->cmd))
   {
   case 0:
      break;
   default:
      endwin ();
      beep ();
      puts (misc->copyright);
      if (misc->use_cuneiform == 0)
      {
         printf ("%s\n", gettext (
      "Be sure the package \"tesseract-ocr\" is installed onto your system."));
         printf (gettext
               ("Language code \"%s\" is not a valid tesseract code."),
               misc->ocr_language);
         printf (gettext ("See the tesseract manual for valid codes."));
         sprintf (misc->str,
    "Be sure the package \"tesseract-ocr-%s\" is installed onto your system.",
                  misc->ocr_language);
         printf ("%s\n", gettext (misc->str));
      }
      else
      {
         printf
   ("Be sure the package \"cuneiform\" is installed onto your system.\n");
         printf (gettext
              ("Language code \"%s\" is not a valid cuneiform code."),
               misc->ocr_language);
         printf ("\n%s\n", gettext ("See the cuneiform manual for valid codes."));
      } // if
      fflush (stdout);
      remove_tmp_dir (misc);
      _exit (EXIT_SUCCESS);
   } // switch
   strncpy (str, misc->daisy_mp, MAX_STR);
   strncat (str, "/out.txt", MAX_STR);
   pandoc_to_epub (misc, "markdown", str);
} // start_OCR

void browse (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy,
             audio_info_t *sound_devices, char *file)
{
   int old;

   misc->current = misc->phrase_nr = 0;
   misc->level = 1;
   misc->current_sink = 0;
   get_list_of_sound_devices (misc, sound_devices);
   if (misc->ignore_bookmark == 1)
   {
      if (misc->option_d == NULL)
         select_next_output_device (misc, daisy, sound_devices);
   }
   else
   {
      get_bookmark (misc, my_attribute, daisy, sound_devices);
      load_xml (misc, my_attribute);
      check_phrases (misc, my_attribute, daisy);
      go_to_phrase (misc, my_attribute, daisy, misc->current,
                    ++misc->phrase_nr - 1);
   } // if
   if (misc->option_d)
   {
      int sink;

      if (! strchr (misc->option_d, ':'))
      {
         beep ();
         endwin ();
         printf ("\n");
         usage ();
         _exit (EXIT_FAILURE);
      } // if
      get_list_of_sound_devices (misc, sound_devices);
      for (sink = 0; sink < misc->total_sinks; sink++)
      {
         char dt[50];

         sprintf (dt, "%s:%s", sound_devices[sink].device,
                  sound_devices[sink].type);
         if (strcasecmp (misc->option_d, dt) == 0)
         {
            misc->current_sink = sink;
            break;
         } // if
      } // for
   } // if
   if (*misc->option_t)
   {
      misc->tts_no = 0;
      strcpy (misc->tts[misc->tts_no], misc->option_t);
   } // if
   view_screen (misc, daisy);
   nodelay (misc->screenwin, TRUE);
   remove_double_tts_entries (misc);
   if (misc->scan_flag == 1)
   {
      misc->current = 0;
      misc->playing = -1;
      unlink (misc->tmp_wav);
      unlink ("old.wav");
      misc->phrase_nr = 0;
      misc->just_this_item = -1;
      misc->current_page_number = daisy[misc->playing].page_number;
      view_screen (misc, daisy);
      if (misc->player_pid > -1)
         kill_player (misc);
      misc->player_pid = -2;
   } // if

   wmove (misc->screenwin, daisy[misc->current].y,
          daisy[misc->current].x - 1);
   while (! misc->term_signaled) // forever if not signaled
   {
      switch (wgetch (misc->screenwin))
      {
      case 13: // ENTER
         misc->playing = misc->current;
         unlink (misc->tmp_wav);
         unlink ("old.wav");
         misc->phrase_nr = 0;
         misc->just_this_item = -1;
         misc->current_page_number = daisy[misc->playing].page_number;
         view_screen (misc, daisy);
         if (misc->player_pid > -1)
            kill_player (misc);
         misc->player_pid = -2;
         open_xml_file (misc, my_attribute, daisy,
             daisy[misc->current].xml_file, daisy[misc->current].xml_anchor);
         break;
      case '/':
         search (misc, my_attribute, daisy, sound_devices,
                 misc->current + 1, '/');
         view_screen (misc, daisy);
         break;
      case ' ':
      case KEY_IC:
      case '0':
         if (! misc->update_progress)
         {
            if (misc->playing > -1)
               show_progress (misc, daisy);
            else
               view_total_phrases (misc, daisy, misc->playing);
         } // if
         pause_resume (misc, my_attribute, daisy, sound_devices);
         break;
      case 'b':
         kill_player (misc);
         break_phrase (misc, my_attribute, daisy);
         view_screen (misc, daisy);
         break;
      case 'B':
         misc->current = misc->total_items - 1;
         misc->phrase_nr = daisy[misc->current].n_phrases - 1;
         view_screen (misc, daisy);
         break;
      case 'f':
         if (misc->playing == -1)
         {
            beep ();
            break;
         } // if
         misc->current = misc->playing;
         view_screen (misc, daisy);
         break;
      case 'g':
      {
         char pn[15];

         kill_player (misc);
         misc->player_pid = -2;
         misc->playing = misc->just_this_item = -1;
         view_screen (misc, daisy);
         mvwprintw (misc->titlewin, 1, 0,
                    "----------------------------------------");
         wprintw (misc->titlewin, "----------------------------------------");
         mvwprintw (misc->titlewin, 1, 0, "%s ",
                    gettext ("Go to phrase in current item:"));
         echo ();
         wgetnstr (misc->titlewin, pn, 8);
         noecho ();
         view_screen (misc, daisy);
         if (! *pn || atoi (pn) >= daisy[misc->current].n_phrases)
         {
            beep ();
            pause_resume (misc, my_attribute, daisy, sound_devices);
            pause_resume (misc, my_attribute, daisy, sound_devices);
            break;
         } // if
         go_to_phrase (misc, my_attribute, daisy, misc->current , atoi (pn));
         break;
      }
      case 'G':
         if (misc->total_pages)
            go_to_page_number (misc, my_attribute, daisy, sound_devices);
         else
            beep ();
         break;
      case 'h':
      case '?':
         help (misc, my_attribute, daisy, sound_devices);
         view_screen (misc, daisy);
         break;
      case 'j':
      case '5':
      case KEY_B2:
         if (misc->just_this_item >= 0)
            misc->just_this_item = -1;
         else
         {
            if (misc->playing >= 0)
               misc->just_this_item = misc->playing;
            else
               misc->just_this_item = misc->current;
         } // if
         mvwprintw (misc->screenwin, daisy[misc->current].y, 0, " ");
         if (misc->playing == -1)
         {
            misc->phrase_nr = 0;
            misc->just_this_item = misc->playing = misc->current;
            kill_player (misc);
            misc->player_pid = -2;
            open_xml_file (misc, my_attribute, daisy,
                           daisy[misc->current].xml_file,
                           daisy[misc->current].xml_anchor);
         } // if
         wattron (misc->screenwin, A_BOLD);
         if (misc->just_this_item != -1 &&
             daisy[misc->current].screen == daisy[misc->playing].screen)
            mvwprintw (misc->screenwin, daisy[misc->current].y, 0, "J");
         else
            mvwprintw (misc->screenwin, daisy[misc->current].y, 0, " ");
         wrefresh (misc->screenwin);
         wattroff (misc->screenwin, A_BOLD);
         misc->current_page_number = daisy[misc->playing].page_number;
         break;
      case 'l':
         change_level (misc, daisy, 'l');
         break;
      case 'L':
         change_level (misc, daisy, 'L');
         break;
      case 'm':
         if (strcmp (sound_devices[misc->current_sink].type, "alsa") == 0)
         {
            sprintf (misc->cmd, "/usr/bin/amixer -q -D %s set Master playback toggle",
                     sound_devices[misc->current_sink].device);
            system (misc->cmd);
         }
         else
         {
            if (fork () == 0)
            {
               reset_term_signal_handlers_after_fork ();
               pactl ("set-sink-mute",
                      sound_devices[misc->current_sink].device,
                      "toggle");
               _exit (EXIT_SUCCESS);
            } // if
         } // if
         break;
      case 'n':
         search (misc, my_attribute, daisy, sound_devices,
                 misc->current + 1, 'n');
         view_screen (misc, daisy);
         break;
      case 'N':
         search (misc, my_attribute, daisy, sound_devices,
                 misc->current - 1, 'N');
         view_screen (misc, daisy);
         break;
      case 'o':
         kill_player (misc);
         select_next_output_device (misc, daisy, sound_devices);
         break;
      case 'p':
         put_bookmark (misc);
         save_xml (misc);
         break;
      case 'q':
         quit_eBook_speaker (misc, my_attribute, daisy);
         return;
      case 'r':
      {
         char str[MAX_STR];

         if (misc->scan_flag != 1)
         {
            beep ();
            break;
         } // if
         kill_player (misc);
         xmlTextReaderClose (misc->reader);
         misc->total_items = misc->depth = 1;
         misc->phrase_nr = 0;
         misc->total_phrases = misc->total_pages = 0;
         misc->playing = misc->just_this_item = -1;
         misc->current = 0;
         snprintf (misc->cmd, MAX_CMD - 1,
                   "cd \"%s\" && rm -rf out*", misc->daisy_mp);
         system (misc->cmd);
         wclear (misc->titlewin);
         wclear (misc->screenwin);
         wrefresh (misc->titlewin);
         wrefresh (misc->screenwin);
         wattron (misc->titlewin, A_BOLD);
         mvwprintw (misc->titlewin, 0, 0, "%s - %s", misc->copyright,
                  gettext ("Please wait..."));
         wattroff (misc->titlewin, A_BOLD);
         mvwprintw (misc->titlewin, 1, 0, "Rotating...");
         wrefresh (misc->titlewin);
         snprintf (str, MAX_STR - 1, "%s.rotated", file);
         snprintf (misc->cmd, MAX_CMD, "pnmflip -r90 %s > %s", file, str);
         switch (system (misc->cmd))
         {
         case 1:
            endwin ();
            beep ();
            puts (misc->copyright);
            printf
          ("Be sure the package \"netpbm\" is installed onto your system.\n");
            puts (gettext ("eBook-speaker cannot handle this file."));
            fflush (stdout);
            remove_tmp_dir (misc);
            _exit (EXIT_FAILURE);
         default:
            break;
         } // switch
         file = malloc (MAX_STR + 1);
         strncpy (file, str, MAX_STR);
         start_OCR (misc, file);
         daisy = create_daisy_struct (misc, my_attribute, daisy);
         read_daisy_3 (misc, my_attribute, daisy);
         fill_page_numbers (misc, daisy, my_attribute);
         check_phrases (misc, my_attribute, daisy);
         strcpy (daisy[0].label, "1");
         strcpy (misc->daisy_title, "scanned image");
         misc->level = misc->depth = 1;
         *misc->search_str = 0;
         misc->reader = NULL;
         if (misc->depth <= 0)
            misc->depth = 1;
         nodelay (misc->screenwin, TRUE);
         wclear (misc->titlewin);
         mvwprintw (misc->titlewin, 0, 0, misc->copyright);
         if ((int) strlen (misc->daisy_title) +
             (int) strlen (misc->copyright) >= misc->max_x)
         {
            mvwprintw (misc->titlewin, 0,
                       misc->max_x - (int) strlen
                       (misc->daisy_title) - 4, "... ");
         } // if
         mvwprintw (misc->titlewin, 0,
                    misc->max_x - (int) strlen (misc->daisy_title),
                    "%s", misc->daisy_title);
         mvwprintw (misc->titlewin, 1, 0,
                    "----------------------------------------");
         wprintw (misc->titlewin, "----------------------------------------");
         mvwprintw (misc->titlewin, 1, 0, "%s ",
                    gettext ("Press 'h' for help"));
         wrefresh (misc->titlewin);
         misc->current = 0;
         misc->playing = misc->just_this_item = -1;
         view_screen (misc, daisy);
         kill_player (misc);
         break;
      } // 'r'
      case 's':
         misc->playing = misc->just_this_item = -1;
         view_screen (misc, daisy);
         kill_player (misc);
         break;
      case 'S':
         misc->update_progress = 1 - misc->update_progress;
         break;
      case 't':
         misc->playing = -1;
         select_tts (misc, my_attribute, daisy);
         view_screen (misc, daisy);
         break;
      case 'T':
         misc->current = 0;
         misc->phrase_nr = daisy[misc->current].n_phrases - 1;
         view_screen (misc, daisy);
         break;
      case KEY_DOWN:
      case '2':
         next_item (misc, daisy);
         break;
      case KEY_UP:
      case '8':
         previous_item (misc, daisy);
         break;
      case KEY_RIGHT:
      case '6':
         skip_right (misc, daisy);
         if (! misc->update_progress)
            show_progress (misc, daisy);
         break;
      case KEY_LEFT:
      case '4':
         skip_left (misc, my_attribute, daisy);
         if (! misc->update_progress)
            show_progress (misc, daisy);
         break;
      case KEY_NPAGE:
      case '3':
         if (misc->current / misc->max_y ==
             (misc->total_items - 1) / misc->max_y)
         {
            beep ();
            break;
         } // if
         old = misc->current / misc->max_y;
         while (misc->current / misc->max_y == old)
            next_item (misc, daisy);
         view_screen (misc, daisy);
         break;
      case KEY_PPAGE:
      case '9':
         if (misc->current / misc->max_y == 0)
         {
            beep ();
            break;
         } // if
         old = misc->current / misc->max_y - 1;
         misc->current = 0;
         while (misc->current / misc->max_y != old)
            next_item (misc, daisy);
         view_screen (misc, daisy);
         break;
      case ERR:
         break;
      case 'U':
      case '+':
         if (misc->speed >= 2)
         {
            beep ();
            break;
         } // if
         misc->speed += 0.1;
         if (misc->playing == -1)
            break;
         pause_resume (misc, my_attribute, daisy, sound_devices);
         pause_resume (misc, my_attribute, daisy, sound_devices);
         break;
      case 'd':
         store_item_as_WAV_file (misc, my_attribute, daisy,
                                 sound_devices, NO);
         skip_left (misc, my_attribute, daisy);
         break;
      case 'A':
         store_item_as_ASCII_file (misc, my_attribute, daisy, sound_devices);
         break;
      case 'D':
      case '-':
         if (misc->speed <= 0.1)
         {
            beep ();
            break;
         } // if
         misc->speed -= 0.1;
         if (misc->playing == -1)
            break;
         pause_resume (misc, my_attribute, daisy, sound_devices);
         pause_resume (misc, my_attribute, daisy, sound_devices);
         break;
      case KEY_HOME:
      case '*':
         misc->speed = 1.0;
         if (misc->playing < 0)
            break;
         misc->current = misc->playing;
         pause_resume (misc, my_attribute, daisy, sound_devices);
         pause_resume (misc, my_attribute, daisy, sound_devices);
         break;
      case 'v':
      case '1':
         if (strcmp (sound_devices[misc->current_sink].type, "alsa") == 0)
         {
            sprintf (misc->cmd, "/usr/bin/amixer -q -D %s set Master playback 5%%-",
                     sound_devices[misc->current_sink].device);
            system (misc->cmd);
         }
         else
         {
            if (fork () == 0)
            {
               reset_term_signal_handlers_after_fork ();
               pactl ("set-sink-volume",
                      sound_devices[misc->current_sink].device,
                      "-5%");
               _exit (EXIT_SUCCESS);
            } // if
         } // if
         break;
      case 'V':
      case '7':
         if (strcmp (sound_devices[misc->current_sink].type, "alsa") == 0)
         {
            sprintf (misc->cmd, "/usr/bin/amixer -q -D %s set Master playback 5%%+",
                     sound_devices[misc->current_sink].device);
            system (misc->cmd);
         }
         else
         {
            if (fork () == 0)
            {
               reset_term_signal_handlers_after_fork ();
               pactl ("set-sink-volume",
                      sound_devices[misc->current_sink].device,
                      "+5%");
               _exit (EXIT_SUCCESS);
            } // if
         } // if
         break;
      case 'w':
         store_item_as_WAV_file (misc, my_attribute, daisy,
                                 sound_devices, YES);
         skip_left (misc, my_attribute, daisy);
         break;
      case 'x':
      {
         kill_player (misc);
         put_bookmark (misc);
         misc->playing = misc->just_this_item = -1;
         misc->current = 0;
         misc->total_pages = 0;
         clear_tmp_dir (misc);
         strcpy (file,
                 get_input_file (misc, my_attribute, daisy, file));
         wclear (misc->screenwin);
         wrefresh (misc->screenwin);
         create_epub (misc, my_attribute, daisy, file, 1);
         play_epub (misc, my_attribute, daisy, sound_devices, file);
         break;
      }
      default:
         beep ();
         break;
      } // switch

      if (misc->playing > -1)
      {
         if (kill (misc->player_pid, 0) != 0)
         {
// if not playing
            misc->player_pid = -2;
            get_next_phrase (misc, my_attribute, daisy, sound_devices, 1);

            if (misc->update_progress)
               show_progress (misc, daisy);
            else
               view_total_phrases (misc, daisy, misc->playing);
         } // if
      } // if

      fd_set rfds;
      struct timeval tv;

      FD_ZERO (&rfds);
      FD_SET (0, &rfds);
      tv.tv_sec = 0;
      tv.tv_usec = 100000;
// pause till a key has been pressed or 0.1 seconds are elapsed
      select (1, &rfds, NULL, NULL, &tv);
   } // while
   quit_eBook_speaker (misc, my_attribute, daisy);
} // browse

void extract_epub (misc_t *misc, char *file)
{
   snprintf (misc->cmd, MAX_CMD - 1,
             "unar -q \"%s\" -f -o \"%s\"", file, misc->tmp_dir);
   switch (system (misc->cmd))
   {
   case 0:
      break;
   default:
      endwin ();
      printf
         ("Be sure the package \"unar\" is installed onto your system.\n");
      failure (misc, gettext ("eBook-speaker cannot handle this file."), errno);
   } // switch
} // extract_epub

void lowriter_to_txt (misc_t *misc, char *file)
{
   snprintf (misc->cmd, MAX_CMD - 1,
            "cp \"%s\" out.doc && \
             lowriter --headless --convert-to txt out.doc > /dev/null", file);
   switch (system (misc->cmd))
   {
   case 0:
      return;
   default:
      endwin ();
      printf
("Be sure the package \"libreoffice-writer\" is installed onto your system.\n\
Be sure the package \"libreoffice-common\" is installed onto your system.\n\
Be sure the package \"libreoffice-core\" is installed onto your system.\n");
      failure (misc, gettext ("eBook-speaker cannot handle this file."), errno);
   } // switch
} // lowriter_to_txt

void pandoc_to_epub (misc_t *misc, char *from, char *file)
{
   char str[MAX_STR + 1];

   strncpy (str, misc->tmp_dir, MAX_STR);
   strncat (str, "/out.epub", MAX_STR);
   sprintf (misc->cmd, 
    "(iconv -c -t utf-8 \"%s\" | pandoc -s -f %s -t epub -o %s) 2> /dev/null",
             file, from, str);
   switch (system (misc->cmd))
   {
   case 0:
      break;
   default:
   {
      int e;

      e = EINVAL;
      endwin ();
      printf
           ("Be sure the package \"pandoc\" is installed onto your system.\n");
      failure (misc, gettext ("eBook-speaker cannot handle this file."), e);
   }
   } // switch
   if (access (str, R_OK) == -1)
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, "%s: %s\n", file, gettext ("Unknown format"));
      failure (misc, str, e);
   } // if
   extract_epub (misc, str);
} // pandoc_to_epub            

void pdf_to_txt (misc_t *misc, const char *file)
{
   snprintf (misc->cmd, MAX_CMD - 1, "pdftotext -q \"%s\" out.txt", file);
   switch (system (misc->cmd))
   {
   case 0:
      break;
   default:
   {
      int e;

      e = errno;
      beep ();
      endwin ();
      printf
     ("Be sure the package \"poppler-utils\" is installed onto your system.\n");
      failure (misc, gettext ("eBook-speaker cannot handle this file."), e);
   }
   } // switch
} // pdf_to_txt

void fold (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy, int i)
{
   char *txt_file, *folded_file;
   FILE *p;
   xmlTextWriterPtr writer;
   char *id;

   id = strdup (my_attribute->id);
   txt_file = malloc (strlen (daisy[i].xml_file) + 10);
   sprintf (txt_file, "%s.txt", daisy[i].xml_file);
   write_ascii (misc, my_attribute, daisy, i, txt_file);
   sprintf (misc->cmd, "/usr/bin/fold -s -w %d %s", misc->break_phrase,
            txt_file);
   folded_file = malloc (strlen (txt_file) + 10);
   sprintf (folded_file, "%s.folded", txt_file);
   p = popen (misc->cmd, "r");
   if (! (writer = xmlNewTextWriterFilename (folded_file, 0)))
      return;
   xmlTextWriterSetIndent (writer, 1);
   xmlTextWriterSetIndentString (writer, BAD_CAST "   ");
   xmlTextWriterStartDocument (writer, NULL, NULL, NULL);
   xmlTextWriterStartElement (writer, BAD_CAST "head");
   xmlTextWriterEndElement (writer);
   xmlTextWriterStartElement (writer, BAD_CAST "body");
   xmlTextWriterStartElement (writer, BAD_CAST "div");
   xmlTextWriterWriteFormatAttribute (writer, BAD_CAST "id", "%s", id);
   xmlTextWriterWriteString (writer, BAD_CAST "\n");
   while (1)
   {
      size_t len;
      char *str;

      if (feof (p) != 0)
         break;
      len = 0;
      str = NULL;
      if ((signed) (len = (size_t) getline (&str, &len, p)) == -1)
         break;
      xmlTextWriterWriteFormatString (writer, "%s", str);
      xmlTextWriterStartElement (writer, BAD_CAST "br");
      xmlTextWriterEndElement (writer);
   } // while
   xmlTextWriterEndElement (writer);
   xmlTextWriterEndDocument (writer);
   xmlFreeTextWriter (writer);
   pclose (p);
   daisy[i].xml_file = strdup (folded_file);
   free (txt_file);
   free (folded_file);
} // fold

void check_phrases (misc_t *misc, my_attribute_t *my_attribute,
                    daisy_t *daisy)
{
   int i;

   if (misc->total_items == 0)
      return;

   for (i = 0; i < misc->total_items; i++)
   {
      if (access (daisy[i].xml_file, R_OK) == -1)
         continue;
      open_xml_file (misc, my_attribute, daisy, daisy[i].xml_file,
                     daisy[i].xml_anchor);
      if (misc->break_phrase == 'n' || misc->break_phrase == 'y' ||
          misc->break_phrase == 'j')
      {
         split_phrases (misc, my_attribute, daisy, i);
      }
      else
         fold (misc, my_attribute, daisy, i);
      if (daisy[i].x <= 0)
         daisy[i].x = 2;
      if ((signed) strlen (daisy[i].label) + daisy[i].x >= 53)
         daisy[i].label[57 - daisy[i].x] = 0;
   } // for

   if (misc->total_items == 0)
   {
      endwin ();
      printf ("%s\n",
              gettext ("Please try to play this book with daisy-player"));
      beep ();
      quit_eBook_speaker (misc, my_attribute, daisy);
      _exit (EXIT_FAILURE);
   } // if

   count_phrases (misc, my_attribute, daisy);
   if (misc->total_phrases == 0)
   {
      endwin ();
      printf ("%s\n",
              gettext ("Please try to play this book with daisy-player"));
      beep ();
      quit_eBook_speaker (misc, my_attribute, daisy);
      _exit (EXIT_FAILURE);
   } // if

// remove item with 0 phrases
   int j = 0;

   for (i = 0; i < misc->total_items; i++)
   {
      if (daisy[i].n_phrases > 0)
         continue;
      misc->total_items--;

// move items
      for (j = i; j < misc->total_items; j++)
      {
         daisy[j].n_phrases = daisy[j + 1].n_phrases;
         daisy[j].playorder  = daisy[j + 1].playorder;
         daisy[j].x = daisy[j + 1].x;
         daisy[j].level = daisy[j + 1].level;
         daisy[j].page_number = daisy[j + 1].page_number;
         daisy[j].xml_file = strdup (daisy[j + 1].xml_file);
         daisy[j].xml_anchor = strdup (daisy[j + 1].xml_anchor);
         daisy[j].orig_xml_file = strdup (daisy[j + 1].orig_xml_file);
         strncpy (daisy[j].my_class, daisy[j + 1].my_class, MAX_STR - 1);
         if (! daisy[i].label)
            continue;
         strncpy (daisy[j].label, daisy[j + 1].label, 80);
         if (*daisy[j].label == 0)
         {
            sprintf (daisy[j].label, "%d", j + 1);
         } // if
      } // for
   } // for

   for (i = misc->total_items - 1; i >= 0; i--)
   {
      if (daisy[i].n_phrases == 0)
         misc->total_items--;
   } // for

// calculate y and screen
   int y = 0;

   for (i = 0; i < misc->total_items; i++)
   {
      daisy[i].y = y++;
      if (y >= misc->max_y)
         y = 0;
      daisy[i].screen = i / misc->max_y;

      if (! *daisy[i].label)
      {
         sprintf (daisy[i].label, "%d", i + 1);
      } // if
   } // for
} // check_phrases

void store_item_as_WAV_file (misc_t *misc, my_attribute_t *my_attribute,
            daisy_t *daisy, audio_info_t *sound_devices, int whole_book_flag)
{
   char *label;
   char complete_wav[MAX_STR], complete_cdr[MAX_STR], out_cdr[MAX_STR];
   int i, current, pause_flag, phrase_nr, n_phrases;
   int tot_phrase_nr, total_phrases, w;
   struct passwd *pw;
   FILE *fw;

   pause_flag = misc->playing;
   wclear (misc->screenwin);
   if (whole_book_flag == YES)
   {
      current = 0;
   }
   else
   {
      current = misc->current;
   } // if
   if (whole_book_flag == NO)
   {
      while (1)
      {
         if (current == 0)
            break;
         if (daisy[current].level <= misc->level)
            break;
         current--;
      } // while
   } // if
   label = strdup (daisy[current].label);
   for (i = 0; label[i] != 0; i++)
   {
      if (label[i] == '/' ||
          isspace (label[i]))
         label[i] = '_';
   } // for
   pw = getpwuid (geteuid ());
   sprintf (complete_wav, "%s/%s.wav", pw->pw_dir, label);
   sprintf (complete_cdr, "%s/complete.cdr", misc->tmp_dir);
   unlink (complete_cdr);
   sprintf (out_cdr, "%s/out.cdr", misc->tmp_dir);
   unlink (out_cdr);
   while (access (complete_wav, F_OK) == 0)
      strncat (complete_wav, ".wav", MAX_STR - 1);
   if (pause_flag > -1)
      pause_resume (misc, my_attribute, daisy, sound_devices);

   if (! *misc->tts[misc->tts_no])
   {
      misc->tts_no = 0;
      select_tts (misc, my_attribute, daisy);
   } // if
   wprintw (misc->screenwin,
            "\nStoring \"%s\" as \"%s\" into your home-folder...",
            daisy[current].label, basename (complete_wav));
   wrefresh (misc->screenwin);
   total_phrases = 0;
   i = current;
   while (1)
   {
      total_phrases += daisy[i].n_phrases;
      i++;
      if (i == misc->total_items)
         break;
      if (whole_book_flag == NO)
         if (daisy[i].level <= misc->level)
            break;
   } // while

   w = open (complete_cdr, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR);
   tot_phrase_nr = 1;
   while (1)
   {
      n_phrases = daisy[current].n_phrases;
      open_xml_file (misc, my_attribute, daisy,
             daisy[current].xml_file, daisy[current].xml_anchor);
      phrase_nr = 1;
      while (phrase_nr <= n_phrases)
      {
         int r;

         if (! get_tag_or_label (misc, my_attribute, misc->reader))
            break;
         if (! *misc->label)
            continue;
         mvwprintw (misc->screenwin, 7, 3,
            "Synthesizing phrase %d of %d...",
            tot_phrase_nr, total_phrases);
         wrefresh (misc->screenwin);
         fw = fopen (misc->eBook_speaker_txt, "w");
         fwrite (misc->label, strlen (misc->label), 1, fw);
         fclose (fw);
         system (misc->tts[misc->tts_no]);

         sprintf (misc->cmd,
                  "/usr/bin/sox eBook-speaker.wav -b 16 -c 2 -r  44100 \"%s\"",
                  out_cdr);
         system (misc->cmd);
         r = open (out_cdr, O_RDONLY);
         while (1)
         {
#define SIZE 1024
            char buf[SIZE];
            ssize_t in;

            in = read (r, buf, SIZE);
            write (w, buf, (size_t) in);
            if (in <= 0)
               break;
         } // while
         close (r);
         phrase_nr++;
         tot_phrase_nr++;
      } // while
      current++;
      if (current >= misc->total_items)
         break;
      if (whole_book_flag == NO)
         if (daisy[current].level <= misc->level)
            break;
   } // while
   close (w);
   sprintf (misc->cmd, "/usr/bin/sox \"%s\" -c 1 -r 22050 \"%s\"",
            complete_cdr, complete_wav);
   system (misc->cmd);

   if (pause_flag > -1)
      pause_resume (misc, my_attribute, daisy, sound_devices);
   view_screen (misc, daisy);
} // store_item_as_WAV_file

void write_ascii (misc_t *misc, my_attribute_t *my_attribute,
                  daisy_t *daisy, int i, char *outfile)
{
   FILE *w;

   if ((w = fopen (outfile, "w")) == NULL)
   {
      int e;

      e = errno;
      beep ();
      failure (misc, outfile, e);
   } // if

   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
         break;
      if (*my_attribute->id && *daisy[i + 1].xml_anchor &&
          strcasecmp (my_attribute->id, daisy[i + 1].xml_anchor) == 0)
         break;
      if (! *misc->label)
         continue;;
      fprintf (w, "%s\n", misc->label);
   } // while
   fclose (w);
} // write_ascii

void store_item_as_ASCII_file (misc_t *misc, my_attribute_t *my_attribute,
                       daisy_t *daisy, audio_info_t *sound_devices)
{
   char *str, out_file[MAX_STR];
   int i, pause_flag, current;
   struct passwd *pw;;
   FILE *w;

   pause_flag = misc->playing;
   wclear (misc->screenwin);
   current = misc->current;
   while (1)
   {
// find first item to write to output
      if (current == 0)
         break;
      if (daisy[current].level <= misc->level)
         break;
      current--;
   } // while
   str = strdup (daisy[current].label);
   for (i = 0; str[i] != 0; i++)
   {
      if (str[i] == '/')
         str[i] = '-';
      if (isspace (str[i]))
         str[i] = '_';
   } // for
   wprintw (misc->screenwin,
            "\nStoring \"%s\" as \"%s\" into your home-folder...",
            daisy[current].label, str);
   wrefresh (misc->screenwin);
   pw = getpwuid (geteuid ());
   sprintf (out_file, "%s/%s.txt", pw->pw_dir, str);
   while (access (out_file, F_OK) == 0)
      strcat (out_file, ".txt");
   if (pause_flag > -1)
      pause_resume (misc, my_attribute, daisy, sound_devices);
   if ((w = fopen (out_file, "w")) == NULL)
   {
      int e;

      e = errno;
      beep ();
      failure (misc, out_file, e);
   } // if
   while (1)
   {
      open_xml_file (misc, my_attribute, daisy,
            daisy[current].xml_file, daisy[current].xml_anchor);
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, misc->reader))
            break;
         if (*my_attribute->id && *daisy[current + 1].xml_anchor &&
             strcasecmp (my_attribute->id, daisy[current + 1].xml_anchor) == 0)
            break;
         if (! *misc->label)
            continue;;
         fprintf (w, "%s\n", misc->label);
      } //while
      current++;
      if (daisy[current].level <= misc->level)
         break;
   } // while
   fclose (w);
   if (pause_flag > -1)
      pause_resume (misc, my_attribute, daisy, sound_devices);
   view_screen (misc, daisy);
} // store_item_as_ASCII_file

void usage ()
{
   endwin ();
   printf ("%s %s\n", gettext ("eBook-speaker - Version"), PACKAGE_VERSION);
   puts ("(C)2011-2021 J. Lemmens");
   printf ("\n");
   printf (gettext
      ("Usage: %s [eBook_file | URL | -s [-r resolution]] "
       "[-b n | y | position] [-c] [-d audio_device:audio_type] "
       "[-h] [-H] "
       "[-i] [-o language-code] [-S] [-t TTS_command] [-v]"), PACKAGE);
   printf ("\n");
   fflush (stdout);
} // usage

void use_calibre (misc_t *misc, char *file, char *input_profile)
{      
   char cmd[MAX_CMD];

   snprintf (cmd, MAX_CMD - 1,
             "ebook-convert \"%s\" out.epub --input-profile=%s \
              --no-default-epub-cover --no-svg-cover --epub-inline-toc \
              --disable-font-rescaling --linearize-tables \
              --remove-first-image > /dev/null", file, input_profile);
   switch (system (cmd))
   {
   case 0:
      break;
   default:
      endwin ();
      printf
           ("Be sure the package \"calibre\" is installed onto your system.\n");
      failure (misc, gettext ("eBook-speaker cannot handle this file."), errno);
   } // switch
   extract_epub (misc, "out.epub");
} // use_calibre

char *ascii2html (misc_t *misc, char *file)
{
   FILE *r, *w;
   static char str[MAX_STR + 1];
   char *line;
   size_t bytes;

   if (! (r = fopen (file , "r")))
      failure (misc, file, errno);
   strncpy (str, misc->daisy_mp, MAX_STR);
   strncat (str, "/", MAX_STR);
   strncat (str, basename (file), MAX_STR);
   strncat (str, ".xml", MAX_STR);
   if (! (w = fopen (str, "w")))
      failure (misc, str, errno);
   fprintf (w,
           "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
   fprintf (w, "<html>\n");
   fprintf (w, "<head>\n");
   fprintf (w, "</head>\n");
   fprintf (w, "<body>\n");
   while (1)
   {
      bytes = 0;
      line = NULL;
      bytes = (size_t) getline (&line, &bytes, r);
      if ((int) bytes == -1)
         break;
      fprintf (w, "%s<br>\n", line);
   } // while
   fclose (r);
   fprintf (w, "</body>\n</html>\n");
   fclose (w);
   return str;
} // ascii2html                                            

void play_epub (misc_t *misc, my_attribute_t *my_attribute,
                daisy_t *daisy, audio_info_t *sound_devices, char *file)
{
   int i;

   daisy = create_daisy_struct (misc, my_attribute, daisy);

// first be sure all strings in daisy_t are cleared
   for (i = 0; i < misc->total_items; i++)
   {
      daisy[i].xml_file = strdup ("");
      daisy[i].xml_anchor = strdup ("");
      *daisy[i].class = *daisy[i].my_class = 0;
      *daisy[i].label = 0;
   } // for
   if (access (misc->ncc_html, R_OK) == 0)
   {
// this is daisy2
      htmlDocPtr doc;

      doc = htmlParseFile (misc->ncc_html, "UTF-8");
      if (! (misc->reader = xmlReaderWalker (doc)))
      {
         int e;
         char str[MAX_STR];

         e = errno;
         snprintf (str, MAX_STR, gettext ("Cannot read %s"), misc->ncc_html);
         failure (misc, str, e);
      } // if

      while (1)
      {
// if misc->bookmark_title already is set
         if (*misc->bookmark_title)
            break;
         if (! get_tag_or_label (misc, my_attribute, misc->reader))
            break;
         if (strcasecmp (misc->tag, "title") == 0)
         {
            if (! get_tag_or_label (misc, my_attribute, misc->reader))
               break;
            if (*misc->label)
            {
               strncpy (misc->bookmark_title, misc->label, MAX_STR - 1);
               if (misc->verbose)
               {
                  printf ("\r\nTitle: %s", misc->bookmark_title);
                  fflush (stdout);
               } // if
               break;
            } // if
         } // if
      } // while
      fill_daisy_struct_2 (misc, my_attribute, daisy);
   }
   else
   {
// this is daisy3
      read_daisy_3 (misc, my_attribute, daisy);
      fill_page_numbers (misc, daisy, my_attribute);
   } // if

   check_phrases (misc, my_attribute, daisy);
   misc->reader = NULL;
   misc->just_this_item = misc->playing = -1;
   if (misc->scan_flag == 1)
      strcpy (misc->daisy_title, "scanned image");
   if (! *misc->daisy_title)
      strncpy (misc->daisy_title, basename (file), MAX_STR - 1);
   if (*misc->bookmark_title == 0)
      strncpy (misc->bookmark_title, basename (file), MAX_STR - 1);
   kill_player (misc);
   unlink (misc->tmp_wav);
   unlink ("old.wav");
   for (misc->current = 0; misc->current < misc->total_items; misc->current++)
   {
      daisy[misc->current].screen = misc->current / misc->max_y;
      daisy[misc->current].y =
               misc->current - daisy[misc->current].screen * misc->max_y;
   } // for
   misc->current = 0;
   if (misc->depth <= 0)
      misc->depth = 1;
   view_screen (misc, daisy);

   wclear (misc->titlewin);
   mvwprintw (misc->titlewin, 0, 0, misc->copyright);
   if ((int) strlen (misc->daisy_title) + (int)
       strlen (misc->copyright) >= misc->max_x)
      mvwprintw (misc->titlewin, 0, misc->max_x - (int) 
                strlen (misc->daisy_title) - 4, "... ");
   mvwprintw (misc->titlewin, 0, misc->max_x - (int) strlen (misc->daisy_title),
              "%s", misc->daisy_title);
   mvwprintw (misc->titlewin, 1, 0,
              "----------------------------------------");
   wprintw (misc->titlewin, "----------------------------------------");
   mvwprintw (misc->titlewin, 1, 0, "%s ", gettext ("Press 'h' for help"));
   wrefresh (misc->titlewin);
   browse (misc, my_attribute, daisy, sound_devices, file);
   remove_tmp_dir (misc);
} // play_epub

void create_epub (misc_t *misc, my_attribute_t *my_attribute,
                  daisy_t *daisy, char *file, int print)
{
   magic_t myt;

   misc->daisy_mp = malloc (strlen (misc->tmp_dir) + 5);
   strcpy (misc->daisy_mp, misc->tmp_dir);
   if (misc->daisy_mp[strlen (misc->daisy_mp) - 1] != '/')
      strcat (misc->daisy_mp, "/");

// determine filetype
   myt = magic_open (MAGIC_FLAGS);
   if (magic_load (myt, NULL) != 0)
      failure (misc, "magic_load", errno);
   if (magic_file (myt, file) == NULL)
   {
      int e;

      e = errno;
      endwin ();
      beep ();
      printf ("%s: %s\n", file, strerror (e));
      free (file);
      fflush (stdout);
      remove_tmp_dir (misc);
      _exit (EXIT_FAILURE);
   } // if

   if (print)
   {
      wclear (misc->titlewin);
      mvwprintw (misc->titlewin, 1, 0,
                 "This is a %s file", magic_file (myt, file));
      fflush (stdout);
   } // if
   wattron (misc->titlewin, A_BOLD);
   mvwprintw (misc->titlewin, 0, 0, "%s - %s", misc->copyright,
                  gettext ("Please wait..."));
   wattroff (misc->titlewin, A_BOLD);
   wrefresh (misc->titlewin);
   if (strncasecmp (magic_file (myt, file), "gzip compressed", 15) == 0)
   {
      char str[MAX_STR], *orig;
      FILE *p;

      clear_tmp_dir (misc);
      orig = strdup (file);
      snprintf (misc->cmd, MAX_CMD - 1,
        "unar -q \"%s\" -f -o %s && ls -1 %s",
         file, misc->tmp_dir, misc->tmp_dir);
      p = popen (misc->cmd, "r");
      (void) *fgets (str, MAX_STR, p);
      pclose (p);
      str[strlen (str) - 1] = 0;
      file = malloc (strlen (misc->tmp_dir) + strlen (str) + 5);
      sprintf (file, "%s%s", misc->tmp_dir, str);
      create_epub (misc, my_attribute, daisy, file, 0);
      file = strdup (orig);
      return;
   } // "gzip compressed data"
   else
   if (strcasestr (magic_file (myt, file), "troff or preprocessor input"))
// maybe a man-page
   {
      snprintf (misc->cmd, MAX_CMD, "man2html \"%s\" > %s/out.html",
                file, misc->tmp_dir);
      file = malloc (strlen (misc->tmp_dir) + 10);
      sprintf (file, "%s/out.html", misc->tmp_dir);
      switch (system (misc->cmd))
      {
      case 0:
         break;
      default:
         endwin ();
         printf
       ("Be sure the package \"man2html\" is installed onto your system.\n");
         failure (misc, gettext ("eBook-speaker cannot handle this file."), errno);
      } // switch
      create_epub (misc, my_attribute, daisy, file, 0);
      return;
   } // "troff or preprocessor input"
   else
   if ((strcasestr (magic_file (myt, file), "tar archive") ||
        strcasestr (magic_file (myt, file), "Zip archive data") ||
        strcasestr (magic_file (myt, file), "Zip data") ||
        strcasestr (magic_file (myt, file), "ISO 9660") ||
        strcasestr (magic_file (myt, file), "EPUB")) &&
         ! strcasestr (magic_file (myt, file), "Microsoft Word 2007+"))
   {
      extract_epub (misc, file);
   } // if epub
   else
   if (strcasestr (magic_file (myt, file), "HTML document") ||
       strncasecmp (magic_file (myt, file), "XML", 3) == 0)
   {
      pandoc_to_epub (misc, "html", file);
   } // if
   else
   if (strcasestr (magic_file (myt, file), "PDF document") ||
       strcasestr (magic_file (myt, file), "AppleSingle encoded Macintosh"))
   {
      pdf_to_txt (misc, file);
      create_epub (misc, my_attribute, daisy, "out.txt", 0);
      return;
   } // PDF document
   else
   if (strcasestr (magic_file (myt, file), "PostScript document"))
   {
      char cmd[MAX_CMD + 1];

      snprintf (cmd, MAX_CMD, "ps2txt \"%s\" out.txt", file);
      switch (system (cmd))
      {
      case 0:
         create_epub (misc, my_attribute, daisy, "out.txt", 0);
         return;
      default:
         endwin ();
         printf
       ("Be sure the package \"ghostscript\" is installed onto your system.\n");
         failure (misc, gettext ("eBook-speaker cannot handle this file."), errno);
      } // switch
   } // if
   else
   if (strcasestr (magic_file (myt, file), "Non-ISO"))
   {
      char *str;

      str = malloc (strlen (misc->tmp_dir) + 10);
      sprintf (str, "%s/out.UTF-8", misc->tmp_dir);
      snprintf (misc->cmd, MAX_CMD,
                "iconv -t utf-8 \"%s\" 2> /dev/null | pandoc -t epub -o %s",
                 file, str);
      switch (system (misc->cmd))
      {
      default:
         create_epub (misc, my_attribute, daisy, str, 0);
         free (str);
         return;
      } // switch
   }
   else
   if (strcasestr (magic_file (myt, file), "C source text") ||
       strcasestr (magic_file (myt, file), "C++ source text") ||
       strcasestr (magic_file (myt, file), "ASCII text") ||
       strncasecmp (magic_file (myt, file), "data", 4) == 0 ||
       strcasestr (magic_file (myt, file), "GNU gettext message catalogue") ||
       strcasestr (magic_file (myt, file), "shell script text") ||
       strcasestr (magic_file (myt, file), "makefile script") ||
       strcasestr (magic_file (myt, file), "Python script") ||
       strcasestr (magic_file (myt, file), "Sendmail") ||
       strncasecmp (magic_file (myt, file), "UTF-8 Unicode", 13) == 0)
   {
      char *str;

      str = ascii2html (misc, file);
      pandoc_to_epub (misc, "html", str);
   }
   else
   if (strcasestr (magic_file (myt, file), "ISO-8859 text"))
   {
      char cmd[MAX_CMD + 1];

      snprintf (cmd, MAX_CMD,
                "iconv \"%s\" -f ISO-8859-1 -t UTF-8 -o out.UTF-8", file);
      switch (system (cmd))
      {
      default:
         create_epub (misc, my_attribute, daisy, "out.UTF-8", 0);
         return;
      } // switch
   }
   else
   if (strcasestr (magic_file (myt, file), "(Corel/WP)") ||
       strcasestr (magic_file (myt, file), "WordPerfect document") ||
       strcasestr (magic_file (myt, file), "Composite Document File") ||
       strcasestr (magic_file (myt, file), "Microsoft Word 2007+") ||
       strncasecmp (magic_file (myt, file), "Microsoft Office Document",
                    25) == 0 ||
       strcasestr (magic_file (myt, file), "OpenDocument") ||
       strcasestr (magic_file (myt, file), "Rich Text Format"))
   {
      lowriter_to_txt (misc, file);
      create_epub (misc, my_attribute, daisy, "out.txt", 0);
      return;
   } // if
   else
// use calibre
   if (strcasestr (magic_file (myt, file), "Microsoft Reader eBook")) // lit
   {
      use_calibre (misc, file, "msreader");
   } // Microsoft Reader eBook
   else
   if (strcasestr (magic_file (myt, file), "Mobipocket"))
   {
      use_calibre (misc, file, "mobipocket");
   } // Mobipocket
   else
   if (strcasestr (magic_file (myt, file), "AportisDoc") ||
       strcasestr (magic_file (myt, file), "BBeB ebook data") ||
       strcasestr (magic_file (myt, file), "GutenPalm zTXT") ||
       strcasestr (magic_file (myt, file), "Plucker") ||
       strcasestr (magic_file (myt, file), "PeanutPress PalmOS") ||
       strcasestr (magic_file (myt, file), "MS Windows HtmlHelp Data"))
   {
      use_calibre (misc, file, "default");
   } // if
   else
// use tesseract
   if (strcasestr (magic_file (myt, file), "JPEG image")  ||
       strcasestr (magic_file (myt, file), "PNG image")  ||
       strcasestr (magic_file (myt, file), "Netpbm"))
   {
      start_OCR (misc, file);
   } // if
   else
   if (strcasestr (magic_file (myt, file), "GIF image"))
   {
      char str[MAX_STR + 1];

      if (misc->use_cuneiform == 0)
      {
         sprintf (str, "%s/eBook-speaker", misc->tmp_dir);
         sprintf (misc->cmd,
              "cp %s %s.gif && gif2png %s.gif 2> /dev/null", file, str, str);
         switch (system (misc->cmd))
         {
         case 0:
            break;
         default:
            endwin ();
            beep ();
            puts (misc->copyright);
            printf
        ("Be sure the package \"gif2png\" is installed onto your system.\n");
            fflush (stdout);
            remove_tmp_dir (misc);
            _exit (EXIT_SUCCESS);
         } // switch
         snprintf (str, MAX_STR, "%s/eBook-speaker.png", misc->tmp_dir);
         start_OCR (misc, str);
      }
      else
         start_OCR (misc, file);
   } // if
   else
   if (strcasestr (magic_file (myt, file), "directory"))
   {
      misc->start_arg_is_a_dir = 1;
      snprintf (file, MAX_STR - 1, "%s",
                get_input_file (misc, my_attribute, daisy, file));
      wclear (misc->screenwin);
      wrefresh (misc->screenwin);
      create_epub (misc, my_attribute, daisy, file, 1);
      return;
   } // if directory
   magic_close (myt);
} // create_epub

// pointer to misc->term_signaled, whether we received a termination signal
static int *ebook_speaker_term_signaled;

static void term_eBook_speaker (int sig)
{
sig = sig;
endwin ();
   *ebook_speaker_term_signaled = 1;
} // term_eBook_speaker

int main (int argc, char *argv[])
{
   int opt;
   char c_opt, *r_opt, *t_opt;
   misc_t misc;
   my_attribute_t my_attribute;
   daisy_t *daisy;
   audio_info_t *sound_devices;
   struct sigaction usr_action;

   misc.main_pid = getpid ();
   sigfillset (&usr_action.sa_mask);
   usr_action.sa_handler = player_ended;
   usr_action.sa_flags = SA_RESTART;
   sigaction (SIGCHLD, &usr_action, NULL);
   usr_action.sa_handler = term_eBook_speaker;
   usr_action.sa_flags = (int) SA_RESETHAND;
   sigaction (SIGINT, &usr_action, NULL);
   sigaction (SIGTERM, &usr_action, NULL);
// we get two hangups (one from the terminal app, and one from the kernel
// closing the tty), so don't reset that one
   usr_action.sa_flags = 0;
   sigaction (SIGHUP, &usr_action, NULL);
   daisy = NULL;
   sound_devices = (audio_info_t *) calloc (15, sizeof (audio_info_t));
   misc.label = NULL;
   fclose (stderr);
   setbuf (stdout, NULL);
   setlocale (LC_ALL, "");
   if (setlocale (LC_ALL, "") == NULL)
      misc.locale = strdup ("en");
   else
   {
      misc.locale = strdup (setlocale (LC_ALL, ""));
      misc.locale[2] = 0;
   } // if
   setlocale (LC_NUMERIC, "C");
   textdomain (PACKAGE);
   bindtextdomain (PACKAGE, LOCALEDIR);
   misc.src_dir = strdup (get_current_dir_name ());
   my_attribute.id = strdup ("");
   my_attribute.idref = strdup ("");
   my_attribute.src = strdup ("");
   misc.tmp_dir = strdup ("");
   make_tmp_dir (&misc);
   misc.daisy_mp = misc.tmp_dir;
   misc.current_sink = 01;
   strcpy (sound_devices[0].device, "default");
   strcpy (sound_devices[0].type, "alsa");
   initscr ();
   misc.titlewin = newwin (2, 80,  0, 0);
   misc.screenwin = newwin (23, 80, 2, 0);
   misc.ignore_bookmark = misc.total_pages = 0;
   misc.update_progress = 1;
   misc.option_b = 0;
   misc.option_d = misc.option_o = NULL;
   misc.break_phrase = 'n';
   misc.level = 1;
   misc.show_hidden_files = 0;
   misc.max_x = misc.max_y = 0;
   getmaxyx (misc.screenwin, misc.max_y, misc.max_x);
   misc.max_y -= 2;
   keypad (misc.titlewin, TRUE);
   keypad (misc.screenwin, TRUE);
   meta (misc.screenwin, TRUE);
   nonl ();
   noecho ();
   misc.player_pid = -2;
   sprintf (misc.scan_resolution, "400");
   snprintf (misc.copyright, MAX_STR - 1, "%s %s - (C)2021 J. Lemmens",
             gettext ("eBook-speaker - Version"), PACKAGE_VERSION);
   wattron (misc.titlewin, A_BOLD);
   wprintw (misc.titlewin, "%s - %s", misc.copyright,
            gettext ("Please wait..."));
   wattroff (misc.titlewin, A_BOLD);

   misc.speed = 1;
   for (misc.current = 0; misc.current < 15; misc.current++)
   {
      memset (misc.tts[misc.current], 0, MAX_STR);
   } // for
   misc.tts_no = 0;
   misc.ocr_language = NULL;
   *misc.standalone = 0;
   misc.term_signaled = 0;
   ebook_speaker_term_signaled = &misc.term_signaled;
   memset (misc.xmlversion, 0, MAX_STR);
   memset (misc.encoding, 0, MAX_STR);
   snprintf (misc.eBook_speaker_txt, MAX_STR,
             "%s/eBook-speaker.txt", misc.tmp_dir);
   snprintf (misc.tmp_wav, MAX_STR, "%s/eBook-speaker.wav", misc.tmp_dir);
   if ((misc.tmp_wav_fd = mkstemp (misc.tmp_wav)) == 01)
      failure (&misc, "mkstemp ()", errno);
   misc.scan_flag = misc.use_cuneiform = 0;
   misc.use_NCX = 0;
   misc.use_OPF = 0;
   misc.option_b = c_opt = 0;
   memset (misc.option_t, 0, MAX_STR);
   r_opt = t_opt = NULL;
   misc.verbose = 0;
   misc.use_NCX = 0;
   misc.use_OPF = 0;
   opterr = 0;
   while ((opt = getopt (argc, argv, "b:cd:hHilo:r:sSt:vNO")) != -1)
   {
      switch (opt)
      {
      case 'b':
         if (*optarg == 'j')
            *optarg = 'y';
         if (*optarg == 'n' || *optarg == 'y')
            misc.break_phrase = misc.option_b = *optarg;
         else
            misc.break_phrase = misc.option_b = atoi (optarg);
         break;
      case 'c':
         misc.use_cuneiform = c_opt = 1;
         break;
      case 'd':
      {
         int sink;

         misc.option_d = strdup (optarg);
         if (! strchr (optarg, ':'))
         {
            beep ();
            endwin ();
            printf ("\n");
            usage ();
            _exit (EXIT_FAILURE);
         } // if
         get_list_of_sound_devices (&misc, sound_devices);
         for (sink = 0; sink < misc.total_sinks; sink++)
         {
            char dt[50];

            sprintf (dt, "%s:%s", sound_devices[sink].device,
                     sound_devices[sink].type);
            if (strcasecmp (optarg, dt) == 0)
            {
               misc.current_sink = sink;
               break;
            } // if
         } // for
         break;
      }
      case 'h':
         usage ();
         remove_tmp_dir (&misc);
         _exit (EXIT_SUCCESS);
         break;
      case 'H':
         misc.show_hidden_files = 1;
         break;
      case 'i':
         misc.ignore_bookmark = 1;
         snprintf (misc.tts[0], MAX_STR - 1,
                   "espeak -f eBook-speaker.txt -w eBook-speaker.wav -v %s",
                   misc.locale);
         break;
      case 'l': // Deprecated
         continue;
      case 'o':
         misc.option_o = strdup (optarg);
         break;
      case 'r':
         strncpy (misc.scan_resolution, optarg, 5);
         r_opt = optarg;
         break;
      case 's':
         misc.scan_flag = 1;
         misc.start_arg_is_a_dir = 0;
         break;
      case 'S':
         misc.update_progress = 0;
         break;
      case 't':
         strcpy (misc.option_t, optarg);
         misc.tts_no = 1;
         strncpy (misc.tts[misc.tts_no], optarg, MAX_STR - 1);
         t_opt = optarg;
         break;
      case 'v':
         misc.verbose = 1;
         break;
      case 'N': // ment for debugging
         misc.use_NCX = 1;
         misc.use_OPF = 0;
         break;
      case 'O': // ment for debugging
         misc.use_NCX = 0;
         misc.use_OPF = 1;
         break;
      default:
         beep ();
         usage ();
         remove_tmp_dir (&misc);
         _exit (EXIT_SUCCESS);
      } // switch
   } // while

   if (c_opt)
      misc.use_cuneiform = c_opt;
   if (misc.option_o)
      misc.ocr_language = strdup (misc.option_o);
   if (r_opt)
      strncpy (misc.scan_resolution, r_opt, 5);
   if (t_opt)
   {
      misc.tts_no = 1;
      strncpy (misc.tts[misc.tts_no], t_opt, MAX_STR - 1);
   } // if

   memset (misc.search_str, 0, 29);

   if (misc.scan_flag == 1)
   {
      char *cmd;

      mvwprintw (misc.titlewin, 1, 0, "Scanning...");
      wrefresh (misc.titlewin);
      snprintf (misc.orig_input_file, MAX_STR, "%s/out.pgm", misc.tmp_dir);
      cmd = malloc (50 + strlen (misc.scan_resolution) +
                    strlen (misc.orig_input_file));
      sprintf (cmd, "scanimage --resolution %s --mode binary > %s",
               misc.scan_resolution, misc.orig_input_file);
      switch (system (cmd))
      {
      case 0:
         break;
      default:
         endwin ();
         beep ();
         puts (misc.copyright);
         printf
      ("Be sure the package \"sane-utils\" is installed onto your system.\n");
         puts (gettext ("eBook-speaker cannot handle this file."));
         fflush (stdout);
         remove_tmp_dir (&misc);
         _exit (EXIT_FAILURE);
      } // switch
      load_xml (&misc, &my_attribute);
      create_epub (&misc, &my_attribute, daisy, misc.orig_input_file, 1);
      strcpy (misc.daisy_title, "scanned image");
      play_epub (&misc, &my_attribute, daisy, sound_devices, misc.orig_input_file);
      return 0;
   } // if

   if (! argv[optind])
   {
   // if no argument is given

      misc.start_arg_is_a_dir = 1;
      snprintf (misc.orig_input_file, MAX_STR - 1, "%s",
                get_input_file (&misc, &my_attribute, daisy, misc.src_dir));
      create_epub (&misc, &my_attribute, daisy, misc.orig_input_file, 1);
      play_epub (&misc, &my_attribute, daisy, sound_devices,
                 misc.orig_input_file);
      quit_eBook_speaker (&misc, &my_attribute, daisy);
      return 0;
   } // if

   misc.start_arg_is_a_dir = 0;
   if (*argv[optind] == '/')
   {
// arg start with a slash
      strncpy (misc.orig_input_file, argv[optind], MAX_STR - 1);
   }
   else
   {
      if (strncasecmp (argv[optind], "http://", 7) == 0 ||
          strncasecmp (argv[optind], "https://", 8) == 0 ||
          strncasecmp (argv[optind], "ftp://", 6) == 0)
      {
// URL
         snprintf (misc.cmd, MAX_STR, "wget -q \"%s\" -O %s/wget.out",
                   argv[optind], misc.tmp_dir);
         switch (system (misc.cmd))
         {
         case 0:
            break;
         default:
            endwin ();
            beep ();
            puts (misc.copyright);
            printf
      ("Be sure the package \"wget\" is installed onto your system.\n");
            puts (gettext ("eBook-speaker cannot handle this file."));
            fflush (stdout);
            remove_tmp_dir (&misc);
            _exit (EXIT_FAILURE);
         } // switch
         snprintf (misc.orig_input_file, MAX_STR, "%s/wget.out",
                   misc.tmp_dir);
      }
      else
      {
         strcpy (misc.orig_input_file, misc.src_dir);
         if (misc.orig_input_file[strlen (misc.orig_input_file) - 1] != '/')
            strcat (misc.orig_input_file, "/");
         strncat (misc.orig_input_file, argv[optind], MAX_STR - 1);
      } // if
   } // if

   if (access (misc.orig_input_file, R_OK) == -1)
   {
      failure (&misc, argv[optind], errno);
   } // if

   misc.current = 0;
   create_epub (&misc, &my_attribute, daisy, misc.orig_input_file, 1);
   play_epub (&misc, &my_attribute, daisy, sound_devices,
              misc.orig_input_file);
   remove_tmp_dir (&misc);
} // main
