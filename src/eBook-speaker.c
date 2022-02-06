/* eBook-speaker - read aloud an eBook using a speech synthesizer
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

#include "daisy.h"

void quit_eBook_speaker (misc_t *misc)
{
   save_bookmark_and_xml (misc);
   puts ("");
   remove_tmp_dir (misc);
   _exit (0);
} // quit_eBook_speaker

int get_next_phrase (misc_t *misc, my_attribute_t *my_attribute,
                     daisy_t *daisy)
{
   FILE *w;

   if (access (misc->eBook_speaker_wav, R_OK) == 0)
// when still playing
      return 0;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
      {
         if (misc->playing >= misc->total_items - 1)
         {
            char str[MAX_STR];
            struct passwd *pw;;

            save_bookmark_and_xml (misc);
            pw = getpwuid (geteuid ());
            strncpy (str, pw->pw_dir, MAX_STR - 1);
            strncat (str, "/.eBook-speaker/", MAX_STR - 1);
            strncat (str, misc->bookmark_title, MAX_STR - 1);
            unlink (str);
            _exit (0);
         } // if
         return -1;
      } // if
      if (strcasecmp (misc->tag, "pagenum") == 0)
      {
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, misc->reader))
               break;
            if (strcasecmp (misc->tag, "/pagenum") == 0)
               break;
            if (*misc->label)
            {
               misc->current_page_number = atoi (misc->label);
               break;
            } // if
         } // while
      } // if
      if (misc->playing != misc->total_items &&
          *daisy[misc->playing + 1].anchor &&
          strcasecmp (my_attribute->id, daisy[misc->playing + 1].anchor) == 0)
      {
         return -1;
      } // if
      if (*misc->label)
         break;
   } // while

   if (daisy[misc->playing].screen == daisy[misc->current].screen)
   {
      char str[100];

      wattron (misc->screenwin, A_BOLD);
      if (misc->current_page_number)
         mvwprintw (misc->screenwin, daisy[misc->playing].y, 63,
                    "(%3d)", misc->current_page_number);
      mvwprintw (misc->screenwin, daisy[misc->playing].y, 69, "%5d %5d",
               misc->phrase_nr + 1,
               daisy[misc->playing].n_phrases - misc->phrase_nr - 1);
      (misc->phrase_nr)++;
      wattroff (misc->screenwin, A_BOLD);
      strncpy (str, misc->label, 80);
      mvwprintw (misc->screenwin, 22, 0, str);
      wclrtoeol (misc->screenwin);
      wmove (misc->screenwin, daisy[misc->displaying].y,
             daisy[misc->displaying].x - 1);
      wrefresh (misc->screenwin);
   } // if
   if ((w = fopen (misc->eBook_speaker_txt, "w")) == NULL)
   {
      char str[MAX_STR + 1];

      snprintf (str, MAX_STR, "fopen (%s)", misc->eBook_speaker_txt);
      failure (str, errno);
   } // if
   if (fwrite (misc->label, strlen (misc->label), 1, w) == 0)
   {
      int e;
      char str[MAX_STR];

      e = errno;
      beep ();
      snprintf (str, MAX_STR, "write (\"%s\"): failed.\n", misc->label);
      failure (str, e);
   } // if
   fclose (w);

   if (*misc->tts[misc->tts_no] == 0 && misc->option_t == 0)
   {
      misc->tts_no = 0;
      select_tts (misc, daisy);
   } // if

   switch (system (misc->tts[misc->tts_no]))
   {
   default:
      break;
   } // switch

   switch (misc->player_pid = fork ())
   {
   case -1:
   {
      int e;

      e = errno;
      skip_left (misc, my_attribute, daisy);
      put_bookmark (misc);
      failure ("fork ()", e);
   }
   case 0:
      setsid ();
      playfile (misc);
      _exit (0);
   default:
      return 0;
   } // switch
} // get_next_phrase                                                 

void select_tts (misc_t *misc, daisy_t *daisy)
{
   int n, y, x = 2;

   wclear (misc->screenwin);
   wprintw (misc->screenwin, "\n%s\n\n", gettext
            ("Select a Text-To-Speech application"));
   for (n = 0; n < 10; n++)
   {
      char str[MAX_STR];

      if (*misc->tts[n] == 0)
         break;
      strncpy (str, misc->tts[n], MAX_STR - 1);
      str[72] = 0;
      wprintw (misc->screenwin, "    %d %s\n", n, str);
   } // for
   wprintw (misc->screenwin, "\n%s\n\n", gettext
            ("Provide a new TTS."));
   wprintw (misc->screenwin, "%s\n", gettext
            ("Be sure that the new TTS reads its information from the file"));
   wprintw (misc->screenwin, "%s\n\n", gettext
    ("eBook-speaker.txt and that it writes to the file eBook-speaker.wav."));
   wprintw (misc->screenwin, "%s\n", gettext
            ("Press DEL to delete a TTS"));
   wprintw (misc->screenwin,
           "    -------------------------------------------------------------");
   if (*misc->tts[misc->tts_no] == 0)
      misc->tts_no = 0;
   y = misc->tts_no + 3;
   nodelay (misc->screenwin, FALSE);
   for (;;)
   {
      if (*misc->tts[n] == 0)
         wmove (misc->screenwin, y, x);
      switch (wgetch (misc->screenwin))
      {
      int i;

      case 13:
         misc->tts_no = y - 3;
         view_screen (misc, daisy);
         nodelay (misc->screenwin, TRUE);
         return;
      case KEY_DOWN:
         if (++y == n + 3)
         {
            y = n + 10;
            x = 4;
            wmove (misc->screenwin, y, x);
            nodelay (misc->screenwin, FALSE);
            echo ();
            misc->tts_no = n;
            if (misc->tts_no > 9)
               misc->tts_no = 9;
            wgetnstr (misc->screenwin, misc->tts[misc->tts_no], MAX_STR - 1);
            noecho ();
            if (*misc->tts[misc->tts_no])
            {
               view_screen (misc, daisy);
               nodelay (misc->screenwin, TRUE);
               return;
            } // if
            y = 3;
            x = 2;
         } // if
         break;
      case KEY_UP:
         if (--y == 2)
         {
            y = n + 10;
            x = 4;
            wmove (misc->screenwin, y, x);
            nodelay (misc->screenwin, FALSE);
            echo ();
            misc->tts_no = n;
            if (misc->tts_no > 9)
               misc->tts_no = 9;
            wgetnstr (misc->screenwin, misc->tts[misc->tts_no], MAX_STR - 1);
            noecho ();
            if (*misc->tts[misc->tts_no])
            {
               view_screen (misc, daisy);
               nodelay (misc->screenwin, TRUE);
               return;
            } // if
            y = 3;
            x = 2;
         } // if
         break;
      case KEY_DC:
         if (*misc->tts[1] == 0)
         {
            beep ();
            continue;
         } // if
         for (i = y - 3; i < 10; i++)
            strncpy (misc->tts[i], misc->tts[i + 1], MAX_STR - 1);
         misc->tts_no = y - 3;
         view_screen (misc, daisy);
         nodelay (misc->screenwin, TRUE);
         return;
      default:
         if (x == 2)
         {
            view_screen (misc, daisy);
            nodelay (misc->screenwin, TRUE);
            return;
         } // if
      } // switch
   } // for
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
      if (! *daisy[item].smil_file)
         continue;
      if (! (d = htmlParseFile (daisy[item].smil_file, "UTF-8")))
         failure ("htmlParseFile ()", errno);
      if (! (r = xmlReaderWalker (d)))
         failure ("xmlReaderWalker ()", errno);
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
         if (strcasecmp (daisy[item].smil_file,
             daisy[item + 1].smil_file) != 0)
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
   char new[MAX_STR + 1], *p;
   int start_of_new_phrase;
   htmlDocPtr doc;
   xmlTextWriterPtr writer;

   if ((doc = htmlParseFile (daisy[i].smil_file, "UTF-8")) == NULL)
      failure (daisy[i].smil_file, errno);
   if ((reader = xmlReaderWalker (doc))  == NULL)
      failure (daisy[i].smil_file, errno);
   snprintf (new, MAX_STR, "%s.splitted.xhtml", daisy[i].smil_file);
   if ((writer = xmlNewTextWriterFilename (new, 0)) == NULL)
   {
      int e;
      char str[MAX_STR], str2[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, "fopen (%s): %s\n", str2, strerror (e));
      failure (str2, e);
   } // if
   if (! *misc->xmlversion)
      strncpy (misc->xmlversion, "1.0", 5);
   if (! *misc->encoding)
      strncpy (misc->encoding, "utf-8", 8);
   xmlTextWriterStartDocument
                (writer, misc->xmlversion, misc->encoding, misc->standalone);
   xmlTextWriterSetIndent (writer, 1);
   xmlTextWriterSetIndentString (writer, BAD_CAST "");
   xmlTextWriterStartElement (writer, BAD_CAST "html");
   xmlTextWriterWriteString (writer, BAD_CAST "\n");
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, reader))
         break;
      if (strcasecmp (misc->tag, "head") == 0)
      {
         xmlTextWriterStartElement (writer, (const xmlChar *) misc->tag);
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, reader))
               break;
            if (strcasecmp (misc->tag, "/head") == 0)
            {
               xmlTextWriterEndElement (writer);
               break;
            } // if /hed
            if (strcasecmp (misc->tag, "title") == 0)
            {
               if (xmlTextReaderIsEmptyElement (reader))
               {
                  continue;
               } // if
               xmlTextWriterStartElement (writer,
                                          (const xmlChar *) misc->tag);
               while (1)
               {
                  if (! get_tag_or_label (misc, my_attribute, reader))
                     break;
                  if (*misc->label)
                     xmlTextWriterWriteString (writer, BAD_CAST misc->label);
                  if (strcasecmp (misc->tag, "/title") == 0)
                  {
                     xmlTextWriterEndElement (writer);
                     break;
                  } // if /title
               } // while
            } // if title
            if (strcasecmp (misc->tag, "/head") == 0)
            {
               xmlTextWriterWriteString (writer, BAD_CAST "  ");
               xmlTextWriterEndElement (writer);
               break;
            } // /head
         } // while
      } // if head
      if (strcasecmp (misc->tag, "title") == 0)
      {
         if (xmlTextReaderIsEmptyElement (reader))
         {
            get_tag_or_label (misc, my_attribute, reader);
            continue;
         } // if
         xmlTextWriterStartElement (writer, (const xmlChar *) misc->tag);
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, reader))
               break;
            if (*misc->label)
               xmlTextWriterWriteString (writer, BAD_CAST misc->label);
            if (strcasecmp (misc->tag, "/title") == 0)
            {
               xmlTextWriterEndElement (writer);
               break;
            } // if /title
         } // while
      }  // if title
      if (strcasecmp (misc->tag, "body") == 0)
      {
         xmlTextWriterStartElement (writer, BAD_CAST "body");
         xmlTextWriterWriteString (writer, BAD_CAST "\n");
         continue;
      } // if 'body
      if (*my_attribute->id)
      {
         if (xmlTextReaderIsEmptyElement (reader))
            continue;
         xmlTextWriterStartElement (writer, (const xmlChar *) misc->tag);
         xmlTextWriterWriteFormatAttribute
                    (writer, BAD_CAST "id", "%s", my_attribute->id);
         xmlTextWriterWriteString (writer, BAD_CAST "\n");
         continue;
      } // if my_attribute->id
      if (strcasecmp (misc->tag, "text") == 0)
      {
         xmlTextWriterStartElement (writer, (const xmlChar *) misc->tag);
         xmlTextWriterWriteFormatAttribute
                        (writer, BAD_CAST "src", "%s", my_attribute->src);
         xmlTextWriterEndAttribute (writer);
         xmlTextWriterEndElement (writer);
         continue;
      } // if text
      if (strcasecmp (misc->tag, "h1") == 0 ||
          strcasecmp (misc->tag, "h2") == 0 ||
          strcasecmp (misc->tag, "h3") == 0 ||
          strcasecmp (misc->tag, "h4") == 0 ||
          strcasecmp (misc->tag, "h5") == 0 ||
          strcasecmp (misc->tag, "h6") == 0)
      {
         if (xmlTextReaderIsEmptyElement (reader))
            continue;
         xmlTextWriterStartElement (writer, (const xmlChar *) misc->tag);
         xmlTextWriterWriteFormatAttribute
            (writer, BAD_CAST "id", "%s", my_attribute->id);
         xmlTextWriterWriteString (writer, BAD_CAST "\n");
         continue;
      } // if "h1"
      if (misc->tag[0] == '/')
      {
         xmlTextWriterEndElement (writer);
         continue;
      } // if /h1
      if (strcasecmp (misc->tag, "pagenum") == 0)
      {
         xmlTextWriterStartElement (writer, (const xmlChar *) misc->tag);
         xmlTextWriterWriteFormatAttribute
                (writer, BAD_CAST "id", "%s", my_attribute->id);
         xmlTextWriterWriteFormatAttribute
                (writer, BAD_CAST "smilref", "%s", my_attribute->smilref);
         xmlTextWriterWriteString (writer, BAD_CAST "\n");
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, reader))
               break;
            if (*misc->label)
            {
               xmlTextWriterWriteString (writer, BAD_CAST misc->label);
               xmlTextWriterWriteString (writer, BAD_CAST "\n    ");
            } // if
            if (strcasecmp (misc->tag, "/pagenum") == 0)
               break;
         } // while
         xmlTextWriterEndElement (writer);
         continue;
      }  // if pagenum
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
         if ((*p == '.' || *p == ',' || *p == '!' || *p == '?' || *p == '/' ||
              *p == ':' || *p == ';' || *p == '-' || *p == '*') &&
             (*(p + 1) == ' ' || *(p + 1) == '\r' || *(p + 1) == '\n' ||
              *(p + 1) == 0 || *(p + 1) == '\'' ||
// ----
              *(p + 1) == -62))
              // I don't understand it, but it works
              // Some files use -62 -96 (&nbsp;) as space
// ----
         {
            xmlTextWriterWriteFormatString (writer, "%c", *p);
            xmlTextWriterWriteString (writer, BAD_CAST "\n");
            xmlTextWriterWriteElement (writer, BAD_CAST "br", NULL);
            p++;
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
   strncpy (daisy[i].smil_file, new, MAX_STR - 1);
} // split_phrases

void playfile (misc_t *misc)
{
   sox_format_t *in, *out; /* input and output files */
   sox_effects_chain_t *chain;
   sox_effect_t *e;
   char *args[MAX_STR], str[MAX_STR];

   sox_globals.verbosity = 0;
   sox_init ();
   if (! (in = sox_open_read (misc->eBook_speaker_wav, NULL, NULL, "wav")))
   {
      int e;
      char str[MAX_STR + 1];

      e = errno;
      endwin ();
      snprintf (str, MAX_STR, "sox_open_read: %s: %s",
                misc->eBook_speaker_wav, strerror (e));
      printf ("%s\n", str);
      printf (gettext
              ("Be sure that the used TTS is installed:\n\n   \"%s\"\n"),
              misc->tts[misc->tts_no]);
      fflush (stdout);
      beep ();
      kill (getppid (), SIGTERM);
   } // if
   if ((out = sox_open_write (misc->sound_dev, &in->signal,
           NULL, "alsa", NULL, NULL)) == NULL)
   {
      if ((out = sox_open_write ("default", &in->signal,
                                 NULL, "alsa", NULL, NULL)) == NULL)
      {
         endwin ();
         printf ("%s: %s\n", misc->sound_dev, gettext (strerror (EINVAL)));
         strncpy (misc->sound_dev, "hw:0", 5);
         save_xml (misc);
         beep ();
         fflush (stdout);
         kill (getppid (), SIGQUIT);
      } // if
   } // if

   chain = sox_create_effects_chain (&in->encoding, &out->encoding);

   e = sox_create_effect (sox_find_effect ("input"));
   args[0] = (char *) in, sox_effect_options (e, 1, args);
   sox_add_effect (chain, e, &in->signal, &in->signal);

   e = sox_create_effect (sox_find_effect ("tempo"));
   snprintf (str, MAX_STR, "%lf", misc->speed);
   args[0] = "-s";
   args[1] = str;
   sox_effect_options (e, 2, args);
   sox_add_effect (chain, e, &in->signal, &in->signal);

   e = sox_create_effect (sox_find_effect ("vol"));
   snprintf (str, MAX_STR, "%lf", misc->volume);
   args[0] = str;
   sox_effect_options (e, 1, args);
   sox_add_effect (chain, e, &in->signal, &in->signal);

   snprintf (str, MAX_STR - 1, "%lf", out->signal.rate);
   e = sox_create_effect (sox_find_effect ("rate"));
   args[0] = str, sox_effect_options (e, 1, args);
   sox_add_effect (chain, e, &in->signal, &in->signal);

   snprintf (str, MAX_STR - 1, "%i", out->signal.channels);
   e = sox_create_effect (sox_find_effect ("channels"));
   args[0] = str, sox_effect_options (e, 1, args);
   sox_add_effect (chain, e, &in->signal, &in->signal);

   e = sox_create_effect (sox_find_effect ("output"));
   args[0] = (char *) out, sox_effect_options (e, 1, args);
   sox_add_effect (chain, e, &in->signal, &out->signal);

   sox_flow_effects (chain, NULL, NULL);
   sox_delete_effects_chain (chain);
   sox_close (out);
   sox_close (in);
   sox_quit ();
   unlink (misc->eBook_speaker_wav);
} // playfile

