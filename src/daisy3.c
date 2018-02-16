/* daisy3.c - functions to insert daisy3 info into a struct.
 *
 * Copyright (C) 2017 J. Lemmens
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

void fill_page_numbers (misc_t *misc, daisy_t *daisy,
                        my_attribute_t *my_attribute)
{
   int i;
   htmlDocPtr doc;
   xmlTextReaderPtr page;

   for (i = 0; i < misc->total_items; i++)
   {
      daisy[i].page_number = 0;
      if (! (doc = htmlParseFile (daisy[i].xml_file, "UTF-8")))
         failure (misc, daisy[i].xml_file, errno);
      if (! (page = xmlReaderWalker (doc)))
         failure (misc, daisy[i].xml_file, errno);

      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, page))
            break;
#ifdef DAISY_PLAYER
         if (*my_attribute->id)
         {
            if (! *daisy[i].first_id)
               strncpy (daisy[i].first_id, my_attribute->id, MAX_STR);
         } // if
#endif
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
      xmlTextReaderClose (page);
      xmlFreeDoc (doc);
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
         xmlTextReaderPtr text;

         src = malloc
              (strlen (misc->daisy_mp) + strlen (my_attribute->src) + 3);
         strcpy (src, misc->daisy_mp);
         strcat (src, "/");
         strcat (src, my_attribute->src);
         anchor = strdup ("");
         if (strchr (src, '#'))
         {
            anchor = strdup (strchr (src, '#') + 1);
            *strchr (src, '#') = 0;
         } // if
         doc = htmlParseFile (src, "UTF-8");
         if (! (text = xmlReaderWalker (doc)))
            failure (misc, src, errno);

         if (*anchor)
         {
            do
            {
               if (! get_tag_or_label (misc, my_attribute, text))
               {
                  xmlTextReaderClose (text);
                  xmlFreeDoc (doc);
                  return;
               } // if
            } while (strcasecmp (my_attribute->id, anchor) != 0);
         } // if

         do
         {
            if (! get_tag_or_label (misc, my_attribute, text))
            {
               xmlTextReaderClose (text);
               xmlFreeDoc (doc);
               return;
            } // if
         } while (! *misc->label);
         xmlTextReaderClose (text);
         xmlFreeDoc (doc);
         misc->current_page_number = atoi (misc->label);
         return;
      } // if
   } // while
} // parse_page_number

void fill_xml_anchor_ncx (misc_t *misc, my_attribute_t *my_attribute,
                          daisy_t *daisy)
{
// first of all fill daisy struct xml_file and anchor
   htmlDocPtr doc;
   xmlTextReaderPtr ncx;

   misc->total_items = misc->items_in_ncx;
   if (! (doc = htmlParseFile (misc->ncx_name, "UTF-8")))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"), misc->ncx_name);
      failure (misc, str, e);
   } // if
   if (! (ncx = xmlReaderWalker (doc)))
      failure (misc, misc->ncx_name, errno);
   misc->depth = 0;
   misc->current = misc->displaying = 0;
   while (1)
   {
      daisy[misc->current].xml_file = strdup ("");
      daisy[misc->current].anchor = strdup   ("");
      daisy[misc->current].x = 0;
      *daisy[misc->current].label = 0;
      daisy[misc->current].page_number = 0;
      if (misc->current >= misc->total_items)
         break;
      if (! get_tag_or_label (misc, my_attribute, ncx))
         break;
      if (strcasecmp (misc->tag, "doctitle") == 0)
      {
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, ncx))
               break;
            if (strcasecmp (misc->tag, "/doctitle") == 0)
               break;
            if (*misc->label)
            {
               strncpy (misc->daisy_title, misc->label, MAX_STR - 1);
               break;
            } // if
         } // while
      } // if doctitle
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
               daisy[misc->current].xml_file = strdup
                    (convert_URL_name (misc, daisy[misc->current].xml_file));
               doc = htmlParseFile (daisy[misc->current].xml_file, "UTF-8");
               if (! (content = xmlReaderWalker (doc)))
               {
                  int e;
                  char *str;

                  e = errno;
                  str = strdup ((gettext
                      ("Cannot read %s"), daisy[misc->current].xml_file));
                  failure (misc, str, e);
               } // if

               if (*daisy[misc->current].anchor)
               {
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
                  if (*my_attribute->id)
                  {
                     if (! *daisy[misc->current].first_id)
                        strncpy (daisy[misc->current].first_id,
                                 my_attribute->id, MAX_STR);
                  } // if
#endif
                  if (strcasecmp (misc->tag, "text") == 0)
                  {
                     daisy[misc->current].xml_file = malloc (strlen
                          (misc->daisy_mp) + strlen (my_attribute->src) + 5);
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
                     daisy[misc->current].xml_file = strdup
                     (convert_URL_name (misc, daisy[misc->current].xml_file));
                     break;
                  } // if
               } // while
               xmlTextReaderClose (content);
               xmlFreeDoc (doc);
               misc->current++;
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
   daisy[misc->current].clips_file = malloc
               (strlen (misc->daisy_mp) + strlen (my_attribute->src) + 5);
   strcpy (daisy[misc->current].clips_file, misc->daisy_mp);
   strcat (daisy[misc->current].clips_file, "/");
   strcat (daisy[misc->current].clips_file, my_attribute->src);
   if (strchr (daisy[misc->current].clips_file, '#'))
   {
      daisy[misc->current].clips_anchor =
               strdup (strchr (daisy[misc->current].clips_file, '#') + 1);
      *strchr (daisy[misc->current].clips_file, '#') = 0;
   } // if
   daisy[misc->current].clips_file =
      strdup (convert_URL_name (misc, daisy[misc->current].clips_file));
   if (! (doc = htmlParseFile (daisy[misc->current].clips_file, "UTF-8")))
      failure (misc, daisy[misc->current].clips_file, errno);
#endif
#ifdef EBOOK_SPEAKER
   if (! (doc = htmlParseFile (daisy[misc->current].xml_file, "UTF-8")))
      failure (misc, daisy[misc->current].xml_file, errno);
#endif
   if (! (content = xmlReaderWalker (doc)))
   {
      int e;
      char *str;

      e = errno;
      str = strdup
       ((gettext ("Cannot read %s"), daisy[misc->current].xml_file));
      failure (misc, str, e);
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
      if (*my_attribute->id)
      {
         if (! *daisy[misc->current].first_id)
            strncpy (daisy[misc->current].first_id, my_attribute->id,
                     MAX_STR);
      } // if
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
   xmlTextReaderClose (content);
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
   int remainder;

   doc = htmlParseFile (daisy[misc->current].xml_file, "UTF-8");
   if (! (text = xmlReaderWalker (doc)))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"),
                daisy[misc->current].xml_file);
      failure (misc, str, e);
   } // if
                                             
   daisy[misc->current].level = 1;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, text))
         break;
#ifdef DAISY_PLAYER
      if (*my_attribute->id)
      {
         if (! *daisy[misc->current].first_id)
            strncpy (daisy[misc->current].first_id, my_attribute->id,
                     MAX_STR);
      } // if
#endif
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
         {
            daisy[misc->current].level = 1;
         }
         else
            daisy[misc->current].level = misc->tag[1] - 48;
         if (daisy[misc->current].level > misc->depth)
            misc->depth = daisy[misc->current].level;
         if (! *daisy[misc->current].anchor)
            break;
      } // if
      if (*daisy[misc->current].anchor &&
          strcasecmp (my_attribute->id, daisy[misc->current].anchor) == 0)
         break;
   } // while

   *daisy[misc->current].label = 0;
   if (! xmlTextReaderIsEmptyElement (text))
   {
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
            remainder = 75 - strlen (daisy[misc->current].label);
            if (remainder > 0)
            {
               strncat (daisy[misc->current].label, misc->label, remainder);
               strcat (daisy[misc->current].label, " ");
            }
            else
               break;
         } // if
      } // while
   } // ifh
   daisy[misc->current].x = daisy[misc->current].level * 3 - 1;
   daisy[misc->current].screen = misc->current / misc->max_y;
   daisy[misc->current].y =
          misc->current - (daisy[misc->current].screen * misc->max_y);
#ifdef EBOOK_SPEAKER
   daisy[misc->current].n_phrases = 0;
#endif
   xmlTextReaderClose (text);
   xmlFreeDoc (doc);
} // parse_text

void parse_href (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   htmlDocPtr doc;
   xmlTextReaderPtr href;
   int remainder;

   if (! (doc = htmlParseFile (daisy[misc->current].xml_file, "UTF-8")))
      failure (misc, daisy[misc->current].xml_file, errno);
   if (! (href = xmlReaderWalker (doc)))
   {
      int e;

      e = errno;
      snprintf (misc->str, MAX_STR,
                gettext ("Cannot read %s"), my_attribute->href);
      failure (misc, misc->str, e);
   } // if
\
   daisy[misc->current].level = 1;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, href))
         break;
#ifdef DAISY_PLAYER
      if (*my_attribute->id)
      {
         if (! *daisy[misc->current].first_id)
            strncpy (daisy[misc->current].first_id, my_attribute->id,
                     MAX_STR);
      } // if
#endif
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
         daisy[misc->current].xml_file =
            strdup (convert_URL_name (misc, daisy[misc->current].xml_file));
         parse_text (misc, my_attribute, daisy);
         xmlTextReaderClose (href);
         xmlFreeDoc (doc);
         return;
      } // if

// some EPUB use this
      if (strcasecmp (misc->tag, "i") == 0)
      {
         xmlTextReaderClose (href);
         xmlFreeDoc (doc);
         return;
      } // if

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
         *daisy[misc->current].label = 0;
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
            *daisy[misc->current].label = 0;
            if (*misc->label)
            {
               remainder = 75 - strlen (daisy[misc->current].label);
               if (remainder > 0)
               {
                  strncat (daisy[misc->current].label, misc->label,remainder);
                  strcat (daisy[misc->current].label, " ");
               }
               else
                  break;
            } // if
         } // while
         fill_item (misc, daisy);
         xmlTextReaderClose (href);
         xmlFreeDoc (doc);
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
               daisy[misc->current].xml_file =
                  strdup (convert_URL_name (misc, daisy[misc->current].xml_file));
               xmlTextReaderClose (href);
               xmlFreeDoc (doc);
               return;
            } // if
         } // while
         xmlTextReaderClose (href);
         xmlFreeDoc (doc);
         return;
      } // if (strcasecmp (my_attribute->class, "h1") == 0 ||
   } // while
   xmlTextReaderClose (href);
   xmlFreeDoc (doc);
} // parse_href

void parse_manifest (misc_t *misc, my_attribute_t *my_attribute,
                     daisy_t *daisy)
{
   xmlTextReaderPtr manifest;
   htmlDocPtr doc;
   char *idref;

   idref = strdup (my_attribute->idref);
   if (! (doc = htmlParseFile (misc->opf_name, "UTF-8")))
      failure (misc, misc->opf_name, errno);
   if (! (manifest = xmlReaderWalker (doc)))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"), misc->opf_name);
      failure (misc, str, e);
   } // if
   daisy[misc->current].x = 0;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, manifest))
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
         daisy[misc->current].xml_file =
            strdup (convert_URL_name (misc, daisy[misc->current].xml_file));
         parse_href (misc, my_attribute, daisy);
#ifdef DAISY_PLAYER
         daisy[misc->current].clips_file =
                strdup (daisy[misc->current].xml_file);
         daisy[misc->current].clips_anchor =
                strdup (daisy[misc->current].anchor);
         parse_smil_3 (misc, my_attribute, daisy);
#endif
         if (daisy[misc->current].level > misc->depth)
            misc->depth = daisy[misc->current].level;
         misc->current++;
         daisy[misc->current].level = 1;
         daisy[misc->current].x = 0;
         break;
      } // if
   } // while
   xmlTextReaderClose (manifest);
   xmlFreeDoc (doc);
} // parse_manifest

#ifdef DAISY_PLAYER
void parse_clips (misc_t *misc, my_attribute_t *my_attribute,
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
      if (*my_attribute->id)
      {
         if (! *daisy[misc->current].first_id)
            strncpy (daisy[misc->current].first_id, my_attribute->id,
                     MAX_STR);
      } // if
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
   xmlFreeDoc (doc);
} // parse_clips
#endif

#ifdef DAISY_PLAYER
void parse_smil_3 (misc_t *misc, my_attribute_t *my_attribute,
                     daisy_t *daisy)
{
   xmlTextReaderPtr smil;
   htmlDocPtr doc;

   daisy[misc->current].clips_file =
          malloc (strlen (misc->daisy_mp) + strlen (my_attribute->href) + 5);
   strcpy (daisy[misc->current].clips_file, misc->daisy_mp);
   strcat (daisy[misc->current].clips_file, "/");
   strcat (daisy[misc->current].clips_file, my_attribute->href);
   daisy[misc->current].clips_file =
      strdup (convert_URL_name (misc, daisy[misc->current].clips_file));
   if (! (doc = htmlParseFile (daisy[misc->current].clips_file, "UTF-8")))
      failure (misc, daisy[misc->current].clips_file, errno);
   if (! (smil = xmlReaderWalker (doc)))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR,
                gettext ("Cannot read %s"), daisy[misc->current].clips_file);
      failure (misc, str, e);
   } // if
   daisy[misc->current].level = misc->depth = 1;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, smil))
         break;
      if (*my_attribute->id)
      {
         if (! *daisy[misc->current].first_id)
            strncpy (daisy[misc->current].first_id, my_attribute->id,
                     MAX_STR);
      } // if
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
         daisy[misc->current].xml_file =
            strdup (convert_URL_name (misc, daisy[misc->current].xml_file));
      } // if
      if (! *daisy[misc->current].first_id)
      {
         if (*my_attribute->id)
            strncpy (daisy[misc->current].first_id, my_attribute->id,
                     MAX_STR);
      } // if
      if (strcasecmp (misc->tag, "audio") == 0)
      {
         parse_clips (misc, my_attribute, daisy);
      } // if
      misc->current++;
      if (misc->current >= misc->total_items)
      {
         break;
      } // if
   } // while
   xmlTextReaderClose (smil);
   xmlFreeDoc (doc);
} // parse_smil_3
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
      char *str;

      e = errno;
      beep ();
      str = malloc (50 + strlen (misc->opf_name));
      sprintf (str, "fill_xml_anchor_opf %s", misc->opf_name);
      failure (misc, str, e);
   } // if
   if ((opf = xmlReaderWalker (doc)) == NULL)
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"), misc->opf_name);
      failure (misc, str, e);
   } // if

   misc->depth = 0;
   misc->current = 0;
   while (1)
   {
      daisy[misc->current].xml_file = strdup ("");
      daisy[misc->current].anchor = strdup   ("");
      daisy[misc->current].x = 0;
      *daisy[misc->current].label = 0;
      daisy[misc->current].page_number = 0;
      if (! get_tag_or_label (misc, my_attribute, opf))
         break;
#ifdef EBOOK_SPEAKER
      if (strcasecmp (misc->tag, "itemref") == 0)
      {
         htmlDocPtr doc;
         xmlTextReaderPtr ptr;
         char *idref;

         idref = strdup (my_attribute->idref);
         if (! (doc = htmlParseFile (misc->opf_name, "UTF-8")))
            failure (misc, misc->opf_name, errno);
         if (! (ptr = xmlReaderWalker (doc)))
            failure (misc, misc->opf_name, errno);
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
               daisy[misc->current].xml_file =
                  strdup (convert_URL_name (misc, daisy[misc->current].xml_file));
// if it is a smil
               {
                  htmlDocPtr doc;
                  xmlTextReaderPtr smil;

                  if (! (doc =
                       htmlParseFile (daisy[misc->current].xml_file, "UTF-8")))
                     failure (misc, daisy[misc->current].xml_file, errno);
                  if ((smil = xmlReaderWalker (doc)) == NULL)
                     failure (misc, daisy[misc->current].xml_file, errno);
                  while (1)
                  {
                     if (! get_tag_or_label (misc, my_attribute, smil))
                        break;
#ifdef DAISY_PLAYER
                     if (*my_attribute->id)
                     {
                        if (! *daisy[misc->current].first_id)
                           strncpy (daisy[misc->current].first_id,
                                    my_attribute->id, MAX_STR);
                     } // if
#endif
                     if (strcasecmp (misc->tag, "text") == 0)
                     {
                        daisy[misc->current].xml_file = malloc (strlen
                          (misc->daisy_mp) + strlen (my_attribute->src) + 5);
                        strcpy (daisy[misc->current].xml_file,
                                misc->daisy_mp);
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
                        daisy[misc->current].xml_file = strdup
                           (convert_URL_name (misc, daisy[misc->current].xml_file));
                        break;
                     } // if
                  } // while
                  xmlTextReaderClose (smil);
                  xmlFreeDoc (doc);
               } // if it is a smil
               parse_text (misc, my_attribute, daisy);
               break;
            } // if
         } // while
         xmlTextReaderClose (ptr);
         xmlFreeDoc (doc);
         misc->current++;
         if (misc->current >= misc->total_items)
            break;
      } // if itemref
#endif
#ifdef DAISY_PLAYER
      if (strcasestr (my_attribute->media_type, "application/smil"))
      {
         htmlDocPtr doc;
         xmlTextReaderPtr smil;

         daisy[misc->current].clips_file = malloc
                 (strlen (misc->daisy_mp) + strlen (my_attribute->href) + 5);
         strcpy (daisy[misc->current].clips_file, misc->daisy_mp);
         strcat (daisy[misc->current].clips_file, "/");
         strcat (daisy[misc->current].clips_file, my_attribute->href);
         daisy[misc->current].clips_anchor = strdup ("");
         if (strchr (daisy[misc->current].clips_file, '#'))
         {
            daisy[misc->current].clips_anchor = strdup
                      (strchr (daisy[misc->current].clips_file, '#') + 1);
            *strchr (daisy[misc->current].clips_file, '#') = 0;
         } // if
         daisy[misc->current].clips_file =
            strdup(convert_URL_name (misc, daisy[misc->current].clips_file));
         if (! (doc = htmlParseFile (daisy[misc->current].clips_file, "UTF-8")))
            failure (misc, daisy[misc->current].clips_file, errno);
         if ((smil = xmlReaderWalker (doc)) == NULL)
            failure (misc, daisy[misc->current].clips_file, errno);
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, smil))
               break;
            if (*my_attribute->id)
            {
               if (! *daisy[misc->current].first_id)
                  strncpy (daisy[misc->current].first_id, my_attribute->id,
                           MAX_STR);
            } // if
            if (strcasecmp (misc->tag, "text") == 0)
            {
               daisy[misc->current].xml_file = malloc
                 (strlen (misc->daisy_mp) + strlen (my_attribute->src) + 5);
               strcpy (daisy[misc->current].xml_file, misc->daisy_mp);
               strcat (daisy[misc->current].xml_file, "/");               strcat (daisy[misc->current].xml_file, my_attribute->src);
               daisy[misc->current].anchor = strdup ("");
               if (strchr (daisy[misc->current].xml_file, '#'))
               {
                  daisy[misc->current].anchor = strdup
                        (strchr (daisy[misc->current].xml_file, '#') + 1);
                  *strchr (daisy[misc->current].xml_file, '#') = 0;
               } // if
               daisy[misc->current].xml_file =
                  strdup (convert_URL_name (misc, daisy[misc->current].xml_file));
               parse_text (misc, my_attribute, daisy);
               break;
            } // if
         } // while
         xmlTextReaderClose (smil);
         xmlFreeDoc (doc);
         misc->current++;
         if (misc->current >= misc->total_items)
            break;
      } // if
#endif
   } // while
   xmlTextReaderClose (opf);
   misc->total_items = misc->current;
#ifdef DAISY_PLAYER
   if (misc->total_items == 0)
   {
      beep ();
      quit_daisy_player (misc, daisy);
      printf ("%s\n", gettext (
        "This book has no audio. Play this book with eBook-speaker"));
      _exit (0);
   } // if
#endif
} // fill_xml_anchor_opf

void parse_opf (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   int i;
   htmlDocPtr doc;
   xmlTextReaderPtr opf;

   if (misc->items_in_opf == 0)
   {
      endwin ();
      beep ();
      printf ("%s\n", gettext ("No items found. Try option \"-N\"."));
      exit (0);
   } // if

   misc->total_items = misc->items_in_opf;
   if (misc->total_items == 0)
      misc->total_items = 1;
#ifdef DAISY_PLAYER
   for (i = 0; i < misc->total_items; i++)
      *daisy[i].first_id = 0;
#endif
   misc->daisy_mp = strdup (misc->opf_name);
   misc->daisy_mp = dirname (misc->daisy_mp);
   fill_xml_anchor_opf (misc, my_attribute, daisy);
   if (! (doc = htmlParseFile (misc->opf_name, "UTF-8")))
   {
      int e;
      char *str;

      e = errno;
      beep ();
      str = malloc (20 + strlen (misc->opf_name));
      sprintf (str, "parse_opf: %s", misc->opf_name);
      failure (misc, str, e);
   } // if
   if ((opf = xmlReaderWalker (doc)) == NULL)
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"), misc->opf_name);
      failure (misc, str, e);
   } // if
#ifdef DAISY_PLAYER
   misc->total_time = 0;
#endif
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

/* do not use
      if (strcasecmp (misc->tag, "spine") == 0)
      {
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, opf))
               break;
            if (strcasecmp (misc->tag, "itemref") == 0)
               parse_manifest (misc, my_attribute, daisy);
         } // while
      } // if spine
*/

   } // while
   xmlTextReaderClose (opf);
   xmlFreeDoc (doc);
   misc->items_in_opf = misc->total_items;

   for (i = 0; i < misc->items_in_opf; i++)
   {
#ifdef EBOOK_SPEAKER
      daisy[i].orig_smil = strdup (daisy[i].xml_file);
      *daisy[i].class = 0;
#endif
   } //for
} // parse_opf

