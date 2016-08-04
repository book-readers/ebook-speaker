/* daisy3.c - functions to insert daisy3 info into a struct.
 *
 * Copyright (C) 2016 J. Lemmens
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

void fill_page_numbers (misc_t *misc, daisy_t *daisy,
                        my_attribute_t *my_attribute)
{
   int i;
   htmlDocPtr doc;
   xmlTextReaderPtr page;

   for (i = 0; i <misc->total_items; i++)
   {
      daisy[i].page_number = 0;
      if (! (doc = htmlParseFile (daisy[i].xml_file, "UTF-8")))
         failure (daisy[i].xml_file, errno);
      if (! (page = xmlReaderWalker (doc)))
         failure (daisy[i].xml_file, errno);

      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, page))
            break;
         if (strcasecmp (misc->tag, "pagenum") == 0 ||
             strcasecmp (my_attribute->class, "pagenum") == 0)
         {
            parse_page_number (misc, my_attribute, page);
            daisy[i].page_number = misc->current_page_number;
         } // if
         if (*my_attribute->id)
         {
            if (strcasecmp (my_attribute->id, daisy[i].anchor) == 0)
            {
               break;
            } // if
         } // if
         if (i + 1 < misc->total_items)
         {
            if (*daisy[i + 1].anchor)
            {
               if (strcasecmp (my_attribute->id, daisy[i + 1].anchor) == 0)
               {
                  break;
               } // if
            } // if
         } // if
      } // while
   } // for
} // fill_page_numbers

void parse_page_number (misc_t *misc, my_attribute_t *my_attribute,
                        xmlTextReaderPtr page)
{
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, page))
         return;
      if (*misc->label)
      {
         misc->current_page_number = atoi (misc->label);
         return;
      } // if
      if (strcasecmp (misc->tag, "/pagenum") == 0)
         return;
      if (strcasecmp (misc->tag, "text") == 0)
      {
         char *src, *anchor;
         htmlDocPtr doc;
         xmlTextReaderPtr page;

         src = malloc
              (strlen (misc->tmp_dir) + strlen (my_attribute->src) + 3);
         strcpy (src, misc->tmp_dir);
         strcat (src, "/");
         strcat (src, my_attribute->src);
         anchor = strdup ("");
         if (strchr (src, '#'))
         {
            anchor = strdup (strchr (src, '#') + 1);
            *strchr (src, '#') = 0;
         } // if
         doc = htmlParseFile (src, "UTF-8");
         if (! (page = xmlReaderWalker (doc)))
            failure (src, errno);

         if (*anchor)
         {
            do
            {
               if (! get_tag_or_label (misc, my_attribute, page))
               {
                  xmlTextReaderClose (page);
                  xmlFreeDoc (doc);
                  return;
               } // if
            } while (strcasecmp (my_attribute->id, anchor) != 0);
         } // if

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
      } // if
   } // while
} // parse_page_number

void get_attributes (misc_t *misc, my_attribute_t *my_attribute,
                     xmlTextReaderPtr ptr)
{
   char attr[MAX_STR + 1];

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
#ifdef DAISY_PLAYER
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, BAD_CAST "clip-begin"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->clip_begin, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, BAD_CAST "clipbegin"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->clip_begin, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, BAD_CAST "clip-end"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->clip_end, attr, MAX_STR - 1);
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "clipend"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->clip_end, attr, MAX_STR - 1);
#endif      
   snprintf (attr, MAX_STR - 1, "%s", (char *)
             xmlTextReaderGetAttribute (ptr, (const xmlChar *) "href"));
   if (strcmp (attr, "(null)"))
      strncpy (my_attribute->href, attr, MAX_STR - 1);
   if (xmlTextReaderGetAttribute (ptr, (const xmlChar *) "id") != NULL)
   {
      my_attribute->id = strdup ((char *) xmlTextReaderGetAttribute (ptr,
               (const xmlChar *) "id"));
   } // if
   if (xmlTextReaderGetAttribute (ptr, (const xmlChar *) "idref") != NULL)
   {
      my_attribute->idref = strdup ((char *) xmlTextReaderGetAttribute (ptr,
               (const xmlChar *) "idref"));
   } // if
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
   if (xmlTextReaderGetAttribute (ptr, (const xmlChar *) "src") != NULL)
   {
      my_attribute->src = strdup ((char *) xmlTextReaderGetAttribute (ptr,
               (const xmlChar *) "src"));
   } // if
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
   misc->label = strdup ("");;
   *misc->label = 0;
#ifdef EBOOK_SPEAKER
   *my_attribute->my_class = 0;
#endif
   *my_attribute->class =
#ifdef DAISY_PLAYER
   *my_attribute->clip_begin = *my_attribute->clip_end =
#endif
   *my_attribute->href = *my_attribute->media_type =
   *my_attribute->playorder = * my_attribute->smilref =
   *my_attribute->toc = *my_attribute->value = 0;
   my_attribute->id = my_attribute->idref = my_attribute->src = strdup ("");

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
      misc->label = realloc (misc->label, phrase_len + 1);
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

void fill_xml_anchor_ncx (misc_t *misc, my_attribute_t *my_attribute,
                          daisy_t *daisy)
{
// first of all fill daisy struct xml_file and anchor
   htmlDocPtr doc;
   xmlTextReaderPtr ncx;

   if (misc->items_in_ncx == 0)
   {
      endwin ();
      beep ();
      printf ("No items found\n");
      exit (0);
   } // if
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
   misc->current = misc->displaying = 0;
   while (1)
   {
      if (misc->current >= misc->total_items)
         break;
      if (! get_tag_or_label (misc, my_attribute, ncx))
         break;
      if (strcasecmp (misc->tag, "navpoint") == 0)
      {
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, ncx))
               break;
            if (strcasecmp (misc->tag, "content") == 0)
            {
               htmlDocPtr doc;
               xmlTextReaderPtr content;
               char *src;

               src = strdup (my_attribute->src);
               if (*src == 0)
                  daisy[misc->current].xml_file =
                       malloc (strlen (misc->daisy_mp) + 5);
               else
                  daisy[misc->current].xml_file =
                       malloc (strlen (misc->daisy_mp) + strlen (src) + 5);
               strcpy (daisy[misc->current].xml_file, misc->daisy_mp);
               strcat (daisy[misc->current].xml_file, "/");
               if (*src)
                  strcat (daisy[misc->current].xml_file, src);
               daisy[misc->current].anchor = strdup ("");
               if (strchr (daisy[misc->current].xml_file, '#'))
               {
                  daisy[misc->current].anchor = strdup
                       (strchr (daisy[misc->current].xml_file, '#') + 1);
                  *strchr (daisy[misc->current].xml_file, '#') = 0;
               } // if

               doc = htmlParseFile (daisy[misc->current].xml_file, "UTF-8");
               if (! (content = xmlReaderWalker (doc)))
               {
                  int e;
                  char *str;

                  e = errno;
                  str = strdup ((gettext
                      ("Cannot read %s"), daisy[misc->current].xml_file));
                  failure (str, e);
               } // if

               if (daisy[misc->current].anchor)
               {
                  while (1)
                  {
                     if (! get_tag_or_label (misc, my_attribute, content))
                        break;
                     if (*my_attribute->id && *daisy[misc->current].anchor)
                     {
                        if (strcasecmp (my_attribute->id,
                                        daisy[misc->current].anchor) == 0)
                        {
                           break;
                        } // if
                     } // if
                  } // while
               } // if
               while (1)
               { 
                  if (! get_tag_or_label (misc, my_attribute, content))
                     break;
                  if (strcasecmp (misc->tag, "text") == 0)
                  {
                     daisy[misc->current].xml_file = malloc
                  (strlen (misc->daisy_mp) + strlen (my_attribute->src) + 5);
                     strcpy (daisy[misc->current].xml_file, misc->daisy_mp);
                     strcat (daisy[misc->current].xml_file, "/");
                     strcat (daisy[misc->current].xml_file,
                             my_attribute->src);
                     daisy[misc->current].anchor = strdup ("");
                     if (strchr (daisy[misc->current].xml_file, '#'))
                     {
                        daisy[misc->current].anchor = strdup
                            (strchr (daisy[misc->current].xml_file, '#') + 1);
                        *strchr (daisy[misc->current].xml_file, '#') = 0;
                     } // if
                     break;
                  } // if
               } // while
               xmlFreeDoc (doc);
               misc->current++;
               if (misc->current >= misc->total_items)
                  break;
               break;
            } // if (strcasecmp (misc->tag, "content") == 0)
         } // while
      } // if (strcasecmp (misc->tag, "navpoint") == 0)
   } // while
   misc->total_items = misc->current;
} // fill_xml_anchor_ncx

void parse_content (misc_t *misc, my_attribute_t *my_attribute,
                    daisy_t *daisy)
{
   htmlDocPtr doc;
   xmlTextReaderPtr content;

#ifdef DAISY_PLAYER
   daisy[misc->current].xml_file = malloc
               (strlen (misc->daisy_mp) + strlen (my_attribute->src) + 5);
   strcpy (daisy[misc->current].xml_file, misc->daisy_mp);
   strcat (daisy[misc->current].xml_file, "/");
   strcat (daisy[misc->current].xml_file, my_attribute->src);
#endif
   if (strchr (daisy[misc->current].xml_file, '#'))
   {
      daisy[misc->current].anchor =
               strdup (strchr (daisy[misc->current].xml_file, '#') + 1);
      *strchr (daisy[misc->current].xml_file, '#') = 0;
   } // if
   doc = htmlParseFile (daisy[misc->current].xml_file, "UTF-8");
   if (! (content = xmlReaderWalker (doc)))
   {
      int e;
      char *str;

      e = errno;
      str = strdup
       ((gettext ("Cannot read %s"), daisy[misc->current].xml_file));
      failure (str, e);
   } // if

   if (*daisy[misc->current].anchor)
   {
// look for it
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, content))
            break;
         if (*my_attribute->id)
         {
            if (strcasecmp (my_attribute->id,
                            daisy[misc->current].anchor) == 0)
            {
               break;
            } // if
         } // if
      } // while
   } // if

   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, content))
         break;
#ifdef DAISY_PLAYER
      if (strcasecmp (misc->tag, "audio") == 0)
         break;                                
#endif
#ifdef EBOOK_SPEAKER
      if (strcasecmp (misc->tag, "text") == 0)
      {
         parse_text (misc, my_attribute, daisy);
         break;
      } // if
#endif
      if (misc->current + 1 < misc->total_items)
      {
         if (*daisy[misc->current + 1].anchor)
         {
            if (strcasecmp (my_attribute->id,
                            daisy[misc->current + 1].anchor) == 0)
            {
               break;
            } // if
         } // if
      } // if
   } // while
   xmlFreeDoc (doc);
} // parse_content

void fill_item (misc_t *misc, daisy_t *daisy)
{
   daisy[misc->current].x = daisy[misc->current].level * 3 - 1;
   daisy[misc->current].screen = misc->current / misc->max_y;
   daisy[misc->current].y =
                misc->current - (daisy[misc->current].screen * misc->max_y);
} // fill_item

void parse_text (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   htmlDocPtr doc;
   xmlTextReaderPtr text;

   doc = htmlParseFile (daisy[misc->current].xml_file, "UTF-8");
   if (! (text = xmlReaderWalker (doc)))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"),
                daisy[misc->current].xml_file);
      failure (str, e);
   } // if

   misc->depth = 0;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, text))
         break;
      if (strcasecmp (misc->tag, "h1") == 0 ||
          strcasecmp (misc->tag, "h2") == 0 ||
          strcasecmp (misc->tag, "h3") == 0 ||
          strcasecmp (misc->tag, "h4") == 0 ||
          strcasecmp (misc->tag, "h5") == 0 ||
          strcasecmp (misc->tag, "h6") == 0 ||
          strcasecmp (misc->tag, "title") == 0 ||
          strcasecmp (misc->tag, "doctitle") == 0)
      {
         if (strcasecmp (misc->tag, "title") == 0 ||
             strcasecmp (misc->tag, "doctitle") == 0)
            daisy[misc->current].level = 1;
         else
            daisy[misc->current].level = misc->tag[1] - 48;
         if (daisy[misc->current].level > misc->depth)
            misc->depth = daisy[misc->current].level;
      } // if
      if (*daisy[misc->current].anchor &&
          strcasecmp (my_attribute->id, daisy[misc->current].anchor) == 0)
      {
#ifdef DAISY_PLAYER
         daisy[misc->current].xml_file = malloc (strlen (misc->daisy_mp) +
                                    strlen (my_attribute->smilref) + 10);
         strcpy (daisy[misc->current].xml_file, misc->daisy_mp);
         strcat (daisy[misc->current].xml_file, "/");
         strcat (daisy[misc->current].xml_file, my_attribute->smilref);
         daisy[misc->current].anchor = strdup ("");
         if (strchr (daisy[misc->current].xml_file, '#'))
         {
            daisy[misc->current].anchor = strdup
                     (strchr (daisy[misc->current].xml_file, '#') + 1);
            *strchr (daisy[misc->current].xml_file, '#') = 0;
         } // if
#endif
         break;
      } // if
   } // while

   daisy[misc->current].label = strdup ("");
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, text))
         break;
      if (strcasecmp (misc->tag, "/h1") == 0 ||
          strcasecmp (misc->tag, "/h2") == 0 ||
          strcasecmp (misc->tag, "/h3") == 0 ||
          strcasecmp (misc->tag, "/h4") == 0 ||
          strcasecmp (misc->tag, "/h5") == 0 ||
          strcasecmp (misc->tag, "/h6") == 0 ||
          strcasecmp (misc->tag, "/title") == 0 ||
          strcasecmp (misc->tag, "/doctitle") == 0)
      {
         break;
      } // if
      if (*misc->label)
      {
         daisy[misc->current].label =
            realloc (daisy[misc->current].label, strlen (misc->label) + 5);
         strcat (daisy[misc->current].label, misc->label);
         strcat (daisy[misc->current].label, " ");
      } // if
   } // while
   daisy[misc->current].x = daisy[misc->current].level * 3 - 1;
   daisy[misc->current].screen = misc->current / misc->max_y;
   daisy[misc->current].y =
          misc->current - (daisy[misc->current].screen * misc->max_y);
#ifdef EBOOK_SPEAKER
   daisy[misc->current].n_phrases = 0;
#endif
} // parse_text

void parse_href (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   htmlDocPtr doc;
   xmlTextReaderPtr href;

   if (! (doc = htmlParseFile (daisy[misc->current].xml_file, "UTF-8")))
      failure (daisy[misc->current].xml_file, errno);
   if (! (href = xmlReaderWalker (doc)))
   {
      int e;

      e = errno;
      snprintf (misc->str, MAX_STR,
                gettext ("Cannot read %s"), my_attribute->href);
      failure (misc->str, e);
   } // if

   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, href))
         break;
#ifdef EBOOK_SPEAKER
      if (strcasecmp (misc->tag, "text") == 0)
      {
         daisy[misc->current].xml_file = malloc
                (strlen (misc->daisy_mp) + strlen (my_attribute->src) + 1);
         strcpy (daisy[misc->current].xml_file, misc->daisy_mp);
         strcat (daisy[misc->current].xml_file, "/");
         strcat (daisy[misc->current].xml_file, my_attribute->src);
         daisy[misc->current].anchor = strdup ("");
         if (strchr (daisy[misc->current].xml_file, '#'))
         {
            daisy[misc->current].anchor = strdup
                 (strchr (daisy[misc->current].xml_file, '#') + 1);
            *strchr (daisy[misc->current].xml_file, '#') = 0;
         } // if
         parse_text (misc, my_attribute, daisy);
         return;
      } // if

// some EPUB use this
      if (strcasecmp (misc->tag, "i") == 0)
         return;

// or
      if (strcasecmp (misc->tag, "title") == 0 ||
          strcasecmp (misc->tag, "h1") == 0 ||
          strcasecmp (misc->tag, "h2") == 0 ||
          strcasecmp (misc->tag, "h3") == 0 ||
          strcasecmp (misc->tag, "h4") == 0 ||
          strcasecmp (misc->tag, "h5") == 0 ||
          strcasecmp (misc->tag, "h6") == 0)
      {
         if (strcasecmp (misc->tag, "title") == 0)
            daisy[misc->current].level = 1;
         else
            daisy[misc->current].level = misc->tag[1] - 48;
         daisy[misc->current].label = strdup ("");
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, href))
               break;
            if (strcasecmp (misc->tag, "/h1") == 0 ||
                strcasecmp (misc->tag, "/h2") == 0 ||
                strcasecmp (misc->tag, "/h3") == 0 ||
                strcasecmp (misc->tag, "/h4") == 0 ||
                strcasecmp (misc->tag, "/h5") == 0 ||
                strcasecmp (misc->tag, "/h6") == 0 ||
                strcasecmp (misc->tag, "/title") == 0 ||
                strcasecmp (misc->tag, "/doctitle") == 0)
            {
               break;
            } // if
            daisy[misc->current].label = strdup ("");
            if (*misc->label)
            {
               daisy[misc->current].label = realloc
                      (daisy[misc->current].label, strlen (misc->label) + 5);
               strcat (daisy[misc->current].label, misc->label);
               strcat (daisy[misc->current].label, " ");
            } // if
         } // while
         fill_item (misc, daisy);
         return;
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
               daisy[misc->current].xml_file = malloc
                   (strlen (misc->daisy_mp) + strlen (my_attribute->src) + 1);
               strcpy (daisy[misc->current].xml_file, misc->daisy_mp);
               strcat (daisy[misc->current].xml_file, "/");
               strcat (daisy[misc->current].xml_file, my_attribute->src);
               daisy[misc->current].anchor = strdup ("");
               if (strchr (daisy[misc->current].xml_file, '#'))
               {
                  daisy[misc->current].anchor = strdup
                         (strchr (daisy[misc->current].xml_file, '#') + 1);
                  *strchr (daisy[misc->current].xml_file, '#') = 0;
               } // if
               return;
            } // if
         } // while
         return;
      } // if (strcasecmp (my_attribute->class, "h1") == 0 ||
#endif
   } // while
} // parse_href

void parse_manifest (misc_t *misc, my_attribute_t *my_attribute,
                     daisy_t *daisy)
{
   xmlTextReaderPtr manifest;
   htmlDocPtr doc;
   char *idref;

   idref = strdup (my_attribute->idref);
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
/* already has xml_file
         daisy[misc->current].xml_file = malloc
                 (strlen (misc->daisy_mp) + strlen (my_attribute->href) + 5);
         strcpy (daisy[misc->current].xml_file, misc->daisy_mp);
         strcat (daisy[misc->current].xml_file, "/");
         strcat (daisy[misc->current].xml_file, my_attribute->href);
         daisy[misc->current].anchor = strdup ("");
         if (strchr (daisy[misc->current].xml_file, '#'))
         {
            daisy[misc->current].anchor = strdup
                     (strchr (daisy[misc->current].xml_file, '#') + 1);
            *strchr (daisy[misc->current].xml_file, '#') = 0;
         } // if
*/
         parse_href (misc, my_attribute, daisy);
         if (daisy[misc->current].level > misc->depth)
            misc->depth = daisy[misc->current].level;
         misc->current++;
         if (misc->current > misc->total_items)
         {
            misc->total_items = misc->current;
            daisy = (daisy_t *)
                       realloc (daisy, misc->total_items * sizeof (daisy_t));
         } // if
         break;
      } // if
   } // while
} // parse_manifest

