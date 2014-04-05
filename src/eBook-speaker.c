/* eBook-speaker - read aloud an eBook using a speech synthesizer
 *  Copyright (C) 2014 J. Lemmens
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

#include "src/daisy.h"
#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_URL
#undef PACKAGE_VERSION
#undef VERSION
#include "config.h"

daisy_t daisy[2000];
misc_t misc;
my_attribute_t my_attribute;

void get_tag ();
void get_page_number ();
void view_screen ();
void play_now ();
void pause_resume ();
void help ();
void previous_item ();
void next_item ();
void check_phrases ();
void skip_right ();
void kill_player ();
void go_to_page_number ();
void select_next_output_device ();
void pandoc_to_epub (char *);
char *sort_by_playorder ();
void read_out_eBook (const char *);
const char *read_eBook (char *);
void get_eBook_struct (int);
void parse_smil ();
void start_element (void *, const char *, const char **);
void end_element (void *, const char *);
void get_clips (char *, char *);
void save_xml ();
void parse_opf (char *);
void select_tts ();
void playfile (char *, char *, char *, char *, float);
void read_daisy_3 (char *);
int get_tag_or_label (xmlTextReaderPtr);
void save_bookmark_and_xml ();
xmlTextReaderPtr open_text_file (xmlTextReaderPtr, xmlDocPtr, char *, char *);
char *get_input_file (char *);
void put_bookmark ();
xmlTextReaderPtr skip_left (xmlTextReaderPtr, xmlDocPtr);
void store_to_disk (xmlTextReaderPtr, xmlDocPtr);

void quit_eBook_speaker ()
{
   save_bookmark_and_xml ();
   puts ("");
   _exit (0);
} // quit_eBook_speaker

int get_next_phrase (xmlTextReaderPtr reader, xmlDocPtr doc, int pl)
{
   FILE *w;

   if (access ("eBook-speaker.wav", R_OK) == 0)
   // when still playing
      return 0;
   while (1)
   {
      if (! get_tag_or_label (reader))
      {
         if (pl >= misc.total_items - 1)
         {
            char str[MAX_STR];
            struct passwd *pw;;

            save_bookmark_and_xml ();
            pw = getpwuid (geteuid ());
            snprintf (str, MAX_STR - 1, "%s/.eBook-speaker/%s",
                      pw->pw_dir, misc.bookmark_title);
            unlink (str);
            _exit (0);
         } // if
         return -1;
      } // if
      if (*daisy[pl + 1].anchor &&
          strcasecmp (my_attribute.id, daisy[pl + 1].anchor) == 0)
         return -1;
      if (*misc.label)
         break;
   } // while

   if (daisy[pl].screen == daisy[misc.current].screen)
   {
      wattron (misc.screenwin, A_BOLD);
      mvwprintw (misc.screenwin, daisy[pl].y, 69, "%5d %5d",
               misc.phrase_nr + 1, daisy[pl].n_phrases - misc.phrase_nr - 1);
      (misc.phrase_nr)++;
      wattroff (misc.screenwin, A_BOLD);
      mvwprintw (misc.screenwin, 22, 0, "%s", misc.label);
      wclrtoeol (misc.screenwin);
      wmove (misc.screenwin, daisy[misc.displaying].y, daisy[misc.displaying].x);
      wrefresh (misc.screenwin);
   } // if
   if ((w = fopen ("eBook-speaker.txt", "w")) == NULL)
   {
      endwin ();
      beep ();
      printf ("Can't make a temp file %s\n", "eBook-speaker.txt");
      fflush (stdout);
      _exit (0);
   } // if
   if (fwrite (misc.label, strlen (misc.label), 1, w) == 0)
   {
      endwin ();
      beep ();
      printf ("write (\"%s\"): failed.\n", misc.label);
      fflush (stdout);
      kill (getppid (), SIGINT);
   } // if
   fclose (w);

   if (! *misc.tts[misc.tts_no])
   {
      misc.tts_no = 0;
      select_tts ();
   } // if
   switch (system (misc.tts[misc.tts_no]))
   {
   default:
      break;
   } // switch

   switch (misc.player_pid = fork ())
   {
   case -1:
   {
      int e;

      e = errno;
      endwin ();
      beep ();
      reader = skip_left (reader, doc);
      put_bookmark ();
      printf ("%s\n", strerror (e));
      fflush (stdout);
      _exit (1);
   }
   case 0:
      setsid ();
      playfile ("eBook-speaker.wav", "wav", misc.sound_dev, "alsa", misc.speed);
      _exit (0);
   default:
      return 0;
   } // switch
} // get_next_phrase
                                            
void select_tts ()
{
   int n, y, x = 2;

   wclear (misc.screenwin);
   wprintw (misc.screenwin, gettext ("\nSelect a Text-To-Speech application\n\n"));
   for (n = 0; n < 10; n++)
   {
      char str[MAX_STR];

      if (*misc.tts[n] == 0)
         break;
      strncpy (str, misc.tts[n], MAX_STR - 1);
      str[72] = 0;
      wprintw (misc.screenwin, "    %d %s\n", n, str);
   } // for
   wprintw (misc.screenwin, gettext (
"\nProvide a new TTS.\n\
Be sure that the new TTS reads its information from the file\n\
eBook-speaker.txt and that it writes to the file eBook-speaker.wav.\n\n\
Press DEL to delete a TTS\n\n"));
   wprintw (misc.screenwin,
           "    -------------------------------------------------------------");
   y = misc.tts_no + 3;
   nodelay (misc.screenwin, FALSE);
   for (;;)
   {
      wmove (misc.screenwin, y, x);
      switch (wgetch (misc.screenwin))
      {
      int i;

      case 13:
         misc.tts_no = y - 3;
         view_screen ();
         nodelay (misc.screenwin, TRUE);
         return;
      case KEY_DOWN:
         if (++y == n + 3)
         {
            y = n + 10;
            x = 4;
            wmove (misc.screenwin, y, x);
            nodelay (misc.screenwin, FALSE);
            echo ();
            misc.tts_no = n;
            if (misc.tts_no > 9)
               misc.tts_no = 9;
            wgetnstr (misc.screenwin, misc.tts[misc.tts_no], MAX_STR - 1);
            noecho ();
            if (*misc.tts[misc.tts_no])
            {
               view_screen ();
               nodelay (misc.screenwin, TRUE);
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
            wmove (misc.screenwin, y, x);
            nodelay (misc.screenwin, FALSE);
            echo ();
            misc.tts_no = n;
            if (misc.tts_no > 9)
               misc.tts_no = 9;
            wgetnstr (misc.screenwin, misc.tts[misc.tts_no], MAX_STR - 1);
            noecho ();
            if (*misc.tts[misc.tts_no])
            {
               view_screen ();
               nodelay (misc.screenwin, TRUE);
               return;
            } // if
            y = 3;
            x = 2;
         } // if
         break;
      case KEY_DC:
         for (i = y - 3; i < 10; i++)
            strncpy (misc.tts[i], misc.tts[i + 1], MAX_STR - 1);
         misc.tts_no = 0;
         view_screen ();
         return;
      default:
         if (x == 2)
         {
            view_screen ();
            nodelay (misc.screenwin, TRUE);
            return;
         } // if
      } // switch
   } // for
} // select_tts

int count_phrases (char *f_file, char *f_anchor,
                   char *t_file, char *t_anchor)
{
   int n_phrases;
   xmlTextReaderPtr r;
   xmlDocPtr d;

   if (! *f_file)
      return 0;
   d = xmlRecoverFile (f_file);
   if (! (r = xmlReaderWalker (d)))
   {
      int e;

      e = errno;
      endwin ();
      printf ("%s\n", strerror (e));
      beep ();
      fflush (stdout);
      _exit (1);
   } // if
   n_phrases = 0;
   if (*f_anchor)
   {
// if there is a "from" anchor, look for it
      do
      {
         if (! get_tag_or_label (r))
         {
// if f_anchor couldn't be found, start from the beginning
            xmlTextReaderClose (r);
            xmlFreeDoc (d);
            d = xmlRecoverFile (f_file);
            r = xmlReaderWalker (d);
            break;
         } // if
      } while (strcasecmp (my_attribute.id, f_anchor) != 0);
   } // if

// start counting
   while (1)
   {
      if (! get_tag_or_label (r))
      {
         xmlTextReaderClose (r);
         xmlFreeDoc (d);
         return n_phrases + 1;
      } // if
      if (*misc.label)
      {
         n_phrases++;
         continue;
      } // if
      if (*t_anchor)
      {
         if  (strcasecmp (f_file, t_file) == 0)
         {
            if (strcasecmp (my_attribute.id, t_anchor) == 0)
            {
               xmlTextReaderClose (r);
               xmlFreeDoc (d);
               return n_phrases;
            } // if
         } // if
      } // if
   } // while
} // count_phrases

char *re_organize (char *fname)
{
   xmlTextReaderPtr local_reader;
   static char tmp[MAX_STR];
   char *p, text[max_phrase_len];
   xmlDocPtr doc;
   FILE *w;
   int phrases;

   doc = xmlRecoverFile (fname);
   if (! (local_reader = xmlReaderWalker (doc)))
   {
      int e;

      e = errno;
      endwin ();
      printf ("%s: %s\n", fname, strerror (e));
      beep ();
      fflush (stdout);
      _exit (1);
   } // if
   snprintf (tmp, MAX_STR - 1, "%s/%s.tmp.xhtml", misc.tmp_dir, basename (fname));
   if (! (w = fopen (tmp, "w")))
   {
      int e;

      e = errno;
      endwin ();
      printf ("%s\n", strerror (e));
      beep ();
      fflush (stdout);
      _exit (1);
   } /* if */
   fprintf (w, "<html>\n<head>\n</head>\n<body>\n");

   while (1)
   {
// discard header
      if (! get_tag_or_label (local_reader))
         break;
      if (! strcasecmp (misc.tag, "/head"))
         break;
   } // while

   while (1)
   {
      if (! get_tag_or_label (local_reader))
         break;
      if (strcasecmp (misc.tag, "a") == 0)
         continue;
      if (strcasecmp (misc.tag, "/a") == 0)
         continue;
      if (strcasecmp (misc.tag, "h1") == 0 ||
          strcasecmp (misc.tag, "h2") == 0 ||
          strcasecmp (misc.tag, "h3") == 0 ||
          strcasecmp (misc.tag, "h4") == 0 ||
          strcasecmp (misc.tag, "h5") == 0 ||
          strcasecmp (misc.tag, "h6") == 0)
      {
         fprintf (w, "<%s id=\"%s\">\n<br />\n", misc.tag, my_attribute.id);
         continue;
      } // if
      if (strcasecmp (misc.tag, "/h1") == 0 ||
          strcasecmp (misc.tag, "/h2") == 0 ||
          strcasecmp (misc.tag, "/h3") == 0 ||
          strcasecmp (misc.tag, "/h4") == 0 ||
          strcasecmp (misc.tag, "/h5") == 0 ||
          strcasecmp (misc.tag, "/h6") == 0)
      {
         fprintf (w, "<%s>\n<br />\n", misc.tag);
         continue;
      } // if
      if (! *misc.label)
         continue;
      p = misc.label;
      *text = 0;
      phrases = 0;
      while (1)
      {
         if (*p == 0)
         {
            char *t;

            t = text;
            while (1)
            {
               if (*t == ' ')
                  t++;
               if (*t != ' ')
                  break;
            } // while
            if (*t)
            {
               fprintf (w, "%s\n<br />\n", t);
               phrases++;
            } // if
            *text = 0;
            break;
         } // if
         if (*p == '\"' || *p == '\'' || *p == '`')
         {
            strncat (text, " ", 1);
            p++;
            continue;
         } // if
         if (*p == '&')
         {
            strncat (text, " ampersand ", 11);
            p++;
            continue;
         } // if
         if (*p == '.' || *p == ',' || *p == '!' || *p == '?' ||
             *p == ':' || *p == ';' || *p == '-' || *p == '*')
         {
         // split point; start here a new phrase
            strncat (text, p++, 1);
            if (strlen (text) > 1)
            {
               fprintf (w, "%s\n", text);
               phrases++;
            } // if
            fprintf (w, "<br />\n");
            *text = 0;
            continue;
         } // if
         strncat (text, p++, 1);
      } // while
   } // while
   fprintf (w, "</body>\n</html>\n");
   xmlTextReaderClose (local_reader);
   xmlFreeDoc (doc);
   fclose (w);
   return tmp;
} // re_organize

