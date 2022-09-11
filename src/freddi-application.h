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

#include <adwaita.h>

G_BEGIN_DECLS

#define FREDDI_TYPE_APPLICATION (freddi_application_get_type())

G_DECLARE_FINAL_TYPE (FreddiApplication, freddi_application, FREDDI, APPLICATION, AdwApplication)

FreddiApplication *freddi_application_new (gchar *application_id,
                                           GApplicationFlags  flags);

void app_open_file (GApplication  *application,
                    GFile        **files,
                    gint           n_files,
                    const gchar   *hint);

G_END_DECLS
