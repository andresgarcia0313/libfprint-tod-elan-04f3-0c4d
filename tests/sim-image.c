/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Synthetic images for 00 09 (EP 0x82). */
#include "sim-internal.h"
#include "synth.h"

static guint8 raw[ELAN_RAW_SIZE];

/* The frame is made once per 00 09 and served in chunks, so short reads can be tested. */
void
sim_serve_image (FpiUsbTransfer *t)
{
  guint16 px[ELAN_PIXELS];
  gsize off = ELAN_RAW_SIZE - t->length;
  gsize n = MIN ((gsize) t->length, sim->image_chunk ? sim->image_chunk : G_MAXSIZE);
  SimImage kind = sim->images[MIN (sim->next_image, sim->n_images - 1)];

  if (off == 0)
    {
      if (kind == SIM_FINGER)
        synth_ridges (px, ELAN_HEIGHT / 10);
      else if (kind == SIM_EMPTY)
        synth_empty (px);
      else
        synth_flat (px, kind == SIM_ZEROS ? 0 : 0x3FFE);
      for (guint i = 0; i < ELAN_PIXELS; i++)
        {
          raw[2 * i] = px[i] & 0xFF;
          raw[2 * i + 1] = px[i] >> 8;
        }
    }
  memcpy (t->buffer, raw + off, n);
  t->actual_length = n;
  if (off + n >= ELAN_RAW_SIZE)
    sim->next_image++;
}
