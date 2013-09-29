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
     
#include "src/daisy.h"

#define MY_VERSION "2.5.3"

WINDOW *screenwin, *titlewin;
daisy_t daisy[2000];
int discinfo_fp, discinfo = 00, multi = 0, displaying = 0, total_phrases;
int playing, just_this_item, total_time, tts_flag, phrase_nr;
int bytes_read, current_page_number, total_pages, tts_no = 0;
char tag[MAX_TAG], label[max_phrase_len], sound_dev[MAX_STR];
char bookmark_title[MAX_STR], opf_name[MAX_STR], prog_name[MAX_STR];
char daisy_mp[MAX_STR], copyright[MAX_STR], cddb_flag;
char ncx_name[MAX_STR], NCC_HTML[MAX_STR];
char *tmp_dir, daisy_version[25], daisy_title[MAX_STR];
char element[MAX_TAG], search_str[MAX_STR], cd_dev[MAX_STR];
pid_t player_pid, eBook_speaker_pid;
int current, max_y, max_x, total_items, level, depth;
double audio_total_length;
float speed;
char discinfo_html[MAX_STR], ncc_totalTime[MAX_STR], tts[10][MAX_STR];
time_t start_time;
DIR *dir;
my_attribute_t my_attribute;

void get_tag ();
void get_page_number ();
void view_screen ();
void player_ended ();
void play_now ();
void pause_resume ();
void help ();
void previous_item ();
void next_item ();
void skip_right ();
void read_rc ();
void save_rc ();
void kill_player ();
void go_to_page_number ();
void select_next_output_device ();
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
char *get_input_file (char *, char *, WINDOW *, WINDOW *);
void put_bookmark ();
xmlTextReaderPtr skip_left (xmlTextReaderPtr, xmlDocPtr);
void store_to_disk (xmlTextReaderPtr, xmlDocPtr);

void quit_eBook_speaker ()
{
   save_bookmark_and_xml ();
   _exit (0);
} // quit_eBook_speaker

int get_next_phrase (xmlTextReaderPtr reader, xmlDocPtr doc, int playing)
{
   FILE *w;

   if (access ("eBook-speaker.wav", R_OK) == 0)
   // when still playing
      return 0;
   while (1)
   {
      if (! get_tag_or_label (reader))
      {
         if (playing >= total_items - 1)
         {
            char str[MAX_STR];
            struct passwd *pw;;

            save_bookmark_and_xml ();
            pw = getpwuid (geteuid ());
            snprintf (str, MAX_STR - 1, "%s/.eBook-speaker/%s",
                      pw->pw_dir, bookmark_title);
            unlink (str);
            _exit (0);
         } // if
         return -1;
      } // if
      if (*daisy[playing + 1].anchor &&
          strcasecmp (my_attribute.id, daisy[playing + 1].anchor) == 0)
         return -1;
      if (*label)
         break;
   } // while

   wattron (screenwin, A_BOLD);
   mvwprintw (screenwin, daisy[playing].y, 69, "%5d %5d",
               phrase_nr + 1, daisy[playing].n_phrases - phrase_nr - 1);
   (phrase_nr)++;
   wattroff (screenwin, A_BOLD);
   wmove (screenwin, daisy[displaying].y, daisy[displaying].x);
   wrefresh (screenwin);
   if ((w = fopen ("eBook-speaker.txt", "w")) == NULL)
   {
      endwin ();
      beep ();
      printf ("Can't make a temp file %s\n", "eBook-speaker.txt");
      fflush (stdout);
      _exit (0);
   } // if
   if (fwrite (label, strlen (label), 1, w) == 0)
   {
      endwin ();
      beep ();
      printf ("write (\"%s\"): failed.\n", label);
      fflush (stdout);
      kill (getppid (), SIGINT);
   } // if
   fclose (w);

   if (! *tts[tts_no])
   {
      tts_no = 0;
      select_tts ();
   } // if
   switch (system (tts[tts_no]))
   {
   default:
      break;
   } // switch

   switch (player_pid = fork ())
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
      playfile ("eBook-speaker.wav", "wav", sound_dev, "alsa", speed);
      _exit (0);
   default:
      return 0;
   } // switch
} // get_next_phrase

