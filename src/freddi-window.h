/*
Copyright (C) 2022 Yitzchak Schwarz

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2
of the License only.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#pragma once

#include <gtk/gtk.h>
#include <stdbool.h>
#include <flatpak.h>

G_BEGIN_DECLS

#define FREDDI_TYPE_WINDOW (freddi_window_get_type())

G_DECLARE_FINAL_TYPE (FreddiWindow, freddi_window, FREDDI, WINDOW, GtkApplicationWindow)

void
open_file_complete (GObject                *source_object,
                    GAsyncResult           *result,
                    FreddiWindow *self);

struct _appData {
	// Flatpak data:
  char* id;
  char* branch;
  char* title;
  char* isRuntime;
  char* url;
  char* suggestRemoteName;
  char* gpgKey;
  char* runtimeRepo;
  bool app;
  char* type;

	// AppStream data:
};

struct _flatpak {
	FlatpakInstallation * installation;
	FlatpakTransaction * transaction;
	FlatpakRef * ref;
	FlatpakRemoteRef * remote_ref;
	FlatpakInstalledRef * installed_ref;
};

// Installing the app:
static void freddi_window__install_app(GAction *action G_GNUC_UNUSED, GVariant *parameter G_GNUC_UNUSED, FreddiWindow *self);

G_END_DECLS