/* eBook-speaker - read aloud an eBook using a speech synthesizer
 *  Copyright (C) 2011 J. Lemmens
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

#include "eBook-speaker.h"

#define VERSION "2.0"

WINDOW *screenwin, *titlewin;
struct zip_file *text_file_fd;
int discinfo_fp, discinfo = 00, multi = 0, displaying = 0;
int playing, just_this_item, phrase_nr;
int bytes_read, current_page_number, total_pages;
char label[255], bookmark_title[255], dc_language[10], prefix[255];
char item_title[255];
char tag[1024], element[1024], search_str[30], tmp_ncx[255], tmp_wav[255];
char daisy_version[25];
pid_t player_pid;
char eBook_title[255], prog_name[255];
struct zip *eBook;
struct
{
   char a[255],
        class[255],
        content[255],
        dc_title[255],
        dtb_depth[255],
        dtb_totalPageCount[255],
        href[255],
        id[255],
        idref[255],
        media_type[255],
        name[255],
        ncc_depth[255],
        ncc_maxPageNormal[255],
        ncc_totalTime[255],
        playorder[255],
        src[255],
        title[255],
        toc[255],
        type[255],
        version[255];
} attribute;

int current, max_y, max_x, total_items, level, depth, speed, pich;
double audio_total_length;
char OPF[255], discinfo_html[255], ncc_totalTime[10], NCX[255];
char sound_dev[16], eBook_mp[255];
time_t start_time;
DIR *dir;
struct dirent *dirent;

void html_entities_to_utf8 (char *s)
{                     
  int e_flag, x;
  char entity[10], *e, new[255], *n, *orig, *s2;

  orig = s;
  n = new;
  while (*s)
  {
    if (*s == '&')
    {
      e_flag = 0;
      e = entity;
      s2 = s;
      s++;
      while (*s != ';')
      {
        *e++ = *s++;
        if (e - entity == 9)
        {
          s = s2 + 1;
          *n++ = '&';
          e_flag = 1;
          break;
        } // if
      } // while
      if (e_flag)
        continue;
      *e = 0;
      *n = ' ';
      for (x = 0;
           x < sizeof (unicode_entities) / sizeof (UC_entity_info); x++)
      {
        if (strcmp (unicode_entities[x].name, entity) == 0)
        {
          char buf[10];
          int num;

          num = stringprep_unichar_to_utf8 (unicode_entities[x].code, buf);
          strncpy (n, buf, num);
          n += num;
        } // if
      } // for
      if (! *(++s))
        break;
    } // if (*s == '&')
    else
      *n++ = *s++;
  } // while
  *n = 0;
  strncpy (orig, new, 250);
} // html_entities_to_utf8

int read_text (int item, int phrase_nr)
{
   char cmd[255], tmp_txt[255], str[255];
   int nr, w;

   open_text_file (eBook_struct[item].text_file,
                   eBook_struct[item].anchor);
   nr = 1;
   while (1)
   {
      if (get_tag_or_label (text_file_fd) == EOF ||
          (*eBook_struct[item + 1].anchor &&
             strcasestr (element, eBook_struct[item + 1].anchor)) ||
          (*eBook_struct[item + 1].anchor &&
             strcasestr (label, eBook_struct[item + 1].anchor)))
      {
         if (item >= total_items - 1)
         {
            quit_eBook_reader ();
            snprintf (str, 250, "%s/.eBook-speaker/%s",
                      getenv ("HOME"), bookmark_title);
            unlink (str);
            _exit (0);
         } // if
         return EOF;
      } // if
      if (*label)
         if (nr++ == phrase_nr)
            break;
   } // while
   switch (player_pid = fork ())
   {
   case -1:
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
   mvwprintw (screenwin, eBook_struct[item].y, 71, "%4d %4d",
              phrase_nr - 1, eBook_struct[item].n_phrases - phrase_nr + 1);
   wattroff (screenwin, A_BOLD);
   wmove (screenwin, eBook_struct[displaying].y,
                     eBook_struct[displaying].x - 1);
   wrefresh (screenwin);

   snprintf (tmp_txt, 200, "/tmp/eBook-speaker.XXXXXX");
   mkstemp (tmp_txt);
   unlink (tmp_txt);
   strcat (tmp_txt, ".txt");
   if ((w = open (tmp_txt, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf ("Can't make a temp file %s\n", tmp_txt);
      fflush (stdout);
      kill (getppid (), SIGINT);
   } // if
   html_entities_to_utf8 (label);
   if (write (w, label, strlen (label)) == -1)
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf ("write (\"%s\"): failed.\n", label);
      fflush (stdout);
      kill (getppid (), SIGINT);
   } // if
   close (w);
   snprintf (cmd, 250, "espeak -s %d -f %s -v %s", speed, tmp_txt, dc_language);
   system (cmd);
   unlink (tmp_txt);
   _exit (0);
} // read_text

void playfile (char *filename, char *tempo)
{
  sox_format_t *in, *out; /* input and output files */
  sox_effects_chain_t *chain;
  sox_effect_t *e;
  char *args[255], str[255];

  sox_globals.verbosity = 0;
  sox_init();
  in = sox_open_read (filename, NULL, NULL, NULL);
  while (! (out =
         sox_open_write (sound_dev, &in->signal, NULL, "alsa", NULL, NULL)))
  {
    strncpy (sound_dev, "default", 8);
    save_rc ();
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

  snprintf (str, 90, "%lf", out->signal.rate);
  e = sox_create_effect (sox_find_effect ("rate"));
  args[0] = str, sox_effect_options (e, 1, args);
  sox_add_effect (chain, e, &in->signal, &in->signal);

  snprintf (str, 90, "%i", out->signal.channels);
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

char *realname (char *name)
{
   if (! (dir = opendir (eBook_mp)))
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf (gettext ("\nCannot read %s\n"), eBook_mp);
      fflush (stdout);
      _exit (1);
   } // if
   while ((dirent = readdir (dir)) != NULL)
   {
      if (strcasecmp (dirent->d_name, name) == 0)
      {
         closedir (dir);
         return dirent->d_name;
      } // if
   } // while
   closedir (dir);
   return name;
} // realname