void select_tts ()
{
   int n, y, x = 2;

   wclear (screenwin);
   wprintw (screenwin, gettext ("\nSelect a Text-To-Speech application\n\n"));
   for (n = 0; n < 10; n++)
   {
      char str[MAX_STR];

      if (! *tts[n])
         break;
      strncpy (str, tts[n], MAX_STR - 1);
      str[72] = 0;
      wprintw (screenwin, "    %d %s\n", n, str);
   } // for
   wprintw (screenwin, gettext (
"\nProvide a new TTS.\n\
Be sure that the new TTS reads its information from the file\n\
Book-speaker.txt and that it writes to the file eBook-speaker.wav.\n\n\
Press DEL to delete a TTS\n\n"));
   wprintw (screenwin,
           "-------------------------------------------------------------");
   y = tts_no + 3;
   nodelay (screenwin, FALSE);
   for (;;)
   {
      wmove (screenwin, y, x);
      switch (wgetch (screenwin))
      {
      int i;

      case 13:
         tts_no = y - 3;
         view_screen ();
         nodelay (screenwin, TRUE);
         return;
      case KEY_DOWN:
         if (++y == n + 3)
         {
            y = n + 10;
            x = 4;
            wmove (screenwin, y, x);
            nodelay (screenwin, FALSE);
            echo ();
            tts_no = n;
            if (tts_no > 9)
               tts_no = 9;
            wgetnstr (screenwin, tts[tts_no], 250);
            noecho ();
            if (*tts[tts_no])
            {
               view_screen ();
               nodelay (screenwin, TRUE);
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
            wmove (screenwin, y, x);
            nodelay (screenwin, FALSE);
            echo ();
            tts_no = n;
            if (tts_no > 9)
               tts_no = 9;
            wgetnstr (screenwin, tts[tts_no], 250);
            noecho ();
            if (*tts[tts_no])
            {
               view_screen ();
               nodelay (screenwin, TRUE);
               return;
            } // if
            y = 3;
            x = 2;
         } // if
         break;
      case KEY_DC:
         for (i = y - 3; i < 10; i++)
            strncpy (tts[i], tts[i + 1], MAX_STR - 1);
         tts_no = 0;
         view_screen ();
         return;
      default:
         if (x == 2)
         {
            view_screen ();
            nodelay (screenwin, TRUE);
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
      if (*label)
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
   snprintf (tmp, MAX_STR - 1, "%s/%s.tmp.xhtml", tmp_dir, basename (fname));
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
      if (! strcasecmp (tag, "/head"))
         break;
   } // while

   while (1)
   {
      if (! get_tag_or_label (local_reader))
         break;
      if (strcasecmp (tag, "a") == 0)
         continue;
      if (strcasecmp (tag, "/a") == 0)
         continue;
      if (strcasecmp (tag, "h1") == 0 ||
          strcasecmp (tag, "h2") == 0 ||
          strcasecmp (tag, "h3") == 0 ||
          strcasecmp (tag, "h4") == 0 ||
          strcasecmp (tag, "h5") == 0 ||
          strcasecmp (tag, "h6") == 0)
      {
         fprintf (w, "<%s id=\"%s\">\n<br />\n", tag, my_attribute.id);
         continue;
      } // if
      if (strcasecmp (tag, "/h1") == 0 ||
          strcasecmp (tag, "/h2") == 0 ||
          strcasecmp (tag, "/h3") == 0 ||
          strcasecmp (tag, "/h4") == 0 ||
          strcasecmp (tag, "/h5") == 0 ||
          strcasecmp (tag, "/h6") == 0)
      {
         fprintf (w, "<%s>\n<br />\n", tag);
         continue;
      } // if
      if (! *label)
         continue;
      p = label;
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
         if (! isascii (*p) || *p == '\"' || *p == '\'' || *p == '`')
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
            strncat (text, p++, 1);
            if (strlen (text) > 1)
               fprintf (w, "%s\n", text);
            fprintf (w, "<br />\n");
            *text = 0;
            phrases++;
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
               char *out_file, char *out_type, float speed)
{
   sox_format_t *in, *out; /* input and output files */
   sox_effects_chain_t *chain;
   sox_effect_t *e;
   char *args[MAX_STR], str[MAX_STR];

   sox_globals.verbosity = 0;
   sox_init ();
   if (! (in = sox_open_read (in_file, NULL, NULL, in_type)))
      _exit (0);
   if ((out = sox_open_write (out_file, &in->signal,
           NULL, out_type, NULL, NULL)) == NULL)
   {
      endwin ();
      strncpy (sound_dev, "hw:0", MAX_STR - 1);
      save_xml ();
      printf ("\nNo permission to use the soundcard.\n");
      beep ();
      kill (eBook_speaker_pid, SIGKILL);
   } // if

   chain = sox_create_effects_chain (&in->encoding, &out->encoding);

   e = sox_create_effect (sox_find_effect ("input"));
   args[0] = (char *) in, sox_effect_options (e, 1, args);
   sox_add_effect (chain, e, &in->signal, &in->signal);

   e = sox_create_effect (sox_find_effect ("tempo"));
   snprintf (str, MAX_STR, "%lf", speed);
   args[0] = str, sox_effect_options (e, 1, args);
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
             pw->pw_dir, bookmark_title);
   if (! (writer = xmlNewTextWriterFilename (str, 0)))
      return;
   xmlTextWriterStartDocument (writer, NULL, NULL, NULL);
   xmlTextWriterStartElement (writer, BAD_CAST "bookmark");
   if (playing < 0)
      xmlTextWriterWriteFormatAttribute
         (writer, BAD_CAST "item", "%d", current);
   else                                               
      xmlTextWriterWriteFormatAttribute
         (writer, BAD_CAST "item", "%d", playing);
   xmlTextWriterWriteFormatAttribute
      (writer, BAD_CAST "phrase", "%d", --phrase_nr);
   xmlTextWriterWriteFormatAttribute (writer, BAD_CAST "level", "%d", level);
   xmlTextWriterWriteFormatAttribute (writer, BAD_CAST "tts", "%d", tts_no);
   xmlTextWriterWriteFormatAttribute (writer, BAD_CAST "speed", "%f", speed);
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
             pw->pw_dir, bookmark_title);
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
   } while (strcasecmp (tag, "bookmark") != 0);
   xmlTextReaderClose (local_reader);
   xmlFreeDoc (local_doc);
   if (current < 0 || current == total_items)
      current = 0;
   if (! *daisy[current].smil_file)
      return NULL;
   reader = open_text_file (reader, doc,
                            daisy[current].smil_file, daisy[current].anchor);
   if (phrase_nr < 0)
      phrase_nr = 0;
   start = phrase_nr;
   phrase_nr = 0;
   if (level < 1)
      level = 1;
   displaying = playing = current;
   just_this_item = -1;
   if (tts_flag > -1)
      tts_no = tts_flag;
   while (1)
   {
      if (! get_tag_or_label (reader))
         break;
      if (! *label)
         continue;
      phrase_nr++;
      if (phrase_nr == start)
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
   } while (atoi (my_attribute.playorder) != current);
   do
   {
      if (! get_tag_or_label (fd))
         break;
   } while (! *label);
   current_page_number = atoi (label);
   xmlTextReaderClose (fd);
   xmlFreeDoc (doc);
} // get_page_number

void view_screen ()
{
   int i, x, l;

   mvwaddstr (titlewin, 1, 0, "----------------------------------------");
   waddstr (titlewin, "----------------------------------------");
   mvwprintw (titlewin, 1, 0, gettext ("'h' for help "));
   if (! discinfo)
   {
      if (total_pages)
         mvwprintw (titlewin, 1, 15, gettext (" %d pages "), total_pages);
      mvwprintw (titlewin, 1, 28,
         gettext (" level: %d of %d "), level, depth);
      mvwprintw (titlewin, 1, 47, gettext (" total phrases: %d "),
                 total_phrases);
      mvwprintw (titlewin, 1, 74, " %d/%d ",
              current / max_y + 1, (total_items - 1) / max_y + 1);
   } // if
   wrefresh (titlewin);

   wclear (screenwin);
   for (i = 0; daisy[i].screen != daisy[current].screen; i++);
   do
   {
      mvwprintw (screenwin, daisy[i].y, daisy[i].x + 1,
                            daisy[i].label);
      l = strlen (daisy[i].label);
      if (l / 2 * 2 != l)
         waddstr (screenwin, " ");
      for (x = l; x < 61; x += 2)
         waddstr (screenwin, " .");
      if (daisy[i].page_number)
         mvwprintw (screenwin, daisy[i].y, 65,
                    "(%3d)", daisy[i].page_number);
      l = daisy[i].n_phrases;
      x = i + 1;
      while (daisy[x].level > level)
         l += daisy[x++].n_phrases;
      if (daisy[i].level <= level)
         mvwprintw (screenwin, daisy[i].y, 75, "%5d", l);
      if (i >= total_items - 1)
         break;
   } while (daisy[++i].screen == daisy[current].screen);
   if (just_this_item != -1 &&
       daisy[displaying].screen == daisy[playing].screen)
      mvwprintw (screenwin, daisy[current].y, 0, "J");
   wmove (screenwin, daisy[current].y, daisy[current].x);
   wrefresh (screenwin);
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
   } while (strcasecmp (tag, "body") != 0);
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
   if (playing != -1)
      playing = -1;
   else
      playing = displaying;
   if (! reader)
      return;
   else
   {
      if (playing != -1)
      {
         FILE *w;

         kill (player_pid, SIGKILL);
         wattron (screenwin, A_BOLD);
         mvwprintw (screenwin, daisy[playing].y, 69, "%5d %5d",
                    phrase_nr, daisy[playing].n_phrases - phrase_nr - 1);
         wattroff (screenwin, A_BOLD);
         wmove (screenwin, daisy[displaying].y, daisy[displaying].x);
         wrefresh (screenwin);
         if ((w = fopen ("eBook-speaker.txt", "w")) == NULL)
         {
            endwin ();
            beep ();
            printf ("Can't make a temp file %s\n", "eBook-speaker.txt");
            fflush (stdout);
            _exit (0);
         } // if
         if (fwrite (label, strlen (label), 1, w) == 0)
         {
            endwin ();
            beep ();
            printf ("write (\"%s\"): failed.\n", label);
            fflush (stdout);
            kill (getppid (), SIGINT);
         } // if
         fclose (w);

         switch (system (tts[tts_no]))
         {
         default:
            break;
         } // switch

         switch (player_pid = fork ())
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
            playfile ("eBook-speaker.wav", "wav", sound_dev, "alsa", speed);
            _exit (0);
         default:
            return;
         } // switch
      }
      else
         kill_player ();
   } // if
} // pause_resume

