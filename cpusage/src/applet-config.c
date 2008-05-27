
#include <string.h>
#include <cairo-dock.h>

#include "applet-struct.h"
#include "applet-config.h"

CD_APPLET_INCLUDE_MY_VARS

CD_APPLET_GET_CONFIG_BEGIN
	//\_________________ On recupere toutes les valeurs de notre fichier de conf.
	myConfig.defaultTitle = CD_CONFIG_GET_STRING ("Icon", "name");
	myConfig.iCheckInterval = CD_CONFIG_GET_INTEGER ("Configuration", "delay");
	
	myConfig.iInfoDisplay = CD_CONFIG_GET_INTEGER ("Configuration", "info display");
	//On charge le theme :
	myConfig.cThemePath = cairo_dock_get_gauge_key_value(CD_APPLET_MY_CONF_FILE, pKeyFile, "Configuration", "theme", &bFlushConfFileNeeded, "turbo-night-fuel");
	cd_debug ("gauge (cpusage) : Theme(%s)",myConfig.cThemePath);
	
	myConfig.iNbDisplayedProcesses = CD_CONFIG_GET_INTEGER ("Configuration", "top");
	myConfig.iProcessCheckInterval = CD_CONFIG_GET_INTEGER ("Configuration", "top delay");
	
	myConfig.pTopTextDescription = cairo_dock_duplicate_label_description (&g_dialogTextDescription);
	CD_CONFIG_GET_COLOR_RVB ("Configuration", "top color start", myConfig.pTopTextDescription->fColorStart);
	CD_CONFIG_GET_COLOR_RVB ("Configuration", "top color stop", myConfig.pTopTextDescription->fColorStop);
	myConfig.pTopTextDescription->bVerticalPattern = TRUE;
CD_APPLET_GET_CONFIG_END


CD_APPLET_RESET_CONFIG_BEGIN
	g_free (myConfig.defaultTitle);
	g_free (myConfig.cThemePath);
CD_APPLET_RESET_CONFIG_END


CD_APPLET_RESET_DATA_BEGIN
	cairo_dock_free_measure_timer (myData.pMeasureTimer);
	g_timer_destroy (myData.pClock);
	
	free_cd_Gauge(myData.pGauge);
	
	cairo_dock_free_measure_timer (myData.pTopMeasureTimer);
	g_timer_destroy (myData.pTopClock);
	g_free (myData.pTopList);
	g_hash_table_destroy (myData.pProcessTable);
	cairo_surface_destroy (myData.pTopSurface);
CD_APPLET_RESET_DATA_END