void put_bookmark (misc_t *misc)
{
   char str[MAX_STR];
   xmlTextWriterPtr writer;
   struct passwd *pw;;

   pw = getpwuid (geteuid ());
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker", pw->pw_dir);
   mkdir (str, 0755);
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker/%s",
             pw->pw_dir, misc->bookmark_title);
   if (! (writer = xmlNewTextWriterFilename (str, 0)))
      return;
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
} // put_bookmark

void get_bookmark (misc_t *misc, my_attribute_t *my_attribute,
                   daisy_t *daisy)
{
   char str[MAX_STR];
   xmlTextReaderPtr local_reader;
   htmlDocPtr local_doc;
   int tts_no, pn;
   struct passwd *pw;;

   pw = getpwuid (geteuid ());
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker/%s",
             pw->pw_dir, misc->bookmark_title);
   local_doc = htmlParseFile (str, "UTF-8");
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
   if (misc->current < 0 || misc->current > misc->total_items)
      misc->current = 0;
   if (misc->phrase_nr < 0)
      misc->phrase_nr = 0;
   if (misc->level < 1 || misc->level > misc->depth)
      misc->level = 1;
   misc->displaying = misc->playing = misc->current;
   misc->current_page_number = daisy[misc->playing].page_number;
   misc->just_this_item = -1;
   go_to_phrase (misc, my_attribute, daisy, pn);
} // get_bookmark

