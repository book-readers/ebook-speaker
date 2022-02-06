/* daisy3.c - functions to insert daisy3 info into a struct.
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

#include "daisy.h"

void failure (char *str, int e)
{
   endwin ();
   beep ();
   printf ("\n%s: %s\n", str, strerror (e));
   fflush (stdout);
   _exit (-1);
} // failure

#ifdef DAISY_PLAYER
void parse_daisy_smil_3 (misc_t *misc, my_attribute_t *my_attribute,
                   daisy_t *daisy)
{
   int x;
   xmlTextReaderPtr parse;
   htmlDocPtr doc;

   misc->total_time = 0;
   for (x = 0; x < misc->total_items; x++)
   {
      if (*daisy[x].smil_file == 0)
         continue;
      doc = htmlParseFile (daisy[x].smil_file, "UTF-8");
      if (! (parse = xmlReaderWalker (doc)))
      {
         endwin ();
         beep ();
         printf ("\n");
         printf (gettext ("Cannot read %s"), daisy[x].smil_file);
         printf ("\n");
         fflush (stdout);
         _exit (1);
      } // if

// parse this smil
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, parse))
            break;
         if (strcasecmp (daisy[x].anchor, my_attribute->id) == 0)
            break;
      } // while
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, parse))
            break;
// get misc->clip_begin
         if (strcasecmp (misc->tag, "audio") == 0)
         {
            misc->has_audio_tag = 1;
            get_clips (misc, my_attribute);
            daisy[x].begin = misc->clip_begin;
            daisy[x].duration += misc->clip_end - misc->clip_begin;

// get clip_end
            while (1)
            {
               if (! get_tag_or_label (misc, my_attribute, parse))
                  break;
               if (*daisy[x + 1].anchor)
                  if (strcasecmp (my_attribute->id, daisy[x + 1].anchor) == 0)
                     break;
               if (strcasecmp (misc->tag, "audio") == 0)
               {
                  get_clips (misc, my_attribute);
                  daisy[x].duration += misc->clip_end - misc->clip_begin;
               } // IF
            } // while
            if (*daisy[x + 1].anchor)
               if (strcasecmp (my_attribute->id, daisy[x + 1].anchor) == 0)
                  break;
         } // if (strcasecmp (misc->tag, "audio") == 0)
      } // while
      misc->total_time += daisy[x].duration;
      xmlTextReaderClose (parse);
      xmlFreeDoc (doc);
   } // for
} // parse_daisy_smil_3

void get_page_number_3 (misc_t *misc, my_attribute_t *my_attribute)
{
   htmlDocPtr doc;
   xmlTextReaderPtr page;
   char *anchor = 0;

   do
   {
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
         return;
   } while (strcasecmp (misc->tag, "text") != 0);
   if (strchr (my_attribute->src, '#'))
   {
      anchor = strdup (strchr (my_attribute->src, '#') + 1);
      *strchr (my_attribute->src, '#') = 0;
   } // if
   doc = htmlParseFile (my_attribute->src, "UTF-8");
   if (! (page = xmlReaderWalker (doc)))
   {
      endwin ();
      beep ();
      printf ("\n");
      printf (gettext ("Cannot read %s"), my_attribute->src);
      printf ("\n");
      fflush (stdout);
      _exit (1);
   } // if
   do
   {
      if (! get_tag_or_label (misc, my_attribute, page))
      {
         xmlTextReaderClose (page);
         xmlFreeDoc (doc);
         return;
      } // if
   } while (strcasecmp (my_attribute->id, anchor) != 0);
   do
   {
      if (! get_tag_or_label (misc, my_attribute, page))
      {
         xmlTextReaderClose (page);
         xmlFreeDoc (doc);
         return;
      } // if
   } while (! *misc->label);
   xmlTextReaderClose (page);
   xmlFreeDoc (doc);
   misc->current_page_number = atoi (misc->label);
} // get_page_number_3

void get_clips (misc_t *misc, my_attribute_t *my_attribute)
{
   char begin_str[MAX_STR], *begin;
   char *orig_begin, *end;

   orig_begin = my_attribute->clip_begin;
   end = my_attribute->clip_end;
   if (misc->cd_type == CDIO_DISC_MODE_CD_DA)
      return;
   if (! *orig_begin)
      return;
   strncpy (begin_str, orig_begin,  MAX_STR - 1);
   begin = begin_str;
   while (! isdigit (*begin))
      begin++;
   if (strchr (begin, 's'))
      *strchr (begin, 's') = 0;
   if (! strchr (begin, ':'))
      misc->clip_begin = (float) atof (begin);
   else
      misc->clip_begin = read_time (begin);

// fill end
   while (! isdigit (*end))
      end++;
   if (strchr (end, 's'))
      *strchr (end, 's') = 0;
   if (! strchr (end, ':'))
      misc->clip_end = (float) atof (end);
   else
      misc->clip_end = read_time (end);
} // get_clips

float read_time (char *p)
{
   char *h, *m, *s;

   s = strrchr (p, ':') + 1;
   if (s > p)
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
   return atoi (h) * 3600 + atoi (m) * 60 + (float) atof (s);
} // read_time
#endif

void parse_page_number (misc_t *misc, my_attribute_t *my_attribute,
                        daisy_t *daisy, xmlTextReaderPtr page)
{
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, page))
         return;
      if (strcasecmp (misc->tag, "/par") == 0)
         return;
      if (strcasecmp (misc->tag, "text") == 0)
      {
         htmlDocPtr doc;
         xmlTextReaderPtr page;
         char str[MAX_STR + 1], *anchor = 0;

         strncpy (str, misc->daisy_mp, MAX_STR);
         strncat (str, "/", MAX_STR);
         strncat (str, my_attribute->src, MAX_STR);
         if (strchr (str, '#'))
         {
            anchor = strchr (str, '#') + 1;
            *strchr (str, '#') = 0;
         } // if
         doc = htmlParseFile (str, "UTF-8");
         if (! (page = xmlReaderWalker (doc)))
         {
            int e;
            char str2[MAX_STR];

            e = errno;
            snprintf (str2, MAX_STR, gettext ("Cannot read %s"), str);
            failure (str2, e);
         } // if
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, page))
            {
               xmlFreeDoc (doc);
               return;
            } // if
            if (strcasecmp (my_attribute->id, anchor) != 0)
               continue;
            do
            {
               if (! get_tag_or_label (misc, my_attribute, page))
               {
                  xmlFreeDoc (doc);
                  return;
               } // if
               if (strcasecmp (misc->tag, "/pagenum") == 0)
               {
                  xmlFreeDoc (doc);
                  return;
               } // if (strcasecmp (misc->tag, "/pagenum") == 0)
            } while (! misc->label);
            daisy[misc->current].page_number = atoi (misc->label);
            misc->current_page_number = atoi (misc->label);
            xmlFreeDoc (doc);
            return;
         } // while
      } // if (strcasecmp (misc->tag, "text") == 0)
   } // while
} // parse_page_number

void get_attributes (misc_t *misc, my_attribute_t *my_attribute,
                     xmlTextReaderPtr ptr)
{
   char attr[MAX_STR];

   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, BAD_CAST "class"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute->class, MAX_STR - 1, "%s", attr);
#ifdef EBOOK_SPEAKER
   if (misc->option_b == 0)
   {
      snprintf (attr, MAX_STR - 1, "%s", (char *)
                xmlTextReaderGetAttribute
                               (ptr, (xmlChar *) "break_on_eol"));
      if (strcmp (attr, "(null)"))
      {
         misc->break_on_EOL = attr[0];
         if (misc->break_on_EOL != 'y' && misc->break_on_EOL != 'n')
            misc->break_on_EOL = 'n';
      } // if
   } // if
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, BAD_CAST "my_class"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->my_class, attr, MAX_STR - 1);
#endif
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, BAD_CAST "clip-begin"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->clip_begin, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, BAD_CAST "clipBegin"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->clip_begin, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, BAD_CAST "clip-end"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->clip_end, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "clipEnd"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->clip_end, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "href"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->href, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "id"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->id, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "idref"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->idref, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "item"));
   if (strcmp (attr, "(null)"))
      misc->current = atoi (attr);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "level"));
   if (strcmp (attr, "(null)"))
      misc->level = atoi ((char *) attr);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
          xmlTextReaderGetAttribute (ptr, (const xmlChar *) "media-type"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->media_type, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "name"));
   if (strcmp (attr, "(null)"))
   {
      char name[MAX_STR], content[MAX_STR];

      *name = 0;
      strncpy (attr, (char *) xmlTextReaderGetAttribute
               (ptr, (const xmlChar *) "name"), MAX_STR - 1);
      if (strcmp (attr, "(null)"))
         strncpy (name, attr, MAX_STR - 1);
      *content = 0;
      snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "content"));
      if (strcmp (attr, "(null)"))
         strncpy (content, attr, MAX_STR - 1);
      if (strcasestr (name, "dc:format"))
         strncpy (misc->daisy_version, content, MAX_STR - 1);
      if (strcasestr (name, "dc:title") && ! *misc->daisy_title)
      {
         strncpy (misc->daisy_title, content, MAX_STR - 1);
         strncpy (misc->bookmark_title, content, MAX_STR - 1);
         if (strchr (misc->bookmark_title, '/'))
            *strchr (misc->bookmark_title, '/') = '-';
      } // if
/* don't use it!!!
      if (strcasestr (name, "dtb:misc->depth"))
         misc->depth = atoi (content);
*/
      if (strcasestr (name, "dtb:totalPageCount"))
         misc->total_pages = atoi (content);
