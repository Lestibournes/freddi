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

#ifndef ARCH

#if defined  ( __amd64__) || defined  (__amd64) || defined  (__x86_64__) || defined  (__x86_64)
#define ARCH "x86_64"
#endif

#if defined  (i386) || defined  (__i386) || defined  (__i386__)
#define ARCH "i386"
#endif

#endif

#include <appstream/appstream.h>
#include <regex.h>
#include <ctype.h>
#include <pthread.h>

#include "freddi-config.h"
#include "freddi-window.h"

// Globals:
struct _appData appData;
struct _flatpak flatpak;
GString *ref_string;
FlatpakRefKind kind;

// Utility functions:
gchar* getRef() {
	return ref_string->str;
}

void setRef(gchar* type, gchar* name, gchar* arch, gchar* branch) {
	g_string_printf(ref_string, "%s/%s/%s/%s", type, name, arch, branch);
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

// Setting up the window:
struct _FreddiWindow
{
	GtkApplicationWindow  parent_instance;

	/* Template widgets */
	GtkHeaderBar				*header_bar;
	GtkButton						*install_button;
	GtkProgressBar			*progress_bar;
	GtkImage						*app_icon;
	GtkLabel						*app_name;
	GtkLabel						*app_developer;
	GtkLabel						*app_summary;
	GtkLabel						*app_description;
	GtkLabel						*app_id;
	GtkLabel						*app_license;
	GtkLabel						*app_origin;
	GtkLabel						*app_branch;
	GtkLabel						*app_download_size;
	GtkLabel						*app_installed_size;
	
	GtkBox							*app_license_box;
	GtkBox							*header_box;
};

G_DEFINE_TYPE (FreddiWindow, freddi_window, GTK_TYPE_APPLICATION_WINDOW)

static void
freddi_window_class_init (FreddiWindowClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	gtk_widget_class_set_template_from_resource (widget_class, "/com/example/Freddi/freddi-window.ui");
	
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, header_bar);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, install_button);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, progress_bar);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_icon);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_name);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_developer);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_summary);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_description);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_id);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_license);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_origin);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_branch);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_download_size);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_installed_size);

	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_license_box);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, header_box);
}

static void freddi_window_init (FreddiWindow *self) {
	gtk_widget_init_template (GTK_WIDGET (self));

	g_autoptr(GSimpleAction) install_action = g_simple_action_new ("install", NULL);

	g_signal_connect(install_action, "activate", G_CALLBACK (freddi_window__install_app), self);

	g_action_map_add_action (G_ACTION_MAP (self), G_ACTION(install_action));

	gtk_widget_set_sensitive(self->install_button, false);
}
typedef int STATE;
const STATE NOT_INSTALLED = 0, INSTALLING = 1, INSTALLED = 2, UNINSTALLING = 3;

void setState(FreddiWindow *self, STATE state) {
	if (state == NOT_INSTALLED) {
		gtk_widget_set_sensitive(self->install_button, true);
		gtk_button_set_label(self->install_button, "Install");
		gtk_widget_set_visible(self->progress_bar, false);
	}
	else if(state == INSTALLING) {
		gtk_widget_set_sensitive(self->install_button, false);
		gtk_button_set_label(self->install_button, "Installing");
		gtk_widget_set_visible(self->progress_bar, true);
	}
	else if (state == INSTALLED) {
		gtk_widget_set_sensitive(self->install_button, true);
		gtk_button_set_label(self->install_button, "Remove");
		gtk_widget_set_visible(self->progress_bar, false);
	}
	else if (state == UNINSTALLING) {
		gtk_widget_set_sensitive(self->install_button, false);
		gtk_button_set_label(self->install_button, "Uninstalling");
		gtk_progress_bar_set_pulse_step(self->progress_bar, 0.1);
		gtk_widget_set_visible(self->progress_bar, true);
	}
}
// Loading Flatpak data:

static void flatpak_load_data(GTask *task, gpointer source_object, FreddiWindow *window, GCancellable *cancellable) {
	int retval;

	/* Handle cancellation. */
	if (g_task_return_error_if_cancelled(task)) return;

	/* Run the blocking function. */
	GError *err = NULL;
	
	FlatpakRemoteRef *remote_ref = flatpak_installation_fetch_remote_ref_sync(
		flatpak.installation,
		appData.suggestRemoteName,
		kind,
		appData.id,
		ARCH,
		appData.branch,
		cancellable,
		&err
	);

	if (err == NULL) {
		guint64 download_size = flatpak_remote_ref_get_download_size(remote_ref);
		guint64 installed_size = flatpak_remote_ref_get_installed_size(remote_ref);
		
		if (download_size) {
			char buffer[100];
			double MiB = (float) download_size / (1024 * 1024);
			sprintf(buffer, "%.2fMiB", MiB);
			gtk_label_set_label(window->app_download_size, buffer);
		}
		
		if (installed_size) {
			char buffer[100];
			double MiB = (float) installed_size / (1024 * 1024);
			sprintf(buffer, "%.2fMiB", MiB);
			gtk_label_set_label(window->app_installed_size, buffer);
		}

		retval = true;
	}
	else {
		puts("Failed to fetch remote metadata from Flatpak.\n");
		printf("%s\n", err->message);
		retval = false;
	}

	g_task_return_int(task, retval);
}

