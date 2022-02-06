/* playfile.c - plays the audio using the sox library.
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
 *
 * Thanks to Rob Sykes <aquegg@yahoo.co.uk> for this source.
 *
 */

#include "daisy.h"

typedef struct
{
   sox_format_t *file;
} priv_t;

static int getopts (sox_effect_t *effp, int argc, char **argv)
{
   priv_t *p = (priv_t *) effp->priv;
   if (argc != 2 || !(p->file = (sox_format_t *)argv[1]) ||
       p->file->mode != 'w')
      return SOX_EOF;
   return SOX_SUCCESS;
} // getopts

static int flow (sox_effect_t *effp, sox_sample_t const *ibuf,
                 sox_sample_t *obuf, size_t *isamp, size_t *osamp)
{
  priv_t * p = (priv_t *) effp->priv;
  size_t len = *isamp ? sox_write (p->file, ibuf, *isamp) : 0;

  if (len != *isamp)
  {
    lsx_fail("%s: %s", p->file->filename, p->file->sox_errstr);
    return SOX_EOF;
  } // if

  (void) obuf, *osamp = 0;
  return SOX_SUCCESS;
} // flow

static sox_effect_handler_t const *output_effect_fn (void)
{
  static sox_effect_handler_t handler =
  {
    "output", NULL, SOX_EFF_MCHAN, getopts, NULL, flow, NULL, NULL,
    NULL, sizeof (priv_t)
  };
  return &handler;
} // output_effect_fn

void playfile (char *in_file, char *in_type,
               char *out_file, char *out_type, char *tempo)
{
  sox_format_t *in, *out; /* input and output files */
  sox_effects_chain_t *chain;
  sox_effect_t *e;
  sox_signalinfo_t interm_signal;
  char *args[10];

  chain = malloc (1);
  sox_init();

  in = sox_open_read (in_file, NULL, NULL, in_type);
  if (strcasecmp (in_type, "cdda") == 0)
    in->encoding.reverse_bytes = 0;
  out = sox_open_write (out_file, &in->signal, NULL, out_type,
                             NULL, NULL);

  chain = sox_create_effects_chain(&in->encoding, &out->encoding);
  interm_signal = in->signal; /* NB: deep copy */

  e = sox_create_effect(sox_find_effect("input"));
  args[0] = (char *) in, sox_effect_options (e, 1, args);
  sox_add_effect (chain, e, &interm_signal, &in->signal);

  e = sox_create_effect(sox_find_effect("tempo"));
  if (strcasecmp (in_type, "cdda") == 0)
    args[0] = "-m", args[1] = tempo, sox_effect_options (e, 2, args);
  else
    args[0] = "-s", args[1] = tempo, sox_effect_options (e, 2, args);
  sox_add_effect (chain, e, &interm_signal, &in->signal);

  if (in->signal.rate != out->signal.rate)
  {
    e = sox_create_effect(sox_find_effect("rate"));
    sox_effect_options (e, 0, NULL);
    sox_add_effect (chain, e, &interm_signal, &out->signal);
  } // if

  if (in->signal.channels != out->signal.channels)
  {
    e = sox_create_effect(sox_find_effect("channels"));
    sox_effect_options (e, 0, NULL);
    sox_add_effect (chain, e, &interm_signal, &out->signal);
  } // if

  e = sox_create_effect(output_effect_fn());
  args[0] = (char *)out, sox_effect_options(e, 1, args);
  sox_add_effect(chain, e, &interm_signal, &out->signal);

  sox_flow_effects(chain, NULL, NULL);

  sox_delete_effects_chain(chain);
  sox_close(out);
  sox_close(in);
  sox_quit();
} // playfile
