
#pragma once

#include <gtk/gtk.h>
#include <stdbool.h>

G_BEGIN_DECLS

#define FREDDI_TYPE_WINDOW (freddi_window_get_type())

G_DECLARE_FINAL_TYPE (FreddiWindow, freddi_window, FREDDI, WINDOW, GtkApplicationWindow)

void
open_file_complete (GObject                *source_object,
                    GAsyncResult           *result,
                    FreddiWindow *self);

struct fpRefStrings {
  char* name;
  char* branch;
  char* title;
  char* isRuntime;
  char* url;
  char* suggestRemoteName;
  char* gpgKey;
  char* runtimeRepo;
  bool app;
  char* type;
};

G_END_DECLS
