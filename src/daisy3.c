/* daisy3.c - functions to insert daisy3 info into a struct.
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

#include "daisy.h"

extern daisy_t daisy[];
extern misc_t misc;
extern my_attribute_t my_attribute;

void parse_ncx (char *);
void check_phrases ();    
void get_label (int, int);

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

   if (misc.audiocd == 1)
      return;
   strncpy (begin_str, orig_begin,  MAX_STR - 1);
   begin = begin_str;
   while (! isdigit (*begin))
      begin++;
   if (strchr (begin, 's'))
      *strchr (begin, 's') = 0;
   if (! strchr (begin, ':'))
      misc.clip_begin = (float) atof (begin);
   else
      misc.clip_begin = read_time (begin);

// fill end
   while (! isdigit (*end))
      end++;
   if (strchr (end, 's'))
      *strchr (end, 's') = 0;
   if (! strchr (end, ':'))
      misc.clip_end = (float) atof (end);
   else
      misc.clip_end = read_time (end);
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
      misc.current = atoi (attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "level"));
   if (strcmp (attr, "(null)"))
      misc.level = atoi ((char *) attr);
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
         snprintf (misc.daisy_version, MAX_STR - 1, "%s", content);
      if (strcasestr (name, "dc:title") && ! *misc.daisy_title)
      {
         snprintf (misc.daisy_title, MAX_STR - 1, "%s", content);
         snprintf (misc.bookmark_title, MAX_STR - 1, "%s", content);
         if (strchr (misc.bookmark_title, '/'))
            *strchr (misc.bookmark_title, '/') = '-';
      } // if
/* don't use it
      if (strcasestr (name, "dtb:misc.depth"))
         misc.depth = atoi (content);
*/
      if (strcasestr (name, "dtb:totalPageCount"))
         misc.total_pages = atoi (content);
      if (strcasestr (name, "ncc:misc.depth"))
         misc.depth = atoi (content);
      if (strcasestr (name, "ncc:maxPageNormal"))
         misc.total_pages = atoi (content);
      if (strcasestr (name, "ncc:pageNormal"))
         misc.total_pages = atoi (content);
      if (strcasestr (name, "ncc:page-normal"))
         misc.total_pages = atoi (content);
      if (strcasestr (name, "dtb:totalTime")  ||
          strcasestr (name, "ncc:totalTime"))
      {
         snprintf (misc.ncc_totalTime, MAX_STR - 1, "%s", content);
         if (strchr (misc.ncc_totalTime, '.'))
            *strchr (misc.ncc_totalTime, '.') = 0;
      } // if
   } // if
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "playorder"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.playorder, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "phrase"));
   if (strcmp (attr, "(null)"))
      misc.phrase_nr = atoi ((char *) attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "seconds"));
   if (strcmp (attr, "(null)"))
   {
      misc.seconds = atoi (attr);
      if (misc.seconds < 0)
         misc.seconds = 0;
   } // if
   snprintf (attr, MAX_STR - 1, "%s", (char*)
          xmlTextReaderGetAttribute (reader, (const xmlChar *) "smilref"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.smilref, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "sound_dev"));
   if (strcmp (attr, "(null)"))
      snprintf (misc.sound_dev, MAX_STR, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "ocr_language"));
   if (strcmp (attr, "(null)"))
      snprintf (misc.ocr_language, 5, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "cd_dev"));
   if (strcmp (attr, "(null)"))
      snprintf (misc.cd_dev, MAX_STR, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "cddb_flag"));
   if (strcmp (attr, "(null)"))
      misc.cddb_flag = (char) attr[0];
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "speed"));
   if (strcmp (attr, "(null)"))
      misc.speed = atof (attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "src"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.src, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "tts"));
   if (strcmp (attr, "(null)"))
      misc.tts_no = atof ((char *) attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "toc"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.toc, MAX_STR - 1, "%s", attr);
   snprintf (attr, MAX_STR - 1, "%s", (char*)
       xmlTextReaderGetAttribute (reader, (const xmlChar *) "value"));
   if (strcmp (attr, "(null)"))
      snprintf (my_attribute.value, MAX_STR - 1, "%s", attr);
} // get_attributes

