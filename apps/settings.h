/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/*
 * Copyright (C) 2021-2022, Kevin Routley
 *
 * The "settings" tab: GUI definition, callback functions, and inter-thread communication.
 */

#ifndef LIBCAMFLTK_SETTINGS_H
#define LIBCAMFLTK_SETTINGS_H

#include <FL/Fl_Widget.H>

// Camera settings : declaration
extern bool _hflip;
extern bool _vflip;
extern double _bright;
extern double _saturate;
extern double _sharp;
extern double _contrast;
extern double _evComp;

// Inter-process communication
extern bool stateChange;

void onReset(Fl_Widget *, void *d);
void savePreviewLocation();
void getPreviewData();

#endif //LIBCAMFLTK_SETTINGS_H