void view_screen (misc_t *misc, daisy_t *daisy)
{
   int i, x, l;

   mvwaddstr (misc->titlewin, 1, 0,
              "----------------------------------------");
   waddstr (misc->titlewin, "----------------------------------------");
   mvwprintw (misc->titlewin, 1, 0, gettext ("'h' for help "));
   if (! misc->discinfo)
   {
      if (misc->total_pages)
         mvwprintw (misc->titlewin, 1, 15, gettext
                    (" %d pages "), misc->total_pages);
      mvwprintw (misc->titlewin, 1, 28, gettext
                 (" level: %d of %d "), misc->level, misc->depth);
      mvwprintw (misc->titlewin, 1, 47, gettext
                 (" total phrases: %d "), misc->total_phrases);
      mvwprintw (misc->titlewin, 1, 74, " %d/%d ",
              misc->current / misc->max_y + 1, (misc->total_items - 1) / misc->max_y + 1);
   } // if
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

void player_ended ()
{
   wait (NULL);
} // player_ended

void open_text_file (misc_t *misc, my_attribute_t *my_attribute,
        char *text_file, char *anchor)
{
   htmlDocPtr doc;

   doc = htmlParseFile (text_file, "UTF-8");
   if ((misc->reader = xmlReaderWalker (doc)) == NULL)
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, "open_text_file (%s)", text_file);
      failure (str, e);
   } // if
   if (! *anchor)
      return;

// look if anchor exists in this text_file
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
         break;
      if (strcasecmp (anchor, my_attribute->id) == 0)
         break;
   } // while
} // open_text_file

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
            failure ("Can't make a temp file", errno);
         clearerr (w);
         fwrite (misc->label, strlen (misc->label), 1, w);
         if (ferror (w) != 0)
         {
            int e;
            char str[MAX_STR];

            e = errno;
            snprintf (str, MAX_STR, "write (\"%s\"): failed.\n", misc->label);
            failure (str, e);
         } // if
         fclose (w);

         switch (system (misc->tts[misc->tts_no]))
         {
         default:
            break;
         } // switch

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
            failure ("fork ()", e);
         }
         case 0:
            setsid ();
            playfile (misc);
            _exit (0);
         default:
            return;
         } // switch       
      }
      else
         kill_player (misc);
   } // if
} // pause_resume

void write_wav (misc_t *misc, my_attribute_t *my_attribute,
                daisy_t *daisy, char *outfile)
{
   FILE *w;
   sox_format_t *first, *output;
#define MAX_SAMPLES (size_t)2048
   sox_sample_t samples[MAX_SAMPLES];
   size_t number_read;

   if ((w = fopen (misc->eBook_speaker_txt, "w")) == NULL)
      failure ("Can't make a temp file", errno);
   if (fwrite ("init", 4, 1, w) == 0)
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, "write (\"%s\"): failed.\n", misc->label);
      failure (str, e);
   } // if
   fclose (w);
   if (! *misc->tts[misc->tts_no])
   {
      misc->tts_no = 0;
      select_tts (misc, daisy);
   } // if
   switch (system (misc->tts[misc->tts_no]))
   {
   default:
      break;
   } // switch
   sox_init ();
   rename (misc->eBook_speaker_wav, misc->first_eBook_speaker_wav);
   first = sox_open_read (misc->first_eBook_speaker_wav, NULL, NULL, NULL);
   output = sox_open_write (outfile, &first->signal, &first->encoding,
                            NULL, NULL, NULL);
   open_text_file (misc, my_attribute,
                daisy[misc->current].smil_file, daisy[misc->current].anchor);
   while (1)
   {
      sox_format_t *input;

      if (! get_tag_or_label (misc, my_attribute, misc->reader))
         break;
      if (*daisy[misc->current + 1].anchor &&
          strcasecmp (my_attribute->id, daisy[misc->current + 1].anchor) == 0)
         break;
      if (! *misc->label)
         continue;;
      w = fopen (misc->eBook_speaker_txt, "w");
      fwrite (misc->label, strlen (misc->label), 1, w);
      fclose (w);
      switch (system (misc->tts[misc->tts_no]))
      {
      default:
         break;
      } // switch
      input = sox_open_read (misc->eBook_speaker_wav, NULL, NULL, NULL);
      while ((number_read = sox_read (input, samples, MAX_SAMPLES)))
         sox_write (output, samples, number_read);
      sox_close (input);
   } // while
   sox_close (output);
   sox_close (first);
   sox_quit ();
} // write_wav

