/* daisy3.c - functions to insert daisy3 info into a struct.
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

#include "daisy.h"

extern struct my_attribute my_attribute;
int current, displaying, max_y, total_items, current_page_number;
int phrase_nr, tts_no, level, depth, total_time, audiocd, total_phrases;
float speed;
extern char daisy_title[], bookmark_title[], prog_name[];
extern char tag[], label[], sound_dev[], cd_dev[], cddb_flag;
extern char daisy_version[], daisy_mp[], opf_name[], ncx_name[], NCC_HTML[];
float clip_begin, clip_end;
int total_pages;
char ncc_totalTime[MAX_STR];
char daisy_language[MAX_STR];
extern daisy_t daisy[];
time_t seconds;

void parse_ncx (char *);
void check_phrases ();

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

void get_clips (char *orig_begin, char *end)
{
   char begin_str[MAX_STR], *begin;

   if (audiocd == 1)
      return;
   strncpy (begin_str, orig_begin,  MAX_STR - 1);
   begin = begin_str;
   while (! isdigit (*begin))
      begin++;
   if (strchr (begin, 's'))
      *strchr (begin, 's') = 0;
   if (! strchr (begin, ':'))
      clip_begin = (float) atof (begin);
   else
      clip_begin = read_time (begin);

// fill end
   while (! isdigit (*end))
      end++;
   if (strchr (end, 's'))
      *strchr (end, 's') = 0;
   if (! strchr (end, ':'))
      clip_end = (float) atof (end);
   else
      clip_end = read_time (end);
} // get_clips

void get_attributes (xmlTextReaderPtr reader)
{
   char attr[MAX_STR];

   snprintf (attr, MAX_STR - 1, "%s", (char*)
        xmlTextReaderGetAttribute (reader, (const xmlChar *) "class"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.class, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "clip-begin"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.clip_begin, MAX_STR - 1,
                "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
             xmlTextReaderGetAttribute (reader,
                    (const xmlChar *) "clipBegin"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.clip_begin, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "clip-end"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.clip_end, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "clipEnd"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.clip_end, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "href"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.href, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "id"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.id, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "idref"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.idref, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "item"));
   if (strcmp (attr, "(null)"))
      current = atoi (attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "level"));
   if (strcmp (attr, "(null)"))
      level = atoi ((char *) attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "media-type"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.media_type, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "name"));
   if (strcmp (attr, "(null)"))
   {
      char name[MAX_STR], content[MAX_STR];

      *name = 0;
      snprintf (attr, MAX_STR - 1, "%s", (char *)
                xmlTextReaderGetAttribute (reader, (const xmlChar *) "name"));
      if (strcmp (attr, "(null)"))
         snprintf (name, MAX_STR - 1, "%s", attr);
      *content = 0;
      snprintf (attr, MAX_STR - 1, "%s", (char *)
                xmlTextReaderGetAttribute (reader, (const xmlChar *) "content"));
      if (strcmp (attr, "(null)"))
         snprintf (content, MAX_STR - 1, "%s", attr);
      if (strcasestr (name, "dc:format"))
         snprintf (daisy_version, MAX_STR - 1, "%s", content);
      if (strcasestr (name, "dc:title") && ! *daisy_title)
      {
         snprintf (daisy_title, MAX_STR - 1, "%s", content);
         snprintf (bookmark_title, MAX_STR - 1, "%s", content);
         if (strchr (bookmark_title, '/'))
            *strchr (bookmark_title, '/') = '-';
      } // if
/* don't use it
      if (strcasestr (name, "dtb:depth"))
         depth = atoi (content);
*/
      if (strcasestr (name, "dtb:totalPageCount"))
         total_pages = atoi (content);
      if (strcasestr (name, "ncc:depth"))
         depth = atoi (content);
      if (strcasestr (name, "ncc:maxPageNormal"))
         total_pages = atoi (content);
      if (strcasestr (name, "ncc:pageNormal"))
         total_pages = atoi (content);
      if (strcasestr (name, "ncc:page-normal"))
         total_pages = atoi (content);
      if (strcasestr (name, "dtb:totalTime")  ||
          strcasestr (name, "ncc:totalTime"))
      {
         snprintf (ncc_totalTime, MAX_STR - 1, "%s", content);
         if (strchr (ncc_totalTime, '.'))
            *strchr (ncc_totalTime, '.') = 0;
      } // if
   } // if
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "playorder"));
   if (strcmp (attr, "(null)"))                                  
      snprintf (my_attribute.playorder, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "phrase"));
   if (strcmp (attr, "(null)"))
      phrase_nr = atoi ((char *) attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "seconds"));
   if (strcmp (attr, "(null)"))
   {
      seconds = atoi (attr);
      if (seconds < 0)
         seconds = 0;
   } // if
   snprintf (attr, MAX_STR - 1, "%s", (char*)
          xmlTextReaderGetAttribute (reader, (const xmlChar *) "smilref"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.smilref, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "sound_dev"));
   if (strcmp (attr, "(null)"))
      snprintf (sound_dev, MAX_STR, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "cd_dev"));
   if (strcmp (attr, "(null)"))
      snprintf (cd_dev, MAX_STR, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "cddb_flag"));
   if (strcmp (attr, "(null)"))
      cddb_flag = (char) attr[0];
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "speed"));
   if (strcmp (attr, "(null)"))
      speed = atof (attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "src"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.src, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "tts"));
   if (strcmp (attr, "(null)"))
      tts_no = atof ((char *) attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "toc"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.toc, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "value"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.value, MAX_STR - 1, "%s", attr);
} // get_attributes