int get_tag_or_label (xmlTextReaderPtr reader)
{
   int type;

   *misc.tag =  *misc.label = 0;
   *my_attribute.class = *my_attribute.clip_begin = *my_attribute.clip_end =
   *my_attribute.href = *my_attribute.id = *my_attribute.media_type =
   *my_attribute.playorder = * my_attribute.smilref = *my_attribute.src =
   *my_attribute.toc = 0, *my_attribute.value = 0;

   if (! reader)
      return 0;
   switch (xmlTextReaderRead (reader))
   {
   case -1:
   {
      int e;

      e = errno;
      endwin ();
      printf ("%s\n", strerror (e));
      printf ("Can't handle this DTB structure!\n");
      printf ("Don't know how to handle it yet, sorry. :-(\n");
      beep ();
      fflush (stdout);
      _exit (1);
   }
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
      endwin ();
      beep ();
      printf (gettext ("\nCannot read type: %d\n"), type);
      fflush (stdout);
      _exit (1);
   case XML_READER_TYPE_ELEMENT:
      strncpy (misc.tag, (char *) xmlTextReaderConstName (reader),
               MAX_TAG - 1);
      n = xmlTextReaderAttributeCount (reader);
      for (i = 0; i < n; i++)
         get_attributes (reader);
      return 1;
   case XML_READER_TYPE_END_ELEMENT:
      snprintf (misc.tag, MAX_TAG - 1, "/%s",
                (char *) xmlTextReaderName (reader));
      return 1;
   case XML_READER_TYPE_TEXT:
   {
      int x;

      x = 0;
      while (1)
      {
         if (isspace (xmlTextReaderConstValue (reader)[x]))
            x++;
         else
            break;
      } // while
      strncpy (misc.label, (char *) xmlTextReaderConstValue (reader) + x,
                      max_phrase_len);
      for (x = strlen (misc.label) - 1; x >= 0 && isspace (misc.label[x]); x--)
         misc.label[x] = 0;
      for (x = 0; misc.label[x] > 0; x++)
         if (! isprint (misc.label[x]))
            misc.label[x] = ' ';
      return 1;
   }
   case XML_READER_TYPE_ENTITY_REFERENCE:
   case XML_READER_TYPE_DOCUMENT_TYPE:
   case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
      snprintf (misc.tag, MAX_TAG - 1, "/%s",
                (char *) xmlTextReaderName (reader));
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
   xmlDocPtr doc;
   char *anchor = 0;

   if (strchr (text_file, '#'))
   {
      anchor = strdup (strchr (text_file, '#') + 1);
      *strchr (text_file, '#') = 0;
   } // if
   doc = xmlRecoverFile (text_file);
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
   } while (! *misc.label);
   daisy[misc.current].page_number = atoi (misc.label);
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
   } while (strcasecmp (misc.tag, "text") != 0);
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
      {
         xmlTextReaderClose (page);
         xmlFreeDoc (doc);
         return;
      } // if
   } while (strcasecmp (my_attribute.id, anchor) != 0);
   do
   {
      if (! get_tag_or_label (page))
      {
         xmlTextReaderClose (page);
         xmlFreeDoc (doc);
         return;
      } // if
   } while (! *misc.label);
   xmlTextReaderClose (page);
   xmlFreeDoc (doc);
   misc.current_page_number = atoi (misc.label);
} // get_page_number_3

void parse_smil_3 ()
{
   int x;
   xmlTextReaderPtr parse;
   xmlDocPtr doc;

   misc.total_time = 0;
   for (x = 0; x < misc.total_items; x++)
   {
      if (*daisy[x].smil_file == 0)
         continue;
      doc = xmlRecoverFile (daisy[x].smil_file);
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
         if (strcasecmp (misc.tag, "audio") == 0)
         {
            get_clips (my_attribute.clip_begin, my_attribute.clip_end);
            daisy[x].begin = misc.clip_begin;
            daisy[x].duration += misc.clip_end - misc.clip_begin;
// get clip_end
            while (1)
            {
               if (! get_tag_or_label (parse))
                  break;
               if (*daisy[x + 1].anchor)
                  if (strcasecmp (my_attribute.id, daisy[x + 1].anchor) == 0)
                     break;
               if (strcasecmp (misc.tag, "audio") == 0)
               {
                  get_clips (my_attribute.clip_begin, my_attribute.clip_end);
                  daisy[x].duration += misc.clip_end - misc.clip_begin;
               } // IF
            } // while
            if (*daisy[x + 1].anchor)
               if (strcasecmp (my_attribute.id, daisy[x + 1].anchor) == 0)
                  break;
         } // if (strcasecmp (misc.tag, "audio") == 0)
      } // while
      misc.total_time += daisy[x].duration;
      xmlTextReaderClose (parse);
      xmlFreeDoc (doc);
   } // for
} // parse_smil_3

