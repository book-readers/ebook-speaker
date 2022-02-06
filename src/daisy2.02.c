/* daisy2.02.c - functions to insert daisy2.02 info into a struct.
 *
 * Copyright (C)2003-2018 J. Lemmens
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

int get_page_number_2 (misc_t *misc, my_attribute_t *my_attribute,
                       daisy_t *daisy, char *attr)
{
// function for daisy 2.02
   if (daisy[misc->playing].page_number == 0)
      return 0;
#ifdef DAISY_PLAYER
   char *src, *anchor;
   xmlTextReaderPtr page;
   htmlDocPtr doc;

   src = malloc (strlen (misc->daisy_mp) + strlen (attr) + 3);
   anchor = strdup ("");
   if (strchr (attr, '#'))
   {
      free (anchor);
      anchor = strdup (strchr (attr, '#') + 1);
      *strchr (attr, '#') = 0;
   } // if
   get_path_name (misc->daisy_mp, attr, src);
   doc = htmlParseFile (src, "UTF-8");
   if (! (page = xmlReaderWalker (doc)))
   {
      int e;
      char str[MAX_STR];

      e = errno;
      snprintf (str, MAX_STR, gettext ("Cannot read %s"), src);
      free (anchor);
      free (src);
      failure (misc, str, e);
   } // if
   if (*anchor)
   {
      do
      {
         if (! get_tag_or_label (misc, my_attribute, page))
         {
            xmlTextReaderClose (page);
            xmlFreeDoc (doc);
            free (anchor);
            free (src);
            return 0;
         } // if
      } while (strcasecmp (my_attribute->id, anchor) != 0);
   } // if
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, page))
      {
         xmlTextReaderClose (page);
         xmlFreeDoc (doc);
         free (anchor);
         free (src);
         return 0;
      } // if
      if (*misc->label)
      {
         xmlTextReaderClose (page);
         xmlFreeDoc (doc);
         misc->current_page_number = atoi (misc->label);
         free (anchor);
         free (src);
         return 1;
      } // if
   } // while
#endif

   while (1)
   {
      if (*misc->label)
      {
         misc->current_page_number = atoi (misc->label);
#ifdef DAISY_PLAYER
         free (anchor);
         free (src);
#endif
         return 1;
      } // if
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
      {
#ifdef DAISY_PLAYER
         free (anchor);
         free (src);
#endif
         return 0;
      } // if
   } // while
   attr = attr; // don't need it in eBook-speaker
#ifdef DAISY_PLAYER
   free (anchor);
   free (src);
#endif   
} // get_page_number_2

void parse_smil_2 (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
// function for daisy 2.02
   htmlDocPtr doc;
   xmlTextReaderPtr parse;

#ifdef DAISY_PLAYER
   misc->total_time = 0;
#endif
   misc->current = 0;
   while (1)
   {
      if (misc->current == misc->total_items)
         break;
      if (! daisy[misc->current].clips_file)
         continue;
      if (*daisy[misc->current].clips_file == 0)
         continue;
      if (! (doc = htmlParseFile (daisy[misc->current].clips_file, "UTF-8")))
         failure (misc, daisy[misc->current].clips_file, errno);
      if (! (parse = xmlReaderWalker (doc)))
      {
         int e;
         char str[MAX_STR];

         e = errno;
         snprintf (str, MAX_STR,
                gettext ("Cannot read %s"), daisy[misc->current].clips_file);
         failure (misc, str, e);
      } // if

// parse this smil
      if (*daisy[misc->current].clips_anchor)
      {
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, parse))
               break;
            if (strcasecmp (my_attribute->id,
                            daisy[misc->current].clips_anchor) == 0)
            {
               free (daisy[misc->current].anchor);
               daisy[misc->current].anchor = strdup ("");
               if (strchr (my_attribute->src, '#'))
               {
                  free (daisy[misc->current].anchor);
                  daisy[misc->current].anchor = strdup
                         (strchr (my_attribute->src, '#') + 1);
                  *strchr (my_attribute->src, '#') = 0;
               } // if
               daisy[misc->current].xml_file = realloc
                   (daisy[misc->current].xml_file,
                    strlen (misc->daisy_mp) + strlen (my_attribute->src) + 5);
               get_path_name (misc->daisy_mp,
               convert_URL_name (misc, my_attribute->src), 
               daisy[misc->current].xml_file);
               free (daisy[misc->current].orig_smil);
               daisy[misc->current].orig_smil =
                    strdup (daisy[misc->current].xml_file);
               break;
            } // if
         } // while
      } // if clips_anchor
#ifdef DAISY_PLAYER
      daisy[misc->current].duration = 0;
      *daisy[misc->current].first_id =  0;
#endif
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, parse))
            break;
#ifdef DAISY_PLAYER
         if (strcasecmp (misc->tag, "audio") == 0)
         {
            if (! *daisy[misc->current].first_id)
            {
               if (*my_attribute->id)
                  strncpy (daisy[misc->current].first_id, my_attribute->id,
                           MAX_STR);
            } // if
            misc->has_audio_tag = 1;
            misc->current_audio_file = realloc 
                    (misc->current_audio_file,
                     strlen (misc->daisy_mp) + strlen (my_attribute->src) + 5);
            get_path_name (misc->daisy_mp,
                        convert_URL_name (misc, my_attribute->src),
                        misc->current_audio_file);
            get_clips (misc, my_attribute);
            daisy[misc->current].begin = misc->clip_begin;
            daisy[misc->current].duration +=
                     misc->clip_end - misc->clip_begin;
            while (1)
            {
               if (*my_attribute->id)
                  strncpy (daisy[misc->current].last_id, my_attribute->id,
                           MAX_STR);
               if (! get_tag_or_label (misc, my_attribute, parse))
                  break;
               if (misc->current + 1 < misc->total_items &&
                   daisy[misc->current + 1].clips_anchor != NULL)
               {
                  if (strcasecmp (my_attribute->id,
                                  daisy[misc->current + 1].clips_anchor) == 0)
                  {
                     break;
                  } // if
               } // if
               if (strcasecmp (misc->tag, "audio") == 0)
               {
                  get_clips (misc, my_attribute);
                  daisy[misc->current].duration +=
                                  misc->clip_end - misc->clip_begin;
               } // if (strcasecmp (misc->tag, "audio") == 0)
            } // while
            if (misc->current + 1 < misc->total_items)
               if (*daisy[misc->current + 1].clips_anchor)
                  if (strcasecmp (my_attribute->id,
                            daisy[misc->current + 1].clips_anchor) == 0)
                     break;
         } // if (strcasecmp (misc->tag, "audio") == 0)
#endif
         if (strcasecmp (misc->tag, "text") == 0)
         {
            free (daisy[misc->current].anchor);
            daisy[misc->current].anchor = strdup ("");
            if (strchr (my_attribute->src, '#'))
            {
               free (daisy[misc->current].anchor);
               daisy[misc->current].anchor =
                    strdup (strchr (my_attribute->src, '#') + 1);
               *strchr (my_attribute->src, '#') = 0;
            } // if
            daisy[misc->current].xml_file = realloc 
                   (daisy[misc->current].xml_file,
                    strlen (misc->daisy_mp) + strlen (my_attribute->src) + 5);
            get_path_name (misc->daisy_mp, convert_URL_name (misc,
                           my_attribute->src), daisy[misc->current].xml_file);
            if (misc->current + 1 < misc->total_items &&
                *daisy[misc->current + 1].clips_anchor &&
                strcasecmp (my_attribute->id,
                            daisy[misc->current + 1].clips_anchor) == 0)
            {
               break;
            } // if
         } // if
      } // while
      xmlTextReaderClose (parse);
      xmlFreeDoc (doc);
#ifdef DAISY_PLAYER
      misc->total_time += daisy[misc->current].duration;
#endif
      misc->current++;
      if (misc->current >= misc->total_items)
         return;
   } // while
} // parse_smil_2

void get_label_2 (misc_t *misc, daisy_t *daisy, int indent)
{
   strncpy (daisy[misc->current].label, misc->label, 80);
   daisy[misc->current].label[80] = 0;
   if (misc->displaying == misc->max_y)
      misc->displaying = 1;
   if (*daisy[misc->current].class)
   {
      if (strcasecmp (daisy[misc->current].class, "pagenum") == 0)
         daisy[misc->current].x = 0;
      else
         if (daisy[misc->current].x == 0)
            daisy[misc->current].x = indent + 3;
   } // if         
} // get_label_2

void fill_daisy_struct_2 (misc_t *misc, my_attribute_t *my_attribute,
                   daisy_t *daisy)
{
// function for daisy 2.02
   int indent = 0;
   xmlTextReaderPtr ncc;
   htmlDocPtr doc;

   if (! (doc = htmlParseFile (misc->ncc_html, "UTF-8")))
      failure (misc, "fill_daisy_struct_2 ()", errno);
   if (! (ncc = xmlReaderWalker (doc)))
   {
      int e;

      e = errno;
      snprintf (misc->str, MAX_STR, gettext ("Cannot read %s"), misc->ncc_html);
      failure (misc, misc->str, e);
   } // if

   for (misc->current = 0; misc->current < misc->total_items;
        misc->current++)
   {
      *daisy[misc->current].label = 0;
      free (daisy[misc->current].clips_file);
      daisy[misc->current].clips_file = strdup ("");
      daisy[misc->current].page_number = 0;
#ifdef DAISY_PLAYER
      *daisy[misc->current].class = 0;
#endif
   } // for
   misc->current = misc->displaying = misc->depth = 0;
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, ncc))
         break;
      if (strcasecmp (my_attribute->class, "page-normal") == 0)
      {
         do
         {
            if (! get_tag_or_label (misc, my_attribute, ncc))
               break;
         } while (*misc->label == 0);
         daisy[misc->current].page_number = atoi (misc->label);
      } // if (strcasecmp (my_attribute->class, "page-normal")
      if (strcasecmp (misc->tag, "h1") == 0 ||
          strcasecmp (misc->tag, "h2") == 0 ||
          strcasecmp (misc->tag, "h3") == 0 ||
          strcasecmp (misc->tag, "h4") == 0 ||
          strcasecmp (misc->tag, "h5") == 0 ||
          strcasecmp (misc->tag, "h6") == 0)
      {
         daisy[misc->current].level = misc->tag[1] - '0';
         if (daisy[misc->current].level < 1)
            daisy[misc->current].level = 1;
         if (daisy[misc->current].level > misc->depth)
            misc->depth = daisy[misc->current].level;
         daisy[misc->current].x = daisy[misc->current].level + 3 - 1;
         indent = daisy[misc->current].x =
                     (daisy[misc->current].level - 1) * 3 + 1;
         do
         {
            if (! get_tag_or_label (misc, my_attribute, ncc))
               break;
         } while (strcasecmp (misc->tag, "a") != 0);
         free (daisy[misc->current].clips_anchor);
         daisy[misc->current].clips_anchor = strdup ("");
         if (strchr (my_attribute->href, '#'))
         {
            free (daisy[misc->current].clips_anchor);
            daisy[misc->current].clips_anchor = strdup
                   (strchr (my_attribute->href, '#') + 1);
            *strchr (my_attribute->href, '#') = 0;
         } // if
         daisy[misc->current].clips_file = realloc 
                  (daisy[misc->current].clips_file,
                   strlen (misc->daisy_mp) + strlen (my_attribute->href) + 5);
         get_path_name (misc->daisy_mp, convert_URL_name (misc,
                        my_attribute->href), daisy[misc->current].clips_file);
         do
         {
            if (! get_tag_or_label (misc, my_attribute, ncc))
               break;
         } while (*misc->label == 0);
         get_label_2 (misc, daisy, indent);
         misc->current++;
         daisy[misc->current].page_number =
                    daisy[misc->current - 1].page_number;
      } // if (strcasecmp (misc->tag, "h1") == 0 || ...
   } // while
   misc->displaying = misc->current;
   parse_smil_2 (misc, my_attribute, daisy);
   xmlTextReaderClose (ncc);
   xmlFreeDoc (doc);
} // fill_daisy_struct_2