double read_time (char *p)
{
   char *h, *m, *s;

   s = strrchr (p, ':') + 1;
   *(s - 1) = 0;
   if (strchr (p, ':'))
   {
      m = strrchr (p, ':') + 1;
      *(m - 1) = 0;
      h = p;
   }
   else
   {
      h = "0";
      m = p;
   } // if
   return atoi (h) * 3600 + atoi (m) * 60 + atof (s);
} // read_time

void put_bookmark ()
{
   int w;
   char str[255];

   snprintf (str, 250, "%s/.eBook-speaker", getenv ("HOME"));
   mkdir (str, 0755);
   snprintf (str, 250, "%s/.eBook-speaker/%s",
                       getenv ("HOME"), bookmark_title);
   if ((w = creat (str, 0644)) != -1)
   {
      dprintf (w, "%d\n", current);
      dprintf (w, "%d\n", --phrase_nr);
      dprintf (w, "%d\n", level);
      close (w);
   } // if
} // put_bookmark

void get_bookmark ()
{
   char str[255];
   FILE *r;

   snprintf (str, 250, "%s/.eBook-speaker/%s",
                       getenv ("HOME"), bookmark_title);
   if ((r = fopen (str, "r")) == NULL)
      return;
   fscanf (r, "%d", &current);
   open_text_file (eBook_struct[current].text_file,
                   eBook_struct[current].anchor);
   fscanf (r, "%d", &phrase_nr);
   if (phrase_nr < 1)
      phrase_nr = 1;
   fscanf (r, "%d", &level);
   fclose (r);
   if (level < 1)
      level = 1;
   displaying = playing = current;
   just_this_item = -1;
} // get_bookmark

void get_attributes (char *p)
{
   char name[1024], *value, *begin;
   int break2;

   *attribute.class = 0;
   *attribute.content = 0;
   *attribute.dc_title = 0;
   *attribute.dtb_depth = 0,
   *attribute.dtb_totalPageCount = 0;
   *attribute.href = 0;
   *attribute.id = 0;
   *attribute.idref = 0;
   *attribute.media_type = 0;
   *attribute.name = 0;
   *attribute.ncc_depth = 0;
   *attribute.ncc_maxPageNormal = 0,
   *attribute.ncc_totalTime = 0;
   *attribute.playorder = 0;
   *attribute.src = 0;
   *attribute.title = 0;
   *attribute.toc = 0;
   *attribute.type = 0;
   begin = p;

// skip to first attribute
   while (! isspace (*p))
   {
      if (*p == '>' || *p == '?')
         return;
      if (p - begin > 1000)
      {
         *p = 0;
         return;
      } // if
      p++;
   } // while
   break2 = 0;
   while (1)
   {
      while (isspace (*++p))
      {
         if (*p == '>' || *p == '?')
         {
            break2 = 1;
            break;
         } // if
         if (p - begin > 1000)
         {
            *p = 0;
            break2 = 1;
            break;
         } // if
      } // while
      if (break2)
        break;
      strncpy (name, p, 1000);
      p = name;
      while (! isspace (*p) && *p != '=')
      {
         if (*p == '>' || *p == '?')
         {
            break2 = 1;
            break;
         } // if
         if (p - begin > 1000)
         {
            *p = 0;
            break2 = 1;
            break;
         } // if
         p++;
      } // while
      if (break2)
         break;
      *p = 0;
      while (*p != '"')
      {
         if (*p == '>' || *p == '?')
         {
            break2 = 1;
            break;
         } // if
         if (p - begin > 1000)
         {
            *p = 0;
            break2 = 1;
            return; 
         } // if
         p++;
      } // while
      if (break2)
         break;
      p++;

      value = p;
      p = value;
      while (*p != '"' && *p != '>' && *p != '?')
      {
         if (p - begin > 1000)
         {
            *p = 0;
            break2 = 1;
            break;
         } // if
         p++;
      } // while
      if (break2)
         break;
      *p = 0;

      if (strcasecmp (name, "class") == 0)
         strncpy (attribute.class, value, 90);
      if (strcasecmp (name, "content") == 0)
         strncpy (attribute.content, value, 90);
      if (strcasecmp (name, "href") == 0)
         strncpy (attribute.href, value, 90);
      if (strcasecmp (name, "id") == 0)
         strncpy (attribute.id, value, 90);
      if (strcasecmp (name, "idref") == 0)
         strncpy (attribute.idref, value, 90);
      if (strcasecmp (name, "media-type") == 0)
         strncpy (attribute.media_type, value, 90);
      if (strcasecmp (name, "name") == 0)
      {
         if (strcasecmp (value, "dc:title") == 0)
            strncpy (attribute.dc_title, "prepare for content", 90);
         if (strcasecmp (value, "dtb:depth") == 0)
            strncpy (attribute.dtb_depth, "prepare for content", 90);
         if (strcasecmp (value, "dtb:totalPageCount") == 0)
            strncpy (attribute.ncc_maxPageNormal, "prepare for content", 90);
         if (strcasecmp (value, "dtb:totalTime") == 0)
            strncpy (attribute.ncc_totalTime, "prepare for content", 90);
         if (strcasecmp (value, "ncc:depth") == 0)
            strncpy (attribute.ncc_depth, "prepare for content", 90);
         if (strcasecmp (value, "ncc:maxPageNormal") == 0)
            strncpy (attribute.ncc_maxPageNormal, "prepare for content", 90);
         if (strcasecmp (value, "ncc:totalTime") == 0)
            strncpy (attribute.ncc_totalTime, "prepare for content", 90);
      } // if
      if (strcasecmp (name, "playorder") == 0)
         strncpy (attribute.playorder, value, 90);
      if (strcasecmp (name, "src") == 0)
         strncpy (attribute.src, value, 250);
      if (strcasecmp (name, "toc") == 0)
         strncpy (attribute.toc, value, 90);
      if (strcasecmp (name, "title") == 0)
         strncpy (attribute.title, value, 90);
      if (strcasecmp (name, "type") == 0)
         strncpy (attribute.type, value, 90);
      if (strcasecmp (name, "version") == 0)
         strncpy (attribute.version, value, 90);
   } // while
   if (*attribute.dc_title)
      strncpy (attribute.dc_title, attribute.content, 90);
   if (*attribute.dtb_depth)
      depth = atoi (attribute.content);
   if (*attribute.dtb_totalPageCount)
      total_pages = atoi (attribute.content);
   if (*attribute.ncc_depth)
      depth = atoi (attribute.content);
   if (*attribute.ncc_maxPageNormal)
      total_pages = atoi (attribute.content);
   if (*attribute.ncc_totalTime)
   {
      strncpy (attribute.ncc_totalTime, attribute.content, 90);
      if (strchr (attribute.ncc_totalTime, '.'))
         *strchr (attribute.ncc_totalTime, '.') = 0;
   } // if
} // get_attributes

