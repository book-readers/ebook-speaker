/* daisy2.02.c - functions to insert daisy2.02 info into a struct.
 *
 * Copyright (C)2011-2020 J. Lemmens
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
   char *src, *anchor;
   htmlDocPtr doc;
   xmlTextReaderPtr page;

   if (daisy[misc->playing].page_number == 0)
      return 0;

   src = malloc (strlen (misc->daisy_mp) + strlen (attr) + 3);
   anchor = strdup ("");
   if (strchr (attr, '#'))
   {
      free (anchor);
      anchor = strdup (strchr (attr, '#') + 1);
      *strchr (attr, '#') = 0;
   } // if
   get_real_pathname (misc->daisy_mp, attr, src);
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
         free (anchor);
         free (src);
         return 0;
      } // if
      if (*misc->label && atoi (misc->label) != 0)
      {
         xmlTextReaderClose (page);
         misc->current_page_number = atoi (misc->label);
         free (anchor);
         free (src);
         return 1;
      } // if
   } // while

   while (1)
   {
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
   attr = attr; // don't need it in eBook-speaker
} // get_page_number_2

void parse_smil_2 (misc_t *misc, my_attribute_t *my_attribute, daisy_t *daisy)
{
// function for daisy 2.02
   htmlDocPtr doc;
   xmlTextReaderPtr parse;

   if (misc->verbose)
   {
      printf ("\r\nparse SMIL files ");
      fflush (stdout);
   } // if
   misc->current = 0;
   while (1)
   {
      if (misc->current == misc->total_items)
         break;
      if (! daisy[misc->current].smil_file)
         continue;
      if (*daisy[misc->current].smil_file == 0)
         continue;
      if (! (doc = htmlParseFile (daisy[misc->current].smil_file, "UTF-8")))
         failure (misc, daisy[misc->current].smil_file, errno);
      if (! (parse = xmlReaderWalker (doc)))
      {
         int e;
         char str[MAX_STR];

         e = errno;
         snprintf (str, MAX_STR,
                gettext ("Cannot read %s"), daisy[misc->current].smil_file);
         failure (misc, str, e);
      } // if

// parse this smil
      if (*daisy[misc->current].smil_anchor)
      {
         while (1)
         {
            if (! get_tag_or_label (misc, my_attribute, parse))
               break;
            if (strcasecmp (my_attribute->id,
                            daisy[misc->current].smil_anchor) == 0)
            {
               free (daisy[misc->current].xml_anchor);
               daisy[misc->current].xml_anchor = strdup ("");
               if (strchr (my_attribute->src, '#'))
               {
                  free (daisy[misc->current].xml_anchor);
                  daisy[misc->current].xml_anchor = strdup
                         (strchr (my_attribute->src, '#') + 1);
                  *strchr (my_attribute->src, '#') = 0;
               } // if
               daisy[misc->current].xml_file = realloc
                   (daisy[misc->current].xml_file,
                    strlen (misc->daisy_mp) + strlen (my_attribute->src) + 5);
               get_real_pathname (misc->daisy_mp,
                                  convert_URL_name (misc, my_attribute->src),
                                  daisy[misc->current].xml_file);
               daisy[misc->current].orig_xml_file = strdup
                                             (daisy[misc->current].xml_file);
               break;
            } // if
         } // while
      } // if smil_anchor
      *daisy[misc->current].first_id =  0;
      while (1)
      {
         if (! get_tag_or_label (misc, my_attribute, parse))
            break;
         if (strcasecmp (misc->tag, "text") == 0)
         {
            free (daisy[misc->current].xml_anchor);
            daisy[misc->current].xml_anchor = strdup ("");
            if (strchr (my_attribute->src, '#'))
            {
               free (daisy[misc->current].xml_anchor);
               daisy[misc->current].xml_anchor =
                    strdup (strchr (my_attribute->src, '#') + 1);
               *strchr (my_attribute->src, '#') = 0;
            } // if
            daisy[misc->current].xml_file = realloc
                   (daisy[misc->current].xml_file,
                    strlen (misc->daisy_mp) + strlen (my_attribute->src) + 5);
            get_real_pathname (misc->daisy_mp, convert_URL_name (misc,
                           my_attribute->src), daisy[misc->current].xml_file);
            daisy[misc->current].orig_xml_file = strdup
                                             (daisy[misc->current].xml_file);
            if (misc->current + 1 < misc->total_items &&
                *daisy[misc->current + 1].smil_anchor &&
                strcasecmp (my_attribute->id,
                            daisy[misc->current + 1].smil_anchor) == 0)
            {
               break;
            } // if
         } // if
      } // while
      xmlTextReaderClose (parse);
      misc->current++;
      if (misc->current >= misc->total_items)
         return;
   } // while
} // parse_smil_2

void get_label_2 (misc_t *misc, daisy_t *daisy, int indent, int i)
{
   strncpy (daisy[i].label, misc->label, 80);
   daisy[i].label[80] = 0;
   if (misc->verbose)
      printf ("\r\n%d %s", i + 1, daisy[i].label);
   if (misc->current == misc->max_y)
      misc->current = 1;
   if (*daisy[i].class)
   {
      if (strcasecmp (daisy[i].class, "pagenum") == 0)
         daisy[i].x = 0;
      else
         if (daisy[i].x == 0)
            daisy[i].x = indent + 3;
   } // if
} // get_label_2

void fill_daisy_struct_2 (misc_t *misc, my_attribute_t *my_attribute,
                 daisy_t *daisy)
{
   int indent, i;
   xmlTextReaderPtr ncc;
   htmlDocPtr doc;

   if (! (doc = htmlParseFile (misc->ncc_html, "UTF-8")))
      failure (misc, "fill_daisy_struct_2 ()", errno);
   if (! (ncc = xmlReaderWalker (doc)))
   {
      int e;

      e = errno;
      snprintf (misc->str, MAX_STR, gettext ("Cannot read %s"), misc->ncc_html);      failure (misc, misc->str, e);
   } // if

   i = misc->current = misc->depth = 0;
   indent = 0;
   if (misc->verbose)
      printf ("\n");
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
         daisy[i].page_number = atoi (misc->label);
      } // if (strcasecmp (my_attribute->class, "page-normal")

      if (strcasecmp (misc->tag, "h1") == 0 ||
          strcasecmp (misc->tag, "h2") == 0 ||
          strcasecmp (misc->tag, "h3") == 0 ||
          strcasecmp (misc->tag, "h4") == 0 ||
          strcasecmp (misc->tag, "h5") == 0 ||
          strcasecmp (misc->tag, "h6") == 0)
      {
         daisy[i].level = misc->tag[1] - '0';
         if (daisy[i].level < 1)
            daisy[i].level = 1;
         if (daisy[i].level > misc->depth)
            misc->depth = daisy[i].level;
         daisy[i].x = daisy[i].level + 3 - 1;
         indent = daisy[i].x =
                     (daisy[i].level - 1) * 3 + 1;
         do
         {
            if (! get_tag_or_label (misc, my_attribute, ncc))
               break;
         } while (strcasecmp (misc->tag, "a") != 0);
         free (daisy[i].smil_anchor);
         daisy[i].smil_anchor = strdup ("");
         if (strchr (my_attribute->href, '#'))
         {
            free (daisy[i].smil_anchor);
            daisy[i].smil_anchor = strdup
                   (strchr (my_attribute->href, '#') + 1);
            *strchr (my_attribute->href, '#') = 0;
         } // if
         daisy[i].smil_file = realloc
                  (daisy[i].smil_file,
                   strlen (misc->daisy_mp) + strlen (my_attribute->href) + 5);
         get_real_pathname (misc->daisy_mp, convert_URL_name (misc,
                        my_attribute->href), daisy[i].smil_file);
         do
         {
            if (! get_tag_or_label (misc, my_attribute, ncc))
               break;
         } while (*misc->label == 0);
         get_label_2 (misc, daisy, indent, i);
         if (misc->verbose)
         {
            if (daisy[i].page_number != 0)
               printf (" (%d) ", daisy[i].page_number);
         } // if
         i++;
         daisy[i].page_number =
                    daisy[i - 1].page_number;
      } // if (strcasecmp (misc->tag, "h1") == 0 || ...
   } // while
   misc->current = i;
   parse_smil_2 (misc, my_attribute, daisy);
   xmlTextReaderClose (ncc);
} // fill_daisy_struct_2