#ifdef DAISY_PLAYER
void parse_clips (misc_t *misc, my_attribute_t *my_attribute, /
                       daisy_t *daisy)
{
   xmlTextReaderPtr parse;
   htmlDocPtr doc;

   doc = htmlParseFile (daisy[misc->current].xml_file, "UTF-8");
   if (! (parse = xmlReaderWalker (doc)))
   {
      endwin ();
      beep ();
      printf ("\n");
      printf (gettext ("Cannot read %s"), daisy[misc->current].xml_file);
      printf ("\n");
      fflush (stdout);
      _exit (1);
   } // if

// parse this smil
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, parse))
         break;
      if (strcasecmp (daisy[misc->current].anchor, my_attribute->id) == 0)
      {
         break;
      } // if
   } // while
   daisy[misc->current].duration = 0;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, parse))
         break;
// get misc->clip_begin
      if (strcasecmp (misc->tag, "audio") == 0)
      {
         get_clips (misc, my_attribute);
         daisy[misc->current].begin = misc->clip_begin;
         daisy[misc->current].duration += misc->clip_end - misc->clip_begin;
      } // if
   } // while

// get clip_end
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, parse))
         break;
      if (misc->current + 1 < misc->total_items &&
          *daisy[misc->current + 1].anchor)
      {
         if (strcasecmp (my_attribute->id,
                         daisy[misc->current + 1].anchor) == 0)
            break;
      } // if
      if (strcasecmp (misc->tag, "audio") == 0)
      {
         get_clips (misc, my_attribute);
         daisy[misc->current].duration += misc->clip_end - misc->clip_begin;
      } // IF
      if (misc->current + 1 < misc->total_items &&
          *daisy[misc->current + 1].anchor)
      {
         if (strcasecmp (my_attribute->id, daisy[misc->current + 1].anchor) == 0)
         {
            break;
         } // if
      } // if
   } // while
   misc->total_time += daisy[misc->current].duration;
   xmlTextReaderClose (parse);
} // parse_clips

