/*
 * pactl-set-cmd - test the pactl program
 *
 * (C)2018 - Jos Lemmens
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *pactl (char *, char *, char *);

char sink_info[10][100];

int main (int argc, char *argv[])
{
   int x;
   char sink_info[10][100];

   if (argc != 4)
   {
      printf ("Usage: %s set-sink-volume | set-sink-mute | list\n \
       <pulseaudio_device> <volume>\n", *argv);
      return EXIT_FAILURE;
   } // if
   memcpy (sink_info, pactl (argv[1], argv[2], argv[3]), 1000);
   for (x = 0; *sink_info[x]; x++)
      printf ("%d %s\n", x, sink_info[x]);
   return EXIT_SUCCESS;
} // main
