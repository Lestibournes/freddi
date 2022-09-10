#ifndef ARCH

#if defined  ( __amd64__) || defined  (__amd64) || defined  (__x86_64__) || defined  (__x86_64)
#define ARCH "x86_64"
#endif

#if defined  (i386) || defined  (__i386) || defined  (__i386__)
#define ARCH "i386"
#endif

#endif

#include <flatpak.h>
#include <appstream/appstream.h>
#include <regex.h>

#include "freddi-config.h"
#include "freddi-window.h"

struct _appData appData;

int regSearch(char * source, char * regexString, size_t maxMatches, size_t maxGroups) {
	regex_t regexCompiled;
	regmatch_t groupArray[maxGroups];

	if (regcomp(&regexCompiled, regexString, REG_EXTENDED)) {
		printf("Could not compile regular expression.\n");
		return 1;
	};

	// Points to the beginning of the search for each match.
	char * cursor = source;

	for (unsigned int m = 0; m < maxMatches; m ++) {
		// Goes over all the matches until maxMatches.
		// Breaks early if it finds no more matches.
		if (regexec(&regexCompiled, cursor, maxGroups, groupArray, 0)) break;

		for (unsigned int g = 0; g < maxGroups; g++) {
			// Goes over all the groups in the match until maxGroups.
			// Breaks early if the current group doesn't exist.
			if (groupArray[g].rm_so == (size_t)-1) break;

			// Copy of the source string starting from cursor and all the way to the end,
			// but with a null characater placed at the end of each group.
			char cursorCopy[strlen(cursor) + 1];
			strcpy(cursorCopy, cursor);
			cursorCopy[groupArray[g].rm_eo] = 0;

			printf("Match %u, Group %u: [%2u-%2u]: %s\n",
							m, g, groupArray[g].rm_so, groupArray[g].rm_eo,
							cursorCopy + groupArray[g].rm_so);
		}

		cursor += groupArray[0].rm_eo + 1;
	}

	regfree(&regexCompiled);

	return 0;
}

/**
 * Requires two passes to remove both opening and closing tags.
 * @param source the source string. Will not be modified.
 * @param maxMatches how many tags to search for.
 * @param exceptions allowed HTML tag names.
 * @param n_exceptions number of allowed HTML tags.
 * @return the sanitized string, or NULL if the regex could not be compiled.
 */
char* removeTags(char * source, char * pattern, size_t maxMatches, char** exceptions, int n_exceptions) {
	regex_t compiled;
	const size_t maxGroups = 2;
	regmatch_t groupArray[maxGroups];
	regmatch_t nextMatch[maxGroups];

	// The size of the source string in bytes
	int size = strlen(source) * sizeof(char) + 1;

	// The result string
	char* buffer = calloc(strlen(source) + 1, sizeof(char));

	// Where to place the next sanitized string in the buffer.
	int offset = 0;

	if (regcomp(&compiled, pattern, REG_EXTENDED)) return NULL;

	// Points to the beginning of the search for each match.
	char* cursor = source;
	char* nextCursor = source;

	printf("%s\n", cursor);

/*
<p>This basic image editor can resize, crop, or rotate an image, apply simple filters, insert or censor text, and manipulate a selected portion of the picture (cut/copy/paste/drag/…)</p>
<p>And of course, you can draw! Using tools such as the pencil, the straight line, the curve tool, many shapes, several brushes, and their various colors and options.</p>
<p>Supported file types include PNG, JPEG and BMP.</p>
*/

	for (unsigned int m = 0; m < maxMatches; m ++) {
		// Goes over all the matches until maxMatches.
		// Breaks early if it finds no more matches.
		if (regexec(&compiled, cursor, maxGroups, groupArray, 0)) break;
		
		// Check if the current tag is one of the allowed tags.
		bool isAllowed = false;

		int tagLength = groupArray[1].rm_eo - groupArray[1].rm_so;
		char tagName[tagLength + 1];
		strncpy(tagName, cursor + groupArray[1].rm_so, tagLength);
		tagName[tagLength] = 0;
		
		for (int i = 0; i < n_exceptions; i++) {
			if (strcmp(exceptions[i], tagName) == 0) {
				isAllowed = true;
				break;
			}
		}

		// The length of the text between the current tag and the next one.
		int length;
		nextCursor = cursor + groupArray[0].rm_eo;

		printf(
			"Match end:\t%d\n"
			"Cursor length:\t%d\n\n"
			, groupArray[0].rm_eo
			, strlen(cursor)
		);

		printf(
			"Current cursor:\t%p\n"
			"Next cursor:\t%p\n\n"
			, cursor
			, nextCursor
		);

		bool nextExists = regexec(&compiled, nextCursor, maxGroups, nextMatch, 0) == 0;
		printf("Next exists:\t%s\n\n", nextExists ? "Yes" : "No");

		printf(
			"Current start:\t%d\n"
			"Current end:\t%d\n\n"
		, groupArray[0].rm_so
		, groupArray[0].rm_eo
		);

		if (groupArray[0].rm_eo < (strlen(cursor)) && nextExists) {
			printf(
				"Next start:\t%d\n"
				"Next end:\t%d\n\n"
			, nextMatch[0].rm_so
			, nextMatch[0].rm_eo
			);

			if (isAllowed) length = nextMatch[0].rm_so - groupArray[0].rm_so;
			else length = nextMatch[0].rm_so - groupArray[0].rm_eo;
		}
		else {
			if (isAllowed) length = strlen(cursor) - groupArray[0].rm_so;
			else length = strlen(cursor) - groupArray[0].rm_eo;
		}
/*
This basic image editor can resize, crop, or rotate an image, apply simple filters, insert or censor text, and manipulate a selected portion of the picture (cut/copy/paste/drag/…)

And of course, you can draw! Using tools such as the pencil, the straight line, the curve tool, many shapes, several brushes, and their various colors and options.

Supported file types include PNG, JPEG and BMP.
*/
		printf(
			"Offset:\t\t%d\n"
			"Tag length:\t%d\n"
			"Text length:\t%d\n\n"
			, offset
			, groupArray[0].rm_eo - groupArray[0].rm_so
			, length
		);

		memcpy(buffer + offset, cursor + (groupArray[0].rm_eo - groupArray[0].rm_so), length);

		offset += length;
		cursor += groupArray[0].rm_eo;
	}

	regfree(&compiled);

	return buffer;
}

