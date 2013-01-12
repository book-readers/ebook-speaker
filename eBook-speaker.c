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
#include "eBook-speaker.h"

#define VERSION "2.3.1"

WINDOW *screenwin, *titlewin;
daisy_t daisy[2000];
xmlTextReaderPtr reader;
int discinfo_fp, discinfo = 00, multi = 0, displaying = 0;
int playing, just_this_item, phrase_nr;
int bytes_read, current_page_number, total_pages, tts_no = 0;
char tag[MAX_TAG], label[max_phrase_len], sound_dev[MAX_STR];
char bookmark_title[MAX_STR];
char daisy_mp[MAX_STR];
char *tmp_dir, daisy_version[25], daisy_title[MAX_STR], NCC_HTML[MAX_STR];
char ncx_name[MAX_STR], opf_name[MAX_STR];
char element[MAX_TAG], search_str[MAX_STR];
pid_t player_pid;
char prog_name[MAX_STR];
int current, max_y, max_x, total_items, level, depth;
double audio_total_length;
float speed;
char OPF[MAX_STR], discinfo_html[MAX_STR], ncc_totalTime[MAX_STR];
char  NCX[MAX_STR], tts[10][MAX_STR];
time_t start_time;
DIR *dir;
my_attribute_t my_attribute;

void get_clips (char *, char *);
void save_xml ();
void parse_opf (char *, char *);
void select_tts ();
char *open_opf ();
void parse_smil_3 ();

int read_text (int playing, int phrase_nr)
{
   char cmd[MAX_CMD], str[MAX_STR];
   int nr, w;

   open_text_file (daisy[playing].smil_file,
                   daisy[playing].anchor);
   nr = 0;
   while (1)
   {
      if (! get_tag_or_label (reader))
      {
         if (playing >= total_items - 1)
         {
            struct passwd *pw;;

            quit_eBook_speaker ();
            pw = getpwuid (geteuid ());
            snprintf (str, MAX_STR - 1, "%s/.eBook-speaker/%s",
                      pw->pw_dir, bookmark_title);
            unlink (str);
            _exit (0);
         } // if
         return EOF;
      } // if
      if (*daisy[playing + 1].anchor &&
          strcasecmp (my_attribute.id, daisy[playing + 1].anchor) == 0)
         return EOF;
      if (! *label)
         continue;
      if (*label == '.' || *label == ',' || *label == '\'') // Acapela
         *label = ' ';
// if label only contains spaces then get next label
      if (strncmp (label, "                        ", MAX_STR - 1) == 0)
         continue;
      if (nr++ == phrase_nr)
         break;
   } // while
   snprintf (cmd, MAX_CMD - 1, "%s", tts[tts_no]);
   if (! *cmd)
   {
      tts_no = 0;
      select_tts ();
   } // if
      switch (player_pid = fork ())
      {
      case 01:
         endwin ();
         playfile (PREFIX"share/eBook-speaker/error.wav", "1");
         puts ("fork()");
         fflush (stdout);
         _exit (1);
   case 0:
      player_pid = setsid ();
      break;
   default:
      return ! EOF;
   } // switch

   wattron (screenwin, A_BOLD);
   mvwprintw (screenwin, daisy[playing].y, 69, "%5d %5d",
              phrase_nr + 1, daisy[playing].n_phrases - phrase_nr);
   wattroff (screenwin, A_BOLD);
   wmove (screenwin, daisy[displaying].y, daisy[displaying].x);
   wrefresh (screenwin);
   if (speed < 0.1)
      speed = 1;
   snprintf (str, MAX_STR - 1, "%lf", speed);
   if ((w = open ("eBook-speaker.txt",
                  O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) == -1)
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf ("Can't make a temp file %s\n", "eBook-speaker.txt");
      fflush (stdout);
      kill (getppid (), SIGINT);
   } // if
   if (write (w, label, strlen (label)) == -1)
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf ("write (\"%s\"): failed.\n", label);
      fflush (stdout);
      kill (getppid (), SIGINT);
   } // if
   close (w);
   switch (system (cmd))
   {
   default:
      break;
   } // switch
   playfile ("eBook-speaker.wav", str);
   _exit (0);
} // read_text                                           