int get_tag_or_label (xmlTextReaderPtr local_reader)
{
   int type;

   *tag =  *label = 0;
   *my_attribute.class = *my_attribute.clip_begin = *my_attribute.clip_end =
   *my_attribute.href = *my_attribute.id = *my_attribute.media_type =
   *my_attribute.playorder = * my_attribute.smilref = *my_attribute.src =
   *my_attribute.toc = 0, *my_attribute.value = 0;

   if (! local_reader)
      return 0;
   switch (xmlTextReaderRead (local_reader))
   {
   case -1:
      endwin ();
      printf ("%s\n", strerror (errno));
      printf ("Can't handle this DTB structure!\n");
      printf ("Don't know how to handle it yet, sorry. :-(\n");
      beep ();
      fflush (stdout);
      _exit (1);
   case 0:
      return 0;
   case 1:
      break;
   } // swwitch
   type = xmlTextReaderNodeType (local_reader);
   switch (type)
   {
   int n, i;

   case -1:
      endwin ();
      beep ();
      printf (gettext ("\nCannot read\n"));
      fflush (stdout);
      _exit (1);
   case XML_READER_TYPE_ELEMENT:
      strncpy (tag, (char *) xmlTextReaderConstName (local_reader),
               MAX_TAG - 1);
      n = xmlTextReaderAttributeCount (local_reader);
      for (i = 0; i < n; i++)
         get_attributes (local_reader);
      return 1;
   case XML_READER_TYPE_END_ELEMENT:
      snprintf (tag, MAX_TAG - 1, "/%s",
                (char *) xmlTextReaderName (local_reader));
      return 1;
   case XML_READER_TYPE_TEXT:
   {
      int x;

      x = 0;
      while (1)
      {
         if (isspace (xmlTextReaderConstValue (local_reader)[x]))
            x++;
         else
            break;
      } // while
      strncpy (label, (char *) xmlTextReaderConstValue (local_reader) + x,
                      max_phrase_len);
      for (x = strlen (label) - 1; x >= 0 && isspace (label[x]); x--)
         label[x] = 0;
      for (x = 0; label[x] > 0; x++)
         if (! isascii (label[x]))
            label[x] = ' ';
      return 1;
   }
   case XML_READER_TYPE_ENTITY_REFERENCE:
   case XML_READER_TYPE_DOCUMENT_TYPE:
   case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
//      snprintf (tag, MAX_TAG - 1, "/%s",
//                (char *) xmlTextReaderName (local_reader));
      return 1;
   default:
      return 1;
   } // switch
   return 0;
} // get_tag_or_label        

void parse_text_file (char *text_file)
// page-number
{
   xmlTextReaderPtr textptr;
   char *anchor = 0;

   if (strchr (text_file, '#'))
   {
      anchor = strdup (strchr (text_file, '#') + 1);
      *strchr (text_file, '#') = 0;
   } // if
   xmlDocPtr doc = xmlRecoverFile (text_file);
   if (! (textptr = xmlReaderWalker (doc)))
   {
      endwin ();
      beep ();
      printf (gettext ("\nCannot read %s\n"), text_file);
      fflush (stdout);
      _exit (1);
   } // if
   if (anchor)
   {
      do
      {
         if (! get_tag_or_label (textptr))
         {
            xmlTextReaderClose (textptr);
            xmlFreeDoc (doc);
            return;
         } // if
      } while (strcasecmp (my_attribute.id, anchor) != 0);
   } // if
   do
   {
      if (! get_tag_or_label (textptr))
      {
         xmlTextReaderClose (textptr);
         xmlFreeDoc (doc);
         return;
      } // if
   } while (! *label);
   daisy[current].page_number = atoi (label);
   xmlTextReaderClose (textptr);
   xmlFreeDoc (doc);
} // parse_text_file

