
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