int get_tag_or_label (struct zip_file *r)
{
   char *p, h;
   static char read_flag = 0;

   p = element;
   do
   {
      switch (zip_fread (r, p, 1))
      {
      case -1:
         endwin ();
         playfile (PREFIX"share/eBook-speaker/error.wav", "1");
         printf ("%s: \n", zip_file_strerror (r));
         fflush (stdout);
         _exit (1);
      case 0:
         return EOF;
      } // switch
   } while (isspace (*p));
   h = *p;

   if (read_flag)
   {
      *p++ = '<';
      *p = h;
   } // if
   if (*p == '<' || read_flag)
   {
      read_flag = 0;
      *label = 0;
      do
      {
         if (p - element > 250)
         {
            *p = 0;
            strncpy (tag, element + 1, 250);
            get_tag ();
            get_attributes (element);
            return 0;
         } // if
         switch (zip_fread (r, ++p, 1))
         {
         case -1:
            endwin ();
            playfile (PREFIX"share/eBook-speaker/error.wav", "1");
            printf ("get_tag_or_label: %s\n", p);
            fflush (stdout);
            _exit (1);
         case 0:
            *++p = 0;
            strncpy (tag, element + 1, 250);
            get_tag ();
            get_attributes (element);
            return EOF;
         } // switch
      } while (*p != '>');
      *++p = 0;
      strncpy (tag, element + 1, 250);
      get_tag ();
      get_attributes (element);
      return 0;
   } // if
   *label = *p;
   *element = 0;
   p = label;
   do
   {
      if (p - label > 250)
      {
         *p = 0;
         return 0;
      } // if
      switch (zip_fread (r, ++p, 1))
      {
      case -1:
         endwin ();
         playfile (PREFIX"share/eBook-speaker/error.wav", "1");
         puts (gettext ("Maybe a read-error occurred!"));
         fflush (stdout);
         _exit (1);
      case 0:
         *p = 0;
         return EOF;
      } // switch
      if (*p == '\n')
         p--;
   } while (*p != '<');
   read_flag = 1;
   *p = 0;
   strncpy (tag, element + 1, 250);
   get_tag ();
   get_attributes (element);
   return 0;
} // get_tag_or_label

void get_tag ()
{
   char *p;

   p = tag;
   while (*p != ' ' && *p != '>' && p - tag <= 250)
     p++;
   *p = 0;
} // get_tag

void get_page_number ()
{
   struct zip_file *fd;
   char file[255], *anchor;

   if (strstr (daisy_version, "2.02"))
   {
      if (! strcasestr (element, attribute.src))
         return;
      strncpy (file, attribute.src, 250);
      if (strchr (file, '#'))
      {
         anchor = strchr (file, '#') + 1;
         *strchr (file, '#') = 0;
      } // if
      if (! (fd = zip_fopen (eBook, file, ZIP_FL_UNCHANGED)))
      {                          
         endwin ();
         playfile (PREFIX"share/eBook-speaker/error.wav", "1");
         printf ("get_page_number(): %s\n", file);
         fflush (stdout);
         _exit (1);
      } // if
      while (1)
      {
         if (get_tag_or_label (fd) == EOF)
         {
            zip_fclose (fd);
            return;
         } // if
         if (strcasecmp (attribute.id, anchor) == 0)
            break;
      } // while
      while (1)
      {
         if (get_tag_or_label (fd) == EOF)
         {
            zip_fclose (fd);
            return;
         } // if
         if (strcasecmp (tag, "span") == 0)
            break;
         if (tolower (tag[0]) == 'h' && isdigit (tag[1]))
         {
            zip_fclose (fd);
            return;
         } // if
      } // while
      while (1)
      {
         if (get_tag_or_label (fd) == EOF)
         {
            zip_fclose (fd);
            return;
         } // if
         if (isdigit (*label))
         {
            current_page_number = atoi (label);
            zip_fclose (fd);
            return;
         } // if
      } // while
      zip_fclose (fd);
      return;
   } // if
   if (strcmp (daisy_version, "3") == 0)
   {
      fd = zip_fopen (eBook, OPF, ZIP_FL_UNCHANGED);
      do      
      {
         if (get_tag_or_label (fd) == EOF)
            break;
      } while (atoi (attribute.playorder) != current);
      do
      {
         if (get_tag_or_label (fd) == EOF)
            break;
      } while (! *label);
      current_page_number = atoi (label);
      zip_fclose (fd);
      return;
   } // if
} // get_page_number