void write_wav (int current, char *outfile)
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
      printf ("write (\"%s\"): failed.\n", label);
      fflush (stdout);
      _exit (0);
   } // if
   fclose (w);
   if (! *tts[tts_no])
   {
      tts_no = 0;
      select_tts ();
   } // if
   switch (system (tts[tts_no]))
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
                  daisy[current].smil_file, daisy[current].anchor);
   while (1)
   {
      sox_format_t *input;

      if (! get_tag_or_label (local_reader))
         break;
      if (*daisy[current + 1].anchor &&
          strcasecmp (my_attribute.id, daisy[current + 1].anchor) == 0)
         break;
      if (! *label)
         continue;;
      w = fopen ("eBook-speaker.txt", "w");
      fwrite (label, strlen (label), 1, w);
      fclose (w);
      switch (system (tts[tts_no]))
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

   getyx (screenwin, y, x);
   wclear (screenwin);
   waddstr (screenwin, gettext ("\nThese commands are available in this version:\n"));
   waddstr (screenwin, "========================================");
   waddstr (screenwin, "========================================\n\n");
   waddstr (screenwin, gettext ("cursor down     - move cursor to the next item\n"));
   waddstr (screenwin, gettext ("cursor up       - move cursor to the previous item\n"));
   waddstr (screenwin, gettext ("cursor right    - skip to next phrase\n"));
   waddstr (screenwin, gettext ("cursor left     - skip to previous phrase\n"));
   waddstr (screenwin, gettext ("page-down       - view next page\n"));
   waddstr (screenwin, gettext ("page-up         - view previous page\n"));
   waddstr (screenwin, gettext ("enter           - start playing\n"));
   waddstr (screenwin, gettext ("space           - pause/resume playing\n"));
   waddstr (screenwin, gettext ("home            - play on normal speed\n"));
   waddstr (screenwin, "\n");
   waddstr (screenwin, gettext ("Press any key for next page..."));
   nodelay (screenwin, FALSE);
   wgetch (screenwin);
   nodelay (screenwin, TRUE);
   wclear (screenwin);
   waddstr (screenwin, gettext ("\n/               - search for a label\n"));
   waddstr (screenwin, gettext ("B               - move cursor to the last item\n"));
   waddstr (screenwin, gettext ("d               - store current item to disk in WAV-format\n"));

   waddstr (screenwin, gettext ("D               - decrease playing speed\n"));
   waddstr (screenwin, gettext ("f               - find the currently playing item and place the cursor there\n"));
   waddstr (screenwin, gettext ("g               - go to phrase in current item\n"));
   waddstr (screenwin, gettext ("h or ?          - give this help\n"));
   waddstr (screenwin, gettext ("j               - just play current item\n"));
   waddstr (screenwin, gettext ("l               - switch to next level\n"));
   waddstr (screenwin, gettext ("L               - switch to previous level\n"));
   waddstr (screenwin, gettext ("n               - search forwards\n"));
   waddstr (screenwin, gettext ("N               - search backwards\n"));
   waddstr (screenwin, gettext ("o               - select next output sound device\n"));
   waddstr (screenwin, gettext ("p               - place a bookmark\n"));
   waddstr (screenwin, gettext ("q               - quit eBook-speaker and place a bookmark\n"));
   waddstr (screenwin, gettext ("s               - stop playing\n"));
   waddstr (screenwin, gettext ("t               - select next TTS\n"));
   waddstr (screenwin, gettext ("T               - move cursor to the first item\n"));
   waddstr (screenwin, gettext ("U               - increase playing speed\n"));
   waddstr (screenwin, gettext ("\nPress any key to leave help..."));
   nodelay (screenwin, FALSE);
   wgetch (screenwin);
   nodelay (screenwin, TRUE);
   view_screen ();
   wmove (screenwin, y, x);
} // help