void playfile (char *in_file, char *in_type,
               char *out_file, char *out_type, float sp)
{
   sox_format_t *in, *out; /* input and output files */
   sox_effects_chain_t *chain;
   sox_effect_t *e;
   char *args[MAX_STR], str[MAX_STR];

   sox_globals.verbosity = 0;
   sox_init ();
   if (! (in = sox_open_read (in_file, NULL, NULL, in_type)))
   {
      int e;

      e = errno;
      endwin ();
      printf ("sox_open_read: %s: %s\n", in_file, strerror (e));
      fflush (stdout);
      beep ();
      kill (getppid (), SIGQUIT);
   } // if
   if ((out = sox_open_write (out_file, &in->signal,
           NULL, out_type, NULL, NULL)) == NULL)
   {
      endwin ();
      printf ("%s: %s\n", out_file, gettext (strerror (EINVAL)));
      strncpy (misc.sound_dev, "hw:0", 5);
      save_xml ();
      beep ();
      fflush (stdout);
      kill (getppid (), SIGQUIT);
   } // if

   chain = sox_create_effects_chain (&in->encoding, &out->encoding);

   e = sox_create_effect (sox_find_effect ("input"));
   args[0] = (char *) in, sox_effect_options (e, 1, args);
   sox_add_effect (chain, e, &in->signal, &in->signal);

   e = sox_create_effect (sox_find_effect ("tempo"));
   snprintf (str, MAX_STR, "%lf", sp);
   args[0] = "-s";
   args[1] = str;
   sox_effect_options (e, 2, args);
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
   unlink ("eBook-speaker.wav");
} // playfile

void put_bookmark ()
{
   char str[MAX_STR];
   xmlTextWriterPtr writer;
   struct passwd *pw;;

   pw = getpwuid (geteuid ());
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker", pw->pw_dir);
   mkdir (str, 0755);
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker/%s",
             pw->pw_dir, misc.bookmark_title);
   if (! (writer = xmlNewTextWriterFilename (str, 0)))
      return;
   xmlTextWriterStartDocument (writer, NULL, NULL, NULL);
   xmlTextWriterStartElement (writer, BAD_CAST "bookmark");
   if (misc.playing < 0)
      xmlTextWriterWriteFormatAttribute
         (writer, BAD_CAST "item", "%d", misc.current);
   else                                               
      xmlTextWriterWriteFormatAttribute
         (writer, BAD_CAST "item", "%d", misc.playing);
   xmlTextWriterWriteFormatAttribute
      (writer, BAD_CAST "phrase", "%d", --misc.phrase_nr);
   xmlTextWriterWriteFormatAttribute (writer, BAD_CAST "level", "%d", misc.level);
   xmlTextWriterWriteFormatAttribute (writer, BAD_CAST "tts", "%d", misc.tts_no);
   xmlTextWriterWriteFormatAttribute (writer, BAD_CAST "speed", "%f", misc.speed);
   xmlTextWriterEndElement (writer);
   xmlTextWriterEndDocument (writer);
   xmlFreeTextWriter (writer);
} // put_bookmark

xmlTextReaderPtr get_bookmark (xmlTextReaderPtr reader, xmlDocPtr doc)
{
   char str[MAX_STR];
   xmlTextReaderPtr local_reader;
   xmlDocPtr local_doc;
   int start;
   struct passwd *pw;;

   pw = getpwuid (geteuid ());
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker/%s",
             pw->pw_dir, misc.bookmark_title);
   local_doc = xmlRecoverFile (str);
   if (! (local_reader = xmlReaderWalker (local_doc)))
   {
      xmlFreeDoc (local_doc);
      return NULL;
   } // if
   do
   {
      if (! get_tag_or_label (local_reader))
         break;
   } while (strcasecmp (misc.tag, "bookmark") != 0);
   xmlTextReaderClose (local_reader);
   xmlFreeDoc (local_doc);
   if (misc.current < 0 || misc.current == misc.total_items)
      misc.current = 0;
   if (! *daisy[misc.current].smil_file)
      return NULL;
   reader = open_text_file (reader, doc,
                            daisy[misc.current].smil_file, daisy[misc.current].anchor);
   if (misc.phrase_nr < 0)
      misc.phrase_nr = 0;
   start = misc.phrase_nr;
   misc.phrase_nr = 0;
   if (misc.level < 1 || misc.level > misc.depth)
      misc.level = 1;
   misc.displaying = misc.playing = misc.current;
   misc.just_this_item = -1;
   if (misc.tts_flag > -1)
      misc.tts_no = misc.tts_flag;
   while (1)
   {
      if (! get_tag_or_label (reader))
         break;
      if (! *misc.label)
         continue;
      misc.phrase_nr++;
      if (misc.phrase_nr == start)
         break;
   } // while
   return reader;               
} // get_bookmark