void flatpak_load_data_async(FreddiWindow *window, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data) {
	GTask *task = NULL;  /* owned */

	g_return_if_fail(cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	task = g_task_new(NULL, cancellable, callback, user_data);
	g_task_set_source_tag(task, flatpak_load_data_async);

	/* Cancellation should be handled manually using mechanisms specific to
	 * some_blocking_function(). */
	g_task_set_return_on_cancel(task, FALSE);

	g_task_set_task_data(task, window, NULL);

	/* Run the task in a worker thread and return immediately while that continues
	 * in the background. When it’s done it will call @callback in the current
	 * thread default main context. */
	g_task_run_in_thread (task, flatpak_load_data);

	g_object_unref (task);
}

// Installing the app:
static void flatpak_transaction_run_thread(GTask *task, char *transaction, FreddiWindow *window, GCancellable *cancellable) {
	/* Handle cancellation. */
	if (g_task_return_error_if_cancelled(task)) return;

	/* Run the blocking function. */
	GError* err = NULL;
	gboolean success = flatpak_transaction_run(transaction, NULL, NULL);
	
	err = NULL;
	FlatpakInstalledRef *installed_ref = flatpak_installation_get_installed_ref(flatpak.installation, kind, appData.id, ARCH, appData.branch, NULL, &err);

	if (err == NULL || installed_ref != NULL) setState(window, INSTALLED);
	else setState(window, NOT_INSTALLED);

	g_task_return_int(task, success);
}

void flatpak_transaction_run_async(FlatpakTransaction *self, FreddiWindow *window, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data) {
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	GTask *task = g_task_new(self, cancellable, callback, user_data);
	g_task_set_source_tag (task, flatpak_transaction_run_async);
	
	g_task_set_return_on_cancel(task, FALSE);

	g_task_set_task_data(task, window, NULL);

	g_task_run_in_thread(task, flatpak_transaction_run_thread);

	g_object_unref(task);
}

void flatpak_transaction_progress_change(FlatpakTransactionProgress *transaction_progress, FreddiWindow* self) {
	int progress = flatpak_transaction_progress_get_progress(transaction_progress);
	double fraction = ((double) progress) / 100.0;
	
	gtk_progress_bar_set_fraction(self->progress_bar, fraction);
	gtk_progress_bar_pulse(self->progress_bar);
}

void flatpak_transaction_progress_start (FlatpakTransaction *object, FlatpakTransactionOperation *operation, FlatpakTransactionProgress *progress, FreddiWindow *self) {
	g_signal_connect(progress, "changed", G_CALLBACK (flatpak_transaction_progress_change), self);
}

void flatpak_transaction_progress_end(FlatpakTransaction *object, FlatpakTransactionOperation *operation, gchar *commit, gint result, FreddiWindow *self) {
}

gboolean flatpak_transaction_run_finish(GAsyncResult *result, GError **error) {
}

static void freddi_window__install_app(GAction *action G_GNUC_UNUSED, GVariant *parameter G_GNUC_UNUSED, FreddiWindow *self) {
	GError *err = NULL;
	FlatpakInstalledRef *installed_ref = flatpak_installation_get_installed_ref(flatpak.installation, kind, appData.id, ARCH, appData.branch, NULL, &err);

	if (err != NULL || installed_ref == NULL) {
		setState(self, INSTALLING);

		if (appData.id == NULL)	{
			printf("No package is selected.\n");
			return;
		}

		err = NULL;
		GCancellable* cancellable = g_cancellable_new();

		FlatpakTransaction *transaction = flatpak_transaction_new_for_installation(flatpak.installation, cancellable, &err);
		
		if (err == NULL) {
			flatpak_transaction_add_install(transaction, appData.suggestRemoteName, getRef(), NULL, &err);
			
			if (err == NULL) {
				g_signal_connect(transaction, "new-operation", G_CALLBACK (flatpak_transaction_progress_start), self);
				g_signal_connect(transaction, "operation-done", G_CALLBACK (flatpak_transaction_progress_end), self);
				flatpak_transaction_run_async(transaction, self, NULL, flatpak_transaction_run_finish, NULL);
			}
		}
		else {
			printf("Creating a transaction failed.\n");
		}
	}
	else {
		setState(self, UNINSTALLING);

		if (appData.id == NULL)	{
			printf("No package is selected.\n");
			return;
		}

		err = NULL;
		GCancellable* cancellable = g_cancellable_new();

		FlatpakTransaction *transaction = flatpak_transaction_new_for_installation(flatpak.installation, cancellable, &err);
		
		if (err == NULL) {
			flatpak_transaction_add_uninstall(transaction, getRef(), &err);
			
			if (err == NULL) {
				g_signal_connect(transaction, "new-operation", G_CALLBACK (flatpak_transaction_progress_start), self);
				g_signal_connect(transaction, "operation-done”", G_CALLBACK (flatpak_transaction_progress_end), self);
				flatpak_transaction_run_async(transaction, self, NULL, flatpak_transaction_run_finish, NULL);
			}
		}
		else {
			printf("Creating a transaction failed.\n");
		}
	}
}

void open_file_complete (GObject *source_object, GAsyncResult *result, FreddiWindow *self) {
	// # Load the .flatpakref file content.
	GFile *file = G_FILE (source_object);

	g_autofree char *contents = NULL;
	gsize length = 0;

	g_autoptr (GError) error = NULL;

	// Complete the asynchronous operation; this function will either
	// give you the contents of the file as a byte array, or will
	// set the error argument
	g_file_load_contents_finish(file, result, &contents, &length, NULL, &error);

	// Query the display name for the file
	g_autofree char *display_name = NULL;
	g_autoptr (GFileInfo) info = g_file_query_info(file, "standard::display-name", G_FILE_QUERY_INFO_NONE, NULL, NULL);

	if (info != NULL) display_name = g_strdup(g_file_info_get_attribute_string(info, "standard::display-name"));
	else display_name = g_file_get_basename(file);

	// In case of error, print a warning to the standard error output
	// TODO display in GUI instead.
	if (error != NULL) {
		g_printerr ("Unable to open “%s”: %s\n", g_file_peek_path(file), error->message);
		return;
	}

	// Ensure that the file is encoded with UTF-8
	if (!g_utf8_validate(contents, length, NULL)) {
		g_printerr ("Unable to load the contents of “%s”: the file is not encoded with UTF-8\n", g_file_peek_path(file));
		return;
	}
	
	// # Parse the relevant data from the file and store it in a global.
	int count = countChar (contents, '\n');
	char** lines = lineator(contents);
	const char equals[2] = "=";

	for (int i = 0; i < count; i++) {
		char* token = strtok(lines[i], equals);

		if (token != NULL) {
			if(strcmp(token, "Name") == 0) appData.id = strtok(NULL, equals);
			else if(strcmp(token, "Branch") == 0) appData.branch = strtok(NULL, equals);
			else if(strcmp(token, "Title") == 0) appData.title = strtok(NULL, equals);
			else if(strcmp(token, "IsRuntime") == 0) {
				appData.isRuntime = strtok(NULL, equals);

				if (strcmp(appData.isRuntime, "false") == 0) {
					appData.app = true;
					appData.type = "app";
				}
				else {
					appData.app = false;
					appData.type = "runtime";
				}
			}
			else if(strcmp(token, "Url") == 0) appData.url = strtok(NULL, equals);
			else if(strcmp(token, "SuggestRemoteName") == 0) appData.suggestRemoteName = strtok(NULL, equals);
			else if(strcmp(token, "GPGKey") == 0) appData.gpgKey = strtok(NULL, equals);
			else if(strcmp(token, "RuntimeRepo") == 0) appData.runtimeRepo = strtok(NULL, equals);
		}
	}

	ref_string = g_string_new("");
	setRef(appData.type, appData.id, ARCH, appData.branch);

	// Display data from the .flatpakref file:

	if (appData.suggestRemoteName != NULL) {
		char buffer[strlen(appData.suggestRemoteName) + 1];
		strcpy(buffer, appData.suggestRemoteName);
		buffer[strlen(appData.suggestRemoteName)] = 0;

		for (int c = 0; buffer[c] != '\0'; c++) {
			if (c == 0 || buffer[c - 1] == ' ') buffer[c] = toupper(buffer[c]);
		}

		gtk_label_set_label(self->app_origin, buffer);
	}

	if (appData.id != NULL) gtk_label_set_label(self->app_id, appData.id);
	if (appData.branch != NULL) gtk_label_set_label(self->app_branch, appData.branch);
	
	// Load Flatpak metadata:
	
	GCancellable* cancellable = g_cancellable_new();
	GError* err = NULL;

	FlatpakRef *ref = flatpak_ref_parse(getRef(), &err);

	if (err != NULL) puts("Application/Runtime not found.\n"); // TODO put the error message in the GUI instead.
	kind = flatpak_ref_get_kind(ref);

	err = NULL;
	flatpak.installation = flatpak_installation_new_system(NULL, &err);

	if (err == NULL) {
		FlatpakInstalledRef *installed_ref = flatpak_installation_get_installed_ref(flatpak.installation, kind, appData.id, ARCH, appData.branch, cancellable, &err);

		if (installed_ref == NULL) setState(self, NOT_INSTALLED);
		else setState(self, INSTALLED);

		err = NULL;
		flatpak_load_data_async(self, NULL, NULL, NULL);
	}
	else {
		puts("Failed to connect to Flatpak instance.\n");
	}

	// Load AppStream metadata:
	// TODO do this on a seperate thread.

	AsPool * pool = as_pool_new();
	as_pool_set_flags(pool, AS_POOL_FLAG_LOAD_FLATPAK);
	as_pool_load(pool, NULL, NULL);

	GPtrArray * components = as_pool_get_components_by_id(pool, appData.id);
	
	if (components->len == 0) {
		gtk_label_set_text(self->app_description, "(Failed to fetch AppStream metadata)");
		gtk_widget_set_visible(self->app_license_box, false);
		gtk_widget_set_visible(self->header_box, false);
		gtk_widget_set_visible(self->app_summary, false);
	}
	else {
		// For now I'm assuming that flathub only has 1 instance of each app,
		// so since I'm only loading flathub metadata in appstream,
		// I only need to check fo the first component.
		// This could turn out to be false, though.
		// In any case, that's why I'm not worried about doing a loop here.
		// It should only have one iteration.
		for (guint i = 0; i < components->len; i++) {
			// Getting the app metadata:
			// TODO get more relevant metadata. At least I want to match all the text metadata on Flathub.
			AsComponent * component = components->pdata[i];

			const gchar * name = as_component_get_name (component); //human-readable name
			const gchar * summary = as_component_get_summary (component);
			const gchar * description = as_component_get_description (component);
			const gchar * license = as_component_get_project_license (component);
			const gchar * developer = as_component_get_developer_name (component);
			const GPtrArray * icons = as_component_get_icons(component);
			
			gpointer biggest_icon = icons->pdata[0];
			guint biggest_size = 0;

			for (guint j = 0; j < icons->len; j++) {
				guint size = as_icon_get_width(icons->pdata[j]) * as_icon_get_height(icons->pdata[j]);

				if (size > biggest_size) {
					biggest_icon = icons->pdata[j];
					biggest_size = size;
				}
			}

			// Display the metadata:
			if (as_icon_get_kind(biggest_icon) == AS_ICON_KIND_CACHED) gtk_image_set_from_file(self->app_icon, as_icon_get_filename(biggest_icon));
			if (name != NULL) gtk_label_set_label(self->app_name, name);
			if (developer != NULL) gtk_label_set_label(self->app_developer, developer);
			if (summary != NULL) gtk_label_set_label(self->app_summary, summary);
			
			if (description != NULL) {
				// Convert HTML tags to regular string values:
				char* cursor = description;
				char buffer[strlen(description + 1)];
				memset(buffer, '\0', strlen(description) + 1);

				// Supported tags:
				char* opening_p = "<p>";
				char* closing_p = "</p>";
				char* opening_ul = "<ul>";
				char* closing_ul = "</ul>";
				char* opening_li = "<li>";
				char* closing_li = "</li>";
/*
Iterate over description. If the current character isn't part of a tag, add it to the buffer. If it is part of a tag, skip it.

So for example, if whe have the text:
<p>hello</p>
<ul><li>you</li><li>are</li></ul>
<p>an idiot!</p>

Then the following steps will occur:
1. Detect <p> at i = 0
2. Skip to i = 3
3. Copy all characters one by one until i = 8.
4. Detect </p> at i = 9
5. Skip to i = 10
6. Copy the character at i = 10
7. Detect <ul> at i = 11
8. Skip to i = 15
9. Detect <il> at i = 15
10. Skip to i = 19
11. Copy all characters one by one until i = 22
etc.
*/
				int index = 0; //The next position in which to write to the buffer.
				char* tag; //The tag that was found.
				// int len; //The length of the tag that was found.

				for (int i = 0; i < strlen(description); i++) {
					cursor = description + i;

					if (strncmp(cursor, opening_p, strlen(opening_p)) == 0) {
						tag = opening_p;
					}
					else if (strncmp(cursor, closing_p, strlen(closing_p)) == 0) {
						tag = closing_p;
						buffer[index] = '\n';
						index++;
					}
					else if (strncmp(cursor, opening_ul, strlen(opening_ul)) == 0) {
						tag = opening_ul;
					}
					else if (strncmp(cursor, closing_ul, strlen(closing_ul)) == 0) {
						tag = closing_ul;
					}
					else if (strncmp(cursor, opening_li, strlen(opening_li)) == 0) {
						tag = opening_li;
						strcat(buffer + index, "\t* ");
						index += 3;
					}
					else if (strncmp(cursor, closing_li, strlen(closing_li)) == 0) {
						tag = closing_li;
						buffer[index] = '\n';
						index++;
					}
					else {
						tag = NULL;
						buffer[index] = description[i];
						index++;
					}

					
					if (tag != NULL) {
						i += strlen(tag) - 1;
					}
				}

				gtk_label_set_label(self->app_description, buffer);
				gtk_label_set_wrap(self->app_description, true);
			}

			if (license != NULL) gtk_label_set_label(self->app_license, license);
		}
	}

	/* Ask the window manager/compositor to present the window. */
	gtk_window_present(self);
}

