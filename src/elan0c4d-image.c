/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Raw image (elan0c4d.py image): 00 09, then 15600 bytes on EP 0x82 as 7800
 * little-endian 14-bit samples. A short read is resumed from the bytes
 * already received, up to 20 reads, like WbfUsbDriver.
 */
#include "elan0c4d-ops.h"

enum { IMG_REQUEST, IMG_READ, IMG_DONE, IMG_STATES };

typedef struct
{
  gsize got;
  guint reads;
} Image;

static const ElanCmd image_request = ELAN_WRITE ("image-request", elan_cmd_image);

static void
image_read_cb (FpiUsbTransfer *transfer, FpDevice *dev, gpointer data, GError *error)
{
  FpiSsm *ssm = transfer->ssm;
  Image *img = fpi_ssm_get_data (ssm);
  gsize n;

  if (error)
    {
      elan_log_error ("image-read", error);
      fpi_ssm_mark_failed (ssm, error);
      return;
    }
  n = MIN ((gsize) MAX (transfer->actual_length, 0), ELAN_RAW_SIZE - img->got);
  memcpy (FPI_DEVICE_ELAN0C4D (dev)->raw + img->got, transfer->buffer, n);
  img->got += n;
  img->reads++;
  g_debug ("image: %zu/%d bytes after %u reads", img->got, ELAN_RAW_SIZE, img->reads);
  if (img->got >= ELAN_RAW_SIZE)
    fpi_ssm_next_state (ssm);
  else if (img->reads >= ELAN_IMAGE_MAX_READS)
    fpi_ssm_mark_failed (ssm, g_error_new (ELAN_ERROR, ELAN_ERROR_PROTOCOL, "short image %zu/%d",
                                           img->got, ELAN_RAW_SIZE));
  else
    fpi_ssm_jump_to_state (ssm, IMG_READ);
}

static void
image_handler (FpiSsm *ssm, FpDevice *dev)
{
  FpiDeviceElan0c4d *self = FPI_DEVICE_ELAN0C4D (dev);
  Image *img = fpi_ssm_get_data (ssm);
  FpiUsbTransfer *transfer;

  switch (fpi_ssm_get_cur_state (ssm))
    {
    case IMG_REQUEST:
      memset (img, 0, sizeof *img);
      fpi_ssm_start_subsm (ssm, elan_cmd_ssm_new (dev, &image_request));
      break;

    case IMG_READ:
      transfer = fpi_usb_transfer_new (dev);
      transfer->ssm = ssm;
      transfer->short_is_error = FALSE;
      fpi_usb_transfer_fill_bulk (transfer, ELAN_EP_IMG, ELAN_RAW_SIZE - img->got);
      fpi_usb_transfer_submit (transfer, ELAN_IMAGE_TIMEOUT_MS, fpi_device_get_cancellable (dev),
                               image_read_cb, NULL);
      break;

    default:
      for (guint i = 0; i < ELAN_PIXELS; i++)
        self->frame[i] = (guint16) (self->raw[2 * i] | (self->raw[2 * i + 1] << 8));
      fpi_ssm_mark_completed (ssm);
    }
}

FpiSsm *
elan_image_ssm_new (FpDevice *dev, gconstpointer arg)
{
  FpiSsm *ssm = fpi_ssm_new_full (dev, image_handler, IMG_STATES, IMG_STATES, "elan-image");

  fpi_ssm_set_data (ssm, g_new0 (Image, 1), g_free);
  return ssm;
}