void parse_smil (misc_t *misc, my_attribute_t *my_attribute,
                     daisy_t *daisy)
{
   xmlTextReaderPtr smil;
   htmlDocPtr doc;

#ifdef DAISY_PLAYER
   daisy[misc->current].xml_file =
      malloc (strlen (misc->daisy_mp) + strlen (my_attribute->href) + 5);
   strcpy (daisy[misc->current].xml_file, misc->daisy_mp);
   strcat (daisy[misc->current].xml_file, "/");
   strcat (daisy[misc->current].xml_file, my_attribute->href);
#endif
   if (! (doc = htmlParseFile (daisy[misc->current].xml_file, "UTF-8")))
      failure (daisy[misc->current].xml_file, errno);
   if (! (smil = xmlReaderWalker (doc)))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR,
                gettext ("Cannot read %s"), daisy[misc->current].xml_file);
      failure (str, e);
   } // if
   daisy[misc->current].level = misc->depth = 1;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, smil))
         break;
      if (strcasecmp (misc->tag, "text") == 0)
      {
         daisy[misc->current].xml_file = malloc
                (strlen (misc->daisy_mp) + strlen (my_attribute->src) + 1);
         strcpy (daisy[misc->current].xml_file, misc->daisy_mp);
         strcat (daisy[misc->current].xml_file, "/");
         strcat (daisy[misc->current].xml_file, my_attribute->src);
         daisy[misc->current].anchor = strdup ("");
         if (strchr (daisy[misc->current].xml_file, '#'))
         {
            daisy[misc->current].anchor = strdup
                (strchr (daisy[misc->current].xml_file, '#') + 1);
            *strchr (daisy[misc->current].xml_file, '#') = 0;
         } // if
      } // if
      if (strcasecmp (misc->tag, "audio") == 0)
         parse_clips (misc, my_attribute, daisy);
      misc->current++;                
      if (misc->current > misc->total_items)
      {
         misc->total_items = misc->current;
         daisy = (daisy_t *)
                  realloc (daisy, misc->total_items * sizeof (daisy_t));
      } // if
   } // while
} // parse_smil
#endif