char* removeUnsupportedTags(char* src) {
	char* supported_tags[] = {"markup", "span", "b", "big", "i", "s", "sub", "sup", "small", "tt", "u"};
	const int supported_num = 11;

	char* buffer = removeTags(src, "<(p)>", 100, supported_tags, supported_num);
	printf("%s\n\n", buffer);
	buffer = removeTags(buffer, "</(p)>", 100, supported_tags, supported_num);
	printf("%s\n", buffer);
	
	return buffer;

	// // The size of src in bytes (including null character)
	// const int size = srtlen(src) * sizeof(char) + 1;

	// char* buffer = malloc(size);
	// memset(buffer, '\0', size);

	// regex_t preg;
	// size_t nmatch = 100;
	// size_t ngroups = 2;
	// regmatch_t pmatch[nmatch];

	// int status = regcomp(&preg, pattern, REG_EXTENDED);

	// if (status == 0) {
	// 	status = regexec(&preg, src, nmatch, pmatch, 0);

	// 	if (status == 0) {
	// 		int offset = 0;

	// 		for (int i = 0; i < nmatch; i++) {
	// 			if (pmatch[i].rm_so > -1) {
	// 				int match_length = pmatch[i].rm_eo - pmatch[i].rm_so;
	// 				char match[match_length + 1];
	// 				strncpy(&match, src, match_length);
	// 				match[match_length + 1] = '\0';

	// 				printf("%s:%c\n",match, src[pmatch[i].rm_eo]);

	// 				bool is_supported = false;

	// 				for (int j = 0; j < supported_num; j++) {
	// 					if (strstr(src[pmatch[i].rm_so], supported_tags[j]) == src[pmatch[i].rm_so + 1]) {
	// 						is_supported = true;
	// 						break;
	// 					}
	// 				}

	// 				int length = pmatch[i + 1].rm_so - pmatch[i].rm_eo + 1;

	// 				memcpy(buffer + offset, src[pmatch[i].rm_eo + 1], length);
	// 				offset += length; 
	// 			}
	// 		}
	// 	}
	// }
}

// This should be inlined, but I'm doing it this way to make it easier for me to read the code.
char* getRef(char* type, char* name, char* arch, char* branch) {
	int length = strlen(type) + strlen(name) + strlen(arch) + strlen(branch);
	char* ref = malloc(sizeof(char) * length + 1);
	sprintf(ref, "%s/%s/%s/%s", type, name, arch, branch);
	return ref;
}

struct _FreddiWindow
{
	GtkApplicationWindow  parent_instance;

	/* Template widgets */
	GtkHeaderBar				*header_bar;
	GtkButton						*install_button;
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
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, install_button);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_name);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_developer);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_summary);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_description);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_id);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_license);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_origin);
	gtk_widget_class_bind_template_child (widget_class, FreddiWindow, app_branch);
}

