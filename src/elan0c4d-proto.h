/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * ELAN 04f3:0c4d (bcdDevice 0x2410): protocol constants, non-SGX route.
 * Sequences follow the reference in Python (elan0c4d.py) and the notes of
 * the USB analysis. RUNIAP (42 01) and WDTRST (40 27) are NOT defined here
 * on purpose: elan0c4d-guard.c refuses them if anything ever builds them.
 */
#pragma once

#include <glib.h>

#define ELAN_VID 0x04f3
#define ELAN_PID 0x0c4d
#define ELAN_RELEASE 0x2410

#define ELAN_EP_OUT 0x01
#define ELAN_EP_IMG 0x82
#define ELAN_EP_INFO 0x83

#define ELAN_HEIGHT 150
#define ELAN_WIDTH 52
#define ELAN_PIXELS (ELAN_HEIGHT * ELAN_WIDTH)
#define ELAN_RAW_SIZE (ELAN_PIXELS * 2)
#define ELAN_RESP_MAX 256

#define ELAN_CODE_FINGER_OFF 0xAA
#define ELAN_CODE_FINGER_A 0xAF
#define ELAN_CODE_FINGER_B 0x55
#define ELAN_CODE_KEY_LOST 0xCC
#define ELAN_CALIBRATION_READY 0x03

#define ELAN_WRITE_TIMEOUT_MS 4000
#define ELAN_QUERY_TIMEOUT_MS 2000
#define ELAN_IMAGE_TIMEOUT_MS 4000
#define ELAN_IMAGE_MAX_READS 20
#define ELAN_CALIBRATION_POLLS 50
#define ELAN_CALIBRATION_POLL_MS 10
#define ELAN_RELEASE_WAIT_MS 1500
#define ELAN_RESEND_MS 50
#define ELAN_CAPTURE_TIMEOUT_MS 0      /* 0: wait until fprintd cancels (libfprint contract) */
#define ELAN_FINGER_WINDOW_MS 10000    /* one 40 3f slice when there is no deadline */
#define ELAN_ATTEMPTS 3
#define ELAN_BACKOFF_MS 500

static const guint8 elan_cmd_session[] = { 0x99, 0x60, 0x01 };
static const guint8 elan_cmd_cancel[] = { 0x00, 0x0b };
static const guint8 elan_cmd_precapture[] = { 0x9a, 0x10 };
static const guint8 elan_cmd_precapture_del[] = { 0x9a, 0x14 };
static const guint8 elan_cmd_height[] = { 0x40, 0x21, 0x08 };
static const guint8 elan_cmd_width[] = { 0x40, 0x21, 0x09 };
static const guint8 elan_cmd_fw_version[] = { 0x40, 0x19 };
static const guint8 elan_cmd_sample_version[] = { 0x40, 0x2a };
static const guint8 elan_cmd_sso_on[] = { 0x46, 0x07, 0x80, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00 };
static const guint8 elan_cmd_recognize[] = { 0x46, 0x07, 0x90, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00 };
static const guint8 elan_cmd_session_status[] = { 0x99, 0x68 };
static const guint8 elan_cmd_calib_mean[] = { 0x40, 0x24 };
static const guint8 elan_cmd_calib_start[] = { 0x40, 0x23 };
static const guint8 elan_cmd_wait_release[] = { 0x40, 0x3e };
static const guint8 elan_cmd_wait_press[] = { 0x40, 0x3f };
static const guint8 elan_cmd_gpio_low[] = { 0x40, 0x31 };
static const guint8 elan_cmd_image[] = { 0x00, 0x09 };