/* don't use it!!!
      if (strcasestr (name, "ncc:misc->depth"))
         misc->depth = atoi (content);
*/
      if (strcasestr (name, "ncc:maxPageNormal"))
         misc->total_pages = atoi (content);
      if (strcasestr (name, "ncc:pageNormal"))
         misc->total_pages = atoi (content);
      if (strcasestr (name, "ncc:page-normal"))
         misc->total_pages = atoi (content);
      if (strcasestr (name, "dtb:totalTime")  ||
          strcasestr (name, "ncc:totalTime"))
      {
         strncpy (misc->ncc_totalTime, content, MAX_STR - 1);
         if (strchr (misc->ncc_totalTime, '.'))
            *strchr (misc->ncc_totalTime, '.') = 0;
      } // if
   } // if
   snprintf (attr, MAX_STR - 1, "%s", (char *)
           xmlTextReaderGetAttribute (ptr, (const xmlChar *) "playorder"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->playorder, attr, MAX_STR - 1);
#ifdef EBOOK_SPEAKER
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "phrase"));
   if (strcmp (attr, "(null)"))
      misc->phrase_nr = atoi ((char *) attr);
#endif
#ifdef DAISY_PLAYER
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "seconds"));
   if (strcmp (attr, "(null)"))
   {
      misc->seconds = atoi (attr);
      if (misc->seconds < 0)
         misc->seconds = 0;
   } // if