static void text_viewer_window__install_app(GAction *action G_GNUC_UNUSED, GVariant *parameter G_GNUC_UNUSED, FreddiWindow *self) {
	// TODO install the flatpak.
	// 1. Get the full app ref string from the file.
	// 2. Get a FlatpakRef object by using flatpak_ref_parse() (Is this necessary?).
	// 3. Get a FlatpakTransaction object by using flatpak_transaction_new_for_installation().
	// 4. Add an install to the transation using either flatpak_transaction_add_install_flatpakref() or flatpak_transaction_add_install().
	// 5. Execute the installation with flatpak_transaction_run().
	// This is a blocking function, so either it should be placed in a worker thread, or use an alternative async function.
	// 6. Monitor and display the progress using a FlatpakTransactionOperation object, which is obtained from a signal.
	// The question is, how to register the listener?
	if (appData.id == NULL)	{
		printf("No package is selected.\n");
		return;
	}

	// # Obtain the Flatpak application data from Flatpak
	// TODO I don't need this here. It's redundant as I will only need it when during the install.
	GError *err = NULL;
	char* ref = getRef(appData.type, appData.id, ARCH, appData.branch);
	FlatpakRef * fpref = flatpak_ref_parse(ref, &err);

	if (err != NULL) {
		g_assert (fpref == NULL);
		puts("Can't find the app."); // TODO put the error message in the GUI instead.
	}
	else {
		// # Obtain app metadata from AppStream in order to display it to the user prettily
		g_assert (fpref != NULL);
		const char * refname = flatpak_ref_get_name(fpref);

	}
}

static void
freddi_window_init (FreddiWindow *self)
{
	gtk_widget_init_template (GTK_WIDGET (self));

	g_autoptr(GSimpleAction) install_action =
		g_simple_action_new ("install", NULL);

	g_signal_connect(install_action,
										"activate",
										G_CALLBACK (text_viewer_window__install_app),
										self);

	g_action_map_add_action (G_ACTION_MAP (self),
													 G_ACTION(install_action));
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
	
	AsPool * pool = as_pool_new();
	as_pool_set_flags(pool, AS_POOL_FLAG_LOAD_FLATPAK);
	as_pool_load(pool, NULL, NULL);

	GPtrArray * components = as_pool_get_components_by_id(pool, appData.id);

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

		// High-priority: 
		const gchar * name = as_component_get_name (component); //human-readable name
		const gchar * summary = as_component_get_summary (component);
		const gchar * description = as_component_get_description (component);

		// Additional details:
		const gchar * origin = as_component_get_origin (component);//repository
		const gchar * branch = as_component_get_branch (component);
		const gchar * license = as_component_get_project_license (component);
		const gchar * developer = as_component_get_developer_name (component);

		// Irrelevant:
		const gchar * package = as_component_get_pkgname (component);
		const gchar * source = as_component_get_source_pkgname (component);
		const gchar * metadata_license = as_component_get_metadata_license (component);
		const gchar * group = as_component_get_project_group (component);

		// Display the metadata:
		if (name != NULL) gtk_label_set_label(self->app_name, name);
		if (developer != NULL) gtk_label_set_label(self->app_developer, developer);
		if (summary != NULL) gtk_label_set_label(self->app_summary, summary);

		if (description != NULL) {
			// TODO Strip HTML tags from the description. In particular, the <p></p> tags.
			// printf("%s\n\n", description);
			// char buffer = removeUnsupportedTags(description); //Overenginnered. I just need to get rid of <p>...</p>

			char* cursor = description;
			char* start;
			char* end;
			char buffer[strlen(description + 1)];
			int offset = 0;
			int length;

			while((start = strstr(cursor, "<p>")) != NULL) {
				end = strstr(cursor, "</p>");
				length = end - start - 3;

				memcpy(buffer + offset, start + 3, length);
				cursor = end + 4;
				offset += length;
				buffer[offset] = 0;
				strcat(buffer, "\n\n");
				offset += 2;
			}

			buffer[offset] = 0;

			gtk_label_set_label(self->app_description, buffer);
			gtk_label_set_wrap(self->app_description, true);
		}

		if (appData.id != NULL) gtk_label_set_label(self->app_id, appData.id);
		if (license != NULL) gtk_label_set_label(self->app_license, license);
		if (origin != NULL) gtk_label_set_label(self->app_origin, origin);
		if (branch != NULL) gtk_label_set_label(self->app_branch, branch);
	}

	/* Ask the window manager/compositor to present the window. */
	gtk_window_present(self);
}