int count_phrases (char *f_file, char *f_anchor,
                   char *t_file, char *t_anchor)
{
   int n_phrases;
   struct zip_file *r;

   if (! (r = zip_fopen (eBook, f_file, ZIP_FL_UNCHANGED)))
      return EOF;
   n_phrases = 0;
   if (*f_anchor)
   {
      while (1)
      {
         if (get_tag_or_label (r) == EOF)
// if the given anchor is not there reopen the file to read from the start
         {
            zip_fclose (r);
            r = zip_fopen (eBook, f_file, ZIP_FL_UNCHANGED);
            break;
         } // if
         if (strcasecmp (attribute.id, f_anchor) == 0)
            break;
      } // while
   } // if
// start counting
   while (1)
   {
      if (get_tag_or_label (r) == EOF)
      {
         zip_fclose (r);
         return n_phrases;
      } // if
      if (*t_anchor && strcasecmp (f_file, t_file) == 0)
      {
         if (strcasecmp (attribute.id, t_anchor) == 0)
         {
            zip_fclose (r);
            return n_phrases;
         } // if
      } // if
      if (*label)
         n_phrases++;
   } // while
} // count_phrases

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
   for (i = 0; eBook_struct[i].screen != eBook_struct[current].screen; i++);
   do
   {
      if (*eBook_struct[i].label == 0)
         snprintf (eBook_struct[i].label, 5, "%d",
                 i + eBook_struct[i].screen * max_y + 1);
      mvwprintw (screenwin, eBook_struct[i].y, eBook_struct[i].x,
                            eBook_struct[i].label);
      l = strlen (eBook_struct[i].label);
      if (l / 2 * 2 != l)
         waddstr (screenwin, " ");
      for (x = l; x < 61; x += 2)
         waddstr (screenwin, " .");
      if (eBook_struct[i].page_number)
         mvwprintw (screenwin, eBook_struct[i].y, 65,
                    "(%3d)", eBook_struct[i].page_number);
      l = eBook_struct[i].n_phrases;
      x = i + 1;
      while (eBook_struct[x].level > level)
         l += eBook_struct[x++].n_phrases;
      if (eBook_struct[i].level <= level)
         mvwprintw (screenwin, eBook_struct[i].y, 76, "%4d", l);
      if (i >= total_items - 1)
         break;
   } while (eBook_struct[++i].screen == eBook_struct[current].screen);
   if (just_this_item != -1 &&
       eBook_struct[displaying].screen == eBook_struct[playing].screen)
      mvwprintw (screenwin, eBook_struct[current].y, 0, "J");
   wmove (screenwin, eBook_struct[current].y, eBook_struct[current].x - 1);
   wrefresh (screenwin);
} // view_screen

void get_label (int item, int indent)
{
   html_entities_to_utf8 (label);
   strncpy (eBook_struct[item].label, label, 80);
   eBook_struct[item].label[64 - eBook_struct[item].x] = 0;
   if (displaying == max_y)
      displaying = 0;
   if (strcasecmp (eBook_struct[item].class, "pagenum") == 0)
      eBook_struct[item].x = 0;
   else
      if (eBook_struct[item].x == 0)
         eBook_struct[item].x = indent + 3;
} // get_label

static char *convert (char *s)
{
   int x = 0, n = 0;
   static char new[255];

   do
   {
      if (s[x] == '%')
      {
         char hex[10];

         x++;
         hex[0] = '0';
         hex[1] = 'x';
         hex[2] = s[x++];
         hex[3] = s[x++];
         hex[4] = 0;
         new[n++] = strtod (hex, NULL);
      }
      else
         new[n++] = s[x++];
   } while (s[x - 1]);
   return new;
} // convert

int fill_struct_from_ncx (struct zip_file *ncx, int item)
{
   eBook_struct[item].level = 0;
   while (1)
   {
      if (get_tag_or_label (ncx) == EOF)
         return EOF;
      if (strcasecmp (tag, "navpoint") == 0)
      {
         level++;
         depth = level;
      } // if
      if (strcasecmp (tag, "/navpoint") == 0)
         level--;
      if (strcasecmp (tag, "navlabel") == 0)
      {
         eBook_struct[item].page_number = 0;
         do
         {
            if (get_tag_or_label (ncx) == EOF)
               return EOF;
         } while (*label == 0 && strcasecmp (tag, "/navlabel") != 0);
         eBook_struct[item].x = level * 3 - 1;
         if (*label)
            get_label (item, eBook_struct[item].x);
         do
         {
            if (get_tag_or_label (ncx) == EOF)
               return EOF;
         } while (strcasecmp (tag, "content") != 0);
         strncpy (attribute.src, convert (attribute.src), 250);
         if (strcasecmp (prefix, ".") != 0)
            snprintf (eBook_struct[item].text_file, 250,
                      "%s/%s", prefix, attribute.src);
         else
            snprintf (eBook_struct[item].text_file, 250, "%s", attribute.src);
         if (strchr (eBook_struct[item].text_file, '#'))
         {
            strncpy (eBook_struct[item].anchor,
                     strchr (eBook_struct[item].text_file, '#') + 1, 250);
            *strchr (eBook_struct[item].text_file, '#') = 0;
         } // if
         break;
      } // if (strcasecmp (tag, "navlabel") == 0)
      if (strcasecmp (tag, "style") == 0)
      {
         while (strcasecmp (tag, "/style") != 0)
            get_tag_or_label (ncx);
      } // if (strcasecmp (tag, "style") == 0)
   } // while
   return 0;
} // fill_struct_from_ncx

void read_ncx (char *id)
{
   int item;
   struct zip_file *ncx, *opf;
   char str[255];

   strncpy (str, id, 90);
   opf = zip_fopen (eBook, OPF, ZIP_FL_UNCHANGED);
   while (1)
   {
      get_tag_or_label (opf);
      if (strcasecmp (tag, "item") == 0)
         if (strcasecmp (str, attribute.id) == 0)
            break;
   } // while
   zip_fclose (opf);
   if (strcasecmp (prefix, ".") != 0)
      snprintf (NCX, 90, "%s/%s", prefix, attribute.href);
   else
      snprintf (NCX, 90, "%s", attribute.href);
   if ((ncx = zip_fopen (eBook, NCX, ZIP_FL_UNCHANGED)) == NULL)
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf (gettext ("Corrupt eBook structure %s\n"), NCX);
      fflush (stdout);
      _exit (1);
   } // if
   item = 0;
   level = 0;
   while (1)
   {
      if (fill_struct_from_ncx (ncx, item) == EOF)
         break;
      eBook_struct[item].level = level;
      eBook_struct[item].screen = item / max_y;
      eBook_struct[item].y = item - (eBook_struct[item].screen * max_y);
      item++;
   } // while
   total_items = item;
   zip_fclose (ncx);
} // read_ncx