void get_page_number_3 (xmlTextReaderPtr reader)
{
   xmlDocPtr doc;
   xmlTextReaderPtr page;
   char *anchor = 0;

   do
   {
      if (! get_tag_or_label (reader))
         return;
   } while (strcasecmp (tag, "text") != 0);
   if (strchr (my_attribute.src, '#'))
   {
      anchor = strdup (strchr (my_attribute.src, '#') + 1);
      *strchr (my_attribute.src, '#') = 0;
   } // if
   doc = xmlRecoverFile (my_attribute.src);
   if (! (page = xmlReaderWalker (doc)))
   {
      endwin ();
      beep ();
      printf (gettext ("\nCannot read %s\n"), my_attribute.src);
      fflush (stdout);
      _exit (1);
   } // if
   do
   {
      if (! get_tag_or_label (page))
         return;
   } while (strcasecmp (my_attribute.id, anchor) != 0);
   do
   {
      if (! get_tag_or_label (page))
         return;
   } while (! *label);
   xmlTextReaderClose (page);
   xmlFreeDoc (doc);
   current_page_number = atoi (label);
} // get_page_number_3

void parse_smil_3 ()
{
   int x;
   xmlTextReaderPtr parse;

   total_time = 0;
   for (x = 0; x < total_items; x++)
   {
      if (*daisy[x].smil_file == 0)
         continue;
      xmlDocPtr doc = xmlRecoverFile (daisy[x].smil_file);
      if (! (parse = xmlReaderWalker (doc)))
      {
         endwin ();
         beep ();
         printf (gettext ("\nCannot read %s\n"), daisy[x].smil_file);
         fflush (stdout);
         _exit (1);
      } // if

// parse this smil
      while (1)
      {
         if (! get_tag_or_label (parse))
            break;
         if (strcasecmp (daisy[x].anchor, my_attribute.id) == 0)
            break;
      } // while
      daisy[x].duration = 0;
      while (1)
      {
         if (! get_tag_or_label (parse))
            break;
// get clip_begin
         if (strcasecmp (tag, "audio") == 0)
         {
            get_clips (my_attribute.clip_begin, my_attribute.clip_end);
            daisy[x].begin = clip_begin;
            daisy[x].duration += clip_end - clip_begin;
// get clip_end
            while (1)
            {
               if (! get_tag_or_label (parse))
                  break;
               if (*daisy[x + 1].anchor)
                  if (strcasecmp (my_attribute.id, daisy[x + 1].anchor) == 0)
                     break;
               if (strcasecmp (tag, "audio") == 0)
               {
                  get_clips (my_attribute.clip_begin, my_attribute.clip_end);
                  daisy[x].duration += clip_end - clip_begin;
               } // IF
            } // while
            if (*daisy[x + 1].anchor)
               if (strcasecmp (my_attribute.id, daisy[x + 1].anchor) == 0)
                  break;
         } // if (strcasecmp (tag, "audio") == 0)
      } // while
      total_time += daisy[x].duration;
      xmlTextReaderClose (parse);
      xmlFreeDoc (doc);
   } // for
} // parse_smil_3

void parse_content (char *src)
{
   int found_anchor, found_text;
   char text_file[MAX_STR];
   xmlTextReaderPtr content;

   strncpy (daisy[current].smil_file, src, MAX_STR - 1);
   if (strchr (daisy[current].smil_file, '#'))
   {
      strncpy (daisy[current].anchor,
               strchr (daisy[current].smil_file, '#') + 1, MAX_STR - 1);
      *strchr (daisy[current].smil_file, '#') = 0;
   } // if
   xmlDocPtr doc = xmlRecoverFile (daisy[current].smil_file);
   if (! (content = xmlReaderWalker (doc)))
   {
      endwin ();
      beep ();
      printf (gettext ("\nCannot read %s\n"), daisy[current].smil_file);
      fflush (stdout);
      _exit (1);
   } // if
   found_anchor = found_text = 0;
   while (1)
   {
      if (! get_tag_or_label (content))
      {
         xmlTextReaderClose (content);
         xmlFreeDoc (doc);
         return;
      } // if
      if (strcasecmp (tag, "text") == 0)
      {
         found_text = 1;
         strncpy (text_file, my_attribute.src, MAX_STR - 1);
      } // if
      if (strcasecmp (my_attribute.id, daisy[current].anchor) == 0)
         found_anchor = 1;
      if (found_anchor && found_text)
         break;
   } // while
   parse_text_file (text_file);
   xmlTextReaderClose (content);
   xmlFreeDoc (doc);
} // parse_content