#endif
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "smilref"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->smilref, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
           xmlTextReaderGetAttribute (ptr, (const xmlChar *) "sound_dev"));
   if (strcmp (attr, "(null)"))
      strncpy (misc->sound_dev, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
        xmlTextReaderGetAttribute (ptr, (const xmlChar *) "ocr_language"));
   if (strcmp (attr, "(null)"))
      strncpy (misc->ocr_language, attr, 5);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "cd_dev"));
   if (strcmp (attr, "(null)"))
      strncpy (misc->cd_dev, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
           xmlTextReaderGetAttribute (ptr, (const xmlChar *) "cddb_flag"));
   if (strcmp (attr, "(null)"))
      misc->cddb_flag = (char) attr[0];
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "speed"));
   if (strcmp (attr, "(null)"))
   {
      misc->speed = atof (attr);
      if (misc->speed <= 0.1 || misc->speed >= 2)
         misc->speed = 1.0;
   } // if
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "src"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->src, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "tts"));
   if (strcmp (attr, "(null)"))
      misc->tts_no = atof ((char *) attr);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "toc"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->toc, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "value"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->value, attr, MAX_STR - 1);
} // get_attributes

int get_tag_or_label (misc_t *misc, my_attribute_t *my_attribute,
                      xmlTextReaderPtr reader)
{
   int type;

   *misc->tag = 0;
   if (misc->label)
      free (misc->label);
   misc->label = malloc (1);
   *misc->label = 0;
#ifdef EBOOK_SPEAKER
   *my_attribute->my_class = 0;
#endif
   *my_attribute->class =
   *my_attribute->clip_begin = *my_attribute->clip_end =
   *my_attribute->href = *my_attribute->id = *my_attribute->media_type =
   *my_attribute->playorder = * my_attribute->smilref = *my_attribute->src =
   *my_attribute->toc = *my_attribute->value = 0;

   if (! reader)
      return 0;
   switch (xmlTextReaderRead (reader))
   {
   case -1:
      failure ("xmlTextReaderRead ()\n"
               "Can't handle this DTB structure!\n"
               "Don't know how to handle it yet, sorry. :-(\n", errno);
   case 0:
      return 0;
   case 1:
      break;
   } // swwitch
   type = xmlTextReaderNodeType (reader);
   switch (type)
   {
   int n, i;

   case -1:
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read type: %d"), type);
      failure (str, e);
   }
   case XML_READER_TYPE_ELEMENT:
      strncpy (misc->tag, (char *) xmlTextReaderConstName (reader),
               MAX_TAG - 1);
      n = xmlTextReaderAttributeCount (reader);
      for (i = 0; i < n; i++)
         get_attributes (misc, my_attribute, reader);
      return 1;
   case XML_READER_TYPE_END_ELEMENT:
      snprintf (misc->tag, MAX_TAG - 1, "/%s",
                (char *) xmlTextReaderName (reader));
      return 1;
   case XML_READER_TYPE_TEXT:
   case XML_READER_TYPE_CDATA:
   case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
   {
      int x;
      size_t phrase_len;

      phrase_len = strlen ((char *) xmlTextReaderConstValue (reader));
      x = 0;
      while (1)
      {
         if (isspace (xmlTextReaderValue (reader)[x]))
            x++;
         else
            break;
      } // while
      misc->label = (char *) realloc (misc->label, phrase_len + 1);
      snprintf (misc->label, phrase_len - x + 1, "%s", (char *)
               xmlTextReaderValue (reader) + x);
      for (x = strlen (misc->label) - 1; x >= 0 && isspace (misc->label[x]);
           x--)
         misc->label[x] = 0;
      for (x = 0; misc->label[x] > 0; x++)
         if (! isprint (misc->label[x]))
            misc->label[x] = ' ';
      return 1;
   }
   case XML_READER_TYPE_ENTITY_REFERENCE:
      snprintf (misc->tag, MAX_TAG - 1, "/%s",
                (char *) xmlTextReaderName (reader));
      return 1;
   default:
      return 1;
   } // switch
   return 0;
} // get_tag_or_label