void fill_xml_anchor_opf (misc_t *misc, my_attribute_t *my_attribute,
                           daisy_t *daisy)
{
// first of all fill daisy struct xml_file and anchor
   htmlDocPtr doc;
   xmlTextReaderPtr opf;

   if (! (doc = htmlParseFile (misc->opf_name, "UTF-8")))
   {
      int e;
      char str[MAX_STR + 1];
                                        
      e = errno;
      beep ();
      snprintf (str, MAX_STR, "fill_xml_anchor_opf %s", misc->opf_name);
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

   misc->current = 0;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, opf))
         break;
      if (strcasecmp (misc->tag, "itemref") == 0)
      {
         htmlDocPtr doc;
         xmlTextReaderPtr ptr;
         char *idref;

         idref = strdup (my_attribute->idref);
         if (! (doc = htmlParseFile (misc->opf_name, "UTF-8")))
            failure (misc->opf_name, errno);
         if (! (ptr = xmlReaderWalker (doc)))
            failure (misc->opf_name, errno);
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, ptr))
               break;
            if (strcasecmp (my_attribute->id, idref) == 0)
            {
               daisy[misc->current].xml_file = malloc
                 (strlen (misc->daisy_mp) + strlen (my_attribute->href) + 5);
               strcpy (daisy[misc->current].xml_file, misc->daisy_mp);
               strcat (daisy[misc->current].xml_file, "/");
               strcat (daisy[misc->current].xml_file, my_attribute->href);
               daisy[misc->current].anchor = strdup ("");
               if (strchr (daisy[misc->current].xml_file, '#'))
               {
                  daisy[misc->current].anchor = strdup
                           (strchr (daisy[misc->current].xml_file, '#') + 1);
                  *strchr (daisy[misc->current].xml_file, '#') = 0;
               } // if
               break;
            } // if
         } // while
         misc->current++;
      } // if
   } // while
} // fill_xml_anchor_opf