void parse_xml (char *name)
{
   xmlTextReaderPtr xml;
   int indent = 0;
   char xml_name[MAX_STR];

   strncpy (xml_name, name, MAX_STR - 1);
   xmlDocPtr doc = xmlRecoverFile (ncx_name);
   if (! (xml = xmlReaderWalker (doc)))
   {
      endwin ();
      beep ();
      printf (gettext ("\nCannot read %s\n"), ncx_name);
      fflush (stdout);
      _exit (1);
   } // if
   total_pages = depth = 0;
   while (1)
   {
// get dtb:totalPageCount
      if (! get_tag_or_label (xml))
         break;
   } // while
   xmlTextReaderClose (xml);
   xmlFreeDoc (doc);

   doc = xmlRecoverFile (xml_name);
   if (! (xml = xmlReaderWalker (doc)))
   {
      endwin ();
      beep ();
      printf (gettext ("\nCannot read %s\n"), xml_name);
      fflush (stdout);
      _exit (1);
   } // if
   current = 0;
   while (1)
   {
      if (! get_tag_or_label (xml))
         break;
      if (strcasecmp (tag, "pagenum") == 0)
      {
         do
         {
            if (! get_tag_or_label (xml))
               break;
         } while (! *label);
         daisy[current].page_number = atoi (label);
      } // if pagenum
      if (strcasecmp (tag, "h1") == 0 ||
          strcasecmp (tag, "h2") == 0 ||
          strcasecmp (tag, "h3") == 0 ||
          strcasecmp (tag, "h4") == 0 ||
          strcasecmp (tag, "h5") == 0 ||
          strcasecmp (tag, "h6") == 0)
      {
         int l;

         l = tag[1] - '0';
         if (l > depth)
            depth = l;
         daisy[current].level = l;
         daisy[current].x = l + 3 - 1;
         indent = daisy[current].x = (l - 1) * 3 + 1;
         if (! *my_attribute.smilref)
            continue;
         strncpy (daisy[current].smil_file,
                  xml_name, MAX_STR - 1);
         if (strchr (my_attribute.smilref, '#'))
         {
            strncpy (daisy[current].anchor,
                     strchr (my_attribute.smilref, '#') + 1,
                     MAX_STR - 1);
         } // if
         do
         {
            if (! get_tag_or_label (xml))
               break;
         } while (*label == 0);
         get_label (current, indent);
         current++;
         displaying++;
      } // if (strcasecmp (tag, "h1") == 0 || ...
   } // while
   xmlTextReaderClose (xml);
   xmlFreeDoc (doc);
} // parse_xml

void parse_manifest (char *name, char *id_ptr)
{
   xmlTextReaderPtr manifest;
   char *id, *toc;

   id = strdup (id_ptr);
   if (! *id)
      return;
   toc = strdup (my_attribute.toc);
   xmlDocPtr doc = xmlRecoverFile (name);
   if (! (manifest = xmlReaderWalker (doc)))
   {
      endwin ();
      beep ();
      printf (gettext ("\nCannot read %s\n"), name);
      fflush (stdout);
      _exit (1);
   } // if
   do
   {
      if (! get_tag_or_label (manifest))
         break;
      if (strcasecmp (tag, "dc:language") == 0  && ! *daisy_language)
      {
         do
         {
            if (! get_tag_or_label (manifest))
               break;
            if (*label)
               break;
         } while (strcasecmp (tag, "/dc:language") != 0);
         strncpy (daisy_language, label, MAX_STR - 1);
      } // if dc:language
      if (strcasecmp (tag, "dc:title") == 0 && ! *daisy_title)
      {
         do
         {
            if (! get_tag_or_label (manifest))
               break;
            if (*label)
               break;
         } while (strcasecmp (tag, "/dc:title") != 0);
         strncpy (daisy_title, label, MAX_STR - 1);
         strncpy (bookmark_title, label, MAX_STR - 1);
         if (strchr (bookmark_title, '/'))
            *strchr (bookmark_title, '/') = '-';
      } // if dc:title
   } while (strcasecmp (tag, "manifest") != 0);
   while (1)
   {
      if (! get_tag_or_label (manifest))
         break;
      if (*toc && strcasecmp (my_attribute.id, id) == 0)
      {
         parse_ncx (my_attribute.href);
         xmlTextReaderClose (manifest);
         xmlFreeDoc (doc);
         return;
      } // if toc
      if (*id && strcasecmp (my_attribute.id, id) == 0)
      {
         strncpy (daisy[current].smil_file, my_attribute.href,
                  MAX_STR - 1);
         snprintf (daisy[current].label, MAX_STR - 1, "%d", current + 1);
         daisy[current].screen = current / max_y;
         daisy[current].y = current - (daisy[current].screen * max_y);
         daisy[current].x = 1;
         current++;
      } // if (strcasecmp (my_attribute.id, id) == 0)
   } // while
   xmlTextReaderClose (manifest);
   xmlFreeDoc (doc);
} // parse_manifest