void read_daisy_3 (char *daisy_mp)
{
   DIR *dir;
   struct dirent *dirent;

   if (! (dir = opendir (daisy_mp)))
   {
      endwin ();
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      printf (gettext ("\nCannot read %s\n"), daisy_mp);
      fflush (stdout);
      _exit (1);
   } // if
   while ((dirent = readdir (dir)) != NULL)
   {
      if (strcasecmp (dirent->d_name +
                      strlen (dirent->d_name) - 4, ".ncx") == 0)
         strncpy (ncx_name, dirent->d_name, MAX_STR - 1);
      if (strcasecmp (dirent->d_name +
                      strlen (dirent->d_name) - 4, ".opf") == 0)
         strncpy (opf_name, dirent->d_name, MAX_STR - 1);
   } // while
   closedir (dir);
   parse_opf (opf_name, "");
   total_items = current;
} // read_daisy_3

void select_tts ()
{
   int n, y, x = 2;

   wclear (screenwin);
   wprintw (screenwin, "\nSelect a Text-To-Speech application\n\n");
   for (n = 0; n < 10; n++)
   {
      char str[MAX_STR];

      if (! *tts[n])
         break;
      strncpy (str, tts[n], MAX_STR - 1);
      str[72] = 0;
      wprintw (screenwin, "    %d %s\n", n, str);
   } // for
   wprintw (screenwin, "\n\
    Provide a new TTS.\n\
    Be sure that the new TTS reads its information from the file\n\
    eBook-speaker.txt and that it writes to the file eBook-speaker.wav.\n\n\
    -------------------------------------------------------------");
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
            y = n + 8;
            x = 4;
            wmove (screenwin, y, x);
            nodelay (screenwin, FALSE);
            echo ();
            tts_no = n;
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
            y = n + 8;
            x = 4;
            wmove (screenwin, y, x);
            nodelay (screenwin, FALSE);
            echo ();
            tts_no = n;
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
   xmlDocPtr doc;

   if (! *f_file)
      return 0;
   doc = xmlRecoverFile (f_file);
   if (! (r = xmlReaderWalker (doc)))
   {
      endwin ();
      printf (gettext ("%s\n"), strerror (errno));
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      fflush (stdout);
      _exit (1);
   } // if
   n_phrases = 1;
   if (*f_anchor)
   {
      do
      {
         if (! get_tag_or_label (r))
         {
// if f_anchor couldn't be found, start from the beginning
            xmlTextReaderClose (r);
            xmlFreeDoc (doc);
            doc = xmlRecoverFile (f_file);
            r = xmlReaderWalker (doc);
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
         xmlFreeDoc (doc);
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
               xmlFreeDoc (doc);
               return n_phrases;
            } // if
         } // if
      } // if
   } // while
} // count_phrases