void read_manifest (char *idref, int item)
{
   struct zip_file *manifest;

   manifest = zip_fopen (eBook, OPF, ZIP_FL_UNCHANGED);
   while (1)
   {
      if (get_tag_or_label (manifest) == EOF)
         break;
      if (strcasecmp (attribute.id, idref) == 0)
      {
         snprintf (eBook_struct[item].label, 90, "%d", item + 1);
         strncpy (eBook_struct[item].text_file, attribute.href, 250);
         eBook_struct[item].screen = item / max_y;
         eBook_struct[item].y = item - eBook_struct[item].screen * max_y;
         if (strchr (eBook_struct[item].text_file, '#'))
         {
            strncpy (eBook_struct[item].anchor,
                     strchr (eBook_struct[item].text_file, '#') + 1, 250);
            *strchr (eBook_struct[item].text_file, '#') = 0;
         } // if
         eBook_struct[item].level = 1;
         eBook_struct[item].x = eBook_struct[item].level * 3 - 1;
         break;
      } // if
   } // while
   zip_fclose (manifest);
}  // read_manifest

void read_tours (struct zip_file *opf)
{
   int item = 0;

   depth = 1;
   while (1)
   {
      if (get_tag_or_label (opf) == EOF)
         break;
      if (strcasecmp (tag, "site") == 0)
      {
         strncpy (eBook_struct[item].label, attribute.title, 90);
         strncpy (eBook_struct[item].text_file, attribute.href, 250);
         eBook_struct[item].screen = item / max_y;
         eBook_struct[item].y = item - eBook_struct[item].screen * max_y;
         if (strchr (eBook_struct[item].text_file, '#'))
         {
            strncpy (eBook_struct[item].anchor,
                     strchr (eBook_struct[item].text_file, '#') + 1, 250);
            *strchr (eBook_struct[item].text_file, '#') = 0;
         } // if
         eBook_struct[item].level = 1;
         eBook_struct[item].x = eBook_struct[item].level * 3 - 1;
         item++;
      } // if
      if (strcasecmp (tag, "/tours") == 0)
         break;
   } // while
   total_items = item;
}  // read_tours

void read_opf (struct zip_file *opf)
{
   int item = 0;

   depth = 1;
   while (1)
   {
      if (get_tag_or_label (opf) == EOF)
         break;
      if (strcasecmp (tag, "itemref") == 0)
      {
         char idref[255];

         strncpy (idref, attribute.idref, 90);
         read_manifest (idref, item);
         item++;
      } // if
      if (strcasecmp (tag, "/spine") == 0)
         total_items = item;
      if (strcasecmp (tag, "tours") == 0)
      {
// read tours tag and forget manifest
         read_tours (opf);
         break;
      } // if
   } // while
} // read_opf

void read_eBook_struct ()
{
   int item = 0;
   struct zip_file *opf;

   if ((opf = zip_fopen (eBook, OPF, ZIP_FL_UNCHANGED)) == NULL)
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf (gettext ("Corrupt eBook structure %s\n"), OPF);
      fflush (stdout);
      _exit (1);
   } // if
   while (1)
   {
      if (get_tag_or_label (opf) == EOF)
         break;
      if (strcasecmp (tag, "spine") == 0)
      {
         if (*attribute.toc)
         {
            read_ncx (attribute.toc);
// if toc is nevertheless empty, read opf
            if (total_items == 0)
               read_opf (opf);
            break;
         } // if
         read_opf (opf);
      } // if
   } // while
   zip_fclose (opf);
   for (item = 0; item < total_items; item++)
      eBook_struct[item].n_phrases = count_phrases
         (eBook_struct[item].text_file, eBook_struct[item].anchor,
          eBook_struct[item + 1].text_file, eBook_struct[item + 1].anchor);
} // read_eBook_struct

void player_ended ()
{
   wait (NULL);
} // player_ended

void open_text_file (char *text_file, char *anchor)
{
   if (text_file_fd)
      zip_fclose (text_file_fd);
   if (! (text_file_fd = zip_fopen (eBook, text_file, ZIP_FL_UNCHANGED)))
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf ("open_text_file(): %s\n", realname (text_file));
      fflush (stdout);
      _exit (1);
   } // if
   do
   {
      if (get_tag_or_label (text_file_fd) == EOF)
         break;
   } while (strcasecmp (tag, "body") != 0);

// look if anchor exists in this text_file
   if (*anchor != 0)
   {
      strncpy (item_title, anchor, 250);
      while (1)
      {
         if (get_tag_or_label (text_file_fd) == EOF)
            break;
         if (strcasecmp (anchor, attribute.id) == 0 ||
             (*label && strcasecmp (anchor, label) == 0))
            break;
      } // while
   } // if
} // open_text_file

void pause_resume ()
{
   if (playing != -1)
      playing = -1;
   else
      playing = displaying;
   if (text_file_fd == NULL)
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
  char str[255];

  in = sox_open_read (infile, NULL, NULL, NULL);
  snprintf (str, 250, "%s/%s", getenv ("HOME"), outfile);
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
   waddstr (screenwin, gettext ("g               - go to page number (if any)\n"));
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
         displaying = current = 0;
         beep ();
         break;
      } // if
   } while (eBook_struct[current].level > level);
   displaying = current;
   phrase_nr = eBook_struct[playing].n_phrases - 3;
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
   } while (eBook_struct[current].level > level);
   view_screen ();
} // next_item