void help (misc_t *misc, daisy_t *daisy)
{
   int y, x;

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

void skip_left (misc_t *misc, my_attribute_t *my_attribute,
                daisy_t *daisy)
{
   int just = misc->just_this_item;

   if (misc->playing == -1)
   {
      beep ();
      return;
   } // if

   if (misc->phrase_nr == 1)
   {
      if (misc->playing == 0)
      {
         beep ();
         return;
      } // if
      misc->playing--;
      misc->displaying = misc->current = misc->playing;
      kill_player (misc);
      misc->player_pid = -2;
      open_text_file (misc, my_attribute,
               daisy[misc->playing].smil_file, daisy[misc->playing].anchor);
      misc->just_this_item = just;
      misc->phrase_nr = 0;
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, misc->reader))
            return;
         if (! *misc->label)
            continue;
         if (misc->phrase_nr++ == daisy[misc->playing].n_phrases - 2)
            return;
      } // while
   } // if

   kill_player (misc);
   misc->player_pid = -2;
   go_to_phrase (misc, my_attribute, daisy, misc->phrase_nr - 1);
   misc->just_this_item = just;
} // skip_left

void skip_right (misc_t *misc)
{
   if (misc->playing == -1)
   {
      beep ();
      return;
   } // if
   kill_player (misc);
} // skip_right

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

void read_xml (misc_t *misc, my_attribute_t *my_attribute)
{
   int x = 1, i;
   char str[MAX_STR], *p;
   struct passwd *pw;;
   xmlTextReaderPtr local_reader;
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
   if ((local_reader = xmlReaderWalker (doc)))
   {
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, local_reader))
            break;
         if (xmlTextReaderIsEmptyElement (local_reader))
            continue;
         if (strcasecmp (misc->tag, "tts") == 0)
         {
            do
            {
               if (! get_tag_or_label (misc, my_attribute, local_reader))
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
      xmlTextReaderClose (local_reader);
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
} // read_xml

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

void search (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy,
             int start, char mode)
{
   int c, found = 0;

   pause_resume (misc, my_attribute, daisy);
   if (mode == '/' || *misc->search_str == 0)
   {
      mvwprintw (misc->titlewin, 1, 0,
                 "                                        ");
      wprintw (misc->titlewin, "                                        ");
      mvwprintw (misc->titlewin, 1, 0,
                 gettext ("What do you search? "));
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
      remove (misc->eBook_speaker_wav);
      remove ("old.wav");
      open_text_file (misc, my_attribute,
                      daisy[misc->current].smil_file, daisy[misc->current].anchor);
   }
   else
   {
      beep ();
      pause_resume (misc, my_attribute, daisy);
   } // if
} // search

void break_on_EOL (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy,
             xmlTextReaderPtr reader, htmlDocPtr doc)
{
   int  level;

   kill_player (misc);
   xmlTextReaderClose (reader);
   xmlFreeDoc (doc);
   remove (misc->eBook_speaker_wav);
   remove ("old.wav");
   misc->phrase_nr = misc->total_phrases = 0;
   level = misc->level;
   mvwprintw (misc->titlewin, 1, 0,
                            "                                        ");
   wprintw (misc->titlewin, "                                        ");
   mvwprintw (misc->titlewin, 1, 0,
      gettext ("Consider the end of an input line as a phrase-break (y/n) "));
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

void kill_player (misc_t *misc)
{
   kill (misc->player_pid, SIGKILL);
   unlink (misc->eBook_speaker_txt);
   unlink (misc->eBook_speaker_wav);
} // kill_player

void select_next_output_device (misc_t *misc, daisy_t *daisy)
{
   FILE *r;
   int n, y;
   size_t bytes;
   char *list[10], *trash;

   wclear (misc->screenwin);
   wprintw (misc->screenwin, "\n%s\n\n", gettext ("Select a soundcard:"));
   if (! (r = fopen ("/proc/asound/cards", "r")))
      failure (gettext ("Cannot read /proc/asound/cards"), errno);
   for (n = 0; n < 10; n++)
   {
      list[n] = (char *) malloc (1000);
      bytes = getline (&list[n], &bytes, r);
      if ((int) bytes == -1)
         break;              
      trash = (char *) malloc (1000);
      bytes = getline (&trash, &bytes, r);
      free (trash);
      wprintw (misc->screenwin, "   %s", list[n]);
      free (list[n]);
   } // for
   fclose (r);
   y = 3;
   nodelay (misc->screenwin, FALSE);
   for (;;)
   {
      wmove (misc->screenwin, y, 2);
      switch (wgetch (misc->screenwin))
      {
      case 13: //
         snprintf (misc->sound_dev, MAX_STR - 1, "hw:%i", y - 3);
         view_screen (misc, daisy);
         nodelay (misc->screenwin, TRUE);
         return;
      case KEY_DOWN:
         if (++y == n + 3)
            y = 3;
         break;
      case KEY_UP:
         if (--y == 2)
           y = n + 2;
         break;
      default:
         view_screen (misc, daisy);
         nodelay (misc->screenwin, TRUE);
         return;
      } // switch
   } // for
} // select_next_output_device

void go_to_phrase (misc_t *misc, my_attribute_t *my_attribute,
                      daisy_t *daisy, int pn)
{
   open_text_file (misc, my_attribute,
               daisy[misc->current].smil_file, daisy[misc->current].anchor);
   misc->phrase_nr = 0;
   misc->current_page_number = daisy[misc->current].page_number;
   if (pn > 1)
   {
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, misc->reader))
            break;
         if (strcasecmp (misc->tag, "pagenum") == 0)
         {
            while (1)
            {
               if (! get_tag_or_label (misc, my_attribute, misc->reader))
                  break;
               if (strcasecmp (misc->tag, "/pagenum") == 0)
                  break;
               if (*misc->label)
               {
                  misc->current_page_number = atoi (misc->label);
                  break;
               } // if
            } // while
            continue;
         } // if
         if (! *misc->label)
            continue;
         misc->phrase_nr++;
         if (misc->phrase_nr + 1 == pn)
            break;
      } // while
   } // if
   misc->playing = misc->displaying = misc->current;
   view_screen (misc, daisy);
   mvwprintw (misc->screenwin, daisy[misc->current].y, 69, "%5d %5d",
              misc->phrase_nr + 1, daisy[misc->current].n_phrases - misc->phrase_nr - 1);
   wrefresh (misc->screenwin);
   wmove (misc->screenwin, daisy[misc->current].y,
          daisy[misc->current].x - 1);
   misc->just_this_item = -1;
   remove (misc->eBook_speaker_wav);
   remove ("old.wav");
} // go_to_phrase