void previous_item ()
{
   do
   {
      if (--current < 0)
      {
         beep ();
         displaying = current = 0;
         break;
      } // if
   } while (daisy[current].level > level);
   displaying = current;
   phrase_nr = daisy[current].n_phrases - 1;
   view_screen ();
} // previous_item

void next_item ()
{
   do
   {
      if (++current >= total_items)
      {
         displaying = current = total_items - 1;
         beep ();
         break;
      } // if
      displaying = current;
   } while (daisy[current].level > level);
   view_screen ();
} // next_item

xmlTextReaderPtr skip_left (xmlTextReaderPtr reader, xmlDocPtr doc)
{
   int old_phrase_nr;

   if (playing == -1)
   {
      beep ();
      return NULL;
   } // if
   old_phrase_nr = phrase_nr;
   kill_player ();
   if (phrase_nr <= 2)
   {
      if (playing == 0)
         return reader;
      playing--;
      displaying = current = playing;
      reader = open_text_file (reader, doc, daisy[playing].smil_file,
                               daisy[playing].anchor);
      phrase_nr = 0;
      while (1)
      {
         if (! get_tag_or_label (reader))
            return NULL;
         if (! *label)
            continue;
         if (phrase_nr >= daisy[playing].n_phrases - 1)
            return reader;
         phrase_nr++;
      } // while
   } // if
   reader = open_text_file (reader, doc, daisy[playing].smil_file,
                            daisy[playing].anchor);
   phrase_nr = 0;
   while (1)
   {
      if (! get_tag_or_label (reader))
         return NULL;
      if (! *label)
         continue;
      if (++phrase_nr >= old_phrase_nr - 2)
         return reader;
   } // while
} // skip_left

void skip_right ()
{
   if (playing == -1)
   {
      beep ();
      return;
   } // if
   kill_player ();
} // skip_right      