void parse_content (char *src)
{
   int found_anchor, found_text;
   char text_file[MAX_STR];
   xmlTextReaderPtr content;

   strncpy (daisy[misc.current].smil_file, src, MAX_STR - 1);
   if (strchr (daisy[misc.current].smil_file, '#'))
   {
      strncpy (daisy[misc.current].anchor,
               strchr (daisy[misc.current].smil_file, '#') + 1, MAX_STR - 1);
      *strchr (daisy[misc.current].smil_file, '#') = 0;
   } // if
   xmlDocPtr doc = xmlRecoverFile (daisy[misc.current].smil_file);
   if (! (content = xmlReaderWalker (doc)))
   {
      endwin ();
      beep ();
      printf (gettext ("\nCannot read %s\n"), daisy[misc.current].smil_file);
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
      if (strcasecmp (misc.tag, "text") == 0)
      {
         found_text = 1;
         strncpy (text_file, my_attribute.src, MAX_STR - 1);
      } // if
      if (strcasecmp (my_attribute.id, daisy[misc.current].anchor) == 0)
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
   xmlDocPtr doc = xmlRecoverFile (misc.ncx_name);
   if (! (xml = xmlReaderWalker (doc)))
   {
      endwin ();
      beep ();
      printf (gettext ("\nCannot read %s\n"), misc.ncx_name);
      fflush (stdout);
      _exit (1);
   } // if
   misc.total_pages = misc.depth = 0;
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
   misc.current = 0;
   while (1)
   {
      if (! get_tag_or_label (xml))
         break;
      if (strcasecmp (misc.tag, "pagenum") == 0)
      {
         do
         {
            if (! get_tag_or_label (xml))
               break;
         } while (! *misc.label);
         daisy[misc.current].page_number = atoi (misc.label);
      } // if pagenum
      if (strcasecmp (misc.tag, "h1") == 0 ||
          strcasecmp (misc.tag, "h2") == 0 ||
          strcasecmp (misc.tag, "h3") == 0 ||
          strcasecmp (misc.tag, "h4") == 0 ||
          strcasecmp (misc.tag, "h5") == 0 ||
          strcasecmp (misc.tag, "h6") == 0)
      {                       
         int l;

         l = misc.tag[1] - '0';
         if (l > misc.depth)
            misc.depth = l;
         daisy[misc.current].level = l;
         daisy[misc.current].x = l + 3 - 1;
         indent = daisy[misc.current].x = (l - 1) * 3 + 1;
         if (! *my_attribute.smilref)
            continue;
         strncpy (daisy[misc.current].smil_file,
                  xml_name, MAX_STR - 1);
         if (strchr (my_attribute.smilref, '#'))
         {
            strncpy (daisy[misc.current].anchor,
                     strchr (my_attribute.smilref, '#') + 1,
                     MAX_STR - 1);
         } // if
         do
         {
            if (! get_tag_or_label (xml))
               break;
         } while (*misc.label == 0);
         get_label (misc.current, indent);
         misc.current++;
         misc.displaying++;
      } // if (strcasecmp (misc.tag, "h1") == 0 || ...
   } // while
   xmlTextReaderClose (xml);
   xmlFreeDoc (doc);
} // parse_xml

void parse_manifest (char *name, char *id_ptr)
{
   xmlTextReaderPtr manifest;
   xmlDocPtr doc;
   char *id, *toc;

   id = strdup (id_ptr);
   if (! *id)
   {
      free (id);
      return;
   } // if   
   toc = strdup (my_attribute.toc);
   doc = xmlRecoverFile (name);
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
      if (strcasecmp (misc.tag, "dc:language") == 0  && ! *misc.daisy_language)
      {
         do
         {
            if (! get_tag_or_label (manifest))
               break;
            if (*misc.label)
               break;
         } while (strcasecmp (misc.tag, "/dc:language") != 0);
         strncpy (misc.daisy_language, misc.label, MAX_STR - 1);
      } // if dc:language
      if (strcasecmp (misc.tag, "dc:title") == 0 && ! *misc.daisy_title)
      {
         do
         {
            if (! get_tag_or_label (manifest))
               break;
            if (*misc.label)
               break;
         } while (strcasecmp (misc.tag, "/dc:title") != 0);
         strncpy (misc.daisy_title, misc.label, MAX_STR - 1);
         strncpy (misc.bookmark_title, misc.label, MAX_STR - 1);
         if (strchr (misc.bookmark_title, '/'))
            *strchr (misc.bookmark_title, '/') = '-';
      } // if dc:title
   } while (strcasecmp (misc.tag, "manifest") != 0);
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
         strncpy (daisy[misc.current].smil_file, my_attribute.href,
                  MAX_STR - 1);
         snprintf (daisy[misc.current].label, MAX_STR - 1, "%d", misc.current + 1);
         daisy[misc.current].screen = misc.current / misc.max_y;
         daisy[misc.current].y = misc.current - (daisy[misc.current].screen * misc.max_y);
         daisy[misc.current].x = 1;
         misc.current++;
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
   misc.current = misc.displaying = 0;
   while (1)
   {
      if (! get_tag_or_label (opf))
         break;
      if (strcasecmp (misc.tag, "dc:language") == 0)
      {
         do
         {
            if (! get_tag_or_label (opf))
               break;
         } while (! *misc.label);
         strncpy (misc.daisy_language, misc.label, MAX_STR - 1);
      } // if dc:language
      if (strcasecmp (misc.tag, "dtb:totalTime") == 0)
      {
         do
         {
            if (! get_tag_or_label (opf))
               break;
         } while (! *misc.label);
         strncpy (misc.ncc_totalTime, misc.label, MAX_STR - 1);
      } // if (strcasestr (misc.tag, "dtb:totalTime") == 0)
      if (strcasecmp (misc.tag, "dc:title") == 0 && ! *misc.daisy_title)
      {
         do
         {
            if (! get_tag_or_label (opf))
               break;
         } while (! *misc.label);
         strncpy (misc.daisy_title, misc.label, MAX_STR - 1);
         strncpy (misc.bookmark_title, misc.label, MAX_STR - 1);
         if (strchr (misc.bookmark_title, '/'))
            *strchr (misc.bookmark_title, '/') = '-';
      } // if dc:title
      if (strcasecmp (my_attribute.media_type,
                      "application/x-dtbook+xml") == 0)
      {
         parse_xml (my_attribute.href);
         xmlTextReaderClose (opf);
         xmlFreeDoc (doc);
         return;
      } // if "application/x-dtbook+xml"
      if (strcasecmp (misc.tag, "spine") == 0)
      {
         if (*my_attribute.toc)
         {
            parse_ncx (misc.ncx_name);
            if (misc.total_phrases > 0)
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
            if (strcasecmp (misc.tag, "itemref") == 0)
               parse_manifest (name, my_attribute.idref);
         } while (strcasecmp (misc.tag, "/spine") != 0);
      } // if spine
   } // while
   misc.total_items = misc.current;
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
      printf ("%s\n", gettext (strerror (ENOENT)));
      beep ();
      fflush (stdout);
      _exit (0);
   } // if
   strncpy (misc.ncx_name, basename (path), MAX_STR - 1);
   strncpy (misc.NCC_HTML, misc.ncx_name, MAX_STR - 1);
   snprintf (daisy_mp, MAX_STR - 1, "%s/%s",
             get_current_dir_name (), dirname (path));
   ret = chdir (daisy_mp);
   snprintf (cmd, MAX_CMD - 1, "find -iname \"*.opf\" -print0");
   r = popen (cmd, "r");
   ret = fread (path, MAX_STR - 1, 1, r);
   fclose (r);
   ret = ret; // discard compiler warning
   strncpy (misc.opf_name, basename (path), MAX_STR - 1);
   parse_opf (misc.opf_name);
   misc.total_items = misc.current;
} // read_daisy_3

