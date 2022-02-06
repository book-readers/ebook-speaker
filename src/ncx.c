/* daisy3.c - functions to insert daisy3 info into a struct. (ncx)
 *
 * Copyright (C) 2020 J. Lemmens
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

void fill_xml_anchor_ncx (misc_t *misc, my_attribute_t *my_attribute,
                          daisy_t *daisy)
{
// first of all fill daisy struct smil/xml_file and anchor
   htmlDocPtr doc;
   xmlTextReaderPtr ncx;

   misc->total_items = misc->items_in_ncx;
   if (! (doc = htmlParseFile (misc->ncx_name, "UTF-8")))
   {
      misc->ncx_failed = 1;
      return;
   } // if
   if (! (ncx = xmlReaderWalker (doc)))
   {
      misc->ncx_failed = 1;
      return;
   } // if

   misc->depth = 0;
   misc->current = 0;
   if (misc->verbose)
   {
      printf ("\n\rParsing SMIL files\n");
      fflush (stdout);
   } // if
   while (1)
   {
      free (daisy[misc->current].xml_file);
      daisy[misc->current].xml_file = strdup ("");
      free (daisy[misc->current].xml_anchor);
      daisy[misc->current].xml_anchor = strdup   ("");
      daisy[misc->current].x = 0;
      *daisy[misc->current].label = 0;
      daisy[misc->current].page_number = 0;
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
         if (misc->verbose)
         {
            printf ("\n\rTitle: %s\r\n", misc->daisy_title);
            fflush (stdout);
         } // if
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

               if (misc->verbose)
               {
                  printf ("\rContent ");
                  fflush (stdout);
               } // if
               src = strdup (my_attribute->src);
               daisy[misc->current].xml_anchor = strdup ("");
               if (strchr (src, '#'))
               {
                  daisy[misc->current].xml_anchor =
                               strdup (strchr (src, '#') + 1);
                  *strchr (src, '#') = 0;
               } // if
               daisy[misc->current].xml_file = realloc (
                  daisy[misc->current].xml_file, strlen (misc->daisy_mp) +
                  strlen (src) + 5);
               *daisy[misc->current].xml_file = 0;
               get_real_pathname (misc->daisy_mp,
                      convert_URL_name (misc, src),
                      daisy[misc->current].xml_file);
               daisy[misc->current].orig_xml_file = strdup
                                       (daisy[misc->current].xml_file);
               doc = htmlParseFile (daisy[misc->current].xml_file, "UTF-8");
               if (! (content = xmlReaderWalker (doc)))
               {
                  misc->ncx_failed = 1;
                  return;
               } // if

               if (*daisy[misc->current].xml_anchor)
               {
                  while (1)
                  {
                     if (! get_tag_or_label (misc, my_attribute, content))
                        break;
                     if (*my_attribute->id)
                     {
                        if (strcasecmp (my_attribute->id,
                                    daisy[misc->current].xml_anchor) == 0)
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
                  if (*my_attribute->id)
                  {
                     if (! *daisy[misc->current].first_id)
                        strncpy (daisy[misc->current].first_id,
                                 my_attribute->id, MAX_STR);
                  } // if
                  if (strcasecmp (misc->tag, "text") == 0)
                  {
                     daisy[misc->current].xml_anchor = strdup ("");
                     if (strchr (my_attribute->src, '#'))
                     {
                        daisy[misc->current].xml_anchor = strdup
                            (strchr (my_attribute->src, '#') + 1);
                        *strchr (my_attribute->src, '#') = 0;
                     } // if
                     daisy[misc->current].xml_file = realloc
                        (daisy[misc->current].xml_file, strlen
                        (misc->daisy_mp) + strlen (my_attribute->src) + 5);
                     *daisy[misc->current].xml_file = 0;
                     get_real_pathname (misc->daisy_mp,
                                  convert_URL_name (misc, my_attribute->src),
                                  daisy[misc->current].xml_file);
                     daisy[misc->current].orig_xml_file =
                                  strdup (daisy[misc->current].xml_file);
                     if (misc->verbose)
                     {
                        printf (". ");
                        fflush (stdout);
                     } // if
                     break;
                  } // if
               } // while
               xmlTextReaderClose (content);
               if (misc->verbose)
               {
                  printf (". ");
                  fflush (stdout);
               } // if
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

void parse_content_ncx (misc_t *misc, my_attribute_t *my_attribute,
                        daisy_t *daisy)
{
   htmlDocPtr doc;
   xmlTextReaderPtr content;

   get_real_pathname (misc->tmp_dir,
                      convert_URL_name (misc, my_attribute->src),
                      daisy[misc->current].xml_file);
   if (! (doc = htmlParseFile (daisy[misc->current].xml_file, "UTF-8")))
   {
      misc->ncx_failed = 1;
      return;
   } // if
   if (! (content = xmlReaderWalker (doc)))
   {
      misc->ncx_failed = 1;
      return;
   } // if

   if (*daisy[misc->current].xml_anchor)
   {
// look for it
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, content))
            break;
         if (*my_attribute->id)
         {
            if (strcasecmp (my_attribute->id,
                            daisy[misc->current].xml_anchor) == 0)
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
      if (*my_attribute->id)
      {
         if (! *daisy[misc->current].first_id)
            strncpy (daisy[misc->current].first_id, my_attribute->id,
                     MAX_STR);
      } // if
      if (strcasecmp (misc->tag, "audio") == 0)
         break;
      if (strcasecmp (misc->tag, "text") == 0)
         break;
      if (misc->current + 1 < misc->total_items)
      {
         if (*daisy[misc->current + 1].xml_anchor)
         {
            if (strcasecmp (my_attribute->id,
                            daisy[misc->current + 1].xml_anchor) == 0)
            {
               break;
            } // if
         } // if
      } // if
   } // while
   xmlTextReaderClose (content);
} // parse_content_ncx

void parse_ncx (misc_t *misc, my_attribute_t *my_attribute,
                       daisy_t *daisy)
{
   htmlDocPtr doc;
   xmlTextReaderPtr ncx;
   int i;

   if (misc->items_in_ncx == 0)
      failure (misc, gettext ("No items found. Try option \"-O\"."), errno);

   misc->total_items = misc->items_in_ncx;
   if (misc->total_items == 0)
      misc->total_items = 1;
   for (i = 0; i < misc->items_in_ncx; i++)
      *daisy[i].first_id = 0;
   fill_xml_anchor_ncx (misc, my_attribute, daisy);
   if (! (doc = htmlParseFile (misc->ncx_name, "UTF-8")))
   {
      misc->ncx_failed = 1;
      return;
   } // if
   ncx = xmlReaderWalker (doc);
   misc->current = 0;
   misc->level = misc->depth = 0;
   daisy[misc->current].x = 0;
   if (misc->verbose)
   {
      printf ("\n");
      fflush (stdout);
   } // if
   while (1)
   {
      if (misc->current >= misc->total_items)
         break;
      if (! get_tag_or_label (misc, my_attribute, ncx))
         break;
      if (! *daisy[misc->current].first_id)
      {
         if (*my_attribute->id)
            strncpy (daisy[misc->current].first_id, my_attribute->id,
                     MAX_STR);
      } // if
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
         strncpy (daisy[misc->current].my_class, my_attribute->my_class,
                  MAX_STR - 1);
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
                     if (misc->verbose)
                     {
                        printf ("\n\r%d %s ", misc->current + 1,
                                            daisy[misc->current].label);
                        fflush (stdout);
                     } // if
                     break;
                  } // if
               } // while
            } //  if (strcasecmp (misc->tag, "text") == 0)
            if (strcasecmp (misc->tag, "content") == 0)
            {
               parse_content_ncx (misc, my_attribute, daisy);
               daisy[misc->current].level = misc->level;
               daisy[misc->current].x = daisy[misc->current].level * 3 - 1;
               daisy[misc->current].screen = misc->current / misc->max_y;
               daisy[misc->current].y =
                  misc->current - (daisy[misc->current].screen * misc->max_y);
               misc->current++;
               break;
            } // if (strcasecmp (misc->tag, "content") == 0)
         } // while
      } // if (strcasecmp (misc->tag, "navpoint") == 0)
      if (strcasecmp (misc->tag, "/navpoint") == 0)
         misc->level--;
   } // while
   if (misc->verbose)
      printf ("\n\n");
   xmlTextReaderClose (ncx);
   misc->total_items = misc->current;

   for (i = 0; i < misc->items_in_ncx; i++)
   {
      *daisy[i].class = 0;
      if (misc->verbose)
      {
         printf ("\r\norig %d %s ", i, daisy[i].orig_xml_file);
         fflush (stdout);
      } // if
   } // for
} // parse_ncx