void get_page_number (char *NCX)
{
   xmlTextReaderPtr fd;
   xmlDocPtr doc;

   doc = xmlRecoverFile (NCX);
   fd = xmlReaderWalker (doc);
   do
   {
      if (! get_tag_or_label (fd))
         break;
   } while (atoi (my_attribute.playorder) != misc.current);
   do
   {
      if (! get_tag_or_label (fd))
         break;
   } while (! *misc.label);
   misc.current_page_number = atoi (misc.label);
   xmlTextReaderClose (fd);
   xmlFreeDoc (doc);
} // get_page_number

void view_screen ()
{
   int i, x, l;

   mvwaddstr (misc.titlewin, 1, 0, "----------------------------------------");
   waddstr (misc.titlewin, "----------------------------------------");
   mvwprintw (misc.titlewin, 1, 0, gettext ("'h' for help "));
   if (! misc.discinfo)
   {
      if (misc.total_pages)
         mvwprintw (misc.titlewin, 1, 15, gettext (" %d pages "), misc.total_pages);
      mvwprintw (misc.titlewin, 1, 28,
         gettext (" level: %d of %d "), misc.level, misc.depth);
      mvwprintw (misc.titlewin, 1, 47, gettext (" total phrases: %d "),
                 misc.total_phrases);
      mvwprintw (misc.titlewin, 1, 74, " %d/%d ",
              misc.current / misc.max_y + 1, (misc.total_items - 1) / misc.max_y + 1);
   } // if
   wrefresh (misc.titlewin);

   wclear (misc.screenwin);
   for (i = 0; daisy[i].screen != daisy[misc.current].screen; i++);
   do
   {
      mvwprintw (misc.screenwin, daisy[i].y, daisy[i].x + 1,
                            daisy[i].label);
      l = strlen (daisy[i].label);
      if (l / 2 * 2 == l)
         waddstr (misc.screenwin, " ");
      for (x = l; x < 61; x += 2)
         waddstr (misc.screenwin, " .");
      if (daisy[i].page_number)
         mvwprintw (misc.screenwin, daisy[i].y, 65,
                    "(%3d)", daisy[i].page_number);
      l = daisy[i].n_phrases;
      x = i + 1;
      while (daisy[x].level > misc.level)
         l += daisy[x++].n_phrases;
      if (daisy[i].level <= misc.level)
         mvwprintw (misc.screenwin, daisy[i].y, 75, "%5d", l);
      if (i >= misc.total_items - 1)
         break;
   } while (daisy[++i].screen == daisy[misc.current].screen);
   if (misc.just_this_item != -1 &&
       daisy[misc.displaying].screen == daisy[misc.playing].screen)
      mvwprintw (misc.screenwin, daisy[misc.current].y, 0, "J");
   mvwprintw (misc.screenwin, 21, 0, "----------------------------------------");
   wprintw (misc.screenwin, "----------------------------------------");
   wmove (misc.screenwin, daisy[misc.current].y, daisy[misc.current].x);
   wrefresh (misc.screenwin);
} // view_screen

void player_ended ()
{
   wait (NULL);
} // player_ended

xmlTextReaderPtr open_text_file (xmlTextReaderPtr reader, xmlDocPtr doc,
                                 char *text_file, char *anchor)
{
   xmlTextReaderClose (reader);
   xmlFreeDoc (doc);
   doc = xmlRecoverFile (text_file);
   if ((reader = xmlReaderWalker (doc)) == NULL)
   {
      endwin ();
      beep ();
      printf ("open_text_file(): %s\n", text_file);
      fflush (stdout);
      _exit (1);
   } // if
   do
   {
      if (! get_tag_or_label (reader))
         break;
   } while (strcasecmp (misc.tag, "body") != 0);
   if (*anchor == 0)
      return reader;

// look if anchor exists in this text_file
   while (1)
   {
      if (! get_tag_or_label (reader))
      {
// if anchor couldn't be found, start from the beginning
         xmlTextReaderClose (reader);
         xmlFreeDoc (doc);
         doc = xmlRecoverFile (text_file);
         reader = xmlReaderWalker (doc);
         break;
      } // if
      if (strcasecmp (anchor, my_attribute.id) == 0)
         break;
   } // while
   return reader;
} // open_text_file

void pause_resume (xmlTextReaderPtr reader, xmlDocPtr doc)
{
   if (misc.playing != -1)
      misc.playing = -1;
   else
      misc.playing = misc.displaying;
   if (! reader)
      return;
   else
   {
      if (misc.playing != -1)
      {
         FILE *w;

         kill (misc.player_pid, SIGKILL);
         wattron (misc.screenwin, A_BOLD);
         mvwprintw (misc.screenwin, daisy[misc.playing].y, 69, "%5d %5d",
                    misc.phrase_nr, daisy[misc.playing].n_phrases - misc.phrase_nr - 1);
         wattroff (misc.screenwin, A_BOLD);
         wmove (misc.screenwin, daisy[misc.displaying].y, daisy[misc.displaying].x);
         wrefresh (misc.screenwin);
         if ((w = fopen ("eBook-speaker.txt", "w")) == NULL)
         {
            endwin ();
            beep ();
            printf ("Can't make a temp file %s\n", "eBook-speaker.txt");
            fflush (stdout);
            _exit (0);
         } // if
         if (fwrite (misc.label, strlen (misc.label), 1, w) == 0)
         {
            endwin ();
            beep ();
            printf ("write (\"%s\"): failed.\n", misc.label);
            fflush (stdout);
            kill (getppid (), SIGINT);
         } // if
         fclose (w);

         switch (system (misc.tts[misc.tts_no]))
         {
         default:
            break;
         } // switch

         switch (misc.player_pid = fork ())
         {
         case -1:
         {
            int e;

            e = errno;
            endwin ();
            beep ();
            reader = skip_left (reader, doc);
            put_bookmark ();
            printf ("%s\n", strerror (e));
            fflush (stdout);
            _exit (1);
         }
         case 0:
            setsid ();
            playfile ("eBook-speaker.wav", "wav", misc.sound_dev, "alsa", misc.speed);
            _exit (0);
         default:
            return;
         } // switch
      }
      else
         kill_player ();
   } // if
} // pause_resume

void write_wav (int c, char *outfile)
{
   FILE *w;
   xmlTextReaderPtr local_reader;
   xmlDocPtr local_doc;
   sox_format_t *first, *output;
   #define MAX_SAMPLES (size_t)2048
   sox_sample_t samples[MAX_SAMPLES];
   size_t number_read;

   if ((w = fopen ("eBook-speaker.txt", "w")) == NULL)
   {
      endwin ();
      beep ();
      printf ("Can't make a temp file %s\n", "eBook-speaker.txt");
      fflush (stdout);
      _exit (0);
   } // if
   if (fwrite ("init", 4, 1, w) == 0)
   {
      endwin ();
      beep ();
      printf ("write (\"%s\"): failed.\n", misc.label);
      fflush (stdout);
      _exit (0);
   } // if
   fclose (w);
   if (! *misc.tts[misc.tts_no])
   {
      misc.tts_no = 0;
      select_tts ();
   } // if
   switch (system (misc.tts[misc.tts_no]))
   {
   default:
      break;
   } // switch
   sox_init ();
   rename ("eBook-speaker.wav", "first-eBook-speaker.wav");
   first = sox_open_read ("first-eBook-speaker.wav", NULL, NULL, NULL);
   output = sox_open_write (outfile, &first->signal, &first->encoding,
                            NULL, NULL, NULL);
   local_doc = NULL;
   local_reader = NULL;
   local_reader = open_text_file (local_reader, local_doc,
                  daisy[c].smil_file, daisy[c].anchor);
   while (1)
   {
      sox_format_t *input;

      if (! get_tag_or_label (local_reader))
         break;
      if (*daisy[c + 1].anchor &&
          strcasecmp (my_attribute.id, daisy[c + 1].anchor) == 0)
         break;
      if (! *misc.label)
         continue;;
      w = fopen ("eBook-speaker.txt", "w");
      fwrite (misc.label, strlen (misc.label), 1, w);
      fclose (w);
      switch (system (misc.tts[misc.tts_no]))
      {
      default:
         break;
      } // switch
      input = sox_open_read ("eBook-speaker.wav", NULL, NULL, NULL);
      while ((number_read = sox_read (input, samples, MAX_SAMPLES)))
         sox_write (output, samples, number_read);
      sox_close (input);
   } // while
   sox_close (output);
   sox_close (first);
   sox_quit ();
} // write_wav

