/* daisy3.c - functions to insert daisy3 info into a struct.
 *
 * Copyright (C) 2019 J. Lemmens
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

int get_page_number_3 (misc_t *misc, my_attribute_t *my_attribute)
{
// function for daisy 3
   while (1)
   {
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
         return 0;
      if (strcasecmp (misc->tag, "text") == 0) // is this necessary?
      {
         char *file, *anchor;
         xmlTextReaderPtr page;
         htmlDocPtr doc;

         anchor = strdup ("");
         if (strchr (my_attribute->src, '#'))
         {
            anchor = strdup (strchr (my_attribute->src, '#') + 1);
            *strchr (my_attribute->src, '#') = 0;
         } // if
         file = malloc (strlen (misc->daisy_mp) +
                        strlen (my_attribute->src) + 5);
         get_realpath_name (misc->daisy_mp,
                        convert_URL_name (misc, my_attribute->src), file);
         doc = htmlParseFile (file, "UTF-8");
         if (! (page = xmlReaderWalker (doc)))
         {
            misc->ncx_failed = 1;
            return EXIT_FAILURE;
         } // if
         if (*anchor)
         {
            do
            {
               if (! get_tag_or_label (misc, my_attribute, page))
               {
                  xmlTextReaderClose (page);
                  xmlFreeDoc (doc);
                  free (file);
                  free (anchor);
                  return 0;
               } // if
            } while (strcasecmp (my_attribute->id, anchor) != 0);
         } // if anchor
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, page))
            {
               xmlTextReaderClose (page);
               xmlFreeDoc (doc);
               free (file);
               free (anchor);
               return 0;
            } // if
            if (*misc->label)
            {
               xmlTextReaderClose (page);
               xmlFreeDoc (doc);
               misc->current_page_number = atoi (misc->label);
               free (file);
               free (anchor);
               return 1;
            } // if
         } // while
      } // if text
      if (strcasecmp (misc->tag, "pagenum") == 0)
      {
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, misc->reader))
               return 0;
            if (*misc->label)
            {
               misc->current_page_number = atoi (misc->label);
               return 1;
            } // if
         } // while
      } // if pagenum

      if (*misc->label)
      {
         misc->current_page_number = atoi (misc->label);
         return 1;
      } // if
      if (! get_tag_or_label (misc, my_attribute, misc->reader))
      {
         return 0;
      } // if
   } // while
} // get_page_number_3

void fill_page_numbers (misc_t *misc, daisy_t *daisy,
                        my_attribute_t *my_attribute)
{
   htmlDocPtr doc;
   xmlTextReaderPtr page;

   if (misc->verbose)
   {
      printf ("\n\n\rGet page numbers ");
      fflush (stdout);
   } // if
   for (misc->current = 0; misc->current < misc->total_items;
        misc->current++)
   {
      daisy[misc->current].page_number = 0;
      if (! (doc = htmlParseFile (daisy[misc->current].xml_file, "UTF-8")))
         return;
      if (! (page = xmlReaderWalker (doc)))
         failure (misc, daisy[misc->current].xml_file, errno);

      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, page))
            break;
         if (strcasecmp (misc->tag, "pagenum") == 0 ||
             strcasecmp (my_attribute->class, "pagenum") == 0)
         {
            parse_page_number (misc, my_attribute, page);
            daisy[misc->current].page_number = misc->current_page_number;
            if (misc->verbose)
            {
               printf (". ");
               fflush (stdout);
            } // if
         } // if
         if (*my_attribute->id)
         {
            if (strcasecmp
                    (my_attribute->id, daisy[misc->current].xml_anchor) == 0)
            {
               break;
            } // if
         } // if
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
      xmlTextReaderClose (page);
      xmlFreeDoc (doc);
      if (misc->verbose)
      {
         printf (". ");
         fflush (stdout);
      } // if
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
         anchor = strdup ("");
         if (strchr (my_attribute->src, '#'))
         {
            anchor = strdup (strchr (my_attribute->src, '#') + 1);
            *strchr (my_attribute->src, '#') = 0;
         } // if
         get_realpath_name (misc->daisy_mp, my_attribute->src, src);
         doc = htmlParseFile (src, "UTF-8");
         if (! (text = xmlReaderWalker (doc)))
         {
            free (anchor);
            free (src);
            failure (misc, src, errno);
         } // if

         if (*anchor)
         {
            do
            {
               if (! get_tag_or_label (misc, my_attribute, text))
               {
                  xmlTextReaderClose (text);
                  xmlFreeDoc (doc);
                  free (anchor);
                  free (src);
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
               free (anchor);
               free (src);
               return;
            } // if
         } while (! *misc->label);
         xmlTextReaderClose (text);
         xmlFreeDoc (doc);
         misc->current_page_number = atoi (misc->label);
         free (anchor);
         free (src);
         return;
      } // if
   } // while
} // parse_page_number

void read_daisy_3 (misc_t *misc, my_attribute_t *my_attribute,
                   daisy_t *daisy)
{
// when OPF or NCX is forced
   if (misc->use_OPF)
      parse_opf (misc, my_attribute, daisy);
   else
   if (misc->use_NCX)
      parse_ncx (misc, my_attribute, daisy);
   else
   {
      if (misc->items_in_opf > misc->items_in_ncx)
         parse_opf (misc, my_attribute, daisy);
      else
         parse_ncx (misc, my_attribute, daisy);
   } // if
} // read_daisy_3