void parse_content (misc_t *misc, my_attribute_t *my_attribute,
                    daisy_t *daisy, char *src)
{
   htmlDocPtr doc;
   xmlTextReaderPtr content;

   strncpy (daisy[misc->current].smil_file, misc->daisy_mp,
            MAX_STR - 1);
   strncat (daisy[misc->current].smil_file, "/", MAX_STR - 1);
   strncat (daisy[misc->current].smil_file, src, MAX_STR - 1);
   *daisy[misc->current].anchor = 0;
   if (strchr (daisy[misc->current].smil_file, '#'))
   {
      strncpy (daisy[misc->current].anchor,
               strchr (daisy[misc->current].smil_file, '#') + 1, MAX_STR - 1);
      *strchr (daisy[misc->current].smil_file, '#') = 0;
   } // if
   if (daisy[misc->current].x == 0)
      daisy[misc->current].x = 2;
   doc = htmlParseFile (daisy[misc->current].smil_file, "UTF-8");
   if (! (content = xmlReaderWalker (doc)))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR,
             gettext ("Cannot read %s"), daisy[misc->current].smil_file);
      failure (str, e);
   } // if

   if (*daisy[misc->current].anchor)
   {
      do
      {
         if (! get_tag_or_label (misc, my_attribute, content))
            break;
      } while (
             strcasecmp (my_attribute->id, daisy[misc->current].anchor) != 0);
   } // if
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, content))
         break;
      if (strcasecmp (my_attribute->class, "pagenum") == 0)
      {
         parse_page_number (misc, my_attribute, daisy, content);
         continue;
      } // if
      if (strcasecmp (misc->tag, "text") == 0)
      {
         parse_text (misc, my_attribute, daisy);
         break;
      } // if
   } // while
   xmlFreeDoc (doc);
} // parse_content

void fill_item (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy,
                int level, xmlTextReaderPtr label)
{
   if (level > 6)
      level = 1;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, label))
         break;
      if (*misc->label)
      {
         snprintf (daisy[misc->current].label, 80, "%s", misc->label);
         break;
      } // if
   } // while
   daisy[misc->current].level = level;
   if (daisy[misc->current].level < 1)
      daisy[misc->current].level = 1;
   if (daisy[misc->current].level > misc->depth)
      misc->depth = daisy[misc->current].level;
   daisy[misc->current].x = daisy[misc->current].level * 3 - 1;
   daisy[misc->current].screen = misc->current / misc->max_y;
   daisy[misc->current].y =
                misc->current - (daisy[misc->current].screen * misc->max_y);
   daisy[misc->current].label[61 - daisy[misc->current].x] = 0;
   if (misc->displaying == misc->max_y)
      misc->displaying = 1;
#ifdef EBOOK_SPEAKER
   daisy[misc->current].n_phrases = 0;
#endif
} // fill_item

