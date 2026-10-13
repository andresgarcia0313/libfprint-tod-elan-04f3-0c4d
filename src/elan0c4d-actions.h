/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Enroll (elan0c4d-enroll.c), verify and identify on top of the capture and the comparator interface. */
#pragma once

#include "elan0c4d.h"
#include "elan0c4d-match.h"

/* Store the stages and the template in a print / read the template back (NULL and error if invalid). */
void    elan_print_store (FpPrint *print, const ElanEnrollment *enrollment, GBytes *tmpl);
GBytes *elan_print_load (FpPrint *print, GError **error);