char *re_organize (char *fname)
{
   xmlTextReaderPtr reader;
   static char tmp[MAX_STR];
   char *p;
   xmlDocPtr doc;
   FILE *w;

   doc = xmlRecoverFile (fname);
   if (! (reader = xmlReaderWalker (doc)))
   {
      endwin ();
      printf (gettext ("%s\n"), strerror (errno));
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      fflush (stdout);
      _exit (1);
   } // if
   snprintf (tmp, MAX_STR - 1, "%s/%s.tmp.xhtml", tmp_dir, basename (fname));
   if (! (w = fopen (tmp, "w")))
   {
      endwin ();
      printf (gettext ("%s\n"), strerror (errno));
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      fflush (stdout);
      _exit (1);
   } /* if */
   fprintf (w, "<html>\n<head>\n</head>\n<body>\n");

   while (1)
   {
      if (! get_tag_or_label (reader))
         break;
      if (! strcasecmp (tag, "body"))
         break;
   } // while

   while (1)
   {
      if (! get_tag_or_label (reader))
         break;
      if (strcasecmp (tag, "a") == 0)
      {
         fprintf (w, "<%s id=\"%s\">", tag, my_attribute.id);
         continue;
      } // if
      if (strcasecmp (tag, "/a") == 0)
      {
         fprintf (w, "<%s>\n", tag);
         continue;
      } // if
      if (strcasecmp (tag, "h1") == 0 ||
          strcasecmp (tag, "h2") == 0 ||
          strcasecmp (tag, "h3") == 0 ||
          strcasecmp (tag, "h4") == 0 ||
          strcasecmp (tag, "h5") == 0 ||
          strcasecmp (tag, "h6") == 0)
      {
         fprintf (w, "<%s id=\"%s\">", tag, my_attribute.id);
         continue;
      } // if
      if (strcasecmp (tag, "/h1") == 0 ||
          strcasecmp (tag, "/h2") == 0 ||
          strcasecmp (tag, "/h3") == 0 ||
          strcasecmp (tag, "/h4") == 0 ||
          strcasecmp (tag, "/h5") == 0 ||
          strcasecmp (tag, "/h6") == 0)
      {
         fprintf (w, "<%s>\n", tag);
         continue;
      } // if
      if (! *label)
         continue;
      p = label;
      while (*p == ' ')
         p++;
      while (1)
      {
         if (*p == 0)
         {
            fprintf (w, "<br />\n");
            break;
         } // if
         if (*p == '\n')
         {
            fprintf (w, "<br />\n");
            p++;
            continue;
         } // if
         if (! isascii (*p) || *p == '\"' || *p == '\'')
         {
            fprintf (w, " ");
            p++;
            continue;
         } // if
         if (*p == '.' || *p == ',' || *p == '!' || *p == '?' ||
             *p == ':' || *p == ';')
         {
            fprintf (w, "%c<br />\n", *p++);
            continue;
         } // if
         fprintf (w, "%c", *p++);
      } // while
   } // while
   fprintf (w, "</body>\n</html>\n");
   xmlTextReaderClose (reader);
   xmlFreeDoc (doc);
   fclose (w);
   return tmp;
} // re_organize

void playfile (char *filename, char *tempo)
{
  sox_format_t *in, *out; /* input and output files */
  sox_effects_chain_t *chain;
  sox_effect_t *e;
  char *args[MAX_STR], str[MAX_STR];

  sox_globals.verbosity = 0;
  if (sox_init ())
     return;
  if (! (in = sox_open_read (filename, NULL, NULL, NULL)))
     return;
  while (! (out =
         sox_open_write (sound_dev, &in->signal, NULL, "alsa", NULL, NULL)))
  {
    strncpy (sound_dev, "default", MAX_STR - 1);
    save_xml ();
    if (out)
      sox_close (out);
  } // while

  chain = sox_create_effects_chain (&in->encoding, &out->encoding);

  e = sox_create_effect (sox_find_effect ("input"));
  args[0] = (char *) in, sox_effect_options (e, 1, args);
  sox_add_effect (chain, e, &in->signal, &in->signal);

  e = sox_create_effect (sox_find_effect ("tempo"));
  args[0] = tempo, sox_effect_options (e, 1, args);
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
} // playfile

void put_bookmark ()
{
   char str[MAX_STR];
   xmlTextWriterPtr writer;
   struct passwd *pw;;

   pw = getpwuid (geteuid ());
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker", pw->pw_dir);
   mkdir (str, 0755);
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker/%s", pw->pw_dir, bookmark_title);
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

void get_bookmark ()
{
   char str[MAX_STR];
   xmlTextReaderPtr reader;
   xmlDocPtr doc;
   struct passwd *pw;;

   pw = getpwuid (geteuid ());
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker/%s", pw->pw_dir, bookmark_title);
   doc = xmlRecoverFile (str);
   if (! (reader = xmlReaderWalker (doc)))
      return;
   do
   {
      if (! get_tag_or_label (reader))
         break;
   } while (strcasecmp (tag, "bookmark") != 0);
   xmlTextReaderClose (reader);
   xmlFreeDoc (doc);
   if (current < 0 || current == total_items)
      current = 0;
   if (! *daisy[current].smil_file)
      return;
   open_text_file (daisy[current].smil_file,
                   daisy[current].anchor);
   if (phrase_nr < 0)
      phrase_nr = 0;
   if (level < 1)
      level = 1;
   displaying = playing = current;
   just_this_item = -1;
} // get_bookmark

void get_page_number (char *NCX)
{
   xmlTextReaderPtr fd;

   xmlDocPtr doc = xmlRecoverFile (NCX);
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
      mvwprintw (titlewin, 1, 28,
                 gettext (" level: %d of %d "), level, depth);
      if (total_pages)
         mvwprintw (titlewin, 1, 15, gettext (" %d pages "), total_pages);
      mvwprintw (titlewin, 1, 74, " %d/%d ", \
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
         l += daisy[x++].n_phrases + 1;
      if (daisy[i].level <= level)
         mvwprintw (screenwin, daisy[i].y, 75, "%5d", l + 1);
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

void open_text_file (char *text_file, char *anchor)
{
   xmlDocPtr doc;

   doc = xmlRecoverFile (text_file);
   if (! (reader = xmlReaderWalker (doc)))
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf ("open_text_file(): %s\n", text_file);
      fflush (stdout);
      _exit (1);
   } // if
   do
   {
      if (! get_tag_or_label (reader))
         break;
   } while (strcasecmp (tag, "body") != 0);
   if (! *anchor)
      return;

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
} // open_text_file

void pause_resume ()
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
         kill_player ();
         read_text (playing, phrase_nr - 1);
      }
      else
         kill_player ();
   } // if
} // pause_resume