void go_to_page_number (misc_t *misc, my_attribute_t *my_attribute,
                        daisy_t *daisy)
{
   char pn[15];
   htmlDocPtr doc;

   kill (misc->player_pid, SIGKILL);
   misc->playing = misc->just_this_item = -1;
   mvwprintw (misc->titlewin, 1, 0, "----------------------------------------");
   wprintw (misc->titlewin, "----------------------------------------");
   mvwprintw (misc->titlewin, 1, 0, gettext ("Go to page number: "));
   echo ();
   wgetnstr (misc->titlewin, pn, 5);
   noecho ();
   view_screen (misc, daisy);
   if (! *pn || atoi (pn) > misc->total_pages || ! atoi (pn))
   {
      beep ();
      pause_resume (misc, my_attribute, daisy);
      pause_resume (misc, my_attribute, daisy);
      return;
   } // if

// start searching
   for (misc->current = misc->total_items - 1; misc->current >= 0;
        misc->current--)
   {
      if (! daisy[misc->current].page_number)
         continue;
      if (daisy[misc->current].page_number == atoi (pn))
      {
         misc->playing = misc->displaying = misc->current;
         misc->phrase_nr = 0;
         misc->just_this_item = -1;
         misc->current_page_number = atoi (pn);;
         view_screen (misc, daisy);
         open_text_file (misc, my_attribute,
                daisy[misc->current].smil_file, daisy[misc->current].anchor);
         pause_resume (misc, my_attribute, daisy);
         pause_resume (misc, my_attribute, daisy);
         return;
      } // if
      if (daisy[misc->current].page_number <= atoi (pn))
         break;
   } // for
   if (misc->current < 0)
      misc->current = 0;
   doc = htmlParseFile (daisy[misc->current].smil_file, "UTF-8");
   if (! (misc->reader = xmlReaderWalker (doc)))
   {
      int e;
      char str[MAX_STR + 1];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"),
                daisy[misc->current].smil_file);
      failure (str, e);
   } // if
   
// first skip to anchor
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
         break;
      if (strcasecmp (my_attribute->id, daisy[misc->current].anchor) == 0)
         break;
   } // while

   misc->phrase_nr = 0;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
         return;
      if (strcasecmp (misc->tag, "pagenum") == 0)
      {
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, misc->reader))
               break;
            if (strcasecmp (misc->tag, "/pagenum") == 0)
               break;
            if (*misc->label)
            {
               misc->current_page_number = atoi (misc->label);
               if (misc->current_page_number == atoi (pn))
               {
                  misc->playing = misc->displaying = misc->current;
                  misc->just_this_item = -1;
                  misc->phrase_nr++;
                  view_screen (misc, daisy);
                  pause_resume (misc, my_attribute, daisy);
                  pause_resume (misc, my_attribute, daisy);
                  return;
               } // if
            } // if
         } // while
      } // if
      if (*misc->label)
         misc->phrase_nr++;
   } // while
} // go_to_page_number

void start_tesseract (misc_t *misc, char *file)
{
   char str[MAX_STR + 1];

   if (! *misc->ocr_language)
   {
      if (strcmp (misc->locale, "da") == 0)
         strncpy (misc->ocr_language, "dan", 5);
      if (strcmp (misc->locale, "de") == 0)
         strncpy (misc->ocr_language, "deu", 5);
      if (strcmp (misc->locale, "en") == 0)
         strncpy (misc->ocr_language, "eng", 5);
      if (strcmp (misc->locale, "fi") == 0)
         strncpy (misc->ocr_language, "fin", 5);
      if (strcmp (misc->locale, "fr") == 0)
         strncpy (misc->ocr_language, "fra", 5);
      if (strcmp (misc->locale, "hu") == 0)
         strncpy (misc->ocr_language, "hun", 5);
      if (strcmp (misc->locale, "nl") == 0)
         strncpy (misc->ocr_language, "nld", 5);
      if (strcmp (misc->locale, "nb") == 0)
         strncpy (misc->ocr_language, "nor", 5);
      if (strcmp (misc->locale, "po") == 0)
         strncpy (misc->ocr_language, "pol", 5);
      if (strcmp (misc->locale, "pt") == 0)
         strncpy (misc->ocr_language, "por", 5);
      if (strcmp (misc->locale, "es") == 0)
         strncpy (misc->ocr_language, "spa", 5);
      if (strcmp (misc->locale, "sv") == 0)
         strncpy (misc->ocr_language, "swe", 5);
   } // if
   snprintf (misc->cmd, MAX_CMD - 1,
             "tesseract \"%s\" \"%s/out\" -l %s 2> /dev/null", file,
              misc->tmp_dir, misc->ocr_language);
   switch (system (misc->cmd))
   {
   case 0:
      break;
   default:
      endwin ();
      beep ();
      puts (misc->copyright);
      printf
   ("Be sure the package \"tesseract-ocr\" is installed onto your system.\n");
      printf (gettext
              ("Language code \"%s\" is not a valid tesseract code.\n"),
               misc->ocr_language);
      printf ("%s\n", gettext ("See the tesseract manual for valid codes."));
      printf (gettext
("Be sure the package \"tesseract-ocr-%s\" is installed onto your system.\n"),
  misc->ocr_language);
      fflush (stdout);
      _exit (0);
   } // switch
   strncpy (str, misc->tmp_dir, MAX_STR);
   strncat (str, "/out.txt", MAX_STR);
   pandoc_to_epub (misc, "markdown", str);
} // start_tesseract

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
      go_to_phrase (misc, my_attribute, daisy, ++misc->phrase_nr);
   }
   view_screen (misc, daisy);
   nodelay (misc->screenwin, TRUE);
   wmove (misc->screenwin, daisy[misc->current].y,
          daisy[misc->current].x - 1);

   for (;;)
   {
      switch (wgetch (misc->screenwin))
      {
      case 13: // ENTER
         misc->playing = misc->displaying = misc->current;
         remove (misc->eBook_speaker_wav);
         remove ("old.wav");
         misc->phrase_nr = 0;
         misc->just_this_item = -1;
         misc->current_page_number = daisy[misc->playing].page_number;
         view_screen (misc, daisy);
         kill_player (misc);
         open_text_file (misc, my_attribute,
                 daisy[misc->current].smil_file, daisy[misc->current].anchor);
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
         break_on_EOL (misc, my_attribute, daisy, misc->reader, NULL);
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

         kill_player (misc);
         misc->player_pid = -2;
         mvwprintw (misc->titlewin, 1, 0,
                    "----------------------------------------");
         wprintw (misc->titlewin, "----------------------------------------");
         mvwprintw (misc->titlewin, 1, 0,
                    gettext ("Go to phrase in current item: "));
         echo ();
         wgetnstr (misc->titlewin, pn, 8);
         noecho ();
         view_screen (misc, daisy);
         if (! *pn || atoi (pn) >= daisy[misc->playing].n_phrases)
         {
            beep ();
            pause_resume (misc, my_attribute, daisy);
            pause_resume (misc, my_attribute, daisy);
            break;
         } // if
         go_to_phrase (misc, my_attribute, daisy, atoi (pn));
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
         if (misc->playing != -1)
            pause_resume (misc, my_attribute, daisy);
         help (misc, daisy);
         pause_resume (misc, my_attribute, daisy);
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
                 daisy[misc->current].smil_file, daisy[misc->current].anchor);
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
              daisy[misc->current].smil_file, daisy[misc->current].anchor);
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
         if (misc->playing == -1)
         {
            beep ();
            break;
         } // if
         pause_resume (misc, my_attribute, daisy);
         select_next_output_device (misc, daisy);
         misc->playing = misc->displaying;
         kill_player (misc);
         if (misc->playing != -1)
         {
            misc->phrase_nr--;
            get_next_phrase (misc, my_attribute, daisy);
         } // if
         view_screen (misc, daisy);
         break;
      case 'p':
         put_bookmark (misc);
         break;
      case 'q':
         quit_eBook_speaker (misc);
         return;
      case 'r':
      {
         char str[MAX_STR];
         int i;

         if (misc->scan_flag != 1)
         {
            beep ();
            break;
         } // if
         kill_player (misc);
         xmlTextReaderClose (misc->reader);
         misc->phrase_nr = 0;
         misc->total_phrases = misc->total_pages = *misc->daisy_title = 0;
         misc->playing = misc->just_this_item = -1;
         misc->displaying = misc->current = 0;
         snprintf (misc->cmd, MAX_CMD - 1,
                   "rm -rf \"%s/out*\"", misc->tmp_dir);
         switch (system (misc->cmd))
         {
         default:
            break;
         } // switch
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
         start_tesseract (misc, file);
         daisy = create_daisy_struct (misc, my_attribute);
         for (i = 0; i < misc->total_items; i++)
         {
            *daisy[i].smil_file = *daisy[i].anchor = *daisy[i].my_class = 0;
            *daisy[i].label = daisy[i].page_number = 0;
         } // for
         read_daisy_3 (misc, my_attribute, daisy);
         check_phrases (misc, my_attribute, daisy);
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
         mvwprintw (misc->titlewin, 1, 0, gettext ("Press 'h' for help "));
         wrefresh (misc->titlewin);
         previous_item (misc, daisy);
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
         skip_right (misc);
         break;
      case KEY_LEFT:
      case '4':
         if (misc->playing == -1)
         {
            beep ();
            break;
         } // if
         skip_left (misc, my_attribute, daisy);
         view_screen (misc, daisy);
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
         if (misc->volume <= 0)
         {
            beep ();
            break;
         } // if
         misc->volume -= 0.1;
         if (misc->playing == -1)
            break;
         pause_resume (misc, my_attribute, daisy);
         pause_resume (misc, my_attribute, daisy);
         break;
      case 'V':
      case '7':
         if (misc->volume >= 3)
         {
            beep ();
            break;
         } // if
         misc->volume += 0.1;
         if (misc->playing == -1)
            break;
         pause_resume (misc, my_attribute, daisy);
         pause_resume (misc, my_attribute, daisy);
         break;
      case 'x':
         kill_player (misc);
         put_bookmark (misc);
         misc->playing = misc->just_this_item = -1;
         strncpy (file, get_input_file (misc, file), MAX_STR - 1);
         switch (chdir (misc->tmp_dir))
         {
         case 0:
            snprintf (misc->cmd, MAX_CMD - 1, "rm -rf *");
            switch (system (misc->cmd))
            {
            case 0:
               break;
            default:
               failure (misc->tmp_dir, errno);
            } // switch
         default:
            break;
         } // switch
         create_epub (misc, my_attribute, daisy, file, 1);
         play_epub (misc, my_attribute, daisy, file);
         break;
      default:
         beep ();
         break;
      } // switch

      if (misc->playing != -1)
      {
         if (get_next_phrase (misc, my_attribute, daisy) == -1)
         {
            misc->phrase_nr = 0;
            misc->playing++;
            if (daisy[misc->playing].level <= misc->level)
               misc->current = misc->displaying = misc->playing;
            if (misc->just_this_item != -1)
               if (daisy[misc->playing].level <= misc->level)
                  misc->playing = misc->just_this_item = -1;
            if (misc->playing > -1)
            {
               remove (misc->eBook_speaker_wav);
               remove ("old.wav");
               open_text_file (misc, my_attribute,
                 daisy[misc->playing].smil_file, daisy[misc->playing].anchor);
            } // if
            view_screen (misc, daisy);
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
         ("Be sure the package \"unar\" is installed onto your system.");
      failure (gettext ("eBook-speaker cannot handle this file."), errno);
   } // switch
} // extract_epub