void parse_opf (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   htmlDocPtr doc;
   xmlTextReaderPtr opf;

   if (misc->items_in_opf == 0)
   {
      endwin ();
      beep ();
      printf ("No items found\n");
      exit (0);
   } // if
   misc->total_items = misc->items_in_opf;
   fill_xml_anchor_opf (misc, my_attribute, daisy);
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
#ifdef DAISY_PLAYER
   misc->total_time = 0;
#endif
   misc->current = misc->displaying = 0;
   daisy[misc->current].level = misc->depth = 1;
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
#ifdef EBOOK_SPEAKER
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
      } // if spine
#endif
#ifdef DAISY_PLAYER
      if (strcasestr (my_attribute->media_type, "application/smil"))
      {
         parse_smil (misc, my_attribute, daisy);
      } // if
#endif
   } // while
} // parse_opf     

daisy_t *create_daisy_struct (misc_t *misc, my_attribute_t *my_attribute)
{
   xmlTextReaderPtr ncx;
   htmlDocPtr doc;
   FILE *p;

   *misc->daisy_version = 0;

// lookfor "ncc.html"
   snprintf (misc->cmd, MAX_CMD - 1,
             "find \"%s\" -maxdepth 5 -iname \"ncc.html\" -print0",
             misc->tmp_dir);
   if ((p = popen (misc->cmd, "r")) == NULL)
      failure (misc->cmd, errno);
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

// lookfor *.ncx
   snprintf (misc->cmd, MAX_CMD - 1,
             "find \"%s\" -maxdepth 5 -iname \"*.ncx\" -print0",
             misc->tmp_dir);
   if ((p = popen (misc->cmd, "r")) == NULL)
      failure (misc->cmd, errno);
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
   snprintf (misc->cmd, MAX_CMD - 1,
             "find \"%s\" -maxdepth 5 -iname \"*.opf\" -print0",
             misc->tmp_dir);
   p = popen (misc->cmd, "r");
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
   if (misc->total_items == 0)
      misc->total_items = 1;
   return (daisy_t *) malloc (misc->total_items * sizeof (daisy_t));
} // create_daisy_struct