void parse_text (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   htmlDocPtr doc;
   xmlTextReaderPtr text;

   strncpy (daisy[misc->current].smil_file, misc->daisy_mp, MAX_STR - 1);
   strncat (daisy[misc->current].smil_file, "/", MAX_STR - 1);
   strncat (daisy[misc->current].smil_file, my_attribute->src, MAX_STR - 1);
   if (strchr (daisy[misc->current].smil_file, '#'))
   {
      strncpy (daisy[misc->current].anchor,
               strchr (daisy[misc->current].smil_file, '#') + 1,
               MAX_STR - 1);
      *strchr (daisy[misc->current].smil_file, '#') = 0;
   } // if
   doc = htmlParseFile (daisy[misc->current].smil_file, "UTF-8");
   if (! (text = xmlReaderWalker (doc)))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"),
                daisy[misc->current].smil_file);
      failure (str, e);
   } // if

   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, text))
         break;
      if (strcasecmp (misc->tag, "pagenum") == 0)
      {
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, text))
               break;
            if (*misc->label)
            {
               daisy[misc->current].page_number = atoi (misc->label);
               break;
            } // if
         } // while
      } // if pagenum
      if (*daisy[misc->current].anchor &&
          strcasecmp (my_attribute->id, daisy[misc->current].anchor) != 0)
         continue;
      while (1)
      {
         if (strcasecmp (misc->tag, "h1") == 0 ||
             strcasecmp (misc->tag, "h2") == 0 ||
             strcasecmp (misc->tag, "h3") == 0 ||
             strcasecmp (misc->tag, "h4") == 0 ||
             strcasecmp (misc->tag, "h5") == 0 ||
             strcasecmp (misc->tag, "h6") == 0)
         {
            fill_item (misc, my_attribute, daisy, misc->tag[1] - 48, text);
            misc->total_items = misc->current + 1;
            return;
         } // if h1
      } // while   
   } // while
} // parse_text

void parse_href (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   char str[MAX_STR];
   htmlDocPtr doc;
   xmlTextReaderPtr href;

   daisy[misc->current].x = 2;
   daisy[misc->current].level = 1;
   strncpy (str, misc->daisy_mp, MAX_STR - 1);
   strncat (str, "/", MAX_STR - 1);
   strncat (str, my_attribute->href, MAX_STR - 1);
   doc = htmlParseFile (str, "UTF-8");
   if (! (href = xmlReaderWalker (doc)))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"), my_attribute->href);
      failure (str, e);
   } // if

   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, href))
         break;

// some EPUB use this
      if (strcasecmp (misc->tag, "i") == 0)
      {
         strncpy (daisy[misc->current].smil_file, str, MAX_STR - 1);
//         fill_item (misc, my_attribute, daisy, misc->tag[1] - 48, href);
         break;
      } // if

// or
      if (strcasecmp (misc->tag, "h1") == 0 ||
          strcasecmp (misc->tag, "h2") == 0 ||
          strcasecmp (misc->tag, "h3") == 0 ||
          strcasecmp (misc->tag, "h4") == 0 ||
          strcasecmp (misc->tag, "h5") == 0 ||
          strcasecmp (misc->tag, "h6") == 0)
      {
         strncpy (daisy[misc->current].smil_file, str, MAX_STR - 1);
         fill_item (misc, my_attribute, daisy, misc->tag[1] - 48, href);
         if (! *daisy[misc->current].label)
            continue;
         continue;
      } // if (strcasecmp (misc->tag, "h1") == 0 ||

// others use this
      if (strcasecmp (my_attribute->class, "h1") == 0 ||
          strcasecmp (my_attribute->class, "h2") == 0 ||
          strcasecmp (my_attribute->class, "h3") == 0 ||
          strcasecmp (my_attribute->class, "h4") == 0 ||
          strcasecmp (my_attribute->class, "h5") == 0 ||
          strcasecmp (my_attribute->class, "h6") == 0)
      {
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, href))
               break;
            if (strcasecmp (misc->tag, "text") == 0)
            {
               parse_text (misc, my_attribute, daisy);
               break;
            } // if
            if (strcasecmp (misc->tag, "/par") == 0)
               break;
         } // while
         continue;
      } // if (strcasecmp (my_attribute->class, "h1") == 0 ||

      if (strcasecmp (my_attribute->class, "pagenum") == 0)
         parse_page_number (misc, my_attribute, daisy, href);
   } // while
} // parse_href