void lowriter_to_txt (char *file)
{
   char cmd[MAX_CMD];

   snprintf (cmd, MAX_CMD - 1,
            "cp \"%s\" out.doc && \
             lowriter --headless --convert-to txt out.doc > /dev/null", file);
   switch (system (cmd))
   {
   case 0:
      return;
   default:
      endwin ();
      printf
("Be sure the package \"libreoffice-writer\" is installed onto your system.");
      failure (gettext ("eBook-speaker cannot handle this file."), errno);
   } // switch
} // lowriter_to_txt

void pandoc_to_epub (misc_t *misc, char *from, char *file)
{
   char str[MAX_STR + 1];

   strncpy (str, misc->tmp_dir, MAX_STR);
   strncat (str, "/out.epub", MAX_STR);
   snprintf (misc->cmd, MAX_CMD,
             "pandoc -s \"%s\" -f %s -t epub -o %s 2> /dev/null",
             file, from, str);
   switch (system (misc->cmd))
   {
   case 0:
      break;
   default:
      endwin ();
      printf
           ("Be sure the package \"pandoc\" is installed onto your system.");
      failure (gettext ("eBook-speaker cannot handle this file."), errno);
   } // switch
   if (access (str, R_OK) != 0)
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, "%s: %s\n", file, gettext ("Unknown format"));
      failure (str, e);
   } // if
   extract_epub (misc, str);
} // pandoc_to_epub            

void pdf_to_txt (const char *file)
{
   char cmd[MAX_CMD];

   snprintf (cmd, MAX_CMD - 1, "pdftotext -q \"%s\" out.txt", file);
   switch (system (cmd))
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
     ("Be sure the package \"poppler-utils\" is installed onto your system.");
      failure (gettext ("eBook-speaker cannot handle this file."), e);
   }
   } // switch
} // pdf_to_txt

void check_phrases (misc_t *misc, my_attribute_t *my_attribute,
                    daisy_t *daisy)
{
   int i;

   for (i = 0; i < misc->total_items; i++)
   {
      strncpy (daisy[i].smil_file, daisy[i].orig_smil, MAX_STR - 1);
      split_phrases (misc, my_attribute, daisy, i);
      if (! *daisy[i].label)
         snprintf (daisy[i].label, 10, "%d", i + 1);
   } // for

   count_phrases (misc, my_attribute, daisy);

// remove item with 0 phrases
   int j = 0;

   for (i = 0; i < misc->total_items - 1; i++)
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
         strncpy (daisy[j].orig_smil, daisy[j + 1].orig_smil, MAX_STR - 1);
         strncpy (daisy[j].smil_file, daisy[j + 1].smil_file, MAX_STR - 1);
         strncpy (daisy[j].anchor, daisy[j + 1].anchor, MAX_STR - 1);
         strncpy (daisy[j].my_class, daisy[j + 1].my_class, MAX_STR - 1);
         strncpy (daisy[j].label, daisy[j + 1].label, 80);
         if (*daisy[j].label == 0)
            sprintf (daisy[j].label, "%d", j + 1);
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
   } // for
} // check_phrases