void write_wav (char *infile, char *outfile)
{
  sox_format_t *out = NULL;
  sox_format_t *in;
  sox_sample_t samples[2048];
  size_t number_read;
  char str[MAX_STR];
  struct passwd *pw;;

  pw = getpwuid (geteuid ());
  in = sox_open_read (infile, NULL, NULL, NULL);
  snprintf (str, MAX_STR - 1, "%s/%s", pw->pw_dir, outfile);
  out = sox_open_write (str, &in->signal, NULL, NULL, NULL, NULL);
  while ((number_read = sox_read (in, samples, 2048)))
    sox_write (out, samples, number_read);
  sox_close (in);
  sox_close (out);
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

void skip_left ()
{
   if (playing == -1)
   {
      beep ();
      return;
   } // if
   kill_player ();
   open_text_file (daisy[playing].smil_file,
                   daisy[playing].anchor);
   while (1)
   {
      if (! get_tag_or_label (reader))
         break;
      if (! *label)
         continue;
      phrase_nr -= 2;
      if (phrase_nr == -1)
      {
         if (playing == 0) // first item
         {
            phrase_nr = 0;
            break;
         } // if
         current = playing;
         previous_item ();
         playing = displaying = current;
         phrase_nr = daisy[playing].n_phrases - 1;
         break;
      }
      else
         break;
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
   xmlTextReaderPtr reader;
   xmlDocPtr doc;

   pw = getpwuid (geteuid ());
   snprintf (str, MAX_STR - 1, "%s/.eBook-speaker.xml", pw->pw_dir);
   if (! (doc = xmlRecoverFile (str)))
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
      save_xml ();
      return;
   } // if
   if ((reader = xmlReaderWalker (doc)))
   {
      while (1)
      {
         if (! get_tag_or_label (reader))
            break;
         if (strcasecmp (tag, "tts") == 0)
         {
            do
            {
               if (! get_tag_or_label (reader))
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
            if (x == 8)
               break;
         } // if
      } // while
      xmlTextReaderClose (reader);
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

void quit_eBook_speaker ()
{
   char cmd[MAX_CMD];

   endwin ();
   kill_player ();
   wait (NULL);
   put_bookmark ();
   save_xml ();
   xmlTextReaderClose (reader);
   xmlCleanupParser ();
   if (! tmp_dir)
      return;
   snprintf (cmd, MAX_CMD - 1, "rm -rf %s", tmp_dir);
   if (system (cmd) > 0)
   {
      endwin ();
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      _exit (1);
   } // if
} // quit_eBook_speaker

void search (int start, char mode)
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
      open_text_file (daisy[current].smil_file,
                      daisy[current].anchor);
   }
   else
   {
      beep ();
      kill_player ();
      read_text (playing, phrase_nr - 1);
   } // if
   view_screen ();
} // search

void kill_player ()
{
   killpg (player_pid, SIGKILL);
} // kill_player

void select_next_output_device ()
{
   FILE *r; 
   int n, y;
   size_t bytes;
   char *list[10], *trash;

   wclear (screenwin);
   wprintw (screenwin, "\nSelect a soundcard:\n\n");
   if (! (r = fopen ("/proc/asound/cards", "r")))
   {
      endwin ();
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      puts (gettext ("Cannot read /proc/asound/cards"));
      fflush (stdout);
      _exit (1);
   } // if
   for (n = 0; n < 10; n++)
   {
      list[n] = (char *) malloc (1000);
      bytes = getline (&list[n], &bytes, r);
      if (bytes == -1)
         break;
      trash = (char *) malloc (1000);
      bytes = getline (&trash, &bytes, r);
      free (trash);
      wprintw (screenwin, "   %s", list[n]);
   } // for
   fclose (r);
   y = 3;
   nodelay (screenwin, FALSE);
   for (;;)
   {
      wmove (screenwin, y, 2);
      switch (wgetch (screenwin))
      {
      case 13:
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

void go_to_phrase ()
{
   char pn[15];

   kill (player_pid, SIGKILL);
   player_pid = -2;
   mvwaddstr (titlewin, 1, 0, "----------------------------------------");
   waddstr (titlewin, "----------------------------------------");
   mvwprintw (titlewin, 1, 0, gettext ("Go to phrase in current item: "));
   echo ();
   wgetnstr (titlewin, pn, 8);
   noecho ();
   view_screen ();
   if (! *pn || atoi (pn) > daisy[current].n_phrases || ! atoi (pn))
   {
      beep ();
      skip_left ();
      skip_right ();
      return;
   } // if
   xmlTextReaderClose (reader);
   xmlDocPtr doc = xmlRecoverFile (daisy[current].smil_file);
   if (! (reader = xmlReaderWalker (doc)))
   {
      endwin ();
      printf (gettext ("\nCannot read %s\n"), daisy[current].smil_file);
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      fflush (stdout);
      _exit (1);
   } // if
   phrase_nr = 0;
   while (1)
   {
      if (! get_tag_or_label (reader))
         return;
      if (*label)
      {
         if (phrase_nr == atoi (pn) - 1)
            break;
         phrase_nr++;
      } // if
   } // while
   playing = displaying = current;
   view_screen ();
   wmove (screenwin, daisy[current].y, daisy[current].x);
   just_this_item = -1;
} // go_to_phrase

void browse ()
{
   int old;

   current = displaying = playing = 0;
   just_this_item = playing = -1;
   reader = NULL;
   get_bookmark ();
   view_screen ();
   nodelay (screenwin, TRUE);
   wmove (screenwin, daisy[current].y, daisy[current].x);
   for (;;)
   {
      signal (SIGCHLD, player_ended);
      switch (wgetch (screenwin))
      {
      case 13:
         playing = displaying = current;
         phrase_nr = 0;
         just_this_item = -1;
         view_screen ();
         kill_player ();
         open_text_file (daisy[current].smil_file,
                         daisy[current].anchor);
         break;
      case '/':
         search (current + 1, '/');
         break;
      case ' ':
      case KEY_IC:
      case '0':
         pause_resume ();
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
         go_to_phrase ();
         break;
      case 'h':
      case '?':
         kill_player ();
         help ();
         kill_player ();
         if (playing != -1)
            read_text (playing, phrase_nr - 1);
         break;
      case 'j':
      case '5':
      case KEY_B2:
         if (just_this_item != -1)
            just_this_item = -1;
         else
         {
            if (playing == -1)
               phrase_nr = 0;
            playing = just_this_item = current;
         } // if
         mvwprintw (screenwin, daisy[current].y, 0, " ");
         if (playing == -1)
         {
            just_this_item = displaying = playing = current;
            phrase_nr = 0;
            kill_player ();
            open_text_file (daisy[current].smil_file,
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
         search (current + 1, 'n');
         break;
      case 'N':
         search (current - 1, 'N');
         break;
      case 'o':
         if (playing == -1)
         {
            beep ();
            break;
         } // if
         pause_resume ();
         select_next_output_device ();
         playing = displaying;
         kill_player ();
         if (playing != -1)
            read_text (playing, phrase_nr - 1);
         break;
      case 'p':
         put_bookmark();
         break;
      case 'q':
         quit_eBook_speaker ();
         _exit (0);
      case 's':
         playing = just_this_item = -1;
         view_screen ();
         kill_player ();
         break;
      case 't':
         if (playing != -1)
            pause_resume ();
         select_tts ();
         playing = displaying;
         kill_player ();
         if (playing != -1)
            read_text (playing, phrase_nr - 1);
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
         skip_left ();
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
         read_text (playing, phrase_nr - 1);
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
         read_text (playing, phrase_nr - 1);
         break;
      case KEY_HOME:
      case '*':
      case '7':
         speed = 1;
         if (playing == -1)
            break;
         kill_player ();
         read_text (playing, phrase_nr - 1);
         break;
      default:
         beep ();
         break;
      } // switch
      if (playing != -1 && kill (player_pid, 0) != 0)
      {
         if (read_text (playing, phrase_nr++) == EOF)
         {
            phrase_nr = 0;
            playing++;
            if (daisy[playing].level <= level)
               current = displaying = playing;
            if (just_this_item != -1 && daisy[playing].level <= level)
               playing = just_this_item = -1;
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

void usage (char *argv)
{
   endwin ();
   playfile (PREFIX"share/eBook-speaker/error.wav", "1");
   printf ("eBook-speaker - Version %s - (C)2013 J. Lemmens\n", VERSION);
   printf (gettext ("Usage: %s eBook_file | -s\n"),
           basename (argv));
   fflush (stdout);
   _exit (1);
} // usage

char *open_opf ()
{
   char cmd[512];
   xmlTextReaderPtr reader;
   FILE *r;

   snprintf (cmd, MAX_CMD - 1, "find -iname \"*.opf\"");
   r = popen (cmd, "r");
   if (fscanf (r, "%s", OPF) == EOF)
      return NULL;
   pclose (r);
   realname (OPF);
   xmlDocPtr doc = xmlRecoverFile (OPF);
   if (! (reader = xmlReaderWalker (doc)))
   {
      endwin ();
      printf (gettext ("This file doesn't contain an eBook!\n"));
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      fflush (stdout);
      _exit (1);
   } // if
   while (1)
   {
      if (! get_tag_or_label (reader))
         break;
      if (*my_attribute.ncc_totalTime)
         strncpy (ncc_totalTime, my_attribute.ncc_totalTime,
                  MAX_STR - 1);
   } // while
   xmlTextReaderClose (reader);
   xmlFreeDoc (doc);
   return OPF;
} // open_opf

void read_epub (char *file)
{
   char cmd[MAX_CMD];

   snprintf (cmd, MAX_CMD - 1, "unzip -qq \"%s\"", file);
   switch (system (cmd))
   {
   default:
      break;
   } // switch
   if (open_opf ())
      strncpy (daisy_mp, dirname (open_opf ()), MAX_STR - 1);
   switch (chdir (daisy_mp))
   {
   default:
      break;
   } // switch
} // read_epub

const char *lowriter_to_pdf (char *file)
{
   char cmd[MAX_CMD];
   DIR *dir;
   const struct dirent *dirent;

   snprintf (cmd, MAX_CMD - 1,
         "lowriter --headless --convert-to pdf \"%s\" > /dev/null", file);
   switch (system (cmd))
   {
   case 0:
      break;
   default:
      endwin ();
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      puts (gettext ("eBook-speaker needs the libreoffice-writer package."));
      fflush (stdout);
      _exit (1);
   } // switch
   if (! (dir = opendir (".")))
   {
      endwin ();
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      printf (gettext ("\n%s\n"), strerror (errno));
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
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      printf (gettext ("Unknown format\n"));
      fflush (stdout);
      _exit (1);
   } // if
   return dirent->d_name;
} // lowriter_to_pdf

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
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf ("eBook-speaker - Version %s - (C)2013 J. Lemmens\n", VERSION);
      puts (gettext ("eBook-speaker needs the pandoc package."));
      fflush (stdout);
      _exit (1);
   } // switch
   if (access ("out.epub", R_OK) != 0)
   {
      endwin ();
      playfile (PREFIX"share/daisy-player/error.wav", "1");
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
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      puts (gettext ("eBook-speaker needs the poppler-utils package."));
      fflush (stdout);
      _exit (1);
   } // switch
} // pdf_to_html

void check_phrases ()
{
   int total_phrases = 0, item;

   for (item = 0; item < total_items; item++)
      strncpy (daisy[item].smil_file,
               re_organize (daisy[item].smil_file), MAX_STR - 1);
   for (item = 0; item < total_items; item++)
   {
      daisy[item].n_phrases =
         count_phrases (daisy[item].smil_file, daisy[item].anchor,
                       daisy[item + 1].smil_file, daisy[item + 1].anchor) - 2;
      total_phrases += daisy[item].n_phrases;
   } // for
   if (total_phrases < 1)
   {
      parse_opf (opf_name, "ignore_ncx");
      for (item = 0; item < total_items; item++)
         strncpy (daisy[item].smil_file,
                  re_organize (daisy[item].smil_file), MAX_STR - 1);
      for (item = 0; item < total_items; item++)
      {
         daisy[item].n_phrases =
            count_phrases (daisy[item].smil_file, daisy[item].anchor,
                    daisy[item + 1].smil_file, daisy[item + 1].anchor) - 2;
      } // for
   } // if
} // check_phrases

int main (int argc, char *argv[])
{
   int opt, scan_flag = 0;
   char str[MAX_STR], file[MAX_STR], cmd[MAX_CMD];

   fclose (stderr);
   setbuf (stdout, NULL);
   initscr ();
   titlewin = newwin (2, 80,  0, 0);
   screenwin = newwin (23, 80, 2, 0);
   getmaxyx (screenwin, max_y, max_x);
   max_y--;
   keypad (screenwin, TRUE);
   meta (screenwin,       TRUE);
   nonl ();
   noecho ();
   player_pid = -2;
   wattron (titlewin, A_BOLD);
   snprintf (str, MAX_STR - 1,
             "eBook-speaker - Version %s - (C)2013 J. Lemmens", VERSION);
   wprintw (titlewin, gettext ("%s - Please wait..."), str);
   strncpy (prog_name, basename (argv[0]), MAX_STR - 1);
   if (argc == 1)
      usage (prog_name);
   if (access ("/usr/share/doc/espeak", R_OK) != 0)
   {
      endwin ();
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      puts (gettext ("eBook-speaker needs the espeak package."));
      printf (gettext ("Please install it and try again.\n"));
      fflush (stdout);
      _exit (1);
   } // if
   speed = 1;
   atexit (quit_eBook_speaker);
   strncpy (sound_dev, "default", MAX_STR - 1);
   setlocale (LC_ALL, getenv ("LANG"));
   setlocale (LC_NUMERIC, "C");
   textdomain (prog_name);
   bindtextdomain (prog_name, PREFIX"share/locale");
   textdomain (prog_name);
   read_xml ();
   tmp_dir = strdup ("/tmp/eBook-speaker.XXXXXX");
   if (! mkdtemp (tmp_dir))
   {
      endwin ();
      printf ("mmkdtemp ()\n");
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      fflush (stdout);
      _exit (1);
   } // if
   opterr = 0;
   while ((opt = getopt (argc, argv, "ls")) != -1)
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
            playfile (PREFIX"share/eBook-speaker/error.wav", "1");
            printf ("eBook-speaker - Version %s - (C)2013 J. Lemmens\n", VERSION);
            puts (gettext ("eBook-speaker needs the sane-utils package."));
            fflush (stdout);
            _exit (1);
         } // switch
         snprintf (file, MAX_STR, "%s/out.pnm", tmp_dir);
         break;
      }
      default:
         usage (prog_name);
      } // switch
   } // while

// check if arg is an eBook
   struct stat st_buf;
   if (! scan_flag)
   {
      if (! argv[optind])
         usage ("");
      if (*argv[optind] == '/')
         snprintf (file, MAX_STR - 1, "%s", argv[optind]);
      else
         snprintf (file, MAX_STR - 1, "%s/%s", get_current_dir_name (), argv[optind]);
   } // if
   if (stat (file, &st_buf) == -1)
   {
      endwin ();
      printf ("eBook-speaker - Version %s - (C)2013 J. Lemmens\n", VERSION);
      printf ("stat: %s: %s\n", strerror (errno), file);
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      fflush (stdout);
      _exit (1);
   } // if
   if (chdir (tmp_dir) == -1)
   {
      endwin ();
      printf ("Can't chdir (\"%s\")\n", tmp_dir);
      playfile (PREFIX"share/daisy-player/error.wav", "1");
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
      char cmd[512];

      switch (chdir (daisy_mp))
      {
      default:
         break;
      } // switch
      sprintf (cmd, "cp -r %s/* .", file);
      switch (system (cmd))
      {
      default:
         break;
      } // switch
      switch (chdir (open_opf ()))
      {
      default:
         break;
      } // switch
      strncpy (daisy_mp, dirname (OPF), MAX_STR - 1);
   } // if directory
   else
   if (strcasestr (magic_file (myt, file), "EPUB") ||
       (strcasestr (magic_file (myt, file), "Zip archive data") &&
        ! strcasestr (magic_file (myt, file), "Microsoft Word 2007+")))
   {
      read_epub (file);
   } // if epub
   else
// use pandoc
   if (strcasestr (magic_file (myt, file), "HTML document"))
   {
      snprintf (cmd, MAX_CMD - 1,
                "cat \"%s\" | html2 | 2html > out.html", file);
      switch (system (cmd))
      {
      case 0:
         break;
      default:
         endwin ();
         playfile (PREFIX"share/eBook-speaker/error.wav", "1");
         puts (gettext ("eBook-speaker needs the xml2 package."));
         fflush (stdout);
         _exit (1);
      } // switch
      pandoc_to_epub ("out.html");
   } // if
   else
   if (strcasestr (magic_file (myt, file), "PDF document"))
   {
      pdf_to_html (file);
      pandoc_to_epub ("out.html");
   } // if
   else
// use lowriter
   if (strcasestr (magic_file (myt, file), "ASCII text") ||
       strcasestr (magic_file (myt, file), "ISO-8859 text") ||
       strcasestr (magic_file (myt, file), "Microsoft Word 2007+") ||
       strcasestr (magic_file (myt, file), "Rich Text Format") ||
       strcasestr (magic_file (myt, file), "(Corel/WP)") ||
       strcasestr (magic_file (myt, file), "Composite Document File") ||
       strcasestr (magic_file (myt, file), "OpenDocument") ||
       strcasestr (magic_file (myt, file), "UTF-8 Unicode"))
   {
      pdf_to_html (lowriter_to_pdf (file));
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
         playfile (PREFIX"share/daisy-player/error.wav", "1");
         puts (gettext ("eBook-speaker needs the calibre package."));
         fflush (stdout);
         _exit (1);
      } // switch
      read_epub ("out.epub");
   }
   else
// use ocr-feeder
   if (strcasestr (magic_file (myt, file), "JPEG image")  ||
       strcasestr (magic_file (myt, file), "GIF image")  ||
       strcasestr (magic_file (myt, file), "PNG image")  ||
       strcasestr (magic_file (myt, file), "Netpbm"))
   {
      snprintf (cmd, MAX_CMD - 1,
                "ocrfeeder-cli -i \"%s\" -o out -f html 2> /dev/null", file);
      switch (system (cmd))
      {
      case 0:
         break;
      default:
         endwin ();
         playfile (PREFIX"share/eBook-speaker/error.wav", "1");
         printf ("eBook-speaker - Version %s - (C)2013 J. Lemmens\n", VERSION);
         puts (gettext ("eBook-speaker needs the ocrfeeder package."));
         fflush (stdout);
         _exit (1);
      } // switch
      pandoc_to_epub ("out/index.html");
   } // if
   else
   {
      endwin ();
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      printf (gettext ("Unknown format\n"));
      fflush (stdout);
      _exit (1);
   } // if
   magic_close (myt);
   read_daisy_3 (".");
   check_phrases ();

   wclear (titlewin);
   mvwprintw (titlewin, 0, 0, gettext (str));
   if (strlen (daisy_title) + strlen (str) >= max_x)
      mvwprintw (titlewin, 0, max_x - strlen (daisy_title) - 4, "... ");
   mvwprintw (titlewin, 0, max_x - strlen (daisy_title), "%s", daisy_title);
   mvwaddstr (titlewin, 1, 0, "----------------------------------------");
   waddstr (titlewin, "----------------------------------------");
   mvwprintw (titlewin, 1, 0, gettext ("Press 'h' for help "));

   wrefresh (titlewin);
   level = 1;
   *search_str = 0;
   browse ();
   return 0;
} // main