void change_level (char key)
{

   if (key == 'l')
      if (++level > depth)
         level = 1;
   if (key == 'L')
      if (--level < 1)
         level = depth;
   if (daisy[playing].level > level)
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
      strncpy (tts[0], "espeak -f eBook-speaker.txt -w eBook-speaker.wav",
               MAX_STR - 1);
      strncpy (tts[1], "flite eBook-speaker.txt eBook-speaker.wav",
               MAX_STR - 1);
      strncpy (tts[2],
               "espeak -f eBook-speaker.txt -w eBook-speaker.wav -v mb-nl2",
               MAX_STR - 1);
      strncpy (tts[3], "text2wave eBook-speaker.txt -o eBook-speaker.wav",
               MAX_STR - 1);
      strncpy (tts[4], "swift -n Lawrence -f eBook-speaker.txt -m text -o eBook-speaker.wav",
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
         if (strcasecmp (tag, "tts") == 0)
         {
            do
            {
               if (! get_tag_or_label (local_reader))
                  break;
            } while (! *label);
            for (i = 0; label[i]; i++)
               if (label[i] != ' ' && label[i] != '\n')
                  break;;
            p = label + i;
            for (i = strlen (p) - 1; i > 0; i--)
               if (p[i] != ' ' && p[i] != '\n')
                  break;
            p[i + 1] = 0;
            strncpy (tts[x++], p, MAX_STR - 1);
            if (x == 10)
               break;
         } // if
      } // while
      tts_no = x;
      xmlTextReaderClose (local_reader);
      xmlFreeDoc (doc);
   } // if
   if (*tts[0])
      return;

// If no TTS; give some examples
   strncpy (tts[0], "espeak -f eBook-speaker.txt -w eBook-speaker.wav",
            MAX_STR - 1);
   strncpy (tts[1], "flite eBook-speaker.txt eBook-speaker.wav", MAX_STR - 1);
   strncpy (tts[2],
            "espeak -f eBook-speaker.txt -w eBook-speaker.wav -v mb-nl2",
            MAX_STR - 1);
   strncpy (tts[3], "text2wave eBook-speaker.txt -o eBook-speaker.wav",
            MAX_STR - 1);
   strncpy (tts[4],
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
      (writer, BAD_CAST "sound_dev", "%s", sound_dev);
   xmlTextWriterEndElement (writer);
   xmlTextWriterWriteString (writer, BAD_CAST "\n   ");
   xmlTextWriterStartElement (writer, BAD_CAST "voices");
   while (1)
   {
      char str[MAX_STR];

      xmlTextWriterWriteString (writer, BAD_CAST "\n      ");
      xmlTextWriterStartElement (writer, BAD_CAST "tts");
      snprintf (str, MAX_STR - 1, "\n         %s\n      ", tts[x]);
      xmlTextWriterWriteString (writer, BAD_CAST str);
      xmlTextWriterEndElement (writer);
      if (! *tts[++x])
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
   if (! tmp_dir)
      return;
   snprintf (cmd, MAX_CMD - 1, "rm -rf %s", tmp_dir);          
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
      if (playing != -1)
         kill_player ();
      mvwaddstr (titlewin, 1, 0, "----------------------------------------");
      waddstr (titlewin, "----------------------------------------");
      mvwprintw (titlewin, 1, 0, gettext ("What do you search?                           "));
      echo ();
      wmove (titlewin, 1, 20);
      wgetnstr (titlewin, search_str, 25);
      noecho ();
   } // if
   if (mode == '/' || mode == 'n')
   {
      for (c = start; c < total_items; c++)
      {
         if (strcasestr (daisy[c].label, search_str))
         {
            found = 1;
            break;
         } // if
      } // for
      if (! found)
      {
         for (c = 0; c < start; c++)
         {
            if (strcasestr (daisy[c].label, search_str))
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
         if (strcasestr (daisy[c].label, search_str))
         {
            found = 1;
            break;
         } // if
      } // for
      if (! found)
      {
         for (c = total_items + 1; c > start; c--)
         {
            if (strcasestr (daisy[c].label, search_str))
            {
               found = 1;
               break;
            } // if
         } // for
      } // if
   } // if
   if (found)
   {
      current = c;
      phrase_nr = 0;
      just_this_item = -1;
      displaying = playing = current;
      if (playing != -1)
         kill_player ();
      xmlTextReaderClose (reader);
      xmlFreeDoc (doc);
      reader = open_text_file (reader, doc, daisy[current].smil_file,
                      daisy[current].anchor);
   }
   else
   {
      beep ();                                            
      kill_player ();
      phrase_nr--;
      get_next_phrase (reader, doc, playing);
   } // if
   view_screen ();
} // search

void kill_player ()
{
   kill (player_pid, SIGKILL);
   unlink ("eBook-speaker.txt");
   unlink ("eBook-speaker.wav");
} // kill_player

void select_next_output_device ()
{
   FILE *r;
   int n, y;
   size_t bytes;
   char *list[10], *trash;

   wclear (screenwin);
   wprintw (screenwin, gettext ("\nSelect a soundcard:\n\n"));
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
      wprintw (screenwin, "   %s", list[n]);
      free (list[n]);
   } // for          
   fclose (r);
   y = 3;
   nodelay (screenwin, FALSE);
   for (;;)
   {
      wmove (screenwin, y, 2);
      switch (wgetch (screenwin))
      {
      case 13: //
         snprintf (sound_dev, MAX_STR - 1, "hw:%i", y - 3);
         view_screen ();
         nodelay (screenwin, TRUE);
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
         nodelay (screenwin, TRUE);
         return;
      } // switch
   } // for
} // select_next_output_device