void store_as_WAV_file (misc_t *misc, my_attribute_t *my_attribute,
                        daisy_t *daisy)
{
   char str[MAX_STR], s2[MAX_STR];
   int i, pause_flag;
   struct passwd *pw;

   pause_flag = misc->playing;
   wclear (misc->screenwin);
   snprintf (str, MAX_STR - 1, "%s", daisy[misc->current].label);
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
   xmlTextReaderPtr local_reader;
   htmlDocPtr local_doc;

   if ((w = fopen (outfile, "a")) == NULL)
   {
      int e;

      e = errno;
      beep ();
      failure (outfile, e);
   } // if
   local_doc = NULL;
   local_reader = NULL;
   open_text_file (misc, my_attribute,
                 daisy[misc->current].smil_file, daisy[misc->current].anchor);
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
   xmlTextReaderClose (local_reader);
   xmlFreeDoc (local_doc);
} // write_ascii

void store_as_ASCII_file (misc_t *misc, my_attribute_t *my_attribute,
                       daisy_t *daisy)
{
   char str[MAX_STR], s2[MAX_STR];
   int i, pause_flag;
   struct passwd *pw;;

   pause_flag = misc->playing;
   wclear (misc->screenwin);
   snprintf (str, MAX_STR - 1, "%s", daisy[misc->current].label);
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
      failure (s2, e);
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
   puts ("(C)2003-2016 J. Lemmens");
   printf ("\n");
   printf (gettext
           ("Usage: %s [eBook_file | -s] [-o language-code] "
            "[-d ALSA_sound_device] [-h] [-i] [-t TTS_command] [-b n | y]"),
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
           ("Be sure the package \"calibre\" is installed onto your system.");
      failure (gettext ("eBook-speaker cannot handle this file."), errno);
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
      failure (file, errno);
   strncpy (str, misc->tmp_dir, MAX_STR);
   strncat (str, "/", MAX_STR);
   strncat (str, basename (file), MAX_STR);
   strncat (str, ".xml", MAX_STR);
   if (! (w = fopen (str, "w")))
      failure (str, errno);
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

void remove_double_tts_entries (misc_t *misc)
{
   int x, y;
   int entry_already_exists_in_misc, tts_no = 0;
   char temp[10][MAX_STR];

   for (x = 0; x < 10; x++)
      strncpy (temp[x], misc->tts[x], MAX_STR - 1);
   bzero (misc->tts, sizeof (misc->tts));
   for (x = 0; x < 10; x++)
   {
      if (! *temp[x])
         break;
      entry_already_exists_in_misc = 0;
      for (y = 0; y < 10; y++)
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

void remove_tmp_dir (misc_t *misc)
{
   snprintf (misc->cmd, MAX_CMD - 1, "rm -rf %s", misc->tmp_dir);
   if (system (misc->cmd) > 0)
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, "%s: %s\n", misc->cmd, strerror (e));
      failure (str, e);
   } // if  
} // remove_tmp_dir

void make_tmp_dir (misc_t *misc)
{
   misc->tmp_dir = strdup ("/tmp/eBook-speaker.XXXXXX");
   if (! mkdtemp (misc->tmp_dir))
   {
      int e;

      e = errno;
      beep ();
      failure (misc->tmp_dir, e);
   } // if
   switch (chdir (misc->tmp_dir))
   {
   default:
      break;
   } // switch
} // make_tmp_dir

void play_epub (misc_t *misc, my_attribute_t *my_attribute,
                daisy_t *daisy, char *file)
{
   int i;

   daisy = create_daisy_struct (misc, my_attribute);

// first be sure all strings in daisy_t are cleared
   for (i = 0; i < misc->total_items; i++)
   {
      *daisy[i].smil_file = *daisy[i].anchor = *daisy[i].my_class = 0;
      *daisy[i].label = daisy[i].page_number = 0;
   } // for

   read_daisy_3 (misc, my_attribute, daisy);
   check_phrases (misc, my_attribute, daisy);
   misc->reader = NULL;
   misc->just_this_item = misc->playing = -1;
   if (! *misc->daisy_title)
      strncpy (misc->daisy_title, basename (file), MAX_STR - 1);
   if (! *misc->bookmark_title)
      strncpy (misc->bookmark_title, basename (file), MAX_STR - 1);
   kill_player (misc);
   remove (misc->eBook_speaker_wav);
   remove ("old.wav");
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
   mvwprintw (misc->titlewin, 1, 0, gettext ("Press 'h' for help "));
   wrefresh (misc->titlewin);
   browse (misc, my_attribute, daisy, file);
   remove_tmp_dir (misc);
} // play_epub

void create_epub (misc_t *misc, my_attribute_t *my_attribute,
                  daisy_t *daisy, char *file, int print)
{
   magic_t myt;

// determine filetype
   myt = magic_open (MAGIC_FLAGS);
   if (magic_load (myt, NULL) != 0)
      failure ("magic_load", errno);
   if (magic_file (myt, file) == NULL)
   {
      int e;

      e = errno;
      endwin ();
      beep ();
      printf ("%s: %s\n", file, strerror (e));
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
   if (strcasestr (magic_file (myt, file), "gzip compressed data"))
   {
      char cmd[512], str[MAX_STR];
      FILE *p;

      snprintf (cmd, MAX_CMD - 1,
        "unar \"%s\" -f -o \"%s\" > /dev/null && ls -1", file, misc->tmp_dir);
      p = popen (cmd, "r");
      switch (*fgets (str, MAX_STR, p))
      {
      default:
         break;
      } // switch
      pclose (p);
      snprintf (file, MAX_STR, "%s", str);
      file[strlen (file) - 1] = 0;
      magic_close (myt);
      create_epub (misc, my_attribute, daisy, file, 0);
      return;
   } // "gzip compressed data"
   else
   if (strcasestr (magic_file (myt, file), "troff or preprocessor input"))
// maybe a man-page
   {
      char cmd[512];

      snprintf (cmd, 500, "man2html \"%s\" > out.html", file);
      switch (system (cmd))
      {
      case 0:
         break;
      default:
         endwin ();
         printf
       ("Be sure the package \"man2html\" is installed onto your system.");
         failure (gettext ("eBook-speaker cannot handle this file."), errno);
      } // switch
      magic_close (myt);
      create_epub (misc, my_attribute, daisy, "out.html", 0);
      return;
   } // "troff or preprocessor input"
   else
   if ((strcasestr (magic_file (myt, file), "Zip archive data") ||
        strcasestr (magic_file (myt, file), "EPUB")) &&
         ! strcasestr (magic_file (myt, file), "Microsoft Word 2007+"))
   {
      extract_epub (misc, file);
   } // if epub
   else
   if (strcasestr (magic_file (myt, file), "HTML document") ||
       strcasestr (magic_file (myt, file), "XML document text"))
   {
      pandoc_to_epub (misc, "html", file);
   } // if
   else
   if (strcasestr (magic_file (myt, file), "PDF document"))
   {
      pdf_to_txt (file);
      create_epub (misc, my_attribute, daisy, "out.txt", 0);
      return;
   }
   else
   if (strcasecmp (magic_file (myt, file), "data") == 0)
   {
      char *str;                                       

      str = ascii2html (misc, file);
      pandoc_to_epub (misc, "html", str);
   }
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
       ("Be sure the package \"ghostscript\" is installed onto your system.");
         failure (gettext ("eBook-speaker cannot handle this file."), errno);
      } // switch
   } // if
   else
   if (strcasestr (magic_file (myt, file), "C source text") ||
       strcasestr (magic_file (myt, file), "ASCII text") ||
       strcasestr (magic_file (myt, file), "GNU gettext message catalogue") ||
       strcasestr (magic_file (myt, file), "shell script text") ||
       strcasestr (magic_file (myt, file), "Python script") ||
       strcasestr (magic_file (myt, file), "Sendmail") ||
       strcasestr (magic_file (myt, file), "UTF-8 Unicode"))
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
       strcasestr (magic_file (myt, file), "Composite Document File") ||
       strcasestr (magic_file (myt, file), "Microsoft Word 2007+") ||
       strcasestr (magic_file (myt, file), "Microsoft Office Document") ||
       strcasestr (magic_file (myt, file), "OpenDocument") ||
       strcasestr (magic_file (myt, file), "Rich Text Format"))
   {
      lowriter_to_txt (file);
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
       strcasestr (magic_file (myt, file), "GIF image")  ||
       strcasestr (magic_file (myt, file), "PNG image")  ||
       strcasestr (magic_file (myt, file), "Netpbm"))
   {
      misc->ignore_bookmark = 1;
      start_tesseract (misc, file);
   } // if
   else
   if (strcasestr (magic_file (myt, file), "directory"))
   {
      snprintf (file, MAX_STR - 1, "%s",
                get_input_file (misc, file));
      switch (chdir (misc->tmp_dir))
      {
      case 0:
         snprintf (misc->cmd, MAX_CMD - 1, "rm -rf *");
         switch (system (misc->cmd))
         {
         case 0:
            break;
         default:
            failure (misc->tmp_dir, errno);
         } // switch
      default:
         break;
      } // switch
      create_epub (misc, my_attribute, daisy, file, 1);
      switch (chdir (misc->tmp_dir))
      {
      default:
         break;
      } // switch
      play_epub (misc, my_attribute, daisy, file);
      quit_eBook_speaker (misc);
      _exit (0);
   } // if directory
   magic_close (myt);
} // create_epub

