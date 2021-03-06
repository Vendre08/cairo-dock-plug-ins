/**
* This file is a part of the Cairo-Dock project
*
* Copyright : (C) see the 'copyright' file.
* E-mail    : see the 'copyright' file.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 3
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib/gstdio.h>

#include "applet-struct.h"
#include "applet-notifications.h"
#include "applet-session.h"
#include "applet-listing.h"
#include "applet-search.h"
#include "applet-backend-files.h"

// sub-listing
static GList *_cd_do_list_file_sub_entries (CDEntry *pEntry, int *iNbEntries);
// fill entry
static gboolean _cd_do_fill_main_entry (CDEntry *pEntry);
static gboolean _cd_do_fill_file_entry (CDEntry *pEntry);
// actions
static void _cd_do_launch_file (CDEntry *pEntry);
static void _cd_do_show_file_location (CDEntry *pEntry);
static void _cd_do_open_terminal_here (CDEntry *pEntry);
static void _cd_do_zip_file (CDEntry *pEntry);
static void _cd_do_zip_folder (CDEntry *pEntry);
static void _cd_do_mail_file (CDEntry *pEntry);
static void _cd_do_move_file (CDEntry *pEntry);
static void _cd_do_copy_url (CDEntry *pEntry);


  //////////
 // INIT //
//////////

static gboolean init (void)
{
	gchar *cResult = cairo_dock_launch_command_sync ("which locate");
	
	gboolean bAvailable = (cResult != NULL && *cResult != '\0');
	g_free (cResult);
	cd_debug ("locate available : %d", bAvailable);
	
	if (bAvailable)
	{
		gchar *cDirPath = g_strdup_printf ("%s/ScoobyDo", g_cCairoDockDataDir);
		if (! g_file_test (cDirPath, G_FILE_TEST_IS_DIR))
		{
			if (g_mkdir (cDirPath, 7*8*8+7*8+5) != 0)
			{
				cd_warning ("couldn't create directory %s", cDirPath);
				g_free (cDirPath);
				return FALSE;
			}
		}
		
		gchar *cDataBase = g_strdup_printf ("%s/ScoobyDo.db", cDirPath);
		gchar *cLastUpdateFile = g_strdup_printf ("%s/.last-update", cDirPath);
		gboolean bNeedsUpdate = FALSE;
		
		if (! g_file_test (cDataBase, G_FILE_TEST_EXISTS))
		{
			bNeedsUpdate = TRUE;
		}
		else
		{
			if (! g_file_test (cLastUpdateFile, G_FILE_TEST_EXISTS))
			{
				bNeedsUpdate = TRUE;
			}
			else
			{
				gsize length = 0;
				gchar *cContent = NULL;
				g_file_get_contents (cLastUpdateFile,
					&cContent,
					&length,
					NULL);
				if (cContent == NULL || *cContent == '\0')
				{
					bNeedsUpdate = TRUE;
				}
				else
				{
					time_t iLastUpdateTime = atoll (cContent);
					time_t iCurrentTime = (time_t) time (NULL);
					if (iCurrentTime - iLastUpdateTime > 86400)
					{
						bNeedsUpdate = TRUE;
					}
				}
				g_free (cContent);
			}
		}
		
		if (bNeedsUpdate)
		{
			cairo_dock_launch_command (MY_APPLET_SHARE_DATA_DIR"/updatedb.sh");
			gchar *cDate = g_strdup_printf ("%ld", time (NULL));
			g_file_set_contents (cLastUpdateFile,
				cDate,
				-1,
				NULL);
			g_free (cDate);
		}
		
		g_free (cDataBase);
		g_free (cLastUpdateFile);
		g_free (cDirPath);
	}
	
	return bAvailable;
}





  /////////////////
 // SUB-LISTING //
/////////////////

#define NB_ACTIONS_ON_FOLDER 4
static GList *_list_folder (const gchar *cPath, gboolean bNoHiddenFile, int *iNbEntries)
{
	cd_debug ("%s (%s)", __func__, cPath);
	// on ouvre le repertoire.
	GError *erreur = NULL;
	GDir *dir = g_dir_open (cPath, 0, &erreur);
	if (erreur != NULL)
	{
		cd_warning (erreur->message);
		g_error_free (erreur);
		*iNbEntries = 0;
		return NULL;
	}
	
	// on ajoute les entrees d'actions sur le repertoire.
	GList *pEntries = NULL;
	CDEntry *pEntry;
	
	pEntry = g_new0 (CDEntry, 1);
	pEntry->cPath = g_strdup (cPath);
	pEntry->cName = g_strdup (D_("Open a terminal here"));
	pEntry->cIconName = g_strdup ("terminal");
	pEntry->fill = cd_do_fill_default_entry;
	pEntry->execute = _cd_do_open_terminal_here;
	pEntries = g_list_prepend (pEntries, pEntry);
	
	pEntry = g_new0 (CDEntry, 1);
	pEntry->cPath = g_strdup (cPath);
	pEntry->cName = g_strdup (D_("Zip folder"));
	pEntry->cIconName = g_strdup ("zip");
	pEntry->fill = cd_do_fill_default_entry;
	pEntry->execute = _cd_do_zip_folder;
	pEntries = g_list_prepend (pEntries, pEntry);
	
	pEntry = g_new0 (CDEntry, 1);
	pEntry->cPath = g_strdup (cPath);
	pEntry->cName = g_strdup (D_("Move to"));
	pEntry->cIconName = g_strdup (GLDI_ICON_NAME_JUMP_TO);
	pEntry->fill = cd_do_fill_default_entry;
	pEntry->execute = _cd_do_move_file;
	pEntries = g_list_prepend (pEntries, pEntry);
	
	pEntry = g_new0 (CDEntry, 1);
	pEntry->cPath = g_strdup (cPath);
	pEntry->cName = g_strdup (D_("Copy URL"));
	pEntry->cIconName = g_strdup (GLDI_ICON_NAME_COPY);
	pEntry->fill = cd_do_fill_default_entry;
	pEntry->execute = _cd_do_copy_url;
	pEntries = g_list_prepend (pEntries, pEntry);
	
	// on ajoute une entree par fichier du repertoire.
	int iNbFiles = 0;
	const gchar *cFileName;
	do
	{
		cFileName = g_dir_read_name (dir);
		if (cFileName == NULL)
			break ;
		if (bNoHiddenFile && *cFileName == '.')
			continue;
		
		pEntry = g_new0 (CDEntry, 1);
		pEntry->cName = g_strdup (cFileName);
		pEntry->cPath = g_strdup_printf ("%s/%s", cPath, cFileName);
		pEntry->fill = _cd_do_fill_file_entry;
		pEntry->execute = _cd_do_launch_file;
		pEntry->list = _cd_do_list_file_sub_entries;
		pEntries = g_list_prepend (pEntries, pEntry);
		iNbFiles ++;
	}
	while (1);
	g_dir_close (dir);
	
	*iNbEntries = iNbFiles + NB_ACTIONS_ON_FOLDER;
	return pEntries;
}

#define NB_ACTIONS_ON_FILE 5
static GList *_list_actions_on_file (const gchar *cPath, int *iNbEntries)
{
	cd_debug ("%s ()", __func__);
	GList *pEntries = NULL;
	CDEntry *pEntry;
	
	pEntry = g_new0 (CDEntry, 1);
	pEntry->cPath = g_strdup (cPath);
	pEntry->cName = g_strdup (D_("Open location"));
	pEntry->cIconName = g_strdup (GLDI_ICON_NAME_DIRECTORY);
	pEntry->fill = cd_do_fill_default_entry;
	pEntry->execute = _cd_do_show_file_location;
	pEntries = g_list_prepend (pEntries, pEntry);
	
	pEntry = g_new0 (CDEntry, 1);
	pEntry->cPath = g_strdup (cPath);
	pEntry->cName = g_strdup (D_("Zip file"));
	pEntry->cIconName = g_strdup ("zip");
	pEntry->fill = cd_do_fill_default_entry;
	pEntry->execute = _cd_do_zip_file;
	pEntries = g_list_prepend (pEntries, pEntry);
	
	pEntry = g_new0 (CDEntry, 1);
	pEntry->cPath = g_strdup (cPath);
	pEntry->cName = g_strdup (D_("Mail file"));
	pEntry->cIconName = g_strdup ("thunderbird");  /// utiliser le client mail par defaut...
	pEntry->fill = cd_do_fill_default_entry;
	pEntry->execute = _cd_do_mail_file;
	pEntries = g_list_prepend (pEntries, pEntry);
	
	pEntry = g_new0 (CDEntry, 1);
	pEntry->cPath = g_strdup (cPath);
	pEntry->cName = g_strdup (D_("Move to"));
	pEntry->cIconName = g_strdup (GLDI_ICON_NAME_JUMP_TO);
	pEntry->fill = cd_do_fill_default_entry;
	pEntry->execute = _cd_do_move_file;
	pEntries = g_list_prepend (pEntries, pEntry);
	
	pEntry = g_new0 (CDEntry, 1);
	pEntry->cPath = g_strdup (cPath);
	pEntry->cName = g_strdup (D_("Copy URL"));
	pEntry->cIconName = g_strdup (GLDI_ICON_NAME_COPY);
	pEntry->fill = cd_do_fill_default_entry;
	pEntry->execute = _cd_do_copy_url;
	pEntries = g_list_prepend (pEntries, pEntry);
	
	*iNbEntries = NB_ACTIONS_ON_FILE;
	return pEntries;
}

static GList *_cd_do_list_file_sub_entries (CDEntry *pEntry, int *iNbEntries)
{
	cd_debug ("%s (%s)", __func__, pEntry->cPath);
	if (pEntry->cPath == NULL)  // on est deja en bout de chaine.
		return NULL;
	if (g_file_test (pEntry->cPath, G_FILE_TEST_IS_DIR))  // on liste les fichiers du repertoire et les actions sur le repertoire.
	{
		return _list_folder (pEntry->cPath, TRUE, iNbEntries);  // TRUE <=> no hidden files.
	}
	else  // on liste les actions sur le fichier.
	{
		return _list_actions_on_file (pEntry->cPath, iNbEntries);
	}
}



  ////////////////
 // FILL ENTRY //
////////////////

static gboolean _cd_do_fill_main_entry (CDEntry *pEntry)
{
	if (pEntry->cIconName && pEntry->pIconSurface == NULL)
	{
		gchar *cImagePath = g_strconcat (MY_APPLET_SHARE_DATA_DIR, "/", pEntry->cIconName, NULL);
		pEntry->pIconSurface = cairo_dock_create_surface_from_icon (cImagePath,
			myDialogsParam.dialogTextDescription.iSize + 2,
			myDialogsParam.dialogTextDescription.iSize + 2);
		g_free (cImagePath);
		return TRUE;
	}
	return FALSE;
}

static gboolean _cd_do_fill_file_entry (CDEntry *pEntry)
{
	gchar *cName = NULL, *cURI = NULL, *cIconName = NULL;
	gboolean bIsDirectory;
	int iVolumeID;
	double fOrder;
	cairo_dock_fm_get_file_info (pEntry->cPath, &cName, &cURI, &cIconName, &bIsDirectory, &iVolumeID, &fOrder, 0);
	g_free (cName);
	g_free (cURI);
	if (cIconName != NULL && pEntry->pIconSurface == NULL)
	{
		pEntry->pIconSurface = cairo_dock_create_surface_from_icon (cIconName,
			myDialogsParam.dialogTextDescription.iSize,
			myDialogsParam.dialogTextDescription.iSize);
		g_free (cIconName);
		return TRUE;
	}
	return FALSE;
}


  /////////////
 // ACTIONS //
/////////////

static void _cd_do_launch_file (CDEntry *pEntry)
{
	cd_debug ("%s (%s)", __func__, pEntry->cPath);
	cairo_dock_fm_launch_uri (pEntry->cPath);
}

static void _cd_do_show_file_location (CDEntry *pEntry)
{
	cd_debug ("%s (%s)", __func__, pEntry->cPath);
	gchar *cPathUp = g_path_get_dirname (pEntry->cPath);
	g_return_if_fail (cPathUp != NULL);
	cairo_dock_fm_launch_uri (cPathUp);
	g_free (cPathUp);
}

static void _cd_do_open_terminal_here (CDEntry *pEntry)
{
	cd_debug ("%s (%s)", __func__, pEntry->cPath);
	gchar *cCommand = NULL;
	if (g_iDesktopEnv == CAIRO_DOCK_GNOME)
		cCommand = g_strdup_printf ("gnome-terminal --working-directory=\"%s\"", pEntry->cPath);
	else if (g_iDesktopEnv == CAIRO_DOCK_KDE)
		cCommand = g_strdup_printf ("konsole --workdir \"%s\"", pEntry->cPath);
	
	if (cCommand != NULL)
	{
		cairo_dock_launch_command (cCommand);
		g_free (cCommand);
	}
}

static void _cd_do_zip_folder (CDEntry *pEntry)
{
	cd_debug ("%s (%s)", __func__, pEntry->cPath);
	gchar *cCommand = g_strdup_printf ("tar cfz '%s.tar.gz' '%s'", pEntry->cPath, pEntry->cPath);
	cairo_dock_launch_command (cCommand);
	g_free (cCommand);
}

static void _cd_do_zip_file (CDEntry *pEntry)
{
	cd_debug ("%s (%s)", __func__, pEntry->cPath);
	gchar *cCommand = g_strdup_printf ("zip '%s.zip' '%s'", pEntry->cPath, pEntry->cPath);
	cairo_dock_launch_command (cCommand);
	g_free (cCommand);
}

static void _cd_do_mail_file (CDEntry *pEntry)
{
	cd_debug ("%s (%s)", __func__, pEntry->cPath);
	gchar *cURI = g_filename_to_uri (pEntry->cPath, NULL, NULL);
	gchar *cCommand = g_strdup_printf ("thunderbird -compose \"attachment=%s\"", cURI);  /// prendre aussi en compte les autres clients mail, et utiliser celui par defaut...
	cairo_dock_launch_command (cCommand);
	g_free (cCommand);
	g_free (cURI);
}

static void _cd_do_move_file (CDEntry *pEntry)
{
	cd_debug ("%s (%s)", __func__, pEntry->cPath);
	GtkWidget* pFileChooserDialog = gtk_file_chooser_dialog_new (
		D_("Pick up a directory"),
		GTK_WINDOW (g_pMainDock->container.pWidget),
		GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		_("Ok"),
		GTK_RESPONSE_OK,
		_("Cancel"),
		GTK_RESPONSE_CANCEL,
		NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (pFileChooserDialog), FALSE);
	
	gtk_widget_show (pFileChooserDialog);
	int answer = gtk_dialog_run (GTK_DIALOG (pFileChooserDialog));
	if (answer == GTK_RESPONSE_OK)
	{
		gchar *cDirPath = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (pFileChooserDialog));
		
		gchar *cFileName = g_path_get_basename (pEntry->cPath);
		gchar *cNewFilePath = g_strdup_printf ("%s/%s", cDirPath, cFileName);
		g_return_if_fail (! g_file_test (cNewFilePath, G_FILE_TEST_EXISTS));
		g_free (cFileName);
		g_free (cNewFilePath);
		
		gchar *cCommand = g_strdup_printf ("mv '%s' '%s'", pEntry->cPath, cDirPath);
		cairo_dock_launch_command (cCommand);
		g_free (cCommand);
	}
	gtk_widget_destroy (pFileChooserDialog);
}

static void _cd_do_copy_url (CDEntry *pEntry)
{
	cd_debug ("%s (%s)", __func__, pEntry->cPath);
	GtkClipboard *pClipBoard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text (pClipBoard, pEntry->cPath, -1);
}


  ////////////
 // SEARCH //
////////////

static gchar *_locate_files (const char *text, int iFilter, gint iLimit)
{
	GString *sCommand = g_string_new ("locate");
	g_string_append_printf (sCommand, " -d '%s/ScoobyDo/ScoobyDo.db'", g_cCairoDockDataDir);
	if (iLimit > 0)
		g_string_append_printf (sCommand, " --limit=%d", iLimit);
	if (! (iFilter & DO_MATCH_CASE))
		g_string_append (sCommand, " -i");
	if (*text != '/')
		g_string_append (sCommand, " -b");
	
	if (iFilter == DO_FILTER_NONE || iFilter == DO_MATCH_CASE)
	{
		g_string_append_printf (sCommand, " \"%s\"", text);
	}
	else
	{
		if (iFilter & DO_TYPE_MUSIC)
		{
			g_string_append_printf (sCommand, " \"*%s*.mp3\" \"*%s*.ogg\" \"*%s*.wav\"", text, text, text);
		}
		if (iFilter & DO_TYPE_IMAGE)
		{
			g_string_append_printf (sCommand, " \"*%s*.jpg\" \"*%s*.jpeg\" \"*%s*.png\"", text, text, text);
		}
		if (iFilter & DO_TYPE_VIDEO)
		{
			g_string_append_printf (sCommand, " \"*%s*.avi\" \"*%s*.mkv\" \"*%s*.ogv\" \"*%s*.wmv\" \"*%s*.mov\"", text, text, text, text, text);
		}
		if (iFilter & DO_TYPE_TEXT)
		{
			g_string_append_printf (sCommand, " \"*%s*.txt\" \"*%s*.odt\" \"*%s*.doc\"", text, text, text);
		}
		if (iFilter & DO_TYPE_HTML)
		{
			g_string_append_printf (sCommand, " \"*%s*.html\" \"*%s*.htm\"", text, text);
		}
		if (iFilter & DO_TYPE_SOURCE)
		{
			g_string_append_printf (sCommand, " \"*%s*.[ch]\" \"*%s*.cpp\"", text, text);
		}
	}
	
	cd_debug (">>> %s", sCommand->str);
	gchar *cResult = cairo_dock_launch_command_sync (sCommand->str);
	if (cResult == NULL || *cResult == '\0')
	{
		g_free (cResult);
		return NULL;
	}
	
	return cResult;
}
static GList * _build_entries (gchar *cResult, int *iNbEntries)
{
	gchar **pMatchingFiles = g_strsplit (cResult, "\n", 0);
	
	GList *pEntries = NULL;
	CDEntry *pEntry;
	int i;
	for (i = 0; pMatchingFiles[i] != NULL; i ++)
	{
		pEntry = g_new0 (CDEntry, 1);
		pEntry->cPath = pMatchingFiles[i];
		pEntry->cName = g_path_get_basename (pEntry->cPath);
		pEntry->fill = _cd_do_fill_file_entry;
		pEntry->execute = _cd_do_launch_file;
		pEntry->list = _cd_do_list_file_sub_entries;
		pEntries = g_list_prepend (pEntries, pEntry);
	}
	g_free (pMatchingFiles);  // ses elements sont dans les entrees.
	
	cd_debug ("%d entries built", i);
	*iNbEntries = i;
	return pEntries;
}
static GList* search (const gchar *cText, int iFilter, gboolean bSearchAll, int *iNbEntries)
{
	cd_debug ("%s (%s)", __func__, cText);
	gchar *cResult = _locate_files (cText, iFilter, (bSearchAll ? 50 : 3));
	
	if (cResult == NULL)
	{
		*iNbEntries = 0;
		return NULL;
	}
	GList *pEntries = _build_entries (cResult, iNbEntries);
	g_free (cResult);
	
	if (!bSearchAll && pEntries != NULL)
	{
		CDEntry *pEntry = g_new0 (CDEntry, 1);
		pEntry->cPath = g_strdup ("Files");
		pEntry->cName = g_strdup (D_("Files"));
		pEntry->cIconName = g_strdup ("files.png");
		pEntry->bMainEntry = TRUE;
		pEntry->fill = _cd_do_fill_main_entry;
		pEntry->list = cd_do_list_main_sub_entry;
		pEntries = g_list_prepend (pEntries, pEntry);
		*iNbEntries = (*iNbEntries + 1);
	}
	
	return pEntries;
}


  //////////////
 // REGISTER //
//////////////

void cd_do_register_files_backend (void)
{
	CDBackend *pBackend = g_new0 (CDBackend, 1);
	pBackend->cName = "Files";
	pBackend->bIsThreaded = TRUE;
	pBackend->init =(CDBackendInitFunc) init;
	pBackend->search = (CDBackendSearchFunc) search;
	myData.pBackends = g_list_prepend (myData.pBackends, pBackend);
}