xmlTextReaderPtr go_to_phrase (xmlTextReaderPtr reader, xmlDocPtr doc)
{
   char pn[15];

   kill_player ();
   player_pid = -2;
   mvwaddstr (titlewin, 1, 0, "----------------------------------------");
   waddstr (titlewin, "----------------------------------------");
   mvwprintw (titlewin, 1, 0, gettext ("Go to phrase in current item: "));
   echo ();
   wgetnstr (titlewin, pn, 8);
   noecho ();
   view_screen ();
   if (! *pn || atoi (pn) > daisy[current].n_phrases + 1)
   {
      beep ();
      reader = skip_left (reader, doc);
      skip_right ();
      return reader;  
   } // if
   reader = open_text_file (reader, doc, daisy[current].smil_file,
                            daisy[current].anchor);
   phrase_nr = 0;
   if (atoi (pn) > 1)
   {
      while (1)
      {
         if (! get_tag_or_label (reader))
            break;
         if (! *label)
            continue;
         (phrase_nr)++;
         if (phrase_nr + 1 == atoi (pn))
            break;
      } // while
   } // if
   playing = displaying = current;
   view_screen ();
   mvwprintw (screenwin, daisy[current].y, 69, "%5d %5d",
              phrase_nr + 1, daisy[current].n_phrases - phrase_nr - 1);
   wrefresh (screenwin);
   wmove (screenwin, daisy[current].y, daisy[current].x);
   just_this_item = -1;
   remove ("eBook-speaker.wav");
   remove ("old.wav");
   return reader;
} // go_to_phrase