void parse_manifest (misc_t *misc, my_attribute_t *my_attribute,
                     daisy_t *daisy)
{
   xmlTextReaderPtr manifest;
   htmlDocPtr doc;
   char idref[MAX_STR + 1];

   strncpy (idref, my_attribute->idref, MAX_STR);
   if (! (doc = htmlParseFile (misc->opf_name, "UTF-8")))
      failure (misc->opf_name, errno);
   if (! (manifest = xmlReaderWalker (doc)))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"), misc->opf_name);
      failure (str, e);
   } // if
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, manifest))
         break;
      if (strcasecmp (my_attribute->id, idref) == 0)
      {
         strncpy (daisy[misc->current].smil_file, misc->daisy_mp,
                  MAX_STR - 1);
         strncat (daisy[misc->current].smil_file, "/", MAX_STR - 1);
         strncat (daisy[misc->current].smil_file, my_attribute->href,
                  MAX_STR - 1);
         *daisy[misc->current].anchor = 0;
         if (strchr (daisy[misc->current].smil_file, '#'))
         {
            strncpy (daisy[misc->current].anchor,
                     strchr (daisy[misc->current].smil_file, '#') + 1,
                     MAX_STR - 1);
            *strchr (daisy[misc->current].smil_file, '#') = 0;
         } // if
         parse_href (misc, my_attribute, daisy);
         misc->current++;
         if (misc->current > misc->total_items && 0)
         {
            misc->total_items = misc->current;
            daisy = (daisy_t *)
                       realloc (daisy, misc->total_items * sizeof (daisy_t));
         } // if
         break;
      } // if
   } // while
} // parse_manifest

void parse_opf (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   htmlDocPtr doc;
   xmlTextReaderPtr opf;

   if (misc->items_in_opf == 0)
      return;
   misc->total_items = misc->items_in_opf;
   if (! (doc = htmlParseFile (misc->opf_name, "UTF-8")))
   {
      int e;
      char str[MAX_STR + 1];

      e = errno;
      beep ();
      snprintf (str, MAX_STR, "parse_opf: %s", misc->opf_name);
      failure (str, e);
   } // if
   if ((opf = xmlReaderWalker (doc)) == NULL)
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"), misc->opf_name);
      failure (str, e);
   } // if
   misc->current = misc->displaying = 0;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, opf))
         break;
      if (strcasecmp (misc->tag, "language") == 0)
      {
         if (xmlTextReaderIsEmptyElement (opf))
            continue;
         do
         {
            if (! get_tag_or_label (misc, my_attribute, opf))
               break;
         } while (! *misc->label);
         strncpy (misc->daisy_language, misc->label, MAX_STR - 1);
      } // if language
      if (strcasecmp (misc->tag, "totalTime") == 0)
      {
         if (xmlTextReaderIsEmptyElement (opf))
            continue;
         do
         {
            if (! get_tag_or_label (misc, my_attribute, opf))
               break;
         } while (! *misc->label);
         strncpy (misc->ncc_totalTime, misc->label, MAX_STR - 1);
      } // if (strcasestr (misc->tag, "totalTime") == 0)
      if (strcasecmp (misc->tag, "Title") == 0)
      {
         if (xmlTextReaderIsEmptyElement (opf))
            continue;
         do
         {
            if (! get_tag_or_label (misc, my_attribute, opf))
               break;
            if (*misc->label)
            {
               snprintf (misc->daisy_title, 75, "%s", misc->label);
               snprintf (misc->bookmark_title, MAX_STR - 1,
                         "%s", misc->label);
               if (strchr (misc->bookmark_title, '/'))
                  *strchr (misc->bookmark_title, '/') = '-';
               break;
            } // if
         } while (strcasestr (misc->tag, "/title") != 0);
      } // if title
      if (strcasecmp (misc->tag, "spine") == 0)
      {
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, opf))
               break;
            if (strcasecmp (misc->tag, "itemref") == 0)
               parse_manifest (misc, my_attribute, daisy);
         } // while
         misc->total_items = misc->current;
      } // spine
   } // while
} // parse_opf