void help ()
{
   int y, x;

   getyx (misc.screenwin, y, x);
   wclear (misc.screenwin);
   waddstr (misc.screenwin, gettext
            ("\nThese commands are available in this version:\n"));
   waddstr (misc.screenwin, "========================================");
   waddstr (misc.screenwin, "========================================\n\n");
   waddstr (misc.screenwin, gettext
            ("cursor down     - move cursor to the next item\n"));
   waddstr (misc.screenwin, gettext
            ("cursor up       - move cursor to the previous item\n"));
   waddstr (misc.screenwin, gettext
            ("cursor right    - skip to next phrase\n"));
   waddstr (misc.screenwin, gettext
            ("cursor left     - skip to previous phrase\n"));
   waddstr (misc.screenwin, gettext ("page-down       - view next page\n"));
   waddstr (misc.screenwin, gettext
            ("page-up         - view previous page\n"));
   waddstr (misc.screenwin, gettext ("enter           - start playing\n"));
   waddstr (misc.screenwin, gettext
            ("space           - pause/resume playing\n"));
   waddstr (misc.screenwin, gettext
            ("home            - play on normal speed\n"));
   waddstr (misc.screenwin, "\n");
   waddstr (misc.screenwin, gettext ("Press any key for next page..."));
   nodelay (misc.screenwin, FALSE);
   wgetch (misc.screenwin);
   nodelay (misc.screenwin, TRUE);
   wclear (misc.screenwin);
   waddstr (misc.screenwin, gettext
            ("\n/               - search for a label\n"));
   waddstr (misc.screenwin, gettext
            ("B               - move cursor to the last item\n"));
   waddstr (misc.screenwin, gettext
            ("d               - store current item to disk in WAV-format\n"));
   waddstr (misc.screenwin, gettext
            ("D               - decrease playing speed\n"));
   waddstr (misc.screenwin, gettext
 ("f               - find the currently playing item and place the cursor there\n"));
   waddstr (misc.screenwin, gettext
            ("g               - go to phrase in current item\n"));
   waddstr (misc.screenwin, gettext ("h or ?          - give this help\n"));
   waddstr (misc.screenwin, gettext
            ("j               - just play current item\n"));
   waddstr (misc.screenwin, gettext
            ("l               - switch to next level\n"));
   waddstr (misc.screenwin, gettext
            ("L               - switch to previous level\n"));
   waddstr (misc.screenwin, gettext ("n               - search forwards\n"));
   waddstr (misc.screenwin, gettext ("N               - search backwards\n"));
   waddstr (misc.screenwin, gettext
            ("o               - select next output sound device\n"));
   waddstr (misc.screenwin, gettext ("p               - place a bookmark\n"));
   waddstr (misc.screenwin, gettext
            ("q               - quit eBook-speaker and place a bookmark\n"));
   if (misc.scan_flag == 1)
      waddstr (misc.screenwin, gettext
               ("r               - rotate scanned document\n"));
   waddstr (misc.screenwin, gettext ("s               - stop playing\n"));
   waddstr (misc.screenwin, gettext ("t               - select next TTS\n"));
   waddstr (misc.screenwin, gettext
            ("T               - move cursor to the first item\n"));
   waddstr (misc.screenwin, gettext
            ("U               - increase playing speed\n"));
   waddstr (misc.screenwin, gettext ("\nPress any key to leave help..."));
   nodelay (misc.screenwin, FALSE);
   wgetch (misc.screenwin);
   nodelay (misc.screenwin, TRUE);
   view_screen ();
   wmove (misc.screenwin, y, x);
} // help

void previous_item ()
{
   do
   {
      if (--misc.current < 0)
      {
         beep ();
         misc.displaying = misc.current = 0;
         break;
      } // if
   } while (daisy[misc.current].level > misc.level);
   misc.displaying = misc.current;
   misc.phrase_nr = daisy[misc.current].n_phrases - 1;
   view_screen ();
} // previous_item

void next_item ()
{
   do
   {
      if (++misc.current >= misc.total_items)
      {
         misc.displaying = misc.current = misc.total_items - 1;
         beep ();
         break;
      } // if
      misc.displaying = misc.current;
   } while (daisy[misc.current].level > misc.level);
   view_screen ();
} // next_item

xmlTextReaderPtr skip_left (xmlTextReaderPtr reader, xmlDocPtr doc)
{
   int old_phrase_nr;

   if (misc.playing == -1)
   {
      beep ();
      return NULL;
   } // if
   old_phrase_nr = misc.phrase_nr;
   kill_player ();
   if (misc.phrase_nr <= 2)
   {
      if (misc.playing == 0)
         return reader;
      misc.playing--;
      misc.displaying = misc.current = misc.playing;
      reader = open_text_file (reader, doc, daisy[misc.playing].smil_file,
                               daisy[misc.playing].anchor);
      misc.phrase_nr = 0;
      while (1)
      {
         if (! get_tag_or_label (reader))
            return NULL;
         if (! *misc.label)
            continue;
         if (misc.phrase_nr >= daisy[misc.playing].n_phrases - 2)
            return reader;
         misc.phrase_nr++;
      } // while
   } // if
   reader = open_text_file (reader, doc, daisy[misc.playing].smil_file,
                            daisy[misc.playing].anchor);
   misc.phrase_nr = 0;
   while (1)
   {
      if (! get_tag_or_label (reader))
         return NULL;
      if (! *misc.label)
         continue;
      if (++misc.phrase_nr >= old_phrase_nr - 2)
         return reader;
   } // while
} // skip_left

void skip_right ()
{
   if (misc.playing == -1)
   {
      beep ();
      return;
   } // if
   kill_player ();
} // skip_right      

void change_level (char key)
{

   if (key == 'l')
      if (++misc.level > misc.depth)
         misc.level = 1;
   if (key == 'L')
      if (--misc.level < 1)
         misc.level = misc.depth;
   if (daisy[misc.playing].level > misc.level)
      previous_item ();
   view_screen ();
} // change_level

void read_xml ()
{
   int x = 0, i;
   char str[MAX_STR], *p;
   struct passwd *pw;;
   xmlTextReaderPtr local_reader;
   xmlDocPtr doc;

   pw = getpwuid (geteuid ());
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker.xml", pw->pw_dir);
   if ((doc = xmlRecoverFile (str)) == NULL)
   {
// If no TTS; give some examples
      strncpy (misc.tts[0], "espeak -f eBook-speaker.txt -w eBook-speaker.wav",
               MAX_STR - 1);
      strncpy (misc.tts[1], "flite eBook-speaker.txt eBook-speaker.wav",
               MAX_STR - 1);
      strncpy (misc.tts[2],
               "espeak -f eBook-speaker.txt -w eBook-speaker.wav -v mb-nl2",
               MAX_STR - 1);
      strncpy (misc.tts[3], "text2wave eBook-speaker.txt -o eBook-speaker.wav",
               MAX_STR - 1);
      strncpy (misc.tts[4], "swift -n Lawrence -f eBook-speaker.txt -m text -o eBook-speaker.wav",
               MAX_STR - 1);
      save_xml ();
      return;
   } // if
   if ((local_reader = xmlReaderWalker (doc)))
   {
      while (1)
      {
         if (! get_tag_or_label (local_reader))
            break;
         if (strcasecmp (misc.tag, "tts") == 0)
         {
            do
            {
               if (! get_tag_or_label (local_reader))
                  break;
            } while (! *misc.label);
            for (i = 0; misc.label[i]; i++)
               if (misc.label[i] != ' ' && misc.label[i] != '\n')
                  break;;
            p = misc.label + i;
            for (i = strlen (p) - 1; i > 0; i--)
               if (p[i] != ' ' && p[i] != '\n')
                  break;
            p[i + 1] = 0;
            strncpy (misc.tts[x++], p, MAX_STR - 1);
            if (x == 10)
               break;
         } // if
      } // while
      misc.tts_no = x;
      xmlTextReaderClose (local_reader);
      xmlFreeDoc (doc);
   } // if
   if (*misc.tts[0])
      return;

// If no TTS; give some examples
   strncpy (misc.tts[0], "espeak -f eBook-speaker.txt -w eBook-speaker.wav",
            MAX_STR - 1);
   strncpy (misc.tts[1], "flite eBook-speaker.txt eBook-speaker.wav", MAX_STR - 1);
   strncpy (misc.tts[2],
            "espeak -f eBook-speaker.txt -w eBook-speaker.wav -v mb-nl2",
            MAX_STR - 1);
   strncpy (misc.tts[3], "text2wave eBook-speaker.txt -o eBook-speaker.wav",
            MAX_STR - 1);
   strncpy (misc.tts[4],
       "swift -n Lawrence -f eBook-speaker.txt -m text -o eBook-speaker.wav",
            MAX_STR - 1);
} // read_xml