void read_daisy_3 (misc_t *misc, my_attribute_t *my_attribute,
                   daisy_t *daisy)
{
// when OPF or NCX is forced
   if (misc->use_OPF)
      parse_opf (misc, my_attribute, daisy);
   else
   if (misc->use_NCX)
      parse_ncx (misc, my_attribute, daisy);

// else do
   else
   {
      if (misc->items_in_opf > misc->items_in_ncx)
         parse_opf (misc, my_attribute, daisy);
      else
         parse_ncx (misc, my_attribute, daisy);
   } // if
} // read_daisy_3

void parse_ncx (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
   htmlDocPtr doc;
   xmlTextReaderPtr ncx;
   int i;

   if (misc->items_in_ncx == 0)
   {
      endwin ();
      beep ();
      printf ("%s\n", gettext ("No items found. Try option \"-O\"."));
      exit (0);
   } // if

   misc->total_items = misc->items_in_ncx;
   if (misc->total_items == 0)
      misc->total_items = 1;
   misc->daisy_mp = strdup (misc->ncx_name);
   misc->daisy_mp = dirname (misc->daisy_mp);
#ifdef DAISY_PLAYER
   for (i = 0; i < misc->items_in_ncx; i++)
      *daisy[i].first_id = 0;
#endif
   fill_xml_anchor_ncx (misc, my_attribute, daisy);
   if (! (doc = htmlParseFile (misc->ncx_name, "UTF-8")))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"), misc->ncx_name);
      failure (misc, str, e);
   } // if
   ncx = xmlReaderWalker (doc);
   misc->current = misc->displaying = 0;
   misc->level = misc->depth = 0;
   daisy[misc->current].x = 0;
   while (1)
   {
      if (misc->current >= misc->total_items)
         break;
      if (! get_tag_or_label (misc, my_attribute, ncx))
         break;
#ifdef DAISY_PLAYER
      if (! *daisy[misc->current].first_id)
      {
         if (*my_attribute->id)
            strncpy (daisy[misc->current].first_id, my_attribute->id,
                     MAX_STR);
      } // if
#endif
      if (strcasecmp (misc->tag, "docAuthor") == 0)
      {
         if (xmlTextReaderIsEmptyElement (ncx))
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
               *daisy[misc->current].label = 0;
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
                     strncpy (daisy[misc->current].label, misc->label,
                           75 - strlen (daisy[misc->current].label));
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
               daisy[misc->current].x = 0;
               break;
            } // if (strcasecmp (misc->tag, "content") == 0)
         } // while
      } // if (strcasecmp (misc->tag, "navpoint") == 0)
      if (strcasecmp (misc->tag, "/navpoint") == 0)
         misc->level--;
   } // while
   xmlTextReaderClose (ncx);
   xmlFreeDoc (doc);
   misc->total_items = misc->current;

   for (i = 0; i < misc->items_in_ncx; i++)
   {
#ifdef EBOOK_SPEAKER
      daisy[i].orig_smil = strdup (daisy[i].xml_file);
      *daisy[i].class = 0;
#endif
   } //for
} // parse_ncx
