
#include "freddi-config.h"
#include "freddi-window.h"

struct _FreddiWindow
{
  GtkApplicationWindow  parent_instance;

  /* Template widgets */
  GtkHeaderBar        *header_bar;
  GtkTextView         *main_text_view;
};

G_DEFINE_TYPE (FreddiWindow, freddi_window, GTK_TYPE_APPLICATION_WINDOW)

static void
freddi_window_class_init (FreddiWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/example/Freddi/freddi-window.ui");
  gtk_widget_class_bind_template_child (widget_class, FreddiWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, FreddiWindow, main_text_view);
}

static void
freddi_window_init (FreddiWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


int countChar(char* str, char c) {
  char* nextChar = strchr(str, c);
  int count = 0;

  while (nextChar) {
    count++;
    nextChar = strchr(nextChar + 1, c);
  }

  return count;
}

char** lineator(char* origin) {
  char* str = (char*) malloc(strlen(origin) + 1);
  strcpy(str, origin);

  int count = countChar(origin, '\n');
  char** lines = (char**) malloc(sizeof(char *) * count);

  char* nextLine = strchr(str, '\n');
  char* currentLine = str;

  int i = 0;

  while (nextLine) {
    *nextLine = '\0';

    lines[i] = malloc(strlen(currentLine) + 1);
    strcpy(lines[i], currentLine);

    currentLine = nextLine + 1;
    nextLine = strchr(currentLine, '\n');

    i++;
  }

  free(str);
  return lines;
}

void
open_file_complete (GObject                *source_object,
                    GAsyncResult           *result,
                    FreddiWindow           *self)
{
  GFile *file = G_FILE (source_object);

  g_autofree char *contents = NULL;
  gsize length = 0;

  g_autoptr (GError) error = NULL;

  // Complete the asynchronous operation; this function will either
  // give you the contents of the file as a byte array, or will
  // set the error argument
  g_file_load_contents_finish (file,
                               result,
                               &contents,
                               &length,
                               NULL,
                               &error);

  // Query the display name for the file
  g_autofree char *display_name = NULL;
  g_autoptr (GFileInfo) info =
    g_file_query_info (file,
                       "standard::display-name",
                       G_FILE_QUERY_INFO_NONE,
                       NULL,
                       NULL);
  if (info != NULL)
    {
      display_name =
        g_strdup (g_file_info_get_attribute_string (info, "standard::display-name"));
    }
  else
    {
      display_name = g_file_get_basename (file);
    }

  // In case of error, print a warning to the standard error output
  if (error != NULL)
    {
      g_printerr ("Unable to open “%s”: %s\n",
                  g_file_peek_path (file),
                  error->message);
      return;
    }

  // Ensure that the file is encoded with UTF-8
  if (!g_utf8_validate (contents, length, NULL))
    {
      g_printerr ("Unable to load the contents of “%s”: "
                  "the file is not encoded with UTF-8\n",
                  g_file_peek_path (file));
      return;
    }

  // Retrieve the GtkTextBuffer instance that stores the
  // text displayed by the GtkTextView widget
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (self->main_text_view);

  // Set the text using the contents of the file
  struct fpRefStrings fprs;

  int count = countChar (contents, '\n');
  char** lines = lineator(contents);
  const char equals[2] = "=";

  for (int i = 0; i < count; i++) {
    char* token = strtok(lines[i], equals);

    if (token != NULL) {
      if(strcmp(token, "Name") == 0) fprs.name = strtok(NULL, equals);
      else if(strcmp(token, "Branch") == 0) fprs.branch = strtok(NULL, equals);
      else if(strcmp(token, "Title") == 0) fprs.title = strtok(NULL, equals);
      else if(strcmp(token, "IsRuntime") == 0) fprs.isRuntime = strtok(NULL, equals);
      else if(strcmp(token, "Url") == 0) fprs.url = strtok(NULL, equals);
      else if(strcmp(token, "SuggestRemoteName") == 0) fprs.suggestRemoteName = strtok(NULL, equals);
      else if(strcmp(token, "GPGKey") == 0) fprs.gpgKey = strtok(NULL, equals);
      else if(strcmp(token, "RuntimeRepo") == 0) fprs.runtimeRepo = strtok(NULL, equals);
    }
  }

  char details[strlen(contents) * 2];
  sprintf(details, "Name: %s\nBranch: %s\nTitle: %s\nIsRuntime: %s\nUrl: %s\nSuggestRemoteName: %s\nGPGKey: %s\nRuntimeRepo: %s\n",
         fprs.name,
         fprs.branch,
         fprs.title,
         fprs.isRuntime,
         fprs.url,
         fprs.suggestRemoteName,
         fprs.gpgKey,
         fprs.runtimeRepo);

  puts(details);

  // TODO  Remove the text view and put each relevant field into a nice pair of labels.
  gtk_text_buffer_set_text (buffer, details, strlen(details));

  // Reposition the cursor so it's at the start of the text
  GtkTextIter start;
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_place_cursor (buffer, &start);

  // Set the title using the display name
  gtk_window_set_title (GTK_WINDOW (self), display_name);

  /* Ask the window manager/compositor to present the window. */
  gtk_window_present (self);
}