void save_xml ()
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
      (writer, BAD_CAST "sound_dev", "%s", misc.sound_dev);
   xmlTextWriterWriteFormatAttribute
      (writer, BAD_CAST "ocr_language", "%s", misc.ocr_language);
   xmlTextWriterEndElement (writer);
   xmlTextWriterWriteString (writer, BAD_CAST "\n   ");
   xmlTextWriterStartElement (writer, BAD_CAST "voices");
   while (1)
   {
      char str[MAX_STR];

      xmlTextWriterWriteString (writer, BAD_CAST "\n      ");
      xmlTextWriterStartElement (writer, BAD_CAST "tts");
      snprintf (str, MAX_STR - 1, "\n         %s\n      ", misc.tts[x]);
      xmlTextWriterWriteString (writer, BAD_CAST str);
      xmlTextWriterEndElement (writer);
      if (! *misc.tts[++x])
         break;
   } // while
   xmlTextWriterWriteString (writer, BAD_CAST "\n   ");
   xmlTextWriterEndElement (writer);
   xmlTextWriterWriteString (writer, BAD_CAST "\n");
   xmlTextWriterEndElement (writer);
   xmlTextWriterEndDocument (writer);
   xmlFreeTextWriter (writer);
} // save_xml

void save_bookmark_and_xml ()
{
   char cmd[MAX_CMD];

   endwin ();
   kill_player ();
   wait (NULL);
   put_bookmark ();
   save_xml ();
   xmlCleanupParser ();
   if (! misc.tmp_dir)
      return;
   snprintf (cmd, MAX_CMD - 1, "rm -rf %s", misc.tmp_dir);
   if (system (cmd) > 0)
   {
      endwin ();
      beep ();
      _exit (1);
   } // if
} // save_bookmark_and_xml

void search (xmlTextReaderPtr reader, xmlDocPtr doc, int start, char mode)
{
   int c, found = 0;

   if (mode == '/')
   {
      if (misc.playing != -1)
         kill_player ();
      mvwaddstr (misc.titlewin, 1, 0, "----------------------------------------");
      waddstr (misc.titlewin, "----------------------------------------");
      mvwprintw (misc.titlewin, 1, 0, gettext ("What do you search?                           "));
      echo ();
      wmove (misc.titlewin, 1, 20);
      wgetnstr (misc.titlewin, misc.search_str, 25);
      noecho ();
   } // if
   if (mode == '/' || mode == 'n')
   {
      for (c = start; c < misc.total_items; c++)
      {
         if (strcasestr (daisy[c].label, misc.search_str))
         {
            found = 1;
            break;
         } // if
      } // for
      if (! found)
      {
         for (c = 0; c < start; c++)
         {
            if (strcasestr (daisy[c].label, misc.search_str))
            {
               found = 1;
               break;
            } // if
         } // for
      } // if
   }
   else
   { // mode == 'N'
      for (c = start; c >= 0; c--)
      {
         if (strcasestr (daisy[c].label, misc.search_str))
         {
            found = 1;
            break;
         } // if
      } // for
      if (! found)
      {
         for (c = misc.total_items + 1; c > start; c--)
         {
            if (strcasestr (daisy[c].label, misc.search_str))
            {
               found = 1;
               break;
            } // if
         } // for
      } // if
   } // if
   if (found)
   {
      misc.current = c;
      misc.phrase_nr = 0;
      misc.just_this_item = -1;
      misc.displaying = misc.playing = misc.current;
      if (misc.playing != -1)
         kill_player ();
      xmlTextReaderClose (reader);
      xmlFreeDoc (doc);
      reader = open_text_file (reader, doc, daisy[misc.current].smil_file,
                      daisy[misc.current].anchor);
   }
   else
   {
      beep ();
      kill_player ();
      misc.phrase_nr--;
      get_next_phrase (reader, doc, misc.playing);
   } // if
   view_screen ();
} // search

void kill_player ()
{
   kill (misc.player_pid, SIGKILL);
   unlink ("eBook-speaker.txt");
   unlink ("eBook-speaker.wav");
} // kill_player

void select_next_output_device ()
{
   FILE *r;
   int n, y;
   size_t bytes;
   char *list[10], *trash;

   wclear (misc.screenwin);
   wprintw (misc.screenwin, gettext ("\nSelect a soundcard:\n\n"));
   if (! (r = fopen ("/proc/asound/cards", "r")))
   {
      endwin ();
      beep ();
      puts (gettext ("Cannot read /proc/asound/cards"));
      fflush (stdout);
      _exit (1);
   } // if
   for (n = 0; n < 10; n++)
   {
      list[n] = (char *) malloc (1000);
      bytes = getline (&list[n], &bytes, r);
      if ((int) bytes == -1)
         break;
      trash = (char *) malloc (1000);
      bytes = getline (&trash, &bytes, r);
      free (trash);
      wprintw (misc.screenwin, "   %s", list[n]);
      free (list[n]);
   } // for          
   fclose (r);
   y = 3;
   nodelay (misc.screenwin, FALSE);
   for (;;)
   {
      wmove (misc.screenwin, y, 2);
      switch (wgetch (misc.screenwin))
      {
      case 13: //
         snprintf (misc.sound_dev, MAX_STR - 1, "hw:%i", y - 3);
         view_screen ();
         nodelay (misc.screenwin, TRUE);
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
         view_screen ();
         nodelay (misc.screenwin, TRUE);
         return;
      } // switch
   } // for
} // select_next_output_device

xmlTextReaderPtr go_to_phrase (xmlTextReaderPtr reader, xmlDocPtr doc)
{
   char pn[15];

   kill_player ();
   misc.player_pid = -2;
   mvwaddstr (misc.titlewin, 1, 0, "----------------------------------------");
   waddstr (misc.titlewin, "----------------------------------------");
   mvwprintw (misc.titlewin, 1, 0, gettext ("Go to phrase in current item: "));
   echo ();
   wgetnstr (misc.titlewin, pn, 8);
   noecho ();
   view_screen ();
   if (! *pn || atoi (pn) > daisy[misc.current].n_phrases + 1)
   {
      beep ();
      reader = skip_left (reader, doc);
      skip_right ();
      return reader;  
   } // if
   reader = open_text_file (reader, doc, daisy[misc.current].smil_file,
                            daisy[misc.current].anchor);
   misc.phrase_nr = 0;
   if (atoi (pn) > 1)
   {
      while (1)
      {
         if (! get_tag_or_label (reader))
            break;
         if (! *misc.label)
            continue;
         (misc.phrase_nr)++;
         if (misc.phrase_nr + 1 == atoi (pn))
            break;
      } // while
   } // if
   misc.playing = misc.displaying = misc.current;
   view_screen ();
   mvwprintw (misc.screenwin, daisy[misc.current].y, 69, "%5d %5d",
              misc.phrase_nr + 1, daisy[misc.current].n_phrases - misc.phrase_nr - 1);
   wrefresh (misc.screenwin);
   wmove (misc.screenwin, daisy[misc.current].y, daisy[misc.current].x);
   misc.just_this_item = -1;
   remove ("eBook-speaker.wav");
   remove ("old.wav");
   return reader;
} // go_to_phrase

void start_tesseract (char *file)
{
   char cmd[MAX_CMD];

   if (*misc.ocr_language)
      snprintf (cmd, MAX_CMD - 1,
                "tesseract \"%s\" out -l %s 2> /dev/null", file,
                misc.ocr_language);
   else
      snprintf (cmd, MAX_CMD - 1,
                "tesseract \"%s\" out 2> /dev/null", file);
   switch (system (cmd))
   {
   case 0:
      break;
   default:
      endwin ();
      beep ();
      puts (misc.copyright);
      printf (gettext (
  "Language code \"%s\" is not a valid tesseract code.\n"), misc.ocr_language);
      puts (gettext ("See the tesseract manual for valid codes."));
      fflush (stdout);
      _exit (1);
   } // switch
   pandoc_to_epub ("out.txt");
} // start_tesseract