void get_label (int item, int indent)
{
   strncpy (daisy[item].label, misc.label, MAX_STR - 1);
   daisy[item].label[64 - daisy[item].x] = 0;
   if (misc.displaying == misc.max_y)
      misc.displaying = 1;
   if (strcasecmp (daisy[item].class, "pagenum") == 0)
      daisy[item].x = 0;
   else
      if (daisy[item].x == 0)
         daisy[item].x = indent + 3;
      daisy[item].screen = item / misc.max_y;
      daisy[item].y = item - daisy[item].screen * misc.max_y;
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
   misc.current = misc.displaying = misc.level = misc.depth = 0;
   while (1)
   {
      if (! get_tag_or_label (ncx))
         break;
      if (strcasecmp (misc.tag, "docTitle") == 0)
      {
         do
         {
            if (! get_tag_or_label (ncx))
               break;
         } while (! *misc.label);
         strncpy (misc.daisy_title, misc.label, MAX_STR - 1);
         strncpy (misc.bookmark_title, misc.label, MAX_STR - 1);
         if (strchr (misc.bookmark_title, '/'))
            *strchr (misc.bookmark_title, '/') = '-';
      } // if (strcasecmp (misc.tag, "docTitle") == 0)
      if (strcasecmp (misc.tag, "docAuthor") == 0)
      {
         do
         {
            if (! get_tag_or_label (ncx))
               break;
         } while (strcasecmp (misc.tag, "/docAuthor") != 0);
      } // if (strcasecmp (misc.tag, "docAuthor") == 0)
      if (strcasecmp (misc.tag, "navPoint") == 0)
      {
         misc.level++;
         if (misc.level > misc.depth)
            misc.depth = misc.level;
         daisy[misc.current].page_number = 0;
         daisy[misc.current].playorder = atoi (my_attribute.playorder);
         daisy[misc.current].level = misc.level;
         if (daisy[misc.current].level < 1)
            daisy[misc.current].level = 1;
         indent = daisy[misc.current].x = misc.level * 3 - 2;
         strncpy (daisy[misc.current].class, my_attribute.class, MAX_STR - 1);
         while (1)
         {
            if (! get_tag_or_label (ncx))
               break;
            if (strcasecmp (misc.tag, "text") == 0)
            {
               while (1)
               {
                  if (! get_tag_or_label (ncx))
                     break;
                  if (strcasecmp (misc.tag, "/text") == 0)
                     break;
                  if (*misc.label)
                  {
                     get_label (misc.current, indent);
                     break;
                  } // if
               } // while
            } // if (strcasecmp (misc.tag, "text") == 0)
            if (strcasecmp (misc.tag, "content") == 0)
            {
               if (*my_attribute.src)
               {
                  parse_content (my_attribute.src);
                  misc.current++;
               } // if
               break;
            } // if (strcasecmp (misc.tag, "content") == 0)
         } // while
      } // if (strcasecmp (misc.tag, "navPoint") == 0)
      if (strcasecmp (misc.tag, "/navpoint") == 0)
         misc.level--;
      if (strcasecmp (misc.tag, "page") == 0 || found_page)
      {
         found_page = 0;
         strncpy (daisy[++misc.current].label, my_attribute.number,
                  max_phrase_len - 1);
         strncpy (daisy[misc.current].smil_file, name, MAX_STR - 1);
         daisy[misc.current].screen = misc.current / misc.max_y;
         daisy[misc.current].y = misc.current - (daisy[misc.current].screen * misc.max_y);
         daisy[misc.current].x = 2;
      } // if (strcasecmp (misc.tag, "page") == 0)
   } // while
   xmlTextReaderClose (ncx);
   xmlFreeDoc (doc);
   misc.total_items = misc.current;
#ifdef EBOOK_SPEAKER
   check_phrases ();
#endif
} // parse_ncx