void skip_left ()
{
   int nr;

   if (playing == -1)
   {
      beep ();
      return;
   } // if
   kill_player ();
   open_text_file (eBook_struct[playing].text_file,
                   eBook_struct[playing].anchor);
   nr = 0;
   while (1)
   {
      if (get_tag_or_label (text_file_fd) == EOF)
         break;
      if (*label)
      {
         if (phrase_nr == 2)
         {
            current = playing--;
            phrase_nr = eBook_struct[playing].n_phrases - 1;
            previous_item ();
            break;
         } // if
/* jos
         if (phrase_nr == eBook_struct[playing].n_phrases)
         {
            phrase_nr = eBook_struct[playing].n_phrases - 3;
            break;
         } // if
*/
         if (nr == phrase_nr)
         {
            phrase_nr = nr - 2;
            break;
         } // if
         nr++;
      } // if
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
   if (eBook_struct[playing].level > level)
      previous_item ();
   view_screen ();
} // change_level

void read_rc ()
{
   FILE *r;
   char line[255], *p;
   struct passwd *pw = getpwuid (geteuid ());

   chdir (pw->pw_dir);
   strncpy (sound_dev, "default", 8);
   if ((r = fopen (".eBook-speaker.rc", "r")) == NULL)
      return;
   while (fgets (line, 250, r))
   {
      if (strchr (line, '#'))
         *strchr (line, '#') = 0;
      if ((p = strstr (line, "sound_dev")))
      {
         p += 8;
         while (*++p != 0)
         {
            if (*p == '=')
            {
               while (! *++p);
               if (*p == 0)
                  break;
               strncpy (sound_dev, p, 15);
               sound_dev[15] = 0;
               break;
            } // if
         } // while
      } // if
      if ((p = strstr (line, "speed")))
      {
         p += 4;
         while (*++p != 0)
         {
            if (*p == '=')
            {
               while (! isdigit (*++p))
                  if (*p == 0)
                     return;
               speed = atoi (p);
               break;
            } // if
         } // while
      } // if
      if ((p = strstr (line, "language")))
      {
         p += 9;
         while (*p == ' ' || *p == '\t' || *p == '\n')
            p++;
         strncpy (dc_language, p, 5);
         p = dc_language;
         while (*p != ' ' && *p != '\t' && *p != 0)
            p++;
         *p = 0;
      } // if
   } // while
   fclose (r);
} // read_rc

void save_rc ()
{
   FILE *w;
   struct passwd *pw = getpwuid (geteuid ());
   chdir (pw->pw_dir);
   if ((w = fopen (".eBook-speaker.rc", "w")) == NULL)
      return;
   fputs ("# This file contains the name of the desired audio device and  the\n", w);
   fputs ("# desired playing speed.\n", w);
   fputs ("#\n", w);
   fputs ("# WARNING\n", w);
   fputs ("# If you edit this file by hand, be sure there is no eBook-speaker active\n", w);
   fputs ("# otherwise your changes will be lost.\n", w);
   fputs ("#\n", w);
   fputs ("# On which ALSA-audio device should eBook-speaker read the book?\n", w);
   fputs ("# default: sound_dev=default\n", w);
   fprintf (w, "sound_dev=%s\n", sound_dev);
   fputs ("#\n", w);
   fputs ("# At wich speed should the book be read?\n", w);
   fputs ("# default: speed=160\n", w);
   fprintf (w, "speed=%i\n", speed);                             
   fputs ("#\n", w);
   fputs ("# What should the language be if it is not specified in the book?\n", w);
   fprintf (w, "language=%s\n", dc_language);
   fclose (w);
} // save_rc

void quit_eBook_reader ()
{
   endwin ();
   kill_player ();
   wait (NULL);
   put_bookmark ();
   save_rc ();
   if (text_file_fd)
      zip_fclose (text_file_fd);
   if (*tmp_ncx)
      unlink (tmp_ncx);
   unlink (tmp_wav);
} // quit_eBook_reader

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
         if (strcasestr (eBook_struct[c].label, search_str))
         {
            found = 1;
            break;
         } // if
      } // for
      if (! found)
      {
         for (c = 0; c < start; c++)
         {
            if (strcasestr (eBook_struct[c].label, search_str))
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
         if (strcasestr (eBook_struct[c].label, search_str))
         {
            found = 1;
            break;
         } // if
      } // for
      if (! found)
      {
         for (c = total_items + 1; c > start; c--)
         {
            if (strcasestr (eBook_struct[c].label, search_str))
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
      phrase_nr = 1;
      just_this_item = -1;
      displaying = playing = current;
      if (playing != -1)
         kill_player ();
      open_text_file (eBook_struct[current].text_file,
                      eBook_struct[current].anchor);
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

void go_to_page_number ()
{
   struct zip_file *fd;
   char *p, filename[255], anchor[255], pre_filename[255], pre_anchor[255];
   char pn[15];

   kill_player ();
   mvwaddstr (titlewin, 1, 0, "----------------------------------------");
   waddstr (titlewin, "----------------------------------------");
   mvwprintw (titlewin, 1, 0, gettext ("Go to page number:        "));
   echo ();
   wmove (titlewin, 1, 19);
   wgetnstr (titlewin, pn, 5);
   noecho ();
   view_screen ();
   if (! *pn || ! isdigit (*pn))
   {
      beep (); 
      skip_left ();
      return;
   } // if
   if (! (fd = zip_fopen (eBook, OPF, ZIP_FL_UNCHANGED)))
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf (gettext ("Something is wrong with the %s file\n"), OPF);
      fflush (stdout);
      _exit (1);
   } // if
   do
   {
      if (get_tag_or_label (fd) == EOF)
      {
         beep ();
         zip_fclose (fd);
         return;
      } // if
      if (strcasecmp (tag, "a") == 0 && ! *label)
      {
         strncpy (pre_filename, filename, 90);
         strncpy (pre_anchor, anchor, 90);
         strncpy (filename, attribute.href, 90);
         p = filename;
         while (*p != 0 && *p != '"' && *p != '\'' && *p != '>' && *p != '#')
            p++;
         *p++ = 0;
         strncpy (anchor, p, 90);
         p = anchor;
         while (*p != 0 && *p != '"' && *p != '\'' && *p != '>' && *p != '#')
            p++;
         *p = 0;
      } // if "a"
      if (strcmp (label, pn) == 0)
      {
         zip_fclose (fd);
         for (current = 0; current <= total_items; current++)
         {
            if (strcasecmp (eBook_struct[current].text_file, pre_filename) == 0)
               break;
         } // for
         view_screen ();
         playing = current;
         just_this_item = -1;
         open_text_file (pre_filename, pre_anchor);
         return;
      } // if
   } while (strcasecmp (tag, "/html") != 0);
   beep ();
} // go_to_page_number

void select_next_output_device ()
{
   FILE *r;
   int n, y;
   char list[10][255], trash[255];

   wclear (screenwin);
   wprintw (screenwin, "\nSelect an soundcard:\n\n");
   if (! (r = fopen ("/proc/asound/cards", "r")))
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      puts (gettext ("Cannot read /proc/asound/cards"));
      fflush (stdout);
      _exit (1);
   } // if
   for (n = 0; n < 10; n++)
   {
      *list[n] = 0;
      fgets (list[n], 250, r);
      fgets (trash, 250, r);
      if (! *list[n])
         break;
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
         snprintf (sound_dev, 15, "default:%i", y - 3);
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

void browse ()
{
   int old;

   current = 0;
   just_this_item = playing = -1;
   text_file_fd = NULL;
   get_bookmark ();
   view_screen ();
   nodelay (screenwin, TRUE);

   for (;;)
   {
      signal (SIGCHLD, player_ended);
      switch (wgetch (screenwin))
      {
      case 13:
         playing = displaying = current;
         phrase_nr = 1;
         just_this_item = -1;
         view_screen ();
         displaying = playing = current;
         kill_player ();
         open_text_file (eBook_struct[current].text_file,
                         eBook_struct[current].anchor);
         break;
      case '/':
         search (current + 1, '/');
         break;
      case ' ':
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
         if (total_pages)
            go_to_page_number ();
         else
            beep ();
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
         if (just_this_item != -1)
            just_this_item = -1;
         else
         {
            if (playing == -1)
            {
               phrase_nr = 1;
               strncpy (item_title, eBook_struct[current].anchor, 250);
            } // if
            playing = just_this_item = current;
         } // if
         mvwprintw (screenwin, eBook_struct[current].y, 0, " ");
         if (playing == -1)
         {
            just_this_item = displaying = playing = current;
            phrase_nr = 1;
            kill_player ();
            open_text_file (eBook_struct[current].text_file,
                            eBook_struct[current].anchor);
         } // if
         if (just_this_item != -1 &&
             eBook_struct[displaying].screen == eBook_struct[playing].screen)
            mvwprintw (screenwin, eBook_struct[current].y, 0, "J");
         else
            mvwprintw (screenwin, eBook_struct[current].y, 0, " ");
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
         if (playing != -1)
            kill_player ();
         select_next_output_device ();
         if (playing != -1)
            kill_player ();
         break;
      case 'p':
         put_bookmark();
         break;
      case 'q':
         quit_eBook_reader ();
         _exit (0);
      case 's':
         playing = just_this_item = -1;
         view_screen ();
         kill_player ();
         break;
      case KEY_DOWN:
         next_item ();
         break;
      case KEY_UP:
         previous_item ();
         break;
      case KEY_RIGHT:
         skip_right ();
         break;
      case KEY_LEFT:
         skip_left ();
         break;
      case KEY_NPAGE:
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
         if (speed >= 300)
         {
            beep ();
            break;
         } // if
         speed += 10;
         if (playing == -1)
            break;
         kill_player ();
         read_text (playing, phrase_nr - 1);
         break;
      case 'D':
         if (speed <= 50)
         {
            beep ();
            break;
         } // if
         speed -= 10;
         if (playing == -1)
            break;
         kill_player ();
         read_text (playing, phrase_nr - 1);
         break;
      case KEY_HOME:
         speed = 160;
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
            phrase_nr = 1;
            playing++;
            if (eBook_struct[playing].level <= level)
               current = displaying = playing;
            if (just_this_item != -1 && eBook_struct[playing].level <= level)
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
   printf (gettext ("eBook-speaker - Version %s\n"), VERSION);
   puts ("(C)2011 J. Lemmens");
   playfile (PREFIX"share/eBook-speaker/error.wav", "1");
   printf (gettext ("\nUsage: %s eBook_file [-l language]\n"),
           basename (argv));
   fflush (stdout);
   _exit (1);
} // usage

char *sort_by_playorder ()
{
   int n, w;
   struct zip_file *r;

   snprintf (tmp_ncx, 200, "/tmp/eBook-speaker.XXXXXX");
   mkstemp (tmp_ncx);
   unlink (tmp_ncx);
   strcat (tmp_ncx, ".ncx");
   if (! (r = zip_fopen (eBook, OPF, ZIP_FL_UNCHANGED)))
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf (gettext ("Something is wrong with the %s file\n"), OPF);
      fflush (stdout);
      _exit (1);
   } // if
   if ((w = open (tmp_ncx, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf ("sort_by_playorder(%s)\n", tmp_ncx);
      fflush (stdout);
      _exit (1);
   } // if
   do
   {
      if (get_tag_or_label (r) == EOF)
         break;
      if (*element)
         dprintf (w, "%s\n", element);
      else
         dprintf (w, "%s\n", label);
   } while (strcasecmp (tag, "navmap") != 0);
   n = 1;
   do
   {
      *attribute.playorder = 0;
      if (get_tag_or_label (r) == EOF)
         break;
      if (atoi (attribute.playorder) == n)
      {
         dprintf (w, "%s\n", element);
         do
         {
            if (get_tag_or_label (r) == EOF)
               break;
            if (*element)
               dprintf (w, "%s\n", element);
            else
               dprintf (w, "%s\n", label);
         } while (strcasecmp (tag, "content") != 0);
         n++;
      } // if
   } while (strcasecmp (tag, "/ncx") != 0);
   close (w);
   zip_fclose (r);
   return tmp_ncx;
} // sort_by_playorder

char *open_eBook (char *file)
{
   int index;
   char *p;
   struct zip_file *opf;

   if (! (eBook = zip_open (file, 0, NULL)))
   {
      endwin ();
      printf ("%s is not an eBook!\n", file);
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      _exit (1);
   } // if
   index = 0;
   do
   {
      if (! (p = (char *)zip_get_name (eBook, index, ZIP_FL_UNCHANGED)))
         break;
      if (strcasecmp (p + strlen (p) - 4, ".opf") == 0)
         break;
   } while (index++ < zip_get_num_files (eBook));
   if (! p)
   {
      endwin ();
      printf ("File %s doesn't contain an eBook!\n", file);
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      _exit (1);
   } // if
   if (! (opf = zip_fopen (eBook, p, ZIP_FL_UNCHANGED)))
   {
      endwin ();
      printf ("Cannot read: %s: %s\n", p, strerror (errno));
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      _exit (1);
   } // if
   while (1)
   {
      if (get_tag_or_label (opf) == EOF)
         break;
      if (*attribute.ncc_totalTime)
         strncpy (ncc_totalTime, attribute.ncc_totalTime, 8);
      if (strcasecmp (tag, "dc:title") == 0)
      {
         do
         {
            get_tag_or_label (opf);
         } while (! *label);
         strncpy (eBook_title, label, 90);
      } // if
   } // while
   zip_fclose (opf);
   strncpy (prefix, p, 90);
   strncpy (prefix, dirname (prefix), 90);
   return p;
} // open_eBook

void set_language ()
{
   struct zip_file *opf;

   opf = zip_fopen (eBook, OPF, ZIP_FL_UNCHANGED);
   do
   {
      if (get_tag_or_label (opf) == EOF)
         break;
   } while (strcasecmp (tag, "dc:language") != 0);
   do
   {
      if (get_tag_or_label (opf) == EOF ||
          strcasecmp (tag, "/dc:language") == 0)
         break;
   } while (! *label);
   if (strcasecmp (label, "dut") == 0)
      strncpy (label, "nl", 5);
   if (! *label || strcasecmp (label, "UND") == 0)
   {
      endwin ();
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      printf ("Cannot determine the language of this eBook.\n");
      printf ("Please select one yourself:\n\n");
      printf ("   %s -l <language-code> <eBook>\n\n", prog_name);
      printf ("Do:\n\n");
      printf ("   espeak --voices\n\n");
      printf ("to get a list of available languages.\n");
      fflush (stdout);
      exit (1);
   } // if
   strncpy (dc_language, label, 5);
   zip_fclose (opf);
} // set_language

int main (int argc, char *argv[])
{
   struct zip_file *opf;
   int opt;
   char str[255], file[255];

   fclose (stderr); // discard SoX messages
   strncpy (prog_name, basename (argv[0]), 90);
   if (argc == 1)
      usage (prog_name);
   if (system ("espeak -h > /dev/null") > 0)
   {
      playfile (PREFIX"share/daisy-player/error.wav", "1");
      printf (gettext ("eBook-speaker needs the \"espeak\" programme.\n"));
      printf (gettext ("Please install it and try again.\n"));
      _exit (1);
   } // if
   speed = 160;
   atexit (quit_eBook_reader);
   read_rc ();
   setlocale (LC_ALL, getenv ("LANG"));
   setlocale (LC_NUMERIC, "C");
   textdomain (prog_name);
   bindtextdomain (prog_name, PREFIX"share/locale");
   textdomain (prog_name);
   opterr = 0;
   while ((opt = getopt (argc, argv, "l:")) != -1)
   {
      switch (opt)
      {
      case 'l':
         strncpy (dc_language, optarg, 5);
         break;
      default:
         usage (prog_name);
      } // switch
   } // while
   puts ("(C)2011 J. Lemmens");
   printf (gettext ("eBook-speaker - Version %s\n"), VERSION);
   puts (gettext ("Plays eBooks on Linux"));
   fflush (stdout);

// check if arg is an eBook
   struct stat st_buf;
   if (*argv[optind] == '/')
      snprintf (file, 250, "%s", argv[optind]);
   else
      snprintf (file, 250, "%s/%s", getenv ("PWD"), argv[optind]);
   if (stat (file, &st_buf) == -1)
   {
      printf ("stat: %s: %s\n", strerror (errno), file);
      playfile (PREFIX"share/eBook-speaker/error.wav", "1");
      _exit (1);
   } // if

// determine filetype
   magic_t myt;

   myt = magic_open (MAGIC_CONTINUE | MAGIC_MIME_TYPE);
   magic_load (myt, NULL);
   if (strcasecmp (magic_file (myt, file), "application/x-ms-reader") == 0)
   {
      char cmd[512], tmpname[255];

      chdir ("/tmp");
      strncpy (tmpname, tempnam (".", "eBook"), 200);
      snprintf (cmd, 500, "clit %s %s/ > /dev/null; \
                           cd %s; \
                           zip -q -1 -r ../%s.zip *", \
               file, tmpname, tmpname, tmpname);
      system (cmd);
      snprintf (file, 200, "/tmp/%s.zip", tmpname);
   } // if
   strncpy (OPF, open_eBook (file), 250);
   magic_close (myt);

   strcpy (daisy_version, "3");

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
   snprintf (str, 250, gettext ("eBook-speaker - Version %s - (C)2011 J. Lemmens"), VERSION);
   wprintw (titlewin, str);
   wrefresh (titlewin);

   if (strstr (daisy_version, "2.02"))
   {
      endwin ();
      puts ("daisy 2.02 is not supported by eBook-speaker");
      _exit (0);
   } // if
   if (strcmp (daisy_version, "3") == 0)
   {
      if ((opf = zip_fopen (eBook, OPF, ZIP_FL_NOCASE)) == NULL)
      {
         endwin ();
         playfile (PREFIX"share/eBook-speaker/error.wav", "1");
         printf (gettext ("\nCannot read %s\n"), OPF);
         fflush (stdout);
         _exit (1);
      } // if
      while (1)
      {
         if (get_tag_or_label (opf) == EOF)
            break;
         if (strcasecmp (tag, "dc:title") == 0)
         {
            do
            {
               if (get_tag_or_label (opf) == EOF)
                  break;
            } while (! *label);

            int i = 0;

            do
            {
               if (label[i] == '/')
                  label[i] = '_';
            } while (label[i++]);
            strncpy (bookmark_title, label, 90);
         } // if
      } // while
      zip_fclose (opf);
   } // if
   read_eBook_struct ();

   if (strlen (eBook_title) + strlen (str) >= 80)
      mvwprintw (titlewin, 0, 80 - strlen (eBook_title) - 4, "... ");
   mvwprintw (titlewin, 0, 80 - strlen (eBook_title), "%s", eBook_title);
   wrefresh (titlewin);
   mvwaddstr (titlewin, 1, 0, "----------------------------------------");
   waddstr (titlewin, "----------------------------------------");
   mvwprintw (titlewin, 1, 0, gettext ("Press 'h' for help "));
   wrefresh (titlewin);
   level = 1;
   *search_str = 0;
   if (! *dc_language)
      set_language ();
   browse ();
   return 0;                                              
} // main
