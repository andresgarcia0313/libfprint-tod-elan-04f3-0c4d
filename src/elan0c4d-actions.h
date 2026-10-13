/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Enroll, verify and identify on top of the capture and the comparator interface. */
#pragma once

#include "elan0c4d.h"
#include "elan0c4d-match.h"

/* Store a template in a print / read it back (NULL if the print holds none). */
void    elan_print_store (FpPrint *print, GBytes *tmpl);
GBytes *elan_print_load (FpPrint *print);