void browse (char *file)
{
   int old;
   xmlTextReaderPtr reader;
   xmlDocPtr doc;

   misc.current = misc.displaying = misc.playing = 0;
   misc.just_this_item = misc.playing = -1;
   reader = NULL;
   doc = NULL;
   if (! misc.scan_flag)
      reader = get_bookmark (reader, doc);
   if (misc.depth <= 0)
      misc.depth = 1;
   view_screen ();
   nodelay (misc.screenwin, TRUE);
   wmove (misc.screenwin, daisy[misc.current].y, daisy[misc.current].x);
   for (;;)
   {
      switch (wgetch (misc.screenwin))
      {
      case 13: // ENTER
         misc.playing = misc.displaying = misc.current;
         remove ("eBook-speaker.wav");
         remove ("old.wav");
         misc.phrase_nr = 0;
         misc.just_this_item = -1;
         view_screen ();
         kill_player ();
         reader = open_text_file (reader, doc, daisy[misc.current].smil_file,
                                  daisy[misc.current].anchor);
         break;
      case '/':
         search (reader, doc, misc.current + 1, '/');
         break;
      case ' ':
      case KEY_IC:
      case '0':
         pause_resume (reader, doc);
         break;
      case 'B':
         misc.displaying = misc.current = misc.total_items - 1;
         misc.phrase_nr = daisy[misc.current].n_phrases - 1;
         view_screen ();
         break;
      case 'f':
         if (misc.playing == -1)
         {
            beep ();
            break;
         } // if
         misc.current = misc.displaying= misc.playing;
         view_screen ();
         break;
      case 'g':
         reader = go_to_phrase (reader, doc);
         break;
      case 'h':
      case '?':
         kill_player ();
         help ();
         kill_player ();
         if (misc.playing != -1)
         {
            misc.phrase_nr--;
            get_next_phrase (reader, doc, misc.playing);
         } // if
         break;
      case 'j':
      case '5':
      case KEY_B2:
         if (misc.just_this_item != -1)
            misc.just_this_item = -1;
         else
         {
            if (misc.playing == -1)
            {
               misc.phrase_nr = 0;
               reader = open_text_file (reader, doc, daisy[misc.current].smil_file,
                                        daisy[misc.current].anchor);
            } // if
            misc.playing = misc.just_this_item = misc.current;
         } // if
         mvwprintw (misc.screenwin, daisy[misc.current].y, 0, " ");
         if (misc.playing == -1)
         {
            misc.just_this_item = misc.displaying = misc.playing = misc.current;
            misc.phrase_nr = 0;
            kill_player ();
            reader = open_text_file (reader, doc, daisy[misc.current].smil_file,
                            daisy[misc.current].anchor);
         } // if
         if (misc.just_this_item != -1 &&
             daisy[misc.displaying].screen == daisy[misc.playing].screen)
            mvwprintw (misc.screenwin, daisy[misc.current].y, 0, "J");
         else
            mvwprintw (misc.screenwin, daisy[misc.current].y, 0, " ");
         wrefresh (misc.screenwin);
         break;
      case 'l':
         change_level ('l');
         break;
      case 'L':
         change_level ('L');
         break;
      case 'n':
         search (reader, doc, misc.current + 1, 'n');
         break;
      case 'N':
         search (reader, doc, misc.current - 1, 'N');
         break;
      case 'o':
         if (misc.playing == -1)
         {
            beep ();
            break;
         } // if
         pause_resume (reader, doc);
         select_next_output_device ();
         misc.playing = misc.displaying;
         kill_player ();
         if (misc.playing != -1)
         {
            misc.phrase_nr--;
            get_next_phrase (reader, doc, misc.playing);
         } // if
         break;
      case 'p':
         put_bookmark ();
         break;
      case 'q':
         quit_eBook_speaker ();
      case 'r':
      {
         char cmd[MAX_CMD + 1], str[MAX_STR];

         if (misc.scan_flag != 1)
         {
            beep ();
            break;
         } // if
         xmlTextReaderClose (reader);
         xmlFreeDoc (doc);
         remove ("eBook-speaker.wav");
         remove ("old.wav");
         misc.phrase_nr = 0;
         view_screen ();
         wattron (misc.titlewin, A_BOLD);
         mvwprintw (misc.titlewin, 0, 0, "%s - %s", misc.copyright,
                  gettext ("Please wait..."));
         wattroff (misc.titlewin, A_BOLD);
         wrefresh (misc.titlewin);
         kill_player ();
         snprintf (cmd, MAX_CMD, "pnmflip -r180 \"%s\" > \"%s.rotated\"",
                   file, file);
         switch (system (cmd))
         {
         default:
            break;
         } // switch
         snprintf (str, MAX_STR, "%s.rotated", file);
         rename (str, file);
         unlink ("out");
         unlink ("out.epub");
         unlink ("out.txt");
         view_screen ();
         start_tesseract (file);
         read_daisy_3 (get_current_dir_name ());
         check_phrases ();
         wclear (misc.titlewin);
         mvwprintw (misc.titlewin, 0, 0, misc.copyright);
         if ((int) strlen (misc.daisy_title) + (int)
             strlen (misc.copyright) >= misc.max_x)
            mvwprintw (misc.titlewin, 0, misc.max_x - strlen (misc.daisy_title) - 4, "... ");
         mvwprintw (misc.titlewin, 0, misc.max_x - strlen (misc.daisy_title),
              "%s", misc.daisy_title);
         mvwaddstr (misc.titlewin, 1, 0, "----------------------------------------");
         waddstr (misc.titlewin, "----------------------------------------");
         mvwprintw (misc.titlewin, 1, 0, gettext ("Press 'h' for help "));
         wrefresh (misc.titlewin);
         misc.level = 1;
         *misc.search_str = 0;
         misc.current = misc.displaying = misc.playing = 0;
         misc.just_this_item = misc.playing = -1;
         reader = NULL;
         doc = NULL;
         if (misc.depth <= 0)
            misc.depth = 1;
         view_screen ();
         nodelay (misc.screenwin, TRUE);
         wmove (misc.screenwin, daisy[misc.current].y, daisy[misc.current].x);
         break;
      }
      case 's':
         misc.playing = misc.just_this_item = -1;
         view_screen ();
         kill_player ();
         break;
      case 't':

         if (misc.playing != -1)
            pause_resume (reader, doc);
         select_tts ();
         misc.playing = misc.displaying;
         kill_player ();
         if (misc.playing != -1)
         {
            misc.phrase_nr--;
            get_next_phrase (reader, doc, misc.playing);
         } // if
         break;
      case 'T':
         misc.displaying = misc.current = 0;
         misc.phrase_nr = daisy[misc.current].n_phrases - 1;
         view_screen ();
         break;
      case KEY_DOWN:
      case '2':
         next_item ();
         break;
      case KEY_UP:
      case '8':
         previous_item ();
         break;
      case KEY_RIGHT:
      case '6':
         skip_right ();
         break;
      case KEY_LEFT:
      case '4':
         reader = skip_left (reader, doc);
         break;
      case KEY_NPAGE:
      case '3':
         if (misc.current / misc.max_y == (misc.total_items - 1) / misc.max_y)
         {
            beep ();
            break;
         } // if
         old = misc.current / misc.max_y;
         while (misc.current / misc.max_y == old)
            next_item ();
         view_screen ();
         break;
      case KEY_PPAGE:
      case '9':
         if (misc.current / misc.max_y == 0)
         {
            beep ();
            break;
         } // if
         old = misc.current / misc.max_y - 1;
         misc.current = 0;
         while (misc.current / misc.max_y != old)
            next_item ();
         view_screen ();
         break;
      case ERR:
         break;
      case 'U':
      case '+':
         if (misc.speed >= 2)
         {
            beep ();
            break;
         } // if
         misc.speed += 0.1;
         if (misc.playing == -1)
            break;
         kill_player ();
         misc.phrase_nr--;
         get_next_phrase (reader, doc, misc.playing);
         break;
      case 'd':
         store_to_disk (reader, doc);
         break;
      case 'D':
      case '-':
         if (misc.speed <= 0.1)
         {
            beep ();
            break;
         } // if
         misc.speed -= 0.1;
         if (misc.playing == -1)
            break;
         kill_player ();
         misc.phrase_nr--;
         get_next_phrase (reader, doc, misc.playing);
         break;
      case KEY_HOME:
      case '*':
      case '7':
         misc.speed = 1.0;
         if (misc.playing == -1)
            break;
         kill_player ();
         misc.phrase_nr--;
         get_next_phrase (reader, doc, misc.playing);
         break;
      default:
         beep ();
         break;
      } // switch

      if (misc.playing != -1)
      {
         if (get_next_phrase (reader, doc, misc.playing) == -1)
         {
            misc.phrase_nr = 0;
            misc.playing++;
            if (daisy[misc.playing].level <= misc.level)
               misc.current = misc.displaying = misc.playing;
            if (misc.just_this_item != -1 && daisy[misc.playing].level <= misc.level)
               misc.playing = misc.just_this_item = -1;
            remove ("eBook-speaker.wav");
            remove ("old.wav");
            reader = open_text_file (reader, doc, daisy[misc.current].smil_file,
                                     daisy[misc.current].anchor);
            view_screen ();
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

void read_epub (char *file)
{
   char cmd[MAX_CMD];

   snprintf (cmd, MAX_CMD - 1, "unar \"%s\"", file);
   switch (system (cmd))
   {
   default:
      break;
   } // switch
   switch (chdir (misc.daisy_mp))
   {
   default:
      break;
   } // switch
} // read_epub

char *lowriter_to (char *file, char *ext)
{
   char cmd[MAX_CMD];
   DIR *dir;
   struct dirent *dirent;

   snprintf (cmd, MAX_CMD - 1,
         "lowriter --headless --convert-to %s \"%s\" > /dev/null", ext, file);
   switch (system (cmd))
   {
   case 0:
      break;
   default:
      endwin ();
      beep ();
      puts (gettext ("eBook-speaker needs the libreoffice-writer package."));
      fflush (stdout);
      _exit (1);
   } // switch
   if (! (dir = opendir (".")))
   {
      endwin ();
      beep ();
      printf ("\n%s\n", strerror (errno));
      fflush (stdout);
      _exit (1);
   } // if
   while ((dirent = readdir (dir)) != NULL)
   {
      if (strcasecmp (dirent->d_name +
          strlen (dirent->d_name) - 4, ".pdf") == 0)
         break;
   } // while                             
   closedir (dir);
   if (access (dirent->d_name, R_OK) != 0)
   {
      endwin ();
      beep ();
      printf (gettext ("Unknown format\n"));
      fflush (stdout);
      _exit (1);
   } // if                                           
   return dirent->d_name;
} // lowriter_to

void pandoc_to_epub (char *file)
{
   char cmd[MAX_CMD];

   snprintf (cmd, MAX_CMD - 1, "pandoc \"%s\" -o out.epub", file);
   switch (system (cmd))
   {
   case 0:
      break;
   default:
      endwin ();
      beep ();
      puts (misc.copyright);
      puts (gettext ("eBook-speaker needs the pandoc package."));
      fflush (stdout);
      _exit (1);
   } // switch
   if (access ("out.epub", R_OK) != 0)
   {
      endwin ();
      beep ();
      printf (gettext ("Unknown format\n"));
      fflush (stdout);
      _exit (1);
   } // if
   read_epub ("out.epub");
} // pandoc_to_epub

void pdf_to_html (const char *file)
{
   char cmd[MAX_CMD];

   snprintf (cmd, MAX_CMD - 1, "pdftohtml -q -s -i -noframes \"%s\" out.html", file);
   switch (system (cmd))
   {
   case 0:
      break;
   default:
      endwin ();
      beep ();
      puts (gettext ("eBook-speaker needs the poppler-utils package."));
      fflush (stdout);
      _exit (1);
   } // switch
} // pdf_to_html

void check_phrases ()
{
   int item;

   misc.total_phrases = 0;
   for (item = 0; item < misc.total_items; item++)
   {
      strncpy (daisy[item].smil_file,
               re_organize (daisy[item].smil_file), MAX_STR - 1);
   } // for
   for (item = 0; item < misc.total_items; item++)
   {
      daisy[item].n_phrases =
         count_phrases (daisy[item].smil_file, daisy[item].anchor,
                        daisy[item + 1].smil_file,
                        daisy[item + 1].anchor) - 1;
      misc.total_phrases += daisy[item].n_phrases;
   } // for

// remove items with 0 phrases
   if (daisy[0].n_phrases == 0)
   {
      int i, x;

      for (i = 1; i < misc.total_items; i++)
         if (daisy[i].n_phrases > 0)
            break;
      item = 0;
      for (x = i; x < misc.total_items; x++)
      {
         strncpy (daisy[item].smil_file, daisy[x].smil_file, MAX_STR - 1);
         daisy[item].n_phrases = daisy[x].n_phrases;
         item++;
      } // for
      misc.total_items = misc.total_items - i;
   } // if
} // check_phrases

void store_to_disk (xmlTextReaderPtr reader, xmlDocPtr doc)
{
   char str[MAX_STR], s2[MAX_STR];
   int i, pause_flag;
   struct passwd *pw;;

   pause_flag = misc.playing;
   wclear (misc.screenwin);
   snprintf (str, MAX_STR - 1, "%s.wav", daisy[misc.current].label);
   for (i = 0; str[i] != 0; i++)
   {
      if (str[i] == '/')
         str[i] = '-';
      if (isspace (str[i]))
         str[i] = '_';
   } // for
   wprintw (misc.screenwin,
            "\nStoring \"%s\" as \"%s\" into your home-folder...",
            daisy[misc.current].label, str);
   wrefresh (misc.screenwin);
   pw = getpwuid (geteuid ());
   snprintf (s2, MAX_STR - 1, "%s/%s", pw->pw_dir, str);
   if (pause_flag > -1)
      pause_resume (reader, doc);
   write_wav (misc.current, s2);
   if (pause_flag > -1)
      pause_resume (reader, doc);
   view_screen ();
} // store_to_disk

void usage ()
{
   endwin ();
   printf (gettext ("eBook-speaker - Version %s\n"), PACKAGE_VERSION);
   puts ("(C)2003-2014 J. Lemmens");
   printf (gettext ("\nUsage: eBook-speaker [eBook_file | -s] "
                    "[-d ALSA_misc.sound_device] [-t TTS_command]\n"));
   fflush (stdout);
   _exit (1);
} // usage

int main (int argc, char *argv[])
{
   int opt;
   char file[MAX_STR], pwd[MAX_STR], str[MAX_STR];

   fclose (stderr);
   setbuf (stdout, NULL);
   setlocale (LC_ALL, "");
   setlocale (LC_NUMERIC, "C");
   textdomain (PACKAGE);
   snprintf (str, MAX_STR, "%s/", LOCALEDIR);
   bindtextdomain (PACKAGE, str);
   initscr ();
   misc.titlewin = newwin (2, 80,  0, 0);
   misc.screenwin = newwin (23, 80, 2, 0);
   getmaxyx (misc.screenwin, misc.max_y, misc.max_x);
   misc.max_y -= 2;
   keypad (misc.screenwin, TRUE);
   meta (misc.screenwin,       TRUE);
   nonl ();
   noecho ();
   misc.eBook_speaker_pid = getpid ();
   misc.player_pid = -2;
   snprintf (misc.copyright, MAX_STR - 1, gettext
             ("eBook-speaker - Version %s - (C)2014 J. Lemmens"), PACKAGE_VERSION);
   wattron (misc.titlewin, A_BOLD);
   wprintw (misc.titlewin, "%s - %s", misc.copyright, gettext ("Please wait..."));
   wattroff (misc.titlewin, A_BOLD);
   if (access ("/usr/share/doc/espeak", R_OK) != 0)
   {
      endwin ();
      beep ();
      puts (gettext ("eBook-speaker needs the espeak package."));
      printf (gettext ("Please install it and try again.\n"));
      fflush (stdout);
      _exit (1);
   } // if
   if (access ("/usr/share/doc/unar", R_OK) != 0)
   {
      endwin ();
      beep ();
      puts (gettext ("eBook-speaker needs the unar package."));
      printf (gettext ("Please install it and try again.\n"));
      fflush (stdout);
      _exit (1);
   } // if
   misc.speed = 1.0;
   atexit (quit_eBook_speaker);
   signal (SIGCHLD, player_ended);
   signal (SIGTERM, quit_eBook_speaker);
   strncpy (misc.sound_dev, "hw:0", MAX_STR - 1);
   *misc.ocr_language = 0;
   read_xml ();
   misc.tmp_dir = strdup ("/tmp/eBook-speaker.XXXXXX");
   if (! mkdtemp (misc.tmp_dir))
   {
      endwin ();
      printf ("mkdtemp ()\n");
      beep ();
      fflush (stdout);
      _exit (1);
   } // if
   opterr = 0;
   misc.tts_flag = -1, misc.scan_flag = 0;
   while ((opt = getopt (argc, argv, "d:hlo:st:")) != -1)
   {
      switch (opt)
      {
      case 'd':
         strncpy (misc.sound_dev, optarg, 15);
         break;
      case 'h':
         usage ();
      case 'l':
         continue;
      case 'o':
         strncpy (misc.ocr_language, optarg, 3);
         break;
      case 's':
      {
         char cmd[MAX_CMD];

         if (access ("/usr/share/doc/tesseract-ocr", R_OK) != 0)
         {
            endwin ();
            beep ();
            puts (gettext ("eBook-speaker needs the tesseract-ocr package."));
            printf (gettext ("Please install it and try again.\n"));
            fflush (stdout);
            _exit (1);
         } // if
         wrefresh (misc.titlewin);
         misc.scan_flag = 1;
         snprintf (cmd, MAX_CMD,
                   "scanimage --resolution 400 > %s/out.pnm", misc.tmp_dir);
         switch (system (cmd))
         {
         case 0:
            break;
         default:
            endwin ();
            beep ();
            puts (misc.copyright);
            puts (gettext ("eBook-speaker needs the sane-utils package."));
            fflush (stdout);
            _exit (1);
         } // switch
         snprintf (file, MAX_STR, "%s/out.pnm", misc.tmp_dir);
         break;
      }
      case 't':
         strncpy (misc.tts[misc.tts_no], argv[optind], MAX_STR - 1);
         misc.tts_flag = misc.tts_no;
         break;
      } // switch
   } // while

// check if arg is an eBook
   struct stat st_buf;

   if (misc.scan_flag == 0)
   {
      if (! argv[optind])
      // if no arguments are given
      {
         snprintf (file, MAX_STR - 1, "%s", get_input_file ("."));
         execl (*argv, *argv, file, NULL);
      }
      else
      {
         if (*argv[optind] == '/')
            snprintf (file, MAX_STR - 1, "%s", argv[optind]);
         else
            snprintf (file, MAX_STR - 1, "%s/%s", get_current_dir_name (), argv[optind]);
      } // if
   } // if
   if (stat (file, &st_buf) == -1)
   {
      int e;

      e = errno;
      endwin ();
      puts (misc.copyright);
      printf ("%s: %s\n", strerror (e), file);
      beep ();
      fflush (stdout);
      _exit (1);
   } // if
   if (chdir (misc.tmp_dir) == -1)
   {
      endwin ();
      printf ("Can't chdir (\"%s\")\n", misc.tmp_dir);
      beep ();
      fflush (stdout);
      _exit (1);
   } // if
   strncpy (misc.daisy_mp, misc.tmp_dir, MAX_STR - 1);

// determine filetype
   magic_t myt;

BEGIN:
   myt = magic_open (MAGIC_CONTINUE | MAGIC_SYMLINK);
   magic_load (myt, NULL);
   wprintw (misc.titlewin, "\nThis is a %s file", magic_file (myt, file));
   wmove (misc.titlewin, 0, 67);
   wrefresh (misc.titlewin);
   if (strcasestr (magic_file (myt, file), "gzip compressed data"))
   {
      char cmd[512], *begin, *end;
      FILE *p;

      snprintf (cmd, MAX_CMD - 1, "unar \"%s\"", file);
      p = popen (cmd, "r");
      switch ( fread (file, MAX_STR, 1, p))
      {
      default:
         break;
      } // switch
      pclose (p);
      begin = file;
      do
      {
         if (*begin == '"')
            break;
      } while (*++begin != 0);
      begin++;
      end = begin;
      do
      {
         if (*end == '"')
            break;
      } while (++end != 0);
      *end = 0;
      snprintf (file, MAX_STR, "%s/%s", get_current_dir_name (), begin);
      goto BEGIN;
   } // "gzip compressed data"
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
         beep ();
         puts (gettext ("eBook-speaker needs the man2html program."));
         fflush (stdout);
         _exit (1);
      } // switch
      snprintf (file, MAX_STR, "%s/out.html", get_current_dir_name ());
      goto BEGIN;
   } // "troff or preprocessor input"
   else
   if (strcasestr (magic_file (myt, file), "directory"))
   {
      snprintf (pwd, MAX_STR - 1, "%s", file);
      snprintf (file, MAX_STR - 1, "%s",
                get_input_file (pwd));
      execlp (*argv, *argv, file, NULL);
   } // if directory
   else
   if (strcasestr (magic_file (myt, file), "EPUB") ||
       (strcasestr (magic_file (myt, file), "Zip archive data") &&
        ! strcasestr (magic_file (myt, file), "Microsoft Word 2007+")))
   {
      read_epub (file);
   } // if epub
   else
   if (strcasestr (magic_file (myt, file), "HTML document") ||
       strcasestr (magic_file (myt, file), "XML document text"))
   {
      char cmd[MAX_CMD];

      snprintf (cmd, MAX_CMD - 1,
                "html2text -ascii -o out.txt \"%s\"", file);
      switch (system (cmd))
      {
      case 0:
         break;
      default:
         endwin ();
         beep ();
         puts (gettext ("eBook-speaker needs the xml2 package."));
         fflush (stdout);
         _exit (1);
      } // switch
      pandoc_to_epub ("out.txt");
   } // if
   else
// use pandoc
   if (strcasestr (magic_file (myt, file), "C source text") ||
       strcasestr (magic_file (myt, file), "ASCII text"))
   {
      pandoc_to_epub (file);
   } // if
   else
   if (strcasestr (magic_file (myt, file), "PDF document"))
   {
      pdf_to_html (file);
      pandoc_to_epub ("out.html");
   } // if
   else
// use lowriter
   if (strcasestr (magic_file (myt, file), "ISO-8859 text") ||
       strcasestr (magic_file (myt, file), "Microsoft Word 2007+") ||
       strcasestr (magic_file (myt, file), "Rich Text Format") ||
       strcasestr (magic_file (myt, file), "(Corel/WP)") ||
       strcasestr (magic_file (myt, file), "Composite Document File") ||
       strcasestr (magic_file (myt, file), "OpenDocument") ||
       strcasestr (magic_file (myt, file), "UTF-8 Unicode"))
   {
      pdf_to_html (lowriter_to (file, "PDF"));
      pandoc_to_epub ("out.html");
   } // if
   else
// use calibre
   if (strcasestr (magic_file (myt, file), "Microsoft Reader eBook") ||
       strcasestr (magic_file (myt, file), "AportisDoc") ||
       strcasestr (magic_file (myt, file), "Mobipocket") ||
       strcasestr (magic_file (myt, file), "BBeB ebook data") ||
       strcasestr (magic_file (myt, file), "GutenPalm zTXT") ||
       strcasestr (magic_file (myt, file), "Plucker") ||
       strcasestr (magic_file (myt, file), "PostScript document") ||
       strcasestr (magic_file (myt, file), "PeanutPress PalmOS") ||
       strcasestr (magic_file (myt, file), "MS Windows HtmlHelp Data"))
   {
      char cmd[512];

      snprintf (cmd, MAX_CMD - 1,
                "ebook-convert \"%s\" out.epub > /dev/null", file);
      switch (system (cmd))
      {
      case 0:
         break;
      default:
         endwin ();
         beep ();
         puts (gettext ("eBook-speaker needs the calibre package."));
         fflush (stdout);
         _exit (1);
      } // switch
      read_epub ("out.epub");
   }
   else
// use tesseract
   if (strcasestr (magic_file (myt, file), "JPEG image")  ||
       strcasestr (magic_file (myt, file), "GIF image")  ||
       strcasestr (magic_file (myt, file), "PNG image")  ||
       strcasestr (magic_file (myt, file), "Netpbm"))
   {
      if (access ("/usr/share/doc/tesseract-ocr", R_OK) != 0)
      {
         endwin ();
         beep ();
         puts (gettext ("eBook-speaker needs the tesseract-ocr package."));
         printf (gettext ("Please install it and try again.\n"));
         fflush (stdout);
         _exit (1);
      } // if
      start_tesseract (file);
   } // if
   else
   {
      endwin ();
      beep ();
      printf ("%s: %s\n", file, magic_file (myt, file));
      printf (gettext ("Unknown format\n"));
      fflush (stdout);
      _exit (1);
   } // if
   magic_close (myt);
   read_daisy_3 (get_current_dir_name ());
   check_phrases ();

   wclear (misc.titlewin);
   mvwprintw (misc.titlewin, 0, 0, misc.copyright);
   if ((int) strlen (misc.daisy_title) + (int)
       strlen (misc.copyright) >= misc.max_x)
      mvwprintw (misc.titlewin, 0, misc.max_x - strlen (misc.daisy_title) - 4, "... ");
   mvwprintw (misc.titlewin, 0, misc.max_x - strlen (misc.daisy_title),
              "%s", misc.daisy_title);
   mvwaddstr (misc.titlewin, 1, 0, "----------------------------------------");
   waddstr (misc.titlewin, "----------------------------------------");
   mvwprintw (misc.titlewin, 1, 0, gettext ("Press 'h' for help "));
   wrefresh (misc.titlewin);
   misc.level = 1;
   *misc.search_str = 0;
   browse (file);
   return 0;
} // main
