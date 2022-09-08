#ifndef ARCH

#if defined  ( __amd64__) || defined  (__amd64) || defined  (__x86_64__) || defined  (__x86_64)
#define ARCH "x86_64"
#endif

#if defined  (i386) || defined  (__i386) || defined  (__i386__)
#define ARCH "i386"
#endif

#endif

#include <appstream/appstream.h>
#include <PackageKit/packagekit-glib2/packagekit.h>

#include "freddi-config.h"
#include "freddi-window.h"

#include <flatpak.h>

struct _FreddiWindow
{
	GtkApplicationWindow  parent_instance;

	/* Template widgets */
	GtkHeaderBar				*header_bar;
	GtkLabel						*app_name;
	GtkLabel						*app_developer;
	GtkLabel						*app_summary;
	GtkLabel						*app_description;
	GtkLabel						*app_id;
	GtkLabel						*app_license;
	GtkLabel						*app_origin;
	GtkLabel						*app_branch;
};

G_DEFINE_TYPE (FreddiWindow, freddi_window, GTK_TYPE_APPLICATION_WINDOW)

static void
freddi_window_class_init (FreddiWindowClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	gtk_widget_class_set_template_from_resource (widget_class, "/com/example/Freddi/freddi-window.ui");
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, header_bar);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_name);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_developer);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_summary);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_description);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_id);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_license);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_origin);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_branch);
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
			else if(strcmp(token, "IsRuntime") == 0) {
				fprs.isRuntime = strtok(NULL, equals);

				if (strcmp(fprs.isRuntime, "false") == 0) {
					fprs.app = true;
					fprs.type = "app";
				}
				else {
					fprs.app = false;
					fprs.type = "runtime";
				}
			}
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

	GError *err = NULL;
	char ref[256];
	sprintf(ref, "%s/%s/%s/%s", fprs.type, fprs.name, ARCH, fprs.branch);
	FlatpakRef * fpref = flatpak_ref_parse(ref, err);

	if (err != NULL) {
		g_assert (fpref == NULL);
		puts("Can't find the app.");
	}
	else {
		g_assert (fpref != NULL);
		const char * refname = flatpak_ref_get_name(fpref);

		AsPool * pool = as_pool_new();
		as_pool_set_flags(pool, AS_POOL_FLAG_LOAD_FLATPAK);
		as_pool_load(pool, NULL, NULL);

		GPtrArray * components = as_pool_get_components_by_id(pool, refname);

		for (int i = 0; i < components->len; i++) {
			AsComponent * component = components->pdata[i];
			const gchar * name = as_component_get_name (component); //human-readable name
			const gchar * origin = as_component_get_origin (component);//repository
			const gchar * branch = as_component_get_branch (component);
			const gchar * package = as_component_get_pkgname (component);
			const gchar * source = as_component_get_source_pkgname (component);
			const gchar * summary = as_component_get_summary (component);
			const gchar * description = as_component_get_description (component);
			const gchar * metadata_license = as_component_get_metadata_license (component);
			const gchar * license = as_component_get_project_license (component);
			const gchar * group = as_component_get_project_group (component);
			const gchar * developer = as_component_get_developer_name (component);

			int count = 100;

			if (name != NULL) {
				gtk_label_set_text(self->app_name, name);
				count += strlen(name);
			}
			if (summary != NULL) {
				count += strlen(summary);
			}
			if (description != NULL) {
				count += strlen(description);
			}
			if (refname != NULL) {
				count += strlen(refname);
			}
			if (origin != NULL) {
				count += strlen(origin);
			}
			if (branch != NULL) {
				count += strlen(branch);
			}
			if (license != NULL) {
				count += strlen(license);
			}
			if (developer != NULL) {
				count += strlen(developer);
			}

			gtk_label_set_label(self->app_name, name);
			gtk_label_set_label(self->app_developer, developer);
			gtk_label_set_label(self->app_summary, summary);

			gtk_label_set_wrap(self->app_description, true);
			gtk_label_set_label(self->app_description, description);

			gtk_label_set_label(self->app_id, refname);
			gtk_label_set_label(self->app_license, license);
			gtk_label_set_label(self->app_origin, origin);
			gtk_label_set_label(self->app_branch, branch);
		}
	}



	/* Ask the window manager/compositor to present the window. */
	gtk_window_present (self);
}