daisy_t *create_daisy_struct (misc_t *misc, my_attribute_t *my_attribute)
{
   xmlTextReaderPtr ncx;
   htmlDocPtr doc;
   char cmd[MAX_CMD];
   FILE *p;

   *misc->daisy_version = 0;

// lookfor "ncc.html"
   snprintf (cmd, MAX_CMD - 1,
             "find \"%s\" -maxdepth 5 -iname \"ncc.html\" -print0",
             misc->tmp_dir);
   if ((p = popen (cmd, "r")) == NULL)
      failure (cmd, errno);
   switch (fread (misc->NCC_HTML, MAX_STR - 1, 1, p))
   {
   default:
      break;
   } // switch
   if (*misc->NCC_HTML)
   {
      strncpy (misc->daisy_version, "2.02", 4);
      doc = htmlParseFile (misc->NCC_HTML, "UTF-8");
      ncx = xmlReaderWalker (doc);
      misc->total_items = 0;
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, ncx))
            break;
         if (strcasecmp (misc->tag, "h1") == 0 ||
             strcasecmp (misc->tag, "h2") == 0 ||
             strcasecmp (misc->tag, "h3") == 0 ||
             strcasecmp (misc->tag, "h4") == 0 ||
             strcasecmp (misc->tag, "h5") == 0 ||
             strcasecmp (misc->tag, "h6") == 0)
         {
            misc->total_items++;
         } // if
      } // while
      xmlTextReaderClose (ncx);
      if (doc)
         xmlFreeDoc (doc);
      return (daisy_t *) malloc ((misc->total_items + 1) * sizeof (daisy_t));
   } // if ncc.html

// lookfor *.ncx"
   snprintf (cmd, MAX_CMD - 1,
             "find \"%s\" -maxdepth 5 -iname \"*.ncx\" -print0",
             misc->tmp_dir);
   if ((p = popen (cmd, "r")) == NULL)
      failure (cmd, errno);
   switch (fread (misc->ncx_name, MAX_STR - 1, 1, p))
   {
   default:
      break;
   } // switch
   pclose (p);
   if (*misc->ncx_name)
      strncpy (misc->daisy_version, "3", 2);
   if (strlen (misc->ncx_name) < 4)
      *misc->ncx_name = 0;

// lookfor "*.opf""
   snprintf (cmd, MAX_STR - 1,
             "find \"%s\" -maxdepth 5 -iname \"*.opf\" -print0",
             misc->tmp_dir);
   p = popen (cmd, "r");
   switch (*fgets (misc->opf_name, MAX_STR - 1, p))
   {
   default:
      break;
   } // switch
   pclose (p);
   if (*misc->opf_name)
      strncpy (misc->daisy_version, "3", 2);
   if (strlen (misc->opf_name) < 4)
      *misc->opf_name = 0;

// count items in opf
   misc->items_in_opf = 0;
   doc = htmlParseFile (misc->opf_name, "UTF-8");
   misc->reader = xmlReaderWalker (doc);
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
         break;
      if (strcasecmp (misc->tag, "itemref") == 0)
         misc->items_in_opf++;
   } // while

// count items in ncx
   misc->items_in_ncx = 0;
   doc = htmlParseFile (misc->ncx_name, "UTF-8");
   misc->reader = xmlReaderWalker (doc);
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
         break;
      if (xmlTextReaderIsEmptyElement (misc->reader))
         continue;
      if (strcasecmp (misc->tag, "navpoint") == 0)
         misc->items_in_ncx++;
   } // while

   if (misc->items_in_ncx == 0 && misc->items_in_opf == 0)
   {
      endwin ();
      printf ("Please try to play this book with daisy-player");
      beep ();
#ifdef DAISY_PLAYER
      quit_daisy_player (misc);
#else
      quit_eBook_speaker (misc);
#endif
      _exit (0);
   } // if
   xmlFreeDoc (doc);

   if (misc->items_in_ncx > misc->items_in_opf)
      misc->total_items = misc->items_in_ncx;
   else
      misc->total_items = misc->items_in_opf;
   return (daisy_t *) malloc (misc->total_items * sizeof (daisy_t));
} // create_daisy_struct

void read_daisy_3 (misc_t *misc, my_attribute_t *my_attribute,
                   daisy_t *daisy)
{
   char str[MAX_STR + 1];
   int i;

   misc->total_items = misc->items_in_opf;
   strncpy (str, misc->opf_name, MAX_STR);
   strncpy (misc->daisy_mp, dirname (str), MAX_STR);
   parse_opf (misc, my_attribute, daisy);
   misc->total_items = misc->items_in_opf;
#ifdef EBOOK_SPEAKER
   count_phrases (misc, my_attribute, daisy);
   misc->phrases_in_opf = misc->total_phrases;
#endif

   misc->total_items = misc->items_in_ncx;
   strncpy (str, misc->ncx_name, MAX_STR);
   strncpy (misc->daisy_mp, dirname (str), MAX_STR);
   parse_ncx (misc, my_attribute, daisy);
   misc->total_items = misc->items_in_ncx;
#ifdef EBOOK_SPEAKER
   count_phrases (misc, my_attribute, daisy);
   misc->phrases_in_ncx = misc->total_phrases;
#endif

#ifdef EBOOK_SPEAKER
   if (misc->phrases_in_ncx > misc->phrases_in_opf)
   {
      misc->total_phrases = misc->phrases_in_ncx;
      parse_ncx (misc, my_attribute, daisy);
      for (i = 0; i < misc->items_in_ncx; i++)
         strncpy (daisy[i].orig_smil, daisy[i].smil_file, MAX_STR - 1);
      return;
   } // if

   misc->total_phrases = misc->phrases_in_opf;
   parse_opf (misc, my_attribute, daisy);
   for (i = 0; i < misc->items_in_opf; i++)
      strncpy (daisy[i].orig_smil, daisy[i].smil_file, MAX_STR - 1);
   return;
#endif
   if (misc->items_in_opf > misc->items_in_ncx)
   {
      parse_opf (misc, my_attribute, daisy);
      return;
   } // if
   parse_ncx (misc, my_attribute, daisy);
} // read_daisy_3

