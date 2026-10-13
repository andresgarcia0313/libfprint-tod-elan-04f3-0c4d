/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Scripted answers of the sensor (what the reference Python flow sees on the bus). */
#include "sim-internal.h"

#define AFTER(a, b) (sim_last[0] == (a) && sim_last[1] == (b))

static gssize
answer_info (guint8 *out)
{
  if (AFTER (0x9a, 0x10))
    return 4;
  if (AFTER (0x40, 0x2a))
    return 2;
  if (AFTER (0x40, 0x21))
    {
      out[0] = sim_last[2] == 8 ? 0x95 : 0x33;  /* height 150, width 52 */
      return 2;
    }
  if (AFTER (0x40, 0x19))
    {
      out[0] = 0x01;
      out[1] = 0x40;
      return 2;
    }
  if (AFTER (0x99, 0x68))
    {
      out[1] = 0x99;
      out[2] = 0x68;
      out[3] = 0x01;
      return 4;
    }
  if (AFTER (0x40, 0x24))
    {
      sim->polls = 0;
      out[0] = 1;
      return 2;
    }
  if (AFTER (0x40, 0x23))
    out[0] = ++sim->polls > sim->ready_after ? 0x03 : 0x00;
  else if (AFTER (0x40, 0x3e))
    out[0] = ELAN_CODE_FINGER_OFF;
  else if (AFTER (0x40, 0x3f))
    {
      out[0] = sim->press[MIN (sim->next_press, sim->n_press - 1)];
      sim->next_press++;
    }
  else
    return -1;
  return 1;
}

gboolean
sim_answer (FpiUsbTransfer *t, GError **error)
{
  gssize n;

  if (sim->fault_count && AFTER (sim->fault_cmd[0], sim->fault_cmd[1]))
    {
      sim->fault_count--;
      g_set_error_literal (error, sim->fault_domain, sim->fault_code, "simulated fault");
      return FALSE;
    }
  if (t->endpoint == ELAN_EP_IMG)
    {
      sim_serve_image (t);
      return TRUE;
    }
  n = answer_info (t->buffer);
  if (n < 0)
    {
      g_set_error (error, ELAN_ERROR, ELAN_ERROR_PROTOCOL, "sim: no answer after %02x%02x",
                   sim_last[0], sim_last[1]);
      return FALSE;
    }
  t->actual_length = n;
  return TRUE;
}
