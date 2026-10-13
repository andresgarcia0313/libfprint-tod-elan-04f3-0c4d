/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Replacements for libfprint / gusb symbols. They live in the test executable
 * and the driver objects are linked into it, so these definitions win over
 * the shared libraries: the driver runs unchanged against the simulation.
 */
#include <gusb.h>
#include "sim-internal.h"

SimSensor    *sim;
GCancellable *sim_cancellable;
guint8        sim_last[16];

typedef struct
{
  FpiUsbTransfer        *transfer;
  FpiUsbTransferCallback cb;
  gpointer               data;
  GError                *error;
} Done;

GCancellable *fpi_device_get_cancellable (FpDevice *device) { return sim_cancellable; }
GUsbDevice *fpi_device_get_usb_device (FpDevice *device) { return (GUsbDevice *) device; }

gboolean
g_usb_device_claim_interface (GUsbDevice *self, gint iface, GUsbDeviceClaimInterfaceFlags flags, GError **error)
{
  g_string_append (sim->wire, " [claim]");
  return TRUE;
}

gboolean
g_usb_device_release_interface (GUsbDevice *self, gint iface, GUsbDeviceClaimInterfaceFlags flags, GError **error)
{
  g_string_append (sim->wire, " [release]");
  return TRUE;
}

static gboolean
finish (gpointer p)
{
  Done *d = p;

  d->cb (d->transfer, d->transfer->device, d->data, d->error);
  fpi_usb_transfer_unref (d->transfer);
  g_free (d);
  return G_SOURCE_REMOVE;
}

static void
on_cancel (GCancellable *c, gpointer p)
{
  Done *d = p;

  g_set_error_literal (&d->error, G_IO_ERROR, G_IO_ERROR_CANCELLED, "cancelled");
  g_idle_add (finish, d);
}

static void
record_write (FpiUsbTransfer *t)
{
  g_autoptr(GString) hex = g_string_new (NULL);

  for (gssize i = 0; i < t->length; i++)
    g_string_append_printf (hex, "%02x", t->buffer[i]);
  g_string_append_printf (sim->wire, "%s%s", sim->wire->len ? " " : "", hex->str);
  memcpy (sim_last, t->buffer, MIN ((gsize) t->length, sizeof sim_last));
  t->actual_length = t->length;
}

void
fpi_usb_transfer_submit (FpiUsbTransfer *t, guint timeout_ms, GCancellable *c, FpiUsbTransferCallback cb,
                         gpointer data)
{
  Done *d = g_new0 (Done, 1);
  gboolean blocks = sim_last[0] == 0x40 && sim_last[1] == 0x3f &&
                    sim->press[MIN (sim->next_press, sim->n_press - 1)] == SIM_BLOCK;

  d->transfer = t;
  d->cb = cb;
  d->data = data;
  if (c && g_cancellable_is_cancelled (c))
    g_set_error_literal (&d->error, G_IO_ERROR, G_IO_ERROR_CANCELLED, "cancelled");
  else if (!(t->endpoint & 0x80))
    record_write (t);
  else if (blocks && t->endpoint == ELAN_EP_INFO)
    {
      g_cancellable_connect (c, G_CALLBACK (on_cancel), d, NULL);
      return;
    }
  else
    sim_answer (t, &d->error);
  g_idle_add (finish, d);
}