void parse_ncx (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   htmlDocPtr doc;
   xmlTextReaderPtr ncx;

   if (misc->items_in_ncx == 0)
      return;
   misc->total_items = misc->items_in_ncx;
   if (! (doc = htmlParseFile (misc->ncx_name, "UTF-8")))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"), misc->ncx_name);
      failure (str, e);
   } // if
   ncx = xmlReaderWalker (doc);
   misc->current = misc->displaying = misc->level = misc->depth = 0;
   while (1)
   {
      daisy[misc->current].level = daisy[misc->current].page_number =
      daisy[misc->current].x = daisy[misc->current].y = 0;
      if (! get_tag_or_label (misc, my_attribute, ncx))
         break;
      if (strcasecmp (misc->tag, "docTitle") == 0)
      {
         *misc->label = 0;
         if (xmlTextReaderIsEmptyElement (misc->reader))
            continue;
         do
         {
            if (! get_tag_or_label (misc, my_attribute, ncx))
               break;
         } while (*misc->label == 0 &&
                  strcasecmp (misc->tag, "/docTitle") != 0);
         if (*misc->label)
         {
            snprintf (misc->daisy_title, 80, "%s", misc->label);
            strncpy (misc->bookmark_title, misc->label, MAX_STR - 1);
         } // if
         if (strchr (misc->bookmark_title, '/'))
            *strchr (misc->bookmark_title, '/') = '-';
      } // if (strcasecmp (misc->tag, "docTitle") == 0)
      if (strcasecmp (misc->tag, "docAuthor") == 0)
      {
         if (xmlTextReaderIsEmptyElement (misc->reader))
            continue;
         do
         {
            if (! get_tag_or_label (misc, my_attribute, ncx))
               break;
         } while (strcasecmp (misc->tag, "/docAuthor") != 0);
      } // if (strcasecmp (misc->tag, "docAuthor") == 0)
      if (strcasecmp (misc->tag, "navpoint") == 0)
      {
         misc->level++;
         if (misc->level < 1)
            misc->level = 1;
         if (misc->level > misc->depth)
            misc->depth = misc->level;
         daisy[misc->current].playorder = atoi (my_attribute->playorder);
#ifdef EBOOK_SPEAKER
         strncpy (daisy[misc->current].my_class, my_attribute->my_class,
                  MAX_STR - 1);
#endif
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, ncx))
               break;
            if (strcasecmp (misc->tag, "text") == 0)
            {
               if (xmlTextReaderIsEmptyElement (ncx))
                  continue;
               do
               {
                  if (! get_tag_or_label (misc, my_attribute, ncx))
                     break;
               } while (! *misc->label);
               snprintf (daisy[misc->current].label, 80, "%s", misc->label);
               if (daisy[misc->current].level < 1)
                  daisy[misc->current].level = 1;
            } //  if (strcasecmp (misc->tag, "text") == 0)
            if (strcasecmp (misc->tag, "content") == 0)
            {
               parse_content (misc, my_attribute, daisy, my_attribute->src);
               if (daisy[misc->current].level < 1)
                  daisy[misc->current].level = 1;
               misc->current++;
               daisy[misc->current].level = daisy[misc->current].page_number =
               daisy[misc->current].x = daisy[misc->current].y = 0;
               break;
            } // if (strcasecmp (misc->tag, "content") == 0)
         } // while
      } // if (strcasecmp (misc->tag, "navpoint") == 0)
      if (strcasecmp (misc->tag, "/navpoint") == 0)
         misc->level--;
   } // while
   misc->total_items = misc->current;
} // parse_ncx