void browse (int scan_flag)
{
   int old;
   xmlTextReaderPtr reader;
   xmlDocPtr doc;

   current = displaying = playing = 0;
   just_this_item = playing = -1;
   reader = NULL;
   doc = NULL;
   if (! scan_flag)
      reader = get_bookmark (reader, doc);
   if (depth <= 0)
      depth = 1;
   view_screen ();
   nodelay (screenwin, TRUE);
   wmove (screenwin, daisy[current].y, daisy[current].x);
   signal (SIGCHLD, player_ended);
   for (;;)
   {
      switch (wgetch (screenwin))
      {
      case 13: // ENTER
         playing = displaying = current;
         remove ("eBook-speaker.wav");
         remove ("old.wav");
         phrase_nr = 0;
         just_this_item = -1;
         view_screen ();
         kill_player ();
         reader = open_text_file (reader, doc, daisy[current].smil_file,
                                  daisy[current].anchor);
         break;
      case '/':
         search (reader, doc, current + 1, '/');
         break;
      case ' ':
      case KEY_IC:
      case '0':
         pause_resume (reader, doc);
         break;
      case 'B':
         displaying = current = total_items - 1;
         phrase_nr = daisy[current].n_phrases - 1;
         view_screen ();
         break;
      case 'f':
         if (playing == -1)
         {
            beep ();
            break;
         } // if
         current = displaying= playing;
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
         if (playing != -1)
         {
            phrase_nr--;
            get_next_phrase (reader, doc, playing);
         } // if
         break;
      case 'j':
      case '5':
      case KEY_B2:
         if (just_this_item != -1)
            just_this_item = -1;
         else
         {
            if (playing == -1)
            {
               phrase_nr = 0;
               reader = open_text_file (reader, doc, daisy[current].smil_file,
                                        daisy[current].anchor);
            } // if
            playing = just_this_item = current;
         } // if
         mvwprintw (screenwin, daisy[current].y, 0, " ");
         if (playing == -1)
         {
            just_this_item = displaying = playing = current;
            phrase_nr = 0;
            kill_player ();
            reader = open_text_file (reader, doc, daisy[current].smil_file,
                            daisy[current].anchor);
         } // if
         if (just_this_item != -1 &&
             daisy[displaying].screen == daisy[playing].screen)
            mvwprintw (screenwin, daisy[current].y, 0, "J");
         else
            mvwprintw (screenwin, daisy[current].y, 0, " ");
         wrefresh (screenwin);
         break;
      case 'l':
         change_level ('l');
         break;
      case 'L':
         change_level ('L');
         break;
      case 'n':
         search (reader, doc, current + 1, 'n');
         break;
      case 'N':
         search (reader, doc, current - 1, 'N');
         break;
      case 'o':
         if (playing == -1)
         {
            beep ();
            break;
         } // if
         pause_resume (reader, doc);
         select_next_output_device ();
         playing = displaying;
         kill_player ();
         if (playing != -1)
         {
            phrase_nr--;
            get_next_phrase (reader, doc, playing);
         } // if
         break;
      case 'p':
         put_bookmark ();
         break;
      case 'q':
         quit_eBook_speaker ();
      case 's':
         playing = just_this_item = -1;
         view_screen ();
         kill_player ();
         break;
      case 't':

         if (playing != -1)
            pause_resume (reader, doc);
         select_tts ();
         playing = displaying;
         kill_player ();
         if (playing != -1)
         {
            phrase_nr--;
            get_next_phrase (reader, doc, playing);
         } // if
         break;
      case 'T':
         displaying = current = 0;
         phrase_nr = daisy[current].n_phrases - 1;
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
         if (current / max_y == (total_items - 1) / max_y)
         {
            beep ();
            break;
         } // if
         old = current / max_y;
         while (current / max_y == old)
            next_item ();
         view_screen ();
         break;
      case KEY_PPAGE:
      case '9':
         if (current / max_y == 0)
         {
            beep ();
            break;
         } // if
         old = current / max_y - 1;
         current = 0;
         while (current / max_y != old)
            next_item ();
         view_screen ();
         break;
      case ERR:
         break;
      case 'U':
      case '+':
         if (speed >= 10)
         {
            beep ();
            break;
         } // if
         speed += 0.1;
         if (playing == -1)
            break;
         kill_player ();
         phrase_nr--;
         get_next_phrase (reader, doc, playing);
         break;
      case 'd':
         store_to_disk (reader, doc);
         break;
      case 'D':
      case '-':
         if (speed <= 0.1)
         {
            beep ();
            break;
         } // if
         speed -= 0.1;
         if (playing == -1)
            break;
         kill_player ();
         phrase_nr--;
         get_next_phrase (reader, doc, playing);
         break;
      case KEY_HOME:
      case '*':
      case '7':
         speed = 1.0;
         if (playing == -1)
            break;
         kill_player ();
         phrase_nr--;
         get_next_phrase (reader, doc, playing);
         break;
      default:
         beep ();
         break;
      } // switch

      if (playing != -1)
      {
         if (get_next_phrase (reader, doc, playing) == -1)
         {
            phrase_nr = 0;
            playing++;
            if (daisy[playing].level <= level)
               current = displaying = playing;
            if (just_this_item != -1 && daisy[playing].level <= level)
               playing = just_this_item = -1;
            remove ("eBook-speaker.wav");
            remove ("old.wav");
            reader = open_text_file (reader, doc, daisy[current].smil_file,
                                     daisy[current].anchor);
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

   snprintf (cmd, MAX_CMD - 1, "unzip -qq \"%s\"", file);
   switch (system (cmd))
   {
   default:
      break;
   } // switch
   switch (chdir (daisy_mp))
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
      puts (copyright);
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

   total_phrases = 0;
   for (item = 0; item < total_items; item++)
   {
      strncpy (daisy[item].smil_file,
               re_organize (daisy[item].smil_file), MAX_STR - 1);
   } // for
   for (item = 0; item < total_items; item++)
   {
      daisy[item].n_phrases =
         count_phrases (daisy[item].smil_file, daisy[item].anchor,
                        daisy[item + 1].smil_file,
                        daisy[item + 1].anchor) - 1;
      total_phrases += daisy[item].n_phrases;
   } // for
} // check_phrases

void store_to_disk (xmlTextReaderPtr reader, xmlDocPtr doc)
{
   char str[MAX_STR], s2[MAX_STR];
   int i, pause_flag;
   struct passwd *pw;;

   pause_flag = playing;
   wclear (screenwin);
   snprintf (str, MAX_STR - 1, "%s.wav", daisy[current].label);
   for (i = 0; str[i] != 0; i++)
   {
      if (str[i] == '/')
         str[i] = '-';
      if (isspace (str[i]))
         str[i] = '_';
   } // for
   wprintw (screenwin,
            "\nStoring \"%s\" as \"%s\" into your home-folder...",
            daisy[current].label, str);
   wrefresh (screenwin);
   pw = getpwuid (geteuid ());
   snprintf (s2, MAX_STR - 1, "%s/%s", pw->pw_dir, str);
   if (pause_flag > -1)
      pause_resume (reader, doc);
   write_wav (current, s2);
   if (pause_flag > -1)
      pause_resume (reader, doc);
   view_screen ();
} // store_to_disk

int main (int argc, char *argv[])
{
   int opt, scan_flag = 0;
   char file[MAX_STR], str[MAX_STR];

   fclose (stderr);
   setbuf (stdout, NULL);
   if (setlocale (LC_ALL, getenv ("LC_ALL")) == NULL)
   {
      int e;
      
      e = errno;
      endwin ();
      printf ("something wrong with your locale: %s\n", strerror (e));
      _exit (0);
   } // if
   strncpy (prog_name, basename (argv[0]), MAX_STR - 1);
   textdomain (prog_name);
   snprintf (str, MAX_STR, "%s/", LOCALEDIR);
   bindtextdomain (prog_name, str);
   initscr ();
   titlewin = newwin (2, 80,  0, 0);
   screenwin = newwin (23, 80, 2, 0);
   getmaxyx (screenwin, max_y, max_x);
   max_y--;
   keypad (screenwin, TRUE);
   meta (screenwin,       TRUE);
   nonl ();
   noecho ();
   eBook_speaker_pid = getpid ();
   player_pid = -2;
   snprintf (copyright, MAX_STR - 1, gettext
             ("eBook-speaker - Version %s - (C)2013 J. Lemmens"), MY_VERSION);
   wattron (titlewin, A_BOLD);
   wprintw (titlewin, "%s - %s", copyright, gettext ("Please wait..."));
   if (access ("/usr/share/doc/espeak", R_OK) != 0)
   {
      endwin ();
      beep ();
      puts (gettext ("eBook-speaker needs the espeak package."));
      printf (gettext ("Please install it and try again.\n"));
      fflush (stdout);
      _exit (1);
   } // if
   speed = 1.0;
   atexit (quit_eBook_speaker);
   strncpy (sound_dev, "hw:0", MAX_STR - 1);
   read_xml ();
   tmp_dir = strdup ("/tmp/eBook-speaker.XXXXXX");
   if (! mkdtemp (tmp_dir))
   {
      endwin ();
      printf ("mkdtemp ()\n");
      beep ();
      fflush (stdout);
      _exit (1);
   } // if
   opterr = 0;
   tts_flag = -1;
   while ((opt = getopt (argc, argv, "lst")) != -1)
   {
      switch (opt)
      {
      case 'l':
         continue;
      case 's':
      {
         char cmd[MAX_CMD];

         wrefresh (titlewin);
         scan_flag = 1;
         snprintf (cmd, MAX_CMD,
                   "scanimage --resolution 400 > %s/out.pnm", tmp_dir);
         switch (system (cmd))
         {
         case 0:
            break;
         default:
            endwin ();
            beep ();
            puts (copyright);
            puts (gettext ("eBook-speaker needs the sane-utils package."));
            fflush (stdout);
            _exit (1);
         } // switch
         snprintf (file, MAX_STR, "%s/out.pnm", tmp_dir);
         break;
      }
      case 't':
         strncpy (tts[tts_no], argv[optind], MAX_STR - 1);
         tts_flag = tts_no;
         break;
      } // switch
   } // while

// check if arg is an eBook
   struct stat st_buf;

   if (! scan_flag)
   {
      if (! argv[optind])
      {
         snprintf (file, MAX_STR - 1, "%s",
                   get_input_file (".", copyright, titlewin,
                   screenwin));
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
      puts (copyright);
      printf ("stat: %s: %s\n", strerror (e), file);
      beep ();
      fflush (stdout);
      _exit (1);
   } // if
   if (chdir (tmp_dir) == -1)
   {
      endwin ();
      printf ("Can't chdir (\"%s\")\n", tmp_dir);
      beep ();
      fflush (stdout);
      _exit (1);
   } // if
   strncpy (daisy_mp, tmp_dir, MAX_STR - 1);

// determine filetype
   magic_t myt;

   myt = magic_open (MAGIC_CONTINUE | MAGIC_SYMLINK);
   magic_load (myt, NULL);
   wprintw (titlewin, "\nThis is a %s file", magic_file (myt, file));
   wmove (titlewin, 0, 67);
   wrefresh (titlewin);
   if (strcasestr (magic_file (myt, file), "directory"))
   {
      switch (chdir (file))
      {
      default:
         break;
      } // switch
      snprintf (file, MAX_STR - 1, "%s",
                get_input_file (".", copyright, titlewin,
                                screenwin));
      argv[1] = file;
      main (1, argv);
      _exit (0);
   } // if directory
   else
   if (strcasestr (magic_file (myt, file), "EPUB") ||
       (strcasestr (magic_file (myt, file), "Zip archive data") &&
        ! strcasestr (magic_file (myt, file), "Microsoft Word 2007+")))
   {
      read_epub (file);
   } // if epub
   else
   if (strcasestr (magic_file (myt, file), "HTML document"))
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
      char cmd[MAX_CMD];

      snprintf (cmd, MAX_CMD - 1,
                "tesseract \"%s\" out 2> /dev/null", file);
      switch (system (cmd))
      {
      case 0:
         break;
      default:
         endwin ();
         beep ();
         puts (copyright);
         puts (gettext ("eBook-speaker needs the tesseract-ocr package."));
         fflush (stdout);
         _exit (1);
      } // switch
      pandoc_to_epub ("out.txt");
   } // if
   else
   {
      endwin ();
      beep ();
      puts ("\n\n");
      printf (gettext ("Unknown format\n"));
      fflush (stdout);
      _exit (1);
   } // if
   magic_close (myt);
   read_daisy_3 (get_current_dir_name ());
   check_phrases ();

   wclear (titlewin);
   mvwprintw (titlewin, 0, 0, copyright);
   if ((int) strlen (daisy_title) + (int) strlen (copyright) >= max_x)
      mvwprintw (titlewin, 0, max_x - strlen (daisy_title) - 4, "... ");
   mvwprintw (titlewin, 0, max_x - strlen (daisy_title), "%s", daisy_title);
   mvwaddstr (titlewin, 1, 0, "----------------------------------------");
   waddstr (titlewin, "----------------------------------------");
   mvwprintw (titlewin, 1, 0, gettext ("Press 'h' for help "));
   wrefresh (titlewin);
   level = 1;
   *search_str = 0;
   browse (scan_flag);
   return 0;
} // main