void parse_opf (char *name)
{
   xmlTextReaderPtr opf;

   xmlDocPtr doc = xmlRecoverFile (name);
   if (! (opf = xmlReaderWalker (doc)))
   {
      endwin ();
      beep ();
      printf (gettext ("\nCannot read %s\n"), name);
      fflush (stdout);
      _exit (1);
   } // if
   current = displaying = 0;
   while (1)
   {
      if (! get_tag_or_label (opf))
         break;
      if (strcasecmp (tag, "dc:language") == 0)
      {
         do
         {
            if (! get_tag_or_label (opf))
               break;
         } while (! *label);
         strncpy (daisy_language, label, MAX_STR - 1);
      } // if dc:language
      if (strcasecmp (tag, "dtb:totalTime") == 0)
      {
         do
         {
            if (! get_tag_or_label (opf))
               break;
         } while (! *label);
         strncpy (ncc_totalTime, label, MAX_STR - 1);
      } // if (strcasestr (tag, "dtb:totalTime") == 0)
      if (strcasecmp (tag, "dc:title") == 0 && ! *daisy_title)
      {
         do
         {
            if (! get_tag_or_label (opf))
               break;
         } while (! *label);
         strncpy (daisy_title, label, MAX_STR - 1);
         strncpy (bookmark_title, label, MAX_STR - 1);
         if (strchr (bookmark_title, '/'))
            *strchr (bookmark_title, '/') = '-';
      } // if dc:title
      if (strcasecmp (my_attribute.media_type,
                      "application/x-dtbook+xml") == 0)
      {
         parse_xml (my_attribute.href);
         xmlTextReaderClose (opf);
         xmlFreeDoc (doc);
         return;
      } // if "application/x-dtbook+xml"
      if (strcasecmp (tag, "spine") == 0)
      {
         if (*my_attribute.toc)
         {
            parse_ncx (ncx_name);
            if (total_phrases > 0)
            {
               xmlTextReaderClose (opf);
               xmlFreeDoc (doc);
               return;
            } // if
         } // if
         do
         {
            if (! get_tag_or_label (opf))
               break;
            if (strcasecmp (tag, "itemref") == 0)
               parse_manifest (name, my_attribute.idref);
         } while (strcasecmp (tag, "/spine") != 0);
      } // if spine
   } // while
   total_items = current;
   xmlTextReaderClose (opf);
   xmlFreeDoc (doc);
} // parse_opf

void read_daisy_3 (char *daisy_mp)
{
   char cmd[MAX_CMD], path[MAX_STR];
   FILE *r;
   int ret;

   snprintf (cmd, MAX_CMD - 1, "find -iname \"*.ncx\" -print0");
   *path = 0;
   r = popen (cmd, "r");
   ret = fread (path, MAX_STR - 1, 1, r);
   fclose (r);
   if (*path == 0)
   {
      endwin ();
      printf (gettext ("\nNo DAISY-CD or Audio-cd found\n"));
      beep ();
      _exit (0);
   } // if
   strncpy (ncx_name, basename (path), MAX_STR - 1);
   strncpy (NCC_HTML, ncx_name, MAX_STR - 1);
   snprintf (daisy_mp, MAX_STR - 1, "%s/%s",
             get_current_dir_name (), dirname (path));
   ret = chdir (daisy_mp);
   snprintf (cmd, MAX_CMD - 1, "find -iname \"*.opf\" -print0");
   r = popen (cmd, "r");
   ret = fread (path, MAX_STR - 1, 1, r);
   fclose (r);
   ret = ret; // discard compiler warning
   strncpy (opf_name, basename (path), MAX_STR - 1);
   if (strcasecmp (prog_name, "daisy-player") == 0)
   {
      parse_ncx (ncx_name);
      total_items = current;
      parse_smil_3 ();
   } // if
   if (strcasecmp (prog_name, "eBook-speaker") == 0)
      parse_opf (opf_name);
   total_items = current;
} // read_daisy_3