void read_daisy_3 (misc_t *misc, my_attribute_t *my_attribute,
                   daisy_t *daisy)
{
   char str[MAX_STR + 1];
#ifdef EBOOK_SPEAKER
   int i;
#endif

   misc->total_items = misc->items_in_opf;
   strncpy (str, misc->opf_name, MAX_STR);
   strncpy (misc->daisy_mp, dirname (str), MAX_STR);
/* temporarily disabled
#ifdef EBOOK_SPEAKER
   parse_opf (misc, my_attribute, daisy);
   misc->total_items = misc->items_in_opf;
   count_phrases (misc, my_attribute, daisy);
   misc->phrases_in_opf = misc->total_phrases;

   misc->total_items = misc->items_in_ncx;
   strncpy (str, misc->ncx_name, MAX_STR);
   strncpy (misc->daisy_mp, dirname (str), MAX_STR);
   parse_ncx (misc, my_attribute, daisy);
   misc->total_items = misc->items_in_ncx;
   count_phrases (misc, my_attribute, daisy);
   misc->phrases_in_ncx = misc->total_phrases;

   if (misc->phrases_in_ncx > misc->phrases_in_opf)
   {
      misc->total_phrases = misc->phrases_in_ncx;
      parse_ncx (misc, my_attribute, daisy);
      for (i = 0; i < misc->items_in_ncx; i++)
         daisy[i].orig_smil = strdup (daisy[i].xml_file);
      return;
   } // if

   if (misc->phrases_in_opf > misc->phrases_in_ncx)
   {
      misc->total_phrases = misc->phrases_in_opf;
      parse_opf (misc, my_attribute, daisy);
      for (i = 0; i < misc->items_in_opf; i++)
         daisy[i].orig_smil = strdup (daisy[i].xml_file);
      return;
   } // if
#endif
*/

   if (misc->use_OPF)
   {
      parse_opf (misc, my_attribute, daisy);
      misc->total_items = misc->items_in_opf;
      if (misc->total_items == 0)
         misc->total_items = 1;
   } // if
   else
   if (misc->use_NCX)
   {
      parse_ncx (misc, my_attribute, daisy);
      misc->total_items = misc->items_in_ncx;
      if (misc->total_items == 0)
         misc->total_items = 1;
   } // if
   else
   if (misc->items_in_opf > misc->items_in_ncx)
   {
      parse_opf (misc, my_attribute, daisy);
      misc->total_items = misc->items_in_opf;
   }
   else
   {
      parse_ncx (misc, my_attribute, daisy);
      misc->total_items = misc->items_in_ncx;
   } // if
#ifdef EBOOK_SPEAKER
   for (i = 0; i < misc->total_items; i++)
      daisy[i].orig_smil = strdup (daisy[i].xml_file);
#endif
} // read_daisy_3

