/**********************************************************************************

This file is a part of the cairo-dock clock applet,
released under the terms of the GNU General Public License.

Written by Fabrice Rey (for any bug report, please mail me to fabounet_03@yahoo.fr)

**********************************************************************************/
#include "stdlib.h"

#include "applet-struct.h"
#include "applet-draw.h"
#include "applet-config.h"
#include "applet-notifications.h"
#include "applet-init.h"


CDClockDatePosition my_iShowDate;
gboolean my_bShowSeconds;
gboolean my_b24Mode;
gboolean my_bOldStyle;
double my_fTextColor[4];

int my_iSidUpdateClock = 0;
gchar *my_cThemePath = NULL;

cairo_surface_t* my_pBackgroundSurface = NULL;
cairo_surface_t* my_pForegroundSurface = NULL;
RsvgDimensionData my_DimensionData;
RsvgHandle *my_pSvgHandles[CLOCK_ELEMENTS];

GPtrArray *my_pAlarms = NULL;
gchar *my_cSetupTimeCommand = NULL;

char my_cFileNames[CLOCK_ELEMENTS][30] = {
	"clock-drop-shadow.svg",
	"clock-face.svg",
	"clock-marks.svg",
	"clock-hour-hand-shadow.svg",
	"clock-minute-hand-shadow.svg",
	"clock-second-hand-shadow.svg",
	"clock-hour-hand.svg",
	"clock-minute-hand.svg",
	"clock-second-hand.svg",
	"clock-face-shadow.svg",
	"clock-glass.svg",
	"clock-frame.svg" };


CD_APPLET_DEFINITION ("clock", 1, 4, 7)


CD_APPLET_INIT_BEGIN (erreur)
	//\_______________ On charge le theme choisi (on n'a pas besoin de connaitre les dimmensions de l'icone).
	if (my_cThemePath != NULL)
	{
		GString *sElementPath = g_string_new ("");
		int i;
		for (i = 0; i < CLOCK_ELEMENTS; i ++)
		{
			g_string_printf (sElementPath, "%s/%s", my_cThemePath, my_cFileNames[i]);

			my_pSvgHandles[i] = rsvg_handle_new_from_file (sElementPath->str, NULL);
			//g_print (" + %s\n", cElementPath);
		}
		g_string_free (sElementPath, TRUE);
		rsvg_handle_get_dimensions (my_pSvgHandles[CLOCK_DROP_SHADOW], &my_DimensionData);
	}
	else
	{
		my_DimensionData.width = 48;  // valeur par defaut si aucun theme.
		my_DimensionData.height = 48;
	}

	//\_______________ On construit les surfaces d'arriere-plan et d'avant-plan une bonne fois pour toutes.
	my_pBackgroundSurface = update_surface (NULL,
		myDrawContext,
		myIcon->fWidth * (1 + g_fAmplitude),
		myIcon->fHeight * (1 + g_fAmplitude),
		KIND_BACKGROUND);
	my_pForegroundSurface = update_surface (NULL,
		myDrawContext,
		myIcon->fWidth * (1 + g_fAmplitude),
		myIcon->fHeight * (1 + g_fAmplitude),
		KIND_FOREGROUND);

	//\_______________ On enregistre nos notifications.
	CD_APPLET_REGISTER_FOR_CLICK_EVENT
	CD_APPLET_REGISTER_FOR_BUILD_MENU_EVENT

	//\_______________ On lance le timer.
	cd_clock_update_with_time (myIcon);
	my_iSidUpdateClock = g_timeout_add ((my_bShowSeconds ? 1e3: 60e3), (GSourceFunc) cd_clock_update_with_time, (gpointer) myIcon);
CD_APPLET_INIT_END

CD_APPLET_CONFIGURE_BEGIN
CD_APPLET_CONFIGURE_END


CD_APPLET_STOP_BEGIN
	//\_______________ On se desabonne de nos notifications.
	CD_APPLET_UNREGISTER_FOR_CLICK_EVENT
	CD_APPLET_UNREGISTER_FOR_BUILD_MENU_EVENT

	//\_______________ On stoppe le timer.
	g_source_remove (my_iSidUpdateClock);
	my_iSidUpdateClock = 0;

	//\_______________ On libere toutes nos ressources.
	int i;
	for (i = 0; i < CLOCK_ELEMENTS; i ++)
	{
		rsvg_handle_free (my_pSvgHandles[i]);
		my_pSvgHandles[i] = NULL;
	}

	cairo_surface_destroy (my_pForegroundSurface);
	my_pForegroundSurface = NULL;
	cairo_surface_destroy (my_pBackgroundSurface);
	my_pBackgroundSurface = NULL;

	g_free (my_cThemePath);
	my_cThemePath = NULL;

	CDClockAlarm *pAlarm;
	for (i = 0; i < my_pAlarms->len; i ++)
	{
		pAlarm = g_ptr_array_index (my_pAlarms, i);
		cd_clock_free_alarm (pAlarm);
	}
	g_ptr_array_free (my_pAlarms, TRUE);
	my_pAlarms = NULL;

	g_free (my_cSetupTimeCommand);
	my_cSetupTimeCommand = NULL;
CD_APPLET_STOP_END
