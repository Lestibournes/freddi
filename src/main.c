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

#include <glib/gi18n.h>

#include "freddi-config.h"
#include "freddi-application.h"

int
main (int   argc,
      char *argv[])
{
	g_autoptr(FreddiApplication) app = NULL;
	int ret;

	/* Set up gettext translations */
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/*
	 * Create a new GtkApplication. The application manages our main loop,
	 * application windows, integration with the window manager/compositor, and
	 * desktop features such as file opening and single-instance applications.
	 */
	app = freddi_application_new ("com.example.Freddi", G_APPLICATION_HANDLES_OPEN);
  g_signal_connect (app, "open", G_CALLBACK (app_open_file), NULL);

	/*
	 * Run the application. This function will block until the application
	 * exits. Upon return, we have our exit code to return to the shell. (This
	 * is the code you see when you do `echo $?` after running a command in a
	 * terminal.
	 *
	 * Since GtkApplication inherits from GApplication, we use the parent class
	 * method "run". But we need to cast, which is what the "G_APPLICATION()"
	 * macro does.
	 */
	ret = g_application_run (G_APPLICATION (app), argc, argv);

	return ret;
}
