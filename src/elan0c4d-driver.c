/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * TOD module for the ELAN 04f3:0c4d reader, bcdDevice 0x2410 only. It is an
 * FpDevice (not an FpImageDevice): the sensor is 150x52 and the bozorth
 * matcher of libfprint does not work on it.
 */
#include <gmodule.h>
#include <gusb.h>
#include "elan0c4d-actions.h"
#include "elan0c4d-guard.h"
#include "elan0c4d-ops.h"

G_DEFINE_TYPE (FpiDeviceElan0c4d, fpi_device_elan0c4d, FP_TYPE_DEVICE)

static const FpIdEntry id_table[] = {
  { .vid = ELAN_VID, .pid = ELAN_PID },
  { .vid = 0, .pid = 0 },
};

static gint
elan_usb_discover (GUsbDevice *usb_device)
{
  guint16 release = g_usb_device_get_release (usb_device);
  gint score = elan_release_score (release);

  g_debug ("04f3:0c4d revision 0x%04x: score %d", release, score);
  return score;
}

static void
fpi_device_elan0c4d_init (FpiDeviceElan0c4d *self)
{
  self->capture_timeout_ms = ELAN_CAPTURE_TIMEOUT_MS;
}

static void
fpi_device_elan0c4d_class_init (FpiDeviceElan0c4dClass *klass)
{
  FpDeviceClass *cls = FP_DEVICE_CLASS (klass);

  cls->id = "elan0c4d-tod";
  cls->full_name = "ELAN 04f3:0c4d (rev 0x2410)";
  cls->type = FP_DEVICE_TYPE_USB;
  cls->id_table = id_table;
  cls->scan_type = FP_SCAN_TYPE_PRESS;
  cls->nr_enroll_stages = 1;
  cls->temp_hot_seconds = -1;

  cls->usb_discover = elan_usb_discover;
  cls->open = elan_open;
  cls->close = elan_close;
  cls->enroll = elan_action_enroll;
  cls->verify = elan_action_verify;
  cls->identify = elan_action_identify;
  fpi_device_class_auto_initialize_features (cls);
}

G_MODULE_EXPORT GType
fpi_tod_shared_driver_get_type (void)
{
  return fpi_device_elan0c4d_get_type ();
}
