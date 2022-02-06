/* eBook-speaker - read aloud an eBook using a speech synthesizer
 *
 *  Copyright (C) 2018 J. Lemmens
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

void quit_eBook_speaker (misc_t *misc)
{
   save_bookmark_and_xml (misc);
   puts ("");
   close (misc->tmp_wav_fd);
   unlink (misc->tmp_wav);
   remove_tmp_dir (misc);
   _exit (0);
} // quit_eBook_speaker

void remove_double_tts_entries (misc_t *misc)
{
   int x, y;
   int entry_already_exists_in_misc, tts_no = 0;
   char temp[12][MAX_STR];

   for (x = 0; x < 12; x++)
      strncpy (temp[x], misc->tts[x], MAX_STR - 1);
   bzero ((void *) misc->tts, sizeof (misc->tts));
   for (x = 0; x < 12; x++)
   {
      if (! *temp[x])
         break;
      entry_already_exists_in_misc = 0;
      for (y = 0; y < 12; y++)
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

void skip_left (misc_t *misc, my_attribute_t *my_attribute,
                daisy_t *daisy)
{
   int phrase_nr;

   phrase_nr = misc->phrase_nr - 1;
   if (misc->playing < 0) // not playing
   {
      beep ();
      return;
   } // if not playing
   if (misc->playing == 0) // first item
   {
      if (misc->phrase_nr == 1)
      {
         beep ();
         return;
      } // if
   } // if first item
   if (misc->player_pid > -1)
   {
      kill_player (misc);
      misc->player_pid = -2;
   } // if
   if (misc->reader)
      xmlTextReaderClose (misc->reader);
   if (misc->doc)
      xmlFreeDoc (misc->doc);
   if (misc->phrase_nr == 1)
   {
      if (misc->just_this_item > -1 &&
          daisy[misc->playing].level  <= misc->level)
      {
         beep ();
         misc->current = misc->displaying = misc->playing;
         misc->playing = misc->just_this_item = -1;
         view_screen (misc, daisy);
         wmove (misc->screenwin, daisy[misc->current].y,
                daisy[misc->current].x - 1);
         return;
      } // if misc->just_this_item
// go to previous item
      misc->playing--;
      phrase_nr = daisy[misc->playing].n_phrases;
   } // go to previous item
   misc->current = misc->displaying = misc->playing;
   misc->current_page_number = daisy[misc->current].page_number;
   open_text_file (misc, my_attribute, daisy[misc->playing].xml_file,
                   daisy[misc->playing].anchor);
   misc->phrase_nr = 0;
   misc->player_pid = -2;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
         break;
      if (*misc->label)
         misc->phrase_nr++;
      else
      {
         if (strcasestr (misc->daisy_version, "2.02") &&
             (strcasecmp (misc->tag, "pagenum") == 0 ||
              strcasecmp (my_attribute->class, "pagenum") == 0 ||
              strcasecmp (my_attribute->class, "page-normal") == 0))
         {
            get_page_number_2 (misc, my_attribute, daisy, my_attribute->src);
         } // if 2.02
         if (strcasestr (misc->daisy_version, "3") &&
             (strcasecmp (misc->tag, "pagenum") == 0 ||
              strcasecmp (my_attribute->class, "pagenum") == 0 ||
              strcasecmp (my_attribute->class, "page-normal") == 0))
         {
            if (get_page_number_3 (misc, my_attribute) == 0)
               break;
         } // if 3
      } // if ! label
      if (misc->phrase_nr >= phrase_nr)
         break;
   } // while
   wattron (misc->screenwin, A_BOLD);
   mvwprintw (misc->screenwin, daisy[misc->playing].y, 69, "%5d %5d",
              misc->phrase_nr,
              daisy[misc->playing].n_phrases - misc->phrase_nr);
   wattroff (misc->screenwin, A_BOLD);
   mvwprintw (misc->screenwin, 22, 0, misc->label );
   wclrtoeol (misc->screenwin);
   wmove (misc->screenwin, daisy[misc->playing].y,
                           daisy[misc->playing].x - 1);
   wrefresh (misc->screenwin);
   start_playing (misc, my_attribute, daisy);
   return;
} // skip_left

void start_playing (misc_t *misc, my_attribute_t *my_attribute,
                    daisy_t *daisy)
{
   FILE *w;
   struct stat buf;

   if (*misc->tts[misc->tts_no] == 0 && misc->option_t == 0)
   {
      misc->tts_no = 0;
      select_tts (misc, daisy);
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
   lseek (misc->tmp_wav_fd, SEEK_SET, 0);
   switch (system (misc->tts[misc->tts_no]));
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
      char tempo_str[15];

      snprintf (tempo_str, 10, "%lf", misc->speed);
      misc->player_pid = setpgrp ();
      playfile (misc, misc->tmp_wav, "wav", misc->sound_dev, "pulseaudio",
                tempo_str);
      _exit (0);
   }
   default: // parent
      return;
   } // switch
} // start_playing

void open_text_file (misc_t *misc, my_attribute_t *my_attribute,
                     char *text_file, char *anchor)
{
   misc->doc = htmlParseFile (text_file, "UTF-8");
   if ((misc->reader = xmlReaderWalker (misc->doc)) == NULL)
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, "open_text_file (%s)", text_file);
      failure (misc, str, e);
   } // if
   if (! *anchor)
      return;

// skip to anchor
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
         break;
      if (strcasecmp (anchor, my_attribute->id) == 0)
         break;
   } // while
} // open_text_file

void go_to_next_item (misc_t *misc, my_attribute_t *my_attribute,
                      daisy_t *daisy)
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
      view_screen (misc, daisy);
      close (misc->tmp_wav_fd);
      unlink (misc->tmp_wav);
      remove_tmp_dir (misc);
      endwin ();
      puts ("");
      _exit (0);
   } // if
   if (daisy[misc->playing].level <= misc->level)
      misc->current = misc->displaying = misc->playing;
   if (misc->just_this_item != -1)
      if (daisy[misc->playing].level <= misc->level)
         misc->playing = misc->just_this_item = -1;
   if (misc->playing > -1)
   {
      unlink (misc->tmp_wav);
      unlink ("old.wav");
      open_text_file (misc, my_attribute,
                      daisy[misc->playing].xml_file, daisy[misc->playing].anchor);
   } // if
   view_screen (misc, daisy);
} // go_to_next_item

void get_next_phrase (misc_t *misc, my_attribute_t *my_attribute,
                      daisy_t *daisy, int give_sound)
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
            if (misc->current_page_number)
               mvwprintw (misc->screenwin, daisy[misc->playing].y, 63,
                          "(%3d)", misc->current_page_number);
            mvwprintw (misc->screenwin, daisy[misc->playing].y, 69, "%5d %5d",
                       misc->phrase_nr + 1,
                       daisy[misc->playing].n_phrases - misc->phrase_nr - 1);
            misc->phrase_nr++;
            wattroff (misc->screenwin, A_BOLD);
            strncpy (misc->str, misc->label, 80);
            mvwprintw (misc->screenwin, 22, 0, misc->str);
            wclrtoeol (misc->screenwin);
            wmove (misc->screenwin, daisy[misc->displaying].y,
                   daisy[misc->displaying].x - 1);
            wrefresh (misc->screenwin);
         } // if
         if (give_sound)
            start_playing (misc, my_attribute, daisy);
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
         go_to_next_item (misc, my_attribute, daisy);
         break;
      } // if
      if (misc->playing + 1 < misc->total_items &&
          *daisy[misc->playing + 1].anchor &&
          strcmp (my_attribute->id, daisy[misc->playing + 1].anchor) == 0)
      {
         go_to_next_item (misc, my_attribute, daisy);
         break;
      } // if
   } // while
} // get_next_phrase

void select_tts (misc_t *misc, daisy_t *daisy)
{
   int y;

   kill_player (misc);
   wclear (misc->screenwin);
   wprintw (misc->screenwin, "\n%s\n\n", gettext
            ("Select a Text-To-Speech application"));
   wprintw (misc->screenwin, "    1 %s\n", misc->tts[0]);
   for (y = 1; y < 10; y++)
   {
      char str[MAX_STR];

      if (*misc->tts[y] == 0)
         break;
      strncpy (str, misc->tts[y], MAX_STR - 1);
      str[72] = 0;
      wprintw (misc->screenwin, "   %2d %s\n", y + 1, str);
   } // for
   wmove (misc->screenwin, 14, 0);
   if (y < 10)
   {
      wprintw (misc->screenwin, "\n%s\n\n", gettext
            ("Provide a new TTS."));
      wprintw (misc->screenwin, "%s\n", gettext
            ("Be sure that the new TTS reads its information from the file"));
      wprintw (misc->screenwin, "%s\n\n", gettext
    ("eBook-speaker.txt and that it writes to the file eBook-speaker.wav."));
   }
   else
   {
      beep ();
      wprintw (misc->screenwin, "\n%s\n\n", gettext
          ("Maximum number of TTS's is reached."));
   } // if
   wprintw (misc->screenwin, "%s\n", gettext
            ("Press DEL to delete a TTS"));
   wprintw (misc->screenwin,
           "    -------------------------------------------------------------");
   nodelay (misc->screenwin, FALSE);
   y = misc->tts_no;
   if (*misc->tts[y])
      wmove (misc->screenwin, y + 3, 2);
   else
      wmove (misc->screenwin, 21, 4);
   while (1)
   {
      switch (wgetch (misc->screenwin))
      {
      int i;

      case 13:
         misc->tts_no = y;
         view_screen (misc, daisy);
         nodelay (misc->screenwin, TRUE);
         return;
      case KEY_DOWN:
         y++;
         if (y > 9)
         {
            y = 0;
            wmove (misc->screenwin, y + 3, 2);
            break;
         } // is
         if (y > 9 || *misc->tts[y] == 0)
         {
            wmove (misc->screenwin, 21, 4);
            nodelay (misc->screenwin, FALSE);
            echo ();
            wgetnstr (misc->screenwin, misc->tts[y], MAX_STR - 1);
            noecho ();
            if (*misc->tts[y])
            {
               view_screen (misc, daisy);
               nodelay (misc->screenwin, TRUE);
               misc->tts_no = y;
               return;
            } // if
            y = 0;
         } // if
         wmove (misc->screenwin, y + 3, 2);
         break;
      case KEY_UP:
         y--;
         if (y < 0)
         {
            if (*misc->tts[9])
            {
               y = 9;
               wmove (misc->screenwin, y + 3, 2);
               break;
            } // if
            wmove (misc->screenwin, 21, 4);
            nodelay (misc->screenwin, FALSE);
            echo ();
            for (y = 0; *misc->tts[y]; y++);
            wgetnstr (misc->screenwin, misc->tts[y], MAX_STR - 1);
            noecho ();
            if (*misc->tts[y])
            {
               misc->tts_no = y;
               view_screen (misc, daisy);
               nodelay (misc->screenwin, TRUE);
               return;
            } // if
            for (y = 0; *misc->tts[y] == 0; y++);
         } // if
         wmove (misc->screenwin, y + 3, 2);
         break;
      case KEY_DC:
         if (y == 0)
         {
            beep ();
            break;
         } // if
         if (*misc->tts[1] == 0)
         {
            beep ();
            continue;
         } // if
         for (i = y; i < 9; i++)
            strncpy (misc->tts[i], misc->tts[i + 1], MAX_STR - 1);
         misc->tts_no = y;
         view_screen (misc, daisy);
         nodelay (misc->screenwin, TRUE);
         return;
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
      if (*daisy[item].anchor)
      {
// if there is a "from" anchor, look for it
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, r))
               break;
            if (strcasecmp (my_attribute->id, daisy[item].anchor) == 0)
               break;
         } // while
      } // if anchor)

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
// no need to search t_anchor
            continue;
         if (*daisy[item + 1].anchor)
// stop at "to" anchor
            if (strcasecmp (my_attribute->id, daisy[item + 1].anchor) == 0)
               break;
      } // while
      misc->total_phrases += daisy[item].n_phrases;
   } // for
} // count_phrases

void split_phrases (misc_t *misc, my_attribute_t *my_attribute,
                    daisy_t *daisy, int i)
{
   xmlTextReaderPtr reader;
   char *new, *p;
   int start_of_new_phrase;
   htmlDocPtr doc;
   xmlTextWriterPtr writer;

   doc = htmlParseFile (daisy[i].xml_file, "UTF-8");
   if ((reader = xmlReaderWalker (doc))  == NULL)
      failure (misc, daisy[i].xml_file, errno);
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
   xmlTextWriterStartDocument
                (writer, misc->xmlversion, misc->encoding, misc->standalone);
   xmlTextWriterSetIndent (writer, 1);
   xmlTextWriterSetIndentString (writer, BAD_CAST "");
   if (*daisy[i].anchor)
   {
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, reader))
            break;
         if (strcasecmp (daisy[i].anchor, my_attribute->id) == 0)
         {
            xmlTextWriterStartElement (writer, (const xmlChar *) misc->tag);
            xmlTextWriterWriteFormatAttribute
                        (writer, BAD_CAST "id", "%s", my_attribute->id);
            xmlTextWriterEndElement (writer);
            break;
         } // if
      } // while
   } // if
   while (1)
   {
      int eof;

      eof = 1 - get_tag_or_label (misc, my_attribute, reader);
      if ((*my_attribute->id && i + 1 < misc->total_items &&
           strcasecmp (my_attribute->id, daisy[i + 1].anchor) == 0) ||
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
               if (! get_tag_or_label (misc, my_attribute, reader))
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
         xmlTextWriterWriteString (writer, (const xmlChar *) "\n");
         continue;
      } // if misc->tag
      if (! *misc->label)
         continue;

      p = misc->label;
      start_of_new_phrase = 1;
      while (1)
      {
         if (start_of_new_phrase == 1 &&
             (*p == ' ' || *p == '\t'))
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
              *(p + 1) == -62))
              // I don't understand it, but it works
              // Some files use -62 -96 (&nbsp;) as space
// ----
         {
            xmlTextWriterWriteFormatString (writer, "%c", *p);
            p++;
            xmlTextWriterWriteString (writer, BAD_CAST "\n");
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
            if (misc->break_on_EOL == 'y')
            {
               xmlTextWriterWriteString (writer, BAD_CAST "\n");
               xmlTextWriterWriteElement (writer, BAD_CAST "br", NULL);
               start_of_new_phrase = 1;
            }
            else
            {
               xmlTextWriterWriteString (writer, BAD_CAST " ");
            } // if
            break;
         } // if
      } // while
   } // while
   xmlTextReaderClose (reader);
   xmlFreeDoc (doc);
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
   xmlTextWriterWriteFormatAttribute
      (writer, BAD_CAST "break_on_EOL", "%c", misc->break_on_EOL);
   xmlTextWriterEndElement (writer);
   xmlTextWriterEndDocument (writer);
   xmlFreeTextWriter (writer);
   free (str);
} // put_bookmark

void get_bookmark (misc_t *misc, my_attribute_t *my_attribute,
                   daisy_t *daisy)
{
   xmlTextReaderPtr local_reader;
   htmlDocPtr local_doc;
   int tts_no, pn;
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
      return;
   tts_no = misc->tts_no;
   do
   {
      if (! get_tag_or_label (misc, my_attribute, local_reader))
         break;
   } while (strcasecmp (misc->tag, "bookmark") != 0);
   xmlTextReaderClose (local_reader);
   xmlFreeDoc (local_doc);
   pn = misc->phrase_nr;
   if (misc->option_t)
      misc->tts_no = tts_no;
   if (! *misc->tts[misc->tts_no])
   {
      for (misc->tts_no = 0; *misc->tts[misc->tts_no]; misc->tts_no++);
      misc->tts_no--;
   } // if
   if (misc->current < 0 || misc->current >= misc->total_items)
      misc->current = 0;
   if (misc->phrase_nr < 0)
      misc->phrase_nr = 0;
   if (misc->level < 1 || misc->level > misc->depth)
      misc->level = 1;
   misc->displaying = misc->playing = misc->current;
   misc->current_page_number = daisy[misc->playing].page_number;
   misc->just_this_item = -1;
   go_to_phrase (misc, my_attribute, daisy, misc->current, pn);
   view_screen (misc, daisy);
} // get_bookmark

void view_screen (misc_t *misc, daisy_t *daisy)
{
   int i, x, l;

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

   wclear (misc->screenwin);
   for (i = 0; daisy[i].screen != daisy[misc->current].screen; i++);
   do
   {
      mvwprintw (misc->screenwin, daisy[i].y, daisy[i].x, "%s",
                 daisy[i].label);
      l = strlen (daisy[i].label) + daisy[i].x;
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
       daisy[misc->displaying].screen == daisy[misc->playing].screen)
      mvwprintw (misc->screenwin, daisy[misc->playing].y, 0, "J");
   mvwprintw (misc->screenwin, 21, 0,
              "----------------------------------------");
   wprintw (misc->screenwin, "----------------------------------------");
   wmove (misc->screenwin, daisy[misc->current].y,
          daisy[misc->current].x - 1);
   wrefresh (misc->screenwin);
} // view_screen

void write_wav (misc_t *misc, my_attribute_t *my_attribute,
                daisy_t *daisy, char *outfile)
{
   FILE *w;
   sox_format_t *first, *output;
   sox_sample_t samples[2048];
   size_t number_read;
   char *first_eBook_speaker_wav;
   float secs;
   int total;

   if (! *misc->tts[misc->tts_no])
   {
      misc->tts_no = 0;
      select_tts (misc, daisy);
   } // if
   wprintw (misc->screenwin,
            "\nStoring \"%s\" as \"%s\" into your home-folder...",
            daisy[misc->current].label, basename (outfile));
   wrefresh (misc->screenwin);
   if ((w = fopen (misc->eBook_speaker_txt, "w")) == NULL)
      failure (misc, "Can't make a temp file", errno);
   if (fwrite ("init\n", 5, 1, w) == 0)
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, "write (\"%s\"): failed.\n", misc->label);
      failure (misc, str, e);
   } // if
   fclose (w);
   switch (system (misc->tts[misc->tts_no]));
   secs = 0;
   while (1)
   {
      if (access (misc->tmp_wav, R_OK) == 0)
         break;
      sleep (0.01);
      secs += 0.01;
      if (secs >= 1)
      {
         int e;
         char *str;

         e = errno;
         endwin ();
         str = malloc (20 + strlen (misc->tmp_wav) +
                       strlen (strerror (e)));
         sprintf (str, "sox_open_read: %s: %s",
                   misc->tmp_wav, strerror (e));
         printf ("%s\n\n", str);
         printf (gettext ("Be sure that the used TTS is installed:"));
         printf ("\n\n   \"%s\"\n\n", misc->tts[misc->tts_no]);
         beep ();
         _exit (0);
      } // if
   } // while

   sox_init ();
   first_eBook_speaker_wav = malloc (strlen (misc->daisy_mp) + 50);
   sprintf (first_eBook_speaker_wav, "%s/first_eBook_speaker.wav",
            misc->daisy_mp);
   rename (misc->tmp_wav, first_eBook_speaker_wav);
   first = sox_open_read (first_eBook_speaker_wav, NULL, NULL, NULL);
   output = sox_open_write (outfile, &first->signal, &first->encoding,
                            NULL, NULL, NULL);
   open_text_file (misc, my_attribute,
                 daisy[misc->current].xml_file, daisy[misc->current].anchor);
   total = misc->total_phrases;
   while (1)
   {
      sox_format_t *input;

      if (! get_tag_or_label (misc, my_attribute, misc->reader))
         break;
      if (misc->current < misc->total_items - 1 &&
          *daisy[misc->current + 1].anchor &&
          strcasecmp (my_attribute->id, daisy[misc->current + 1].anchor) == 0)
         break;
      if (! *misc->label)
         continue;;
      mvwprintw (misc->screenwin, 7, 3, "Writing phrase %d...", total--);
      wrefresh (misc->screenwin);
      w = fopen (misc->eBook_speaker_txt, "w");
      fwrite (misc->label, strlen (misc->label), 1, w);
      fclose (w);
      switch (system (misc->tts[misc->tts_no]));
      input = sox_open_read (misc->tmp_wav, NULL, NULL, NULL);
      while ((number_read = sox_read (input, samples, 2048)))
         sox_write (output, samples, number_read);
      sox_close (input);
   } // while
   sox_close (output);
   sox_close (first);
   sox_quit ();
} // write_wav

void help (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   int y, x, playing;

   playing = misc->playing;
   if (playing > -1)
      pause_resume (misc, my_attribute, daisy);
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
   wprintw (misc->screenwin, "\n\n\n\n");
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
            ("x               - go to the file-manager"));
   wprintw (misc->screenwin, "\n%s", gettext
            ("Press any key to leave help..."));
   nodelay (misc->screenwin, FALSE);
   wgetch (misc->screenwin);
   nodelay (misc->screenwin, TRUE);
   view_screen (misc, daisy);
   wmove (misc->screenwin, y, x);
   if (playing > -1)
      pause_resume (misc, my_attribute, daisy);
} // help

void previous_item (misc_t *misc, daisy_t *daisy)
{
   do
   {
      if (--misc->current < 0)
      {
         beep ();
         misc->displaying = misc->current = 0;
         break;
      } // if
   } while (daisy[misc->current].level > misc->level);
   misc->displaying = misc->current;
   misc->phrase_nr = daisy[misc->current].n_phrases - 1;
   view_screen (misc, daisy);
} // previous_item

void next_item (misc_t *misc, daisy_t *daisy)
{
   do
   {
      if (++misc->current >= misc->total_items)
      {
         misc->displaying = misc->current = misc->total_items - 1;
         beep ();
         break;
      } // if
      misc->displaying = misc->current;
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

void load_xml (misc_t *misc, my_attribute_t *my_attribute)
{
   int x = 1, i;
   char str[MAX_STR], *p;
   struct passwd *pw;;
   xmlTextReaderPtr reader;
   htmlDocPtr doc;

   pw = getpwuid (geteuid ());
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker.xml", pw->pw_dir);
   snprintf (misc->tts[0], MAX_STR - 1,
             "espeak -f eBook-speaker.txt -w eBook-speaker.wav -v %s",
             misc->locale);
   if ((doc = htmlParseFile (str, "UTF-8")) == NULL)
   {
// If no TTS; give some examples
      strncpy (misc->tts[1],
        "flite eBook-speaker.txt eBook-speaker.wav",
         MAX_STR - 1);
      strncpy (misc->tts[2],
        "espeak -f eBook-speaker.txt -w eBook-speaker.wav -v mb-nl2",
         MAX_STR - 1);
      strncpy (misc->tts[3],
        "text2wave eBook-speaker.txt -o eBook-speaker.wav",
         MAX_STR - 1);
      strncpy (misc->tts[4],
        "swift -n Lawrence -f eBook-speaker.txt -m text -o eBook-speaker.wav",
         MAX_STR - 1);
      save_xml (misc);
      return;
   } // if
   if ((reader = xmlReaderWalker (doc)))
   {
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, reader))
            break;
         if (xmlTextReaderIsEmptyElement (reader))
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
            for (i = strlen (p) - 1; i > 0; i--)
               if (p[i] != ' ' && p[i] != '\n')
                  break;
            p[i + 1] = 0;
            strncpy (misc->tts[x++], p, MAX_STR - 1);
            if (x == 10)
               break;
         } // if
      } // while
      misc->tts_no = x;
      xmlTextReaderClose (reader);
      xmlFreeDoc (doc);
   } // if
   if (*misc->tts[0])                  
      return;

// If no TTS; give some examples
   strncpy (misc->tts[1],
      "flite eBook-speaker.txt eBook-speaker.wav", MAX_STR - 1);
   strncpy (misc->tts[2],
   "espeak -f eBook-speaker.txt -w eBook-speaker.wav -v mb-nl2", MAX_STR - 1);
   strncpy (misc->tts[3],
      "text2wave eBook-speaker.txt -o eBook-speaker.wav", MAX_STR - 1);
   strncpy (misc->tts[4],
      "swift -n Lawrence -f eBook-speaker.txt -m text -o eBook-speaker.wav",
            MAX_STR - 1);
} // load_xml

void save_xml (misc_t *misc)
{
   int x = 0;
   char str[MAX_STR];
   xmlTextWriterPtr writer;
   struct passwd *pw;

   pw = getpwuid (geteuid ());
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker.xml", pw->pw_dir);
   if (! (writer = xmlNewTextWriterFilename (str, 0)))
      return;
   xmlTextWriterStartDocument (writer, NULL, NULL, NULL);
   xmlTextWriterStartElement (writer, BAD_CAST "eBook-speaker");
   xmlTextWriterWriteString (writer, BAD_CAST "\n   ");
   xmlTextWriterStartElement (writer, BAD_CAST "prefs");
   xmlTextWriterWriteFormatAttribute
      (writer, BAD_CAST "sound_dev", "%s", misc->sound_dev);
   xmlTextWriterEndElement (writer);
   xmlTextWriterWriteString (writer, BAD_CAST "\n   ");
   xmlTextWriterStartElement (writer, BAD_CAST "voices");
   while (1)
   {
      char str[MAX_STR];

      xmlTextWriterWriteString (writer, BAD_CAST "\n      ");
      xmlTextWriterStartElement (writer, BAD_CAST "tts");
      snprintf (str, MAX_STR - 1, "\n         %s\n      ", misc->tts[x]);
      xmlTextWriterWriteString (writer, BAD_CAST str);
      xmlTextWriterEndElement (writer);
      if (! *misc->tts[++x])
         break;
   } // while
   xmlTextWriterWriteString (writer, BAD_CAST "\n   ");
   xmlTextWriterEndElement (writer);
   xmlTextWriterWriteString (writer, BAD_CAST "\n");
   xmlTextWriterEndElement (writer);
   xmlTextWriterEndDocument (writer);
   xmlFreeTextWriter (writer);
} // save_xml

void save_bookmark_and_xml (misc_t *misc)
{
   endwin ();
   kill_player (misc);
   wait (NULL);
   put_bookmark (misc);
   save_xml (misc);
   xmlCleanupParser ();
   remove_tmp_dir (misc);
} // save_bookmark_and_xml

void pause_resume (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   if (*misc->tts[misc->tts_no] == 0)
   {
      misc->tts_no = 0;
      select_tts (misc, daisy);
   } // if
   if (misc->playing != -1)
      misc->playing = -1;
   else
      misc->playing = misc->displaying;
   if (! misc->reader)
      return;
   else
   {
      if (misc->playing != -1)
      {
         FILE *w;

         kill (misc->player_pid, SIGKILL);
         wattron (misc->screenwin, A_BOLD);
         mvwprintw (misc->screenwin, daisy[misc->playing].y, 69, "%5d %5d",
                    misc->phrase_nr,
                    daisy[misc->playing].n_phrases - misc->phrase_nr);
         wattroff (misc->screenwin, A_BOLD);
         wmove (misc->screenwin, daisy[misc->displaying].y,
                daisy[misc->displaying].x - 1);
         wrefresh (misc->screenwin);
         if ((w = fopen (misc->eBook_speaker_txt, "w")) == NULL)
            failure (misc, "Can't make a temp file", errno);
         clearerr (w);
         fwrite (misc->label, strlen (misc->label), 1, w);
         if (ferror (w) != 0)
         {
            int e;
            char str[MAX_STR];

            e = errno;
            snprintf (str, MAX_STR, "write (\"%s\"): failed.\n", misc->label);
            failure (misc, str, e);
         } // if
         fclose (w);
         switch (system (misc->tts[misc->tts_no]));
         switch (misc->player_pid = fork ())
         {
         case -1:
         {
            int e;

            e = errno;
            endwin ();
            beep ();
            skip_left (misc, my_attribute, daisy);
            put_bookmark (misc);
            failure (misc, "fork ()", e);
            break;
         }
         case 0:
         {
            char tempo_str[15];

            snprintf (tempo_str, 10, "%lf", misc->speed);
            misc->player_pid = setpgrp ();
            playfile (misc, misc->tmp_wav, "wav", misc->sound_dev, "pulseaudio",
                      tempo_str);
            _exit (0);
         }
         default:
            return;
         } // switch       
      }
      else
         kill_player (misc);
   } // if
} // pause_resume

void search (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy,
             int start, char mode)
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
      misc->displaying = misc->playing = misc->current;
      misc->player_pid = -2;
      unlink (misc->tmp_wav);
      unlink ("old.wav");
      open_text_file (misc, my_attribute,
              daisy[misc->current].xml_file, daisy[misc->current].anchor);
   }
   else
   {
      beep ();
      pause_resume (misc, my_attribute, daisy);
   } // if
} // search

void break_on_EOL (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   int  level;

   xmlTextReaderClose (misc->reader);
   unlink (misc->tmp_wav);
   unlink ("old.wav");
   misc->phrase_nr = misc->total_phrases = 0;
   level = misc->level;
   mvwprintw (misc->titlewin, 1, 0,
                            "                                        ");
   wprintw (misc->titlewin, "                                        ");
   mvwprintw (misc->titlewin, 1, 0, "%s ",
      gettext ("Consider the end of an input line as a phrase-break (y/n)"));
   while (1)
   {
      wint_t wch;

      wget_wch (misc->titlewin, &wch);
      misc->break_on_EOL = wch;
      if (misc->break_on_EOL == 'y' || misc->break_on_EOL == 'n')
         break;
      beep ();
   } // while
   save_xml (misc);
   check_phrases (misc, my_attribute, daisy);
   misc->level = level;
   *misc->search_str = 0;
   misc->current = 0;
   misc->displaying = misc->playing = misc->just_this_item = -1;
   nodelay (misc->screenwin, TRUE);
   view_screen (misc, daisy);
} // break_on_EOL

void go_to_phrase (misc_t *misc, my_attribute_t *my_attribute,
                   daisy_t *daisy, int item, int pn)
{
   open_text_file (misc, my_attribute,
                   daisy[item].xml_file, daisy[item].anchor);
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
   misc->playing = misc->displaying = misc->current = item;
   view_screen (misc, daisy);
   mvwprintw (misc->screenwin, daisy[item].y, 69, "%5d %5d",
              misc->phrase_nr + 1, 
              daisy[item].n_phrases - misc->phrase_nr - 1);
   wrefresh (misc->screenwin);
   wmove (misc->screenwin, daisy[item].y, daisy[item].x - 1);
   misc->just_this_item = -1;
   unlink (misc->tmp_wav);
   unlink ("old.wav");
} // go_to_phrase

void start_OCR (misc_t *misc, char *file)
{
   char str[MAX_STR + 1];

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

      if (! *misc->ocr_language)
      {
         for (i = 0; i < 13; i++)
         {
            if (strcmp (misc->locale, codes[i][0]) == 0)
            {
               strncpy (misc->ocr_language, codes[i][1], 3);
               break;
            } // if
         } // for
      } // if
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

      if (! *misc->ocr_language)
      {
         for (i = 0; i < 24; i++)
         {
            if (strcmp (misc->locale, codes[i][0]) == 0)
            {
               strncpy (misc->ocr_language, codes[i][1], 3);
               break;
            } // if
         } // for
      } // if
   } // if

   if (misc->use_cuneiform == 0)
      snprintf (misc->cmd, MAX_CMD - 1,
             "tesseract \"%s\" \"%s/out\" -l %s 2> /dev/null", file,
              misc->daisy_mp, misc->ocr_language);
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
         printf
 ("Be sure the package \"tesseract-ocr\" is installed onto your system.\n\n");
         printf (gettext
               ("Language code \"%s\" is not a valid tesseract code."),
               misc->ocr_language);
         printf ("%s\n", gettext ("See the tesseract manual for valid codes."));
         printf (gettext
  ("Be sure the package \"tesseract-ocr-%s\" is installed onto your system.\n"),
        misc->ocr_language);
         printf ("\n");
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
      _exit (0);
   } // switch
   strncpy (str, misc->daisy_mp, MAX_STR);
   strncat (str, "/out.txt", MAX_STR);
   pandoc_to_epub (misc, "markdown", str);
} // start_OCR

void browse (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy,
             char *file)
{
   int old;

   misc->current = misc->phrase_nr = 0;
   misc->level = 1;
   if (misc->scan_flag == 0 && misc->ignore_bookmark == 0)
   {
      get_bookmark (misc, my_attribute, daisy);
      check_phrases (misc, my_attribute, daisy);
      go_to_phrase (misc, my_attribute, daisy, misc->current,
                    ++misc->phrase_nr);
   } // if
   view_screen (misc, daisy);
   nodelay (misc->screenwin, TRUE);

// convert ALSA device name into pulseaudio device name
   if (strncmp (misc->sound_dev, "hw:", 3) == 0)
      misc->sound_dev += 3;

   wmove (misc->screenwin, daisy[misc->current].y,
          daisy[misc->current].x - 1);

   for (;;)
   {
      switch (wgetch (misc->screenwin))
      {
      case 13: // ENTER
         misc->playing = misc->displaying = misc->current;
         unlink (misc->tmp_wav);
         unlink ("old.wav");
         misc->phrase_nr = 0;
         misc->just_this_item = -1;
         misc->current_page_number = daisy[misc->playing].page_number;
         view_screen (misc, daisy);
         if (misc->player_pid > -1)
            kill_player (misc);
         misc->player_pid = -2;
         open_text_file (misc, my_attribute,
                 daisy[misc->current].xml_file, daisy[misc->current].anchor);
         break;
      case '/':
         search (misc, my_attribute, daisy, misc->current + 1, '/');
         view_screen (misc, daisy);
         break;
      case ' ':
      case KEY_IC:
      case '0':
         pause_resume (misc, my_attribute, daisy);
         break;
      case 'b':
         kill_player (misc);
         break_on_EOL (misc, my_attribute, daisy);
         view_screen (misc, daisy);
         break;
      case 'B':
         misc->displaying = misc->current = misc->total_items - 1;
         misc->phrase_nr = daisy[misc->current].n_phrases - 1;
         view_screen (misc, daisy);
         break;
      case 'f':
         if (misc->playing == -1)
         {
            beep ();
            break;
         } // if
         misc->current = misc->displaying= misc->playing;
         view_screen (misc, daisy);
         break;
      case 'g':
      {
         char pn[15];

         kill (misc->player_pid, SIGKILL);
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
            pause_resume (misc, my_attribute, daisy);
            pause_resume (misc, my_attribute, daisy);
            break;
         } // if
         go_to_phrase (misc, my_attribute, daisy, misc->current , atoi (pn));
         break;
      }
      case 'G':
         if (misc->total_pages)
            go_to_page_number (misc, my_attribute, daisy);
         else
            beep ();
         break;
      case 'h':
      case '?':
         help (misc, my_attribute, daisy);
         view_screen (misc, daisy);
         break;
      case 'j':
      case '5':
      case KEY_B2:
         if (misc->just_this_item != -1)
            misc->just_this_item = -1;
         else
         {
            if (misc->playing == -1)
            {
               misc->phrase_nr = 0;
               open_text_file (misc, my_attribute,
                 daisy[misc->current].xml_file, daisy[misc->current].anchor);
            } // if
            misc->playing = misc->just_this_item = misc->current;
         } // if
         mvwprintw (misc->screenwin, daisy[misc->current].y, 0, " ");
         if (misc->playing == -1)
         {
            misc->just_this_item = misc->displaying = misc->playing = misc->current;
            misc->phrase_nr = 0;
            kill_player (misc);
            open_text_file (misc, my_attribute,
              daisy[misc->current].xml_file, daisy[misc->current].anchor);
         } // if
         if (misc->just_this_item != -1 &&
             daisy[misc->displaying].screen == daisy[misc->playing].screen)
            mvwprintw (misc->screenwin, daisy[misc->playing].y, 0, "J");
         else
            mvwprintw (misc->screenwin, daisy[misc->current].y, 0, " ");
         wrefresh (misc->screenwin);
         break;
      case 'l':
         change_level (misc, daisy, 'l');
         break;
      case 'L':
         change_level (misc, daisy, 'L');
         break;
      case 'n':
         search (misc, my_attribute, daisy, misc->current + 1, 'n');
         view_screen (misc, daisy);
         break;
      case 'N':
         search (misc, my_attribute, daisy, misc->current - 1, 'N');
         view_screen (misc, daisy);
         break;
      case 'o':
         select_next_output_device (misc, my_attribute, daisy);
         break;
      case 'p':
         put_bookmark (misc);
         save_xml (misc);
         break;
      case 'q':
         quit_eBook_speaker (misc);
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
         misc->displaying = misc->current = 0;
         snprintf (misc->cmd, MAX_CMD - 1,
                   "cd \"%s\" && rm -rf out*", misc->daisy_mp);
         switch (system (misc->cmd));
         wclear (misc->titlewin);
         wclear (misc->screenwin);
         wrefresh (misc->titlewin);
         wrefresh (misc->screenwin);
         wattron (misc->titlewin, A_BOLD);
         mvwprintw (misc->titlewin, 0, 0, "%s - %s", misc->copyright,
                  gettext ("Please wait..."));
         wattroff (misc->titlewin, A_BOLD);
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
            _exit (-1);
         default:
            break;
         } // switch
         file = malloc (MAX_STR + 1);
         strncpy (file, str, MAX_STR);
         start_OCR (misc, file);
         daisy = create_daisy_struct (misc, my_attribute);
         *daisy[0].my_class = daisy[0].page_number = 0;
         daisy[0].xml_file = daisy[0].anchor = strdup ("");
         read_daisy_3 (misc, my_attribute, daisy);
         fill_page_numbers (misc, daisy, my_attribute);
         check_phrases (misc, my_attribute, daisy);
         *daisy[0].label = '1';
         strcpy (misc->daisy_title, "scanned image");
         misc->total_items = 1;
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
            mvwprintw (misc->titlewin, 0, misc->max_x - strlen (
                       misc->daisy_title) - 4, "... ");
         } // if
         mvwprintw (misc->titlewin, 0, misc->max_x - strlen (misc->daisy_title),
                    "%s", misc->daisy_title);
         mvwprintw (misc->titlewin, 1, 0,
                    "----------------------------------------");
         wprintw (misc->titlewin, "----------------------------------------");
         mvwprintw (misc->titlewin, 1, 0, "%s ",
                    gettext ("Press 'h' for help"));
         wrefresh (misc->titlewin);
         misc->displaying = misc->current = 0;
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
      case 't':
         misc->playing = -1;
         select_tts (misc, daisy);
         view_screen (misc, daisy);
         remove_double_tts_entries (misc);
         break;
      case 'T':
         misc->displaying = misc->current = 0;
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
         break;
      case KEY_LEFT:
      case '4':
         if (misc->playing == -1)
         {
            beep ();
            break;
         } // if
         skip_left (misc, my_attribute, daisy);
         break;
      case KEY_NPAGE:
      case '3':
         if (misc->current / misc->max_y == (misc->total_items - 1) / misc->max_y)
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
         pause_resume (misc, my_attribute, daisy);
         pause_resume (misc, my_attribute, daisy);
         break;
      case 'd':
         store_as_WAV_file (misc, my_attribute, daisy);
         skip_left (misc, my_attribute, daisy);
         break;
      case 'A':
         store_as_ASCII_file (misc, my_attribute, daisy);
         skip_left (misc, my_attribute, daisy);
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
         pause_resume (misc, my_attribute, daisy);
         pause_resume (misc, my_attribute, daisy);
         break;
      case KEY_HOME:
      case '*':
         misc->speed = 1.0;
         if (misc->playing == -1)
            break;
         pause_resume (misc, my_attribute, daisy);
         pause_resume (misc, my_attribute, daisy);
         break;
      case 'v':
      case '1':
         get_volume (misc);
         if (misc->volume <= misc->min_vol)
         {
            beep ();
            break;
         } // if
         misc->volume -= 1;
         set_volume (misc);
         break;
      case 'V':
      case '7':
         get_volume (misc);
         if (misc->volume >= misc->max_vol)
         {
            beep ();
            break;
         } // if
         misc->volume += 1;
         set_volume (misc);
         break;
      case 'x':
         kill_player (misc);
         put_bookmark (misc);
         misc->playing = misc->just_this_item = -1;
         misc->total_pages = 0;
         clear_tmp_dir (misc);
         file = strdup (get_input_file (misc, file));
         wclear (misc->screenwin);
         wrefresh (misc->screenwin);
         create_epub (misc, my_attribute, daisy, file, 1);
         play_epub (misc, my_attribute, daisy, file);
         break;
      default:
         beep ();
         break;
      } // switch

      if (misc->playing > -1 && kill (misc->player_pid, 0) != 0)
      {
// if not playing
         misc->player_pid = -2;
         get_next_phrase (misc, my_attribute, daisy, 1);
      } // if

      fd_set rfds;
      struct timeval tv;

      FD_ZERO (&rfds);
      FD_SET (0, &rfds);
      tv.tv_sec = 0;
      tv.tv_usec = 100000;
// pause till a key has been pressed or 0.1 seconds are elapsed
      select (1, &rfds, NULL, NULL, &tv);
   } // for
} // browse

void extract_epub (misc_t *misc, char *file)
{
   snprintf (misc->cmd, MAX_CMD - 1,
             "unar \"%s\" -f -o \"%s\" > /dev/null", file, misc->tmp_dir);
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
   snprintf (misc->cmd, MAX_CMD,
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
   if (access (str, R_OK) != 0)
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

void check_phrases (misc_t *misc, my_attribute_t *my_attribute,
                    daisy_t *daisy)
{
   int i;

   for (i = 0; i < misc->total_items; i++)
   {
      daisy[i].xml_file = strdup (daisy[i].orig_smil);
      split_phrases (misc, my_attribute, daisy, i);
      if (daisy[i].x <= 0)
         daisy[i].x = 2;
      if (strlen (daisy[i].label) + daisy[i].x >= 53)
         daisy[i].label[57 - daisy[i].x] = 0;
   } // for

   count_phrases (misc, my_attribute, daisy);

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
         daisy[j].orig_smil = strdup (daisy[j + 1].orig_smil);
         daisy[j].xml_file = strdup (daisy[j + 1].xml_file);
         daisy[j].anchor = strdup (daisy[j + 1].anchor);
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
         snprintf (daisy[i].label, 10, "%d", i + 1);
      } // if
   } // for
} // check_phrases

void store_as_WAV_file (misc_t *misc, my_attribute_t *my_attribute,
                        daisy_t *daisy)
{
   char *str, s2[MAX_STR];
   int i, pause_flag;
   struct passwd *pw;

   pause_flag = misc->playing;
   wclear (misc->screenwin);
   str = strdup (daisy[misc->current].label);
   for (i = 0; str[i] != 0; i++)
   {
      if (str[i] == '/')
         str[i] = '-';
      if (isspace (str[i]))
         str[i] = '_';
   } // for
   pw = getpwuid (geteuid ());
   strncpy (s2, pw->pw_dir, MAX_STR - 1);
   strncat (s2, "/", MAX_STR - 1);
   strncat (s2, str, MAX_STR - 1);
   strncat (s2, ".wav", MAX_STR - 1);
   while (access (s2, F_OK) == 0)
      strncat (s2, ".wav", MAX_STR - 1);
   if (pause_flag > -1)
      pause_resume (misc, my_attribute, daisy);
   write_wav (misc, my_attribute, daisy, s2);
   if (pause_flag > -1)
      pause_resume (misc, my_attribute, daisy);
   view_screen (misc, daisy);
} // store_as_WAV_file

void write_ascii (misc_t *misc, my_attribute_t *my_attribute,
                  daisy_t *daisy, char *outfile)
{
   FILE *w;

   if ((w = fopen (outfile, "a")) == NULL)
   {
      int e;

      e = errno;
      beep ();
      failure (misc, outfile, e);
   } // if
   open_text_file (misc, my_attribute,
                 daisy[misc->current].xml_file, daisy[misc->current].anchor);
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
         break;
      if (*daisy[misc->current + 1].anchor &&
          strcasecmp (my_attribute->id, daisy[misc->current + 1].anchor) == 0)
         break;
      if (! *misc->label)
         continue;;
      fprintf (w, "%s\n", misc->label);
   } // while
   fclose (w);
} // write_ascii

void store_as_ASCII_file (misc_t *misc, my_attribute_t *my_attribute,
                       daisy_t *daisy)
{
   char *str, s2[MAX_STR];
   int i, pause_flag;
   struct passwd *pw;;

   pause_flag = misc->playing;
   wclear (misc->screenwin);
   str = strdup (daisy[misc->current].label);
   for (i = 0; str[i] != 0; i++)
   {
      if (str[i] == '/')
         str[i] = '-';
      if (isspace (str[i]))
         str[i] = '_';
   } // for
   wprintw (misc->screenwin,
            "\nStoring \"%s\" as \"%s\" into your home-folder...",
            daisy[misc->current].label, str);
   wrefresh (misc->screenwin);
   pw = getpwuid (geteuid ());
   strncpy (s2, pw->pw_dir, MAX_STR - 1);
   strncat (s2, "/", MAX_STR - 1);
   strncat (s2, str, MAX_STR - 1);
   strncat (s2, ".txt", MAX_STR - 1);
   while (access (s2, F_OK) == 0)
      strncat (s2, ".txt", MAX_STR - 1);
   if (fopen (s2, "w") == NULL)
   {
      int e;

      e = errno;
      beep ();
      failure (misc, s2, e);
   } // if
   if (pause_flag > -1)
      pause_resume (misc, my_attribute, daisy);
   write_ascii (misc, my_attribute, daisy, s2);
   if (pause_flag > -1)
      pause_resume (misc, my_attribute, daisy);
   view_screen (misc, daisy);
} // store_as_ASCII_file

void usage ()
{
   endwin ();
   printf ("%s %s\n", gettext ("eBook-speaker - Version"), PACKAGE_VERSION);
   puts ("(C)2003-2018 J. Lemmens");
   printf ("\n");
   printf (gettext
      ("Usage: %s [eBook_file | URL | -s [-r resolution]] [-o language-code] "
         "[-d pulseaudio_sound_device] [-h] [-i] [-t TTS_command] [-b n | y] [-c]"),
            PACKAGE);                                                       
   printf ("\n");
   fflush (stdout);
   _exit (0);
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
      bytes = getline (&line, &bytes, r);
      if ((int) bytes == -1)
         break;
      fprintf (w, "%s<br>\n", line);
   } // while
   fclose (r);
   fclose (w);
   return str;
} // ascii2html

void play_epub (misc_t *misc, my_attribute_t *my_attribute,
                daisy_t *daisy, char *file)
{
   int i;

   daisy = create_daisy_struct (misc, my_attribute);

// first be sure all strings in daisy_t are cleared
   for (i = 0; i < misc->total_items; i++)
   {
      *daisy[i].my_class = daisy[i].page_number = 0;
      *daisy[i].label = 0;  
      daisy[i].clips_file = daisy[i].xml_file = daisy[i].anchor = strdup ("");
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
         if (*misc->bookmark_title)
// if misc->bookmark_title already is set
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
   if (misc->total_items == 0)
      misc->total_items = 1;
   if (misc->scan_flag)
      strcpy (misc->daisy_title, "scanned image");
   if (! *misc->daisy_title)
      strncpy (misc->daisy_title, basename (file), MAX_STR - 1);
   if (! *misc->bookmark_title)
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
      mvwprintw (misc->titlewin, 0, misc->max_x - strlen (misc->daisy_title) - 4, "... ");
   mvwprintw (misc->titlewin, 0, misc->max_x - strlen (misc->daisy_title),
              "%s", misc->daisy_title);
   mvwprintw (misc->titlewin, 1, 0,
              "----------------------------------------");
   wprintw (misc->titlewin, "----------------------------------------");
   mvwprintw (misc->titlewin, 1, 0, "%s ", gettext ("Press 'h' for help"));
   wrefresh (misc->titlewin);
   browse (misc, my_attribute, daisy, file);
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
      _exit (-1);
   } // if

   if (print)
   {
      wclear (misc->titlewin);
      mvwprintw (misc->titlewin, 1, 0,
                 "This is a %s file", magic_file (myt, file));
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
        "unar \"%s\" -f -o %s > /dev/null && ls -1 %s",
         file, misc->tmp_dir, misc->tmp_dir);
      p = popen (misc->cmd, "r");
      switch (*fgets (str, MAX_STR, p));
      pclose (p);
      str[strlen (str) - 1] = 0;
      file = malloc (strlen (misc->tmp_dir) + strlen (str) + 5);
      sprintf (file, "%s/%s", misc->tmp_dir, str);
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
       strncasecmp (magic_file (myt, file), "ASCII text", 10) == 0 ||
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
      misc->ignore_bookmark = 1;
      start_OCR (misc, file);
   } // if
   else
   if (strcasestr (magic_file (myt, file), "GIF image"))
   {
      char str[MAX_STR + 1];

      misc->ignore_bookmark = 1;
      if (misc->use_cuneiform == 0)
      {
         snprintf (str, MAX_STR, "%s/eBook-speaker", misc->tmp_dir);
         snprintf (misc->cmd, MAX_CMD - 1,
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
            _exit (0);
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
      snprintf (file, MAX_STR - 1, "%s",
                get_input_file (misc, file));
      wclear (misc->screenwin);
      wrefresh (misc->screenwin);
      create_epub (misc, my_attribute, daisy, file, 1);
      play_epub (misc, my_attribute, daisy, file);
      quit_eBook_speaker (misc);
      _exit (0);
   } // if directory
   magic_close (myt);
} // create_epub

int main (int argc, char *argv[])
{
   int opt;
   char b_opt, c_opt, *d_opt, *o_opt, *r_opt, *t_opt;
   misc_t misc;
   my_attribute_t my_attribute;
   daisy_t *daisy;
   struct sigaction usr_action;

   usr_action.sa_handler = player_ended;
   usr_action.sa_flags = 0;
   sigaction (SIGCHLD, &usr_action, NULL);
   daisy = NULL;
   misc.label = NULL;
   fclose (stderr);
   setbuf (stdout, NULL);
   if (setlocale (LC_ALL, "") == NULL)
      sprintf (misc.locale, "en_US.UTF-8");
   else
      snprintf (misc.locale, 3, "%s", setlocale (LC_ALL, ""));
   setlocale (LC_NUMERIC, "C");
   textdomain (PACKAGE);
   bindtextdomain (PACKAGE, LOCALEDIR);
   misc.src_dir = strdup (get_current_dir_name ());
   make_tmp_dir (&misc);
   misc.daisy_mp = misc.tmp_dir;
   if (access ("/usr/bin/sox", R_OK) != 0)
      failure (&misc, "eBook-speaker needs the sox package.", errno);
   misc.sound_dev = strdup ("0");
   initscr ();
   misc.titlewin = newwin (2, 80,  0, 0);
   misc.screenwin = newwin (23, 80, 2, 0);
   misc.ignore_bookmark = misc.total_pages = 0;
   misc.option_b = 0;
   misc.break_on_EOL = 'n';
   misc.level = 1;
   misc.show_hidden_files = 0;
   getmaxyx (misc.screenwin, misc.max_y, misc.max_x);
   misc.max_y -= 2;
   keypad (misc.titlewin, TRUE);
   keypad (misc.screenwin, TRUE);
   meta (misc.titlewin, TRUE);
   meta (misc.screenwin, TRUE);
   nonl ();
   noecho ();
   misc.player_pid = -2;
   sprintf (misc.scan_resolution, "400");
   snprintf (misc.copyright, MAX_STR - 1, "%s %s - (C)2018 J. Lemmens",
             gettext ("eBook-speaker - Version"), PACKAGE_VERSION);
   wattron (misc.titlewin, A_BOLD);
   wprintw (misc.titlewin, "%s - %s", misc.copyright,
            gettext ("Please wait..."));
   wattroff (misc.titlewin, A_BOLD);

   misc.speed = 1;
   misc.sound_dev = strdup ("0");
   for (misc.current = 0; misc.current < 12; misc.current++)
      *misc.tts[misc.current] = 0;
   snprintf (misc.tts[0], MAX_STR - 1,
             "espeak -f eBook-speaker.txt -w eBook-speaker.wav -v %s",
             misc.locale);
   *misc.ocr_language = 0;
   *misc.standalone = 0;
   *misc.xmlversion = 0;
   *misc.encoding = 0;
   snprintf (misc.eBook_speaker_txt, MAX_STR,
             "%s/eBook-speaker.txt", misc.tmp_dir);
   snprintf (misc.tmp_wav, MAX_STR, "%s/eBook-speaker.wav", misc.tmp_dir);
   if ((misc.tmp_wav_fd = mkstemp (misc.tmp_wav)) == 01)
      failure (&misc, "mkstemp ()", errno);
   misc.scan_flag = misc.option_t = misc.use_cuneiform = 0;
   misc.use_OPF = misc.use_NCX = 0;
   b_opt = c_opt= 0;
   d_opt = o_opt = r_opt = t_opt = NULL;
   opterr = 0;
   while ((opt = getopt (argc, argv, "b:cd:hilo:r:st:NO")) != -1)
   {
      switch (opt)
      {
      case 'b':
         misc.break_on_EOL = b_opt = optarg[0];
         misc.option_b = 1;
         break;
      case 'c':
         misc.use_cuneiform = c_opt = 1;
         break;
      case 'd':
         misc.sound_dev = strdup (optarg);
         d_opt = strdup (optarg);
         break;
      case 'h':
         usage ();
         break;
      case 'i':
         misc.ignore_bookmark = 1;
         break;
      case 'l':
         continue;
      case 'o':
         strncpy (misc.ocr_language, optarg, 3);
         o_opt = optarg;
         break;
      case 'r':
         strncpy (misc.scan_resolution, optarg, 5);
         r_opt = optarg;
         break;
      case 's':
         misc.scan_flag = 1;
         break;
      case 't':
         misc.option_t = 1;
         misc.tts_no = 0;
         strncpy (misc.tts[misc.tts_no], optarg, MAX_STR - 1);
         t_opt = optarg;
         break;
      case 'N': // ment for debugging
         misc.use_NCX = 1;
         break;
      case 'O': // ment for debugging
         misc.use_OPF = 1;
         break;
      default:
         beep ();
         usage ();
      } // switch
   } // while

   if (misc.ignore_bookmark == 0)
      load_xml (&misc, &my_attribute);
   if (b_opt)
   {
      misc.break_on_EOL = b_opt;
      misc.option_b = 1;
   } // if
   if (c_opt)
      misc.use_cuneiform = c_opt;
   if (d_opt)
      misc.sound_dev = strdup (d_opt);
   if (o_opt)
      strncpy (misc.ocr_language, o_opt, 3);
   if (r_opt)
      strncpy (misc.scan_resolution, r_opt, 5);
   if (t_opt)
   {
      misc.tts_no = 0;
      strncpy (misc.tts[misc.tts_no], t_opt, MAX_STR - 1);
   } // if

   remove_double_tts_entries (&misc);
   misc.search_str = strdup ("");

   if (misc.scan_flag == 1)
   {
      char *cmd;

      misc.ignore_bookmark = 1;
      wrefresh (misc.titlewin);
      snprintf (misc.orig_input_file, MAX_STR, "%s/out.pgm", misc.tmp_dir);
/* Temporarily disabled
      snprintf (misc.cmd, MAX_CMD,
          "scanimage --resolution %s --mode Binary > %s",
           misc.scan_resolution, misc.orig_input_file);
*/
      cmd = malloc (50 + strlen (misc.scan_resolution) +
                    strlen (misc.orig_input_file));
      sprintf (cmd, "scanimage --resolution %s > %s",
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
         _exit (-1);
      } // switch
      create_epub (&misc, &my_attribute, daisy, misc.orig_input_file, 1);
      strcpy (misc.daisy_title, "scanned image");
      play_epub (&misc, &my_attribute, daisy, misc.orig_input_file);
      return 0;
   } // if

   if (! argv[optind])
   {
   // if no argument is given
      snprintf (misc.orig_input_file, MAX_STR - 1, "%s",
                get_input_file (&misc, misc.src_dir));
      create_epub (&misc, &my_attribute, daisy, misc.orig_input_file, 1);
      play_epub (&misc, &my_attribute, daisy, misc.orig_input_file);
      quit_eBook_speaker (&misc);
      return 0;
   } // if

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
            _exit (-1);
         } // switch
         snprintf (misc.orig_input_file, MAX_STR, "%s/wget.out",
                   misc.tmp_dir);
      }
      else
      {
         strncpy (misc.orig_input_file, misc.src_dir, MAX_STR);
         if (misc.orig_input_file[strlen (misc.orig_input_file) - 1] != '/')
            strncat (misc.orig_input_file, "/", MAX_STR);
         strncat (misc.orig_input_file, argv[optind], MAX_STR);
      } // if   
   } // if

   if (access (misc.orig_input_file, R_OK) == -1)
   {
      int e;

      e = errno;
      endwin ();
      puts (misc.copyright);
      beep ();
      failure (&misc, argv[optind], e);
   } // if

   misc.current = 0;
   create_epub (&misc, &my_attribute, daisy, misc.orig_input_file, 1);
   play_epub (&misc, &my_attribute, daisy, misc.orig_input_file);
   return 0;
} // main