int main (int argc, char *argv[])
{
   int opt;
   char str[MAX_STR];
   misc_t misc;
   my_attribute_t my_attribute;
   daisy_t *daisy;

   daisy = NULL;
   fclose (stderr);
   setbuf (stdout, NULL);
   if (setlocale (LC_ALL, "") == NULL)
      snprintf (misc.locale, 3, "en_US.UTF-8");
   else
      snprintf (misc.locale, 3, "%s", setlocale (LC_ALL, ""));
   setlocale (LC_NUMERIC, "C");
   textdomain (PACKAGE);
   snprintf (str, MAX_STR, "%s/", LOCALEDIR);
   bindtextdomain (PACKAGE, str);
   strncpy (misc.cmd, argv[0], MAX_CMD);
   strncpy (misc.src_dir, get_current_dir_name (), MAX_STR);
   make_tmp_dir (&misc);
   initscr ();
   misc.titlewin = newwin (2, 80,  0, 0);
   misc.screenwin = newwin (23, 80, 2, 0);
   misc.ignore_bookmark = 0;
   misc.break_on_EOL = 'n';
   misc.level = 1;
   misc.show_hidden_files = 0;
   getmaxyx (misc.screenwin, misc.max_y, misc.max_x);
   misc.max_y -= 2;
   keypad (misc.screenwin, TRUE);
   meta (misc.screenwin,       TRUE);
   nonl ();
   noecho ();
   misc.eBook_speaker_pid = getpid ();
   misc.player_pid = -2;
   snprintf (misc.copyright, MAX_STR - 1, "%s %s - (C)2016 J. Lemmens",
             gettext ("eBook-speaker - Version"), PACKAGE_VERSION);
   wattron (misc.titlewin, A_BOLD);
   wprintw (misc.titlewin, "%s - %s", misc.copyright,
            gettext ("Please wait..."));
   wattroff (misc.titlewin, A_BOLD);

   misc.speed = misc.volume = 1.0;
   signal (SIGCHLD, player_ended);
   strncpy (misc.sound_dev, "hw:0", MAX_STR - 1);
   *misc.ocr_language = 0;
   *misc.standalone = 0;
   *misc.xmlversion = 0;
   *misc.encoding = 0;
   read_xml (&misc, &my_attribute);
   remove_double_tts_entries (&misc);
   snprintf (misc.eBook_speaker_txt, MAX_STR,
             "%s/eBook-speaker.txt", misc.tmp_dir);
   snprintf (misc.eBook_speaker_wav, MAX_STR,
             "%s/eBook-speaker.wav", misc.tmp_dir);
   misc.option_b = misc.option_t = 0;
   opterr = 0;
   while ((opt = getopt (argc, argv, "b:d:hilo:st:")) != -1)
   {
      switch (opt)
      {
      case 'b':
         misc.break_on_EOL = optarg[0];
         misc.option_b = 1;
         break;
      case 'd':
         strncpy (misc.sound_dev, optarg, 15);
         break;
      case 'h':
         usage ();
      case 'i':
         misc.ignore_bookmark = 1;
         break;
      case 'l':
         continue;
      case 'o':
         strncpy (misc.ocr_language, optarg, 3);
         break;
      case 's':
         misc.ignore_bookmark = 1;
         wrefresh (misc.titlewin);
         misc.scan_flag = 1;
         snprintf (misc.orig_file, MAX_STR, "%s/out.pgm", misc.tmp_dir);
         snprintf (misc.cmd, MAX_CMD,
             "scanimage --resolution 300 --mode Gray > %s", misc.orig_file);
         switch (system (misc.cmd))
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
         break;
      case 't':
         misc.option_t = 1;
         misc.tts_no = 0;
         strncpy (misc.tts[misc.tts_no], optarg, MAX_STR - 1);
         remove_double_tts_entries (&misc);
         break;
      } // switch
   } // while

   *misc.search_str = 0;
   if (misc.scan_flag == 1)
   {
      create_epub (&misc, &my_attribute, daisy, misc.orig_file, 1);
      strncpy (misc.daisy_title, "scanned image", MAX_STR - 1);
      play_epub (&misc, &my_attribute, daisy, misc.orig_file);
      return 0;
   } // if

   if (! argv[optind])
   {
// if no arguments are given
      snprintf (misc.orig_file, MAX_STR - 1, "%s",
                get_input_file (&misc, misc.src_dir));
      switch (chdir (misc.tmp_dir))
      {
      case 0:
         snprintf (misc.cmd, MAX_CMD - 1, "rm -rf *");
         switch (system (misc.cmd))
         {
         case 0:
            break;
         default:
            failure (misc.tmp_dir, errno);
         } // switch
      default:
         break;
      } // switch         
      create_epub (&misc, &my_attribute, daisy, misc.orig_file, 1);
      play_epub (&misc, &my_attribute, daisy, misc.orig_file);
      quit_eBook_speaker (&misc);
      return 0;
   } // if

   if (*argv[optind] == '/')
   {
// arg start with a slash
      strncpy (misc.orig_file, argv[optind], MAX_STR - 1);
   }
   else
   {
      strncpy (misc.orig_file, misc.src_dir, MAX_STR);
      strncat (misc.orig_file, "/", MAX_STR);
      strncat (misc.orig_file, argv[optind], MAX_STR);
   } // if

   if (access (misc.orig_file, R_OK) == -1)
   {
      int e;

      e = errno;
      endwin ();
      puts (misc.copyright);
      beep ();
      failure (argv[optind], e);
   } // if

   misc.current = 0;
   create_epub (&misc, &my_attribute, daisy, misc.orig_file, 1);
   play_epub (&misc, &my_attribute, daisy, misc.orig_file);
   return 0;
} // main