void parse_ncx (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   htmlDocPtr doc;
   xmlTextReaderPtr ncx;

   if (misc->items_in_ncx == 0)
   {
      endwin ();
      beep ();
      printf ("No items found\n");
      exit (0);
   } // if
   misc->total_items = misc->items_in_ncx;
   fill_xml_anchor_ncx (misc, my_attribute, daisy);
   if (! (doc = htmlParseFile (misc->ncx_name, "UTF-8")))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"), misc->ncx_name);
      failure (str, e);
   } // if
   ncx = xmlReaderWalker (doc);
   misc->current = misc->displaying = 0;
   misc->level = misc->depth = 0;
   while (1)
   {
      if (misc->current >= misc->total_items)
         break;
      if (! get_tag_or_label (misc, my_attribute, ncx))
         break;
      if (strcasecmp (misc->tag, "docTitle") == 0)
      {
         misc->label = strdup ("");
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
         if (misc->level > misc->depth)
            misc->depth = misc->level;
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
               while (1)
               {
                  if (! get_tag_or_label (misc, my_attribute, ncx))
                     break;
                  if (strcasecmp (misc->tag, "/text") == 0)
                     break;
                  if (*misc->label)
                  {
                     daisy[misc->current].label = strdup (misc->label);
                     break;
                  } // if
               } // while   
            } //  if (strcasecmp (misc->tag, "text") == 0)
            if (strcasecmp (misc->tag, "content") == 0)
            {
               parse_content (misc, my_attribute, daisy);
               daisy[misc->current].level = misc->level;
               fill_item (misc, daisy);
               misc->current++;
               break;
            } // if (strcasecmp (misc->tag, "content") == 0)
         } // while
      } // if (strcasecmp (misc->tag, "navpoint") == 0)
      if (strcasecmp (misc->tag, "/navpoint") == 0)
         misc->level--;
   } // while
   misc->total_items = misc->current;
} // parse_ncx

