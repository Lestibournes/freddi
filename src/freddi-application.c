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

#include "freddi-application.h"
#include "freddi-window.h"

struct _FreddiApplication
{
	GtkApplication parent_instance;
};

G_DEFINE_TYPE (FreddiApplication, freddi_application, ADW_TYPE_APPLICATION)

FreddiApplication *
freddi_application_new (gchar *application_id,
												GApplicationFlags  flags)
{
	return g_object_new (FREDDI_TYPE_APPLICATION,
											 "application-id", application_id,
											 "flags", flags,
											 NULL);
}

static void
freddi_application_finalize (GObject *object)
{
	FreddiApplication *self = (FreddiApplication *)object;

	G_OBJECT_CLASS (freddi_application_parent_class)->finalize (object);
}

static void
freddi_application_activate (GApplication *app)
{
	GtkWindow *window;

	/* It's good practice to check your parameters at the beginning of the
	 * function. It helps catch errors early and in development instead of
	 * by your users.
	 */
	g_assert (GTK_IS_APPLICATION (app));

	/* Get the current window or create one if necessary. */
	window = gtk_application_get_active_window (GTK_APPLICATION (app));
	if (window == NULL)
		window = g_object_new (FREDDI_TYPE_WINDOW,
													 "application", app,
													 NULL);

	/* Ask the window manager/compositor to present the window. */
	gtk_window_present (window);
}


static void
freddi_application_class_init (FreddiApplicationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

	object_class->finalize = freddi_application_finalize;

	/*
	 * We connect to the activate callback to create a window when the application
	 * has been launched. Additionally, this callback notifies us when the user
	 * tries to launch a "second instance" of the application. When they try
	 * to do that, we'll just present any existing window.
	 */
	app_class->activate = freddi_application_activate;
}

static void
freddi_application_show_about (GSimpleAction *action,
															 GVariant      *parameter,
															 gpointer       user_data)
{
	FreddiApplication *self = FREDDI_APPLICATION (user_data);
	GtkWindow *window = NULL;
	const gchar *authors[] = {"Yitzchak Schwarz", NULL};

	g_return_if_fail (FREDDI_IS_APPLICATION (self));

	window = gtk_application_get_active_window (GTK_APPLICATION (self));

	gtk_show_about_dialog (window,
													"program-name", "Freddi",
													"authors", authors,
													"version", "0.1.0",
													"copyright", "Copyright (C) 2022 Yitzchak Schwarz",
													"website", "https://github.com/Lestibournes/freddi",
													"website-label", "Project page on GitHub",
													"license-type", GTK_LICENSE_GPL_2_0_ONLY,
													"comments", "A simple installer for flatpak applications",
													NULL);
}


static void
freddi_application_init (FreddiApplication *self)
{
	g_autoptr (GSimpleAction) quit_action = g_simple_action_new ("quit", NULL);
	g_signal_connect_swapped (quit_action, "activate", G_CALLBACK (g_application_quit), self);
	g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (quit_action));

	g_autoptr (GSimpleAction) about_action = g_simple_action_new ("about", NULL);
	g_signal_connect (about_action, "activate", G_CALLBACK (freddi_application_show_about), self);
	g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (about_action));

	gtk_application_set_accels_for_action (GTK_APPLICATION (self),
																				 "app.quit",
																				 (const char *[]) {
																					 "<primary>q",
																					 NULL,
																				 });
}

void app_open_file (GApplication  *application,
										GFile        **files,
										gint           n_files,
										const gchar   *hint)
{
	GtkWindow *window;

	/* It's good practice to check your parameters at the beginning of the
	 * function. It helps catch errors early and in development instead of
	 * by your users.
	 */
	g_assert (GTK_IS_APPLICATION (application));

	/* Get the current window or create one if necessary. */
	window = gtk_application_get_active_window (GTK_APPLICATION (application));
	if (window == NULL)
		window = g_object_new (FREDDI_TYPE_WINDOW,
													 "application", application,
													 NULL);
	gint i;
	GSList *file_list = NULL;

	for (i = 0; i < n_files; i++)
	{
		file_list = g_slist_prepend (file_list, files[i]);
		gchar* name = g_file_get_basename(files[i]);

		g_file_load_contents_async (files[i],
																NULL,
																(GAsyncReadyCallback) open_file_complete,
																window);
	}
}