void get_label (int item, int indent)
{
   strncpy (daisy[item].label, label, MAX_STR - 1);
   daisy[item].label[64 - daisy[item].x] = 0;
   if (displaying == max_y)
      displaying = 1;
   if (strcasecmp (daisy[item].class, "pagenum") == 0)
      daisy[item].x = 0;
   else
      if (daisy[item].x == 0)
         daisy[item].x = indent + 3;
      daisy[item].screen = item / max_y;
      daisy[item].y = item - daisy[item].screen * max_y;
} // get_label

void parse_ncx (char *name)
{
   int indent, found_page = 0;
   xmlTextReaderPtr ncx;
   xmlDocPtr doc;

   doc = xmlRecoverFile (name);
   if (! (ncx = xmlReaderWalker (doc)))
   {
      endwin ();
      beep ();
      printf (gettext ("\nCannot read %s\n"), name);
      fflush (stdout);
      _exit (1);
   } // if
   current = displaying = level = depth = 0;
   while (1)
   {
      if (! get_tag_or_label (ncx))
         break;
      if (strcasecmp (tag, "docTitle") == 0)
      {
         do
         {
            if (! get_tag_or_label (ncx))
               break;
         } while (! *label);
         strncpy (daisy_title, label, MAX_STR - 1);
         strncpy (bookmark_title, label, MAX_STR - 1);
         if (strchr (bookmark_title, '/'))
            *strchr (bookmark_title, '/') = '-';
      } // if (strcasecmp (tag, "docTitle") == 0)
      if (strcasecmp (tag, "docAuthor") == 0)
      {
         do
         {
            if (! get_tag_or_label (ncx))
               break;
         } while (strcasecmp (tag, "/docAuthor") != 0);
      } // if (strcasecmp (tag, "docAuthor") == 0)
      if (strcasecmp (tag, "navPoint") == 0)
      {
         level++;
         if (level > depth)
            depth = level;
         daisy[current].page_number = 0;
         daisy[current].playorder = atoi (my_attribute.playorder);
         daisy[current].level = level;
         if (daisy[current].level < 1)
            daisy[current].level = 1;
         daisy[current].x = level + 2;
         indent = (level - 1) * 3 + 1;
         strncpy (daisy[current].class, my_attribute.class, MAX_STR - 1);
         while (1)
         {
            if (! get_tag_or_label (ncx))
               break;
            if (strcasecmp (tag, "text") == 0)
            {
               while (1)
               {
                  if (! get_tag_or_label (ncx))
                     break;
                  if (strcasecmp (tag, "/text") == 0)
                     break;
                  if (*label)
                  {
                     get_label (current, indent);
                     break;
                  } // if
               } // while
            } // if (strcasecmp (tag, "text") == 0)
            if (strcasecmp (tag, "content") == 0)
            {
               if (*my_attribute.src)
               {
                  parse_content (my_attribute.src);
                  current++;
               } // if
               break;
            } // if (strcasecmp (tag, "content") == 0)
         } // while
      } // if (strcasecmp (tag, "navPoint") == 0)
      if (strcasecmp (tag, "/navpoint") == 0)
         level--;
      if (strcasecmp (tag, "page") == 0 || found_page)
      {
         found_page = 0;
         strncpy (daisy[++current].label, my_attribute.number,
                  max_phrase_len - 1);
         strncpy (daisy[current].smil_file, name, MAX_STR - 1);
         daisy[current].screen = current / max_y;
         daisy[current].y = current - (daisy[current].screen * max_y);
         daisy[current].x = 2;
      } // if (strcasecmp (tag, "page") == 0)
   } // while
   xmlTextReaderClose (ncx);
   xmlFreeDoc (doc);
   total_items = current;
#ifdef EBOOK_SPEAKER
   check_phrases ();
#endif
} // parse_ncx
