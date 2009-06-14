#include <stdlib.h>
#include <string.h>

#include "applet-struct.h"
#include "applet-cpusage.h"
#include "applet-rame.h"
#include "applet-top.h"
#include "applet-notifications.h"


CD_APPLET_ON_CLICK_BEGIN
	if (myData.bAcquisitionOK)
	{
		if (myData.pTopDialog != NULL)
			cd_sysmonitor_stop_top_dialog (myApplet);
		else
			cd_sysmonitor_start_top_dialog (myApplet);
	}
	else
		cairo_dock_show_temporary_dialog(D_("Data acquisition has failed"), myIcon, myContainer, 3e3);
CD_APPLET_ON_CLICK_END


#define _convert_from_kb(m) (int) (((m >> 20) == 0) ? (m >> 10) : (m >> 20))
#define _unit(m) (((m >> 20) == 0) ? D_("Mb") : D_("Gb"))
CD_APPLET_ON_MIDDLE_CLICK_BEGIN
	if (myData.bAcquisitionOK)
	{
		/// afficher : utilisation de chaque coeur, nbre de processus en cours.
		if (myData.pTopDialog != NULL || cairo_dock_remove_dialog_if_any (myIcon))
			return CAIRO_DOCK_INTERCEPT_NOTIFICATION;
		
		gchar *cUpTime = NULL, *cActivityTime = NULL;
		cd_sysmonitor_get_uptime (&cUpTime, &cActivityTime);
		if (!myConfig.bShowRam && ! myConfig.bShowSwap)
			cd_sysmonitor_get_ram_data (myApplet);  // le thread ne passe pas par la.
		cairo_dock_show_temporary_dialog_with_icon ("%s : %s\n%s : %d MHz (%d %s)\n%s : %s / %s : %s\n%s : %d%s - %s : %d%s\n %s : %d%s - %s : %d%s",
			myIcon, myContainer, 10e3,
			MY_APPLET_SHARE_DATA_DIR"/"MY_APPLET_ICON_FILE,
			D_("Model Name"), myData.cModelName,
			D_("Frequency"), myData.iFrequency,
			myData.iNbCPU, D_("core(s)"),
			D_("Up time"), cUpTime,
			D_("Activity time"), cActivityTime,
			D_("Memory"), _convert_from_kb (myData.ramTotal), _unit (myData.ramTotal),
			D_("Free"), _convert_from_kb (myData.ramFree), _unit (myData.ramFree),
			D_("Cached"), _convert_from_kb (myData.ramCached), _unit (myData.ramCached),
			D_("Buffers"), _convert_from_kb (myData.ramBuffers), _unit (myData.ramBuffers));
		g_free (cUpTime);
		g_free (cActivityTime);
	}
	else
		cairo_dock_show_temporary_dialog(D_("Data acquisition has failed"), myIcon, myContainer, 4e3);
CD_APPLET_ON_MIDDLE_CLICK_END

static void _show_monitor_system (GtkMenuItem *menu_item, CairoDockModuleInstance *myApplet)
{
	if (myConfig.cSystemMonitorCommand != NULL)
	{
		cairo_dock_launch_command (myConfig.cSystemMonitorCommand);
	}
	else if (g_iDesktopEnv == CAIRO_DOCK_KDE)
	{
		system ("kde-system-monitor");
	}
	else
	{
		cairo_dock_fm_show_system_monitor ();
	}
}
CD_APPLET_ON_BUILD_MENU_BEGIN
	GtkWidget *pSubMenu = CD_APPLET_CREATE_MY_SUB_MENU ();
		CD_APPLET_ADD_IN_MENU (D_("Monitor System"), _show_monitor_system, pSubMenu);
		CD_APPLET_ADD_ABOUT_IN_MENU (pSubMenu);
CD_APPLET_ON_BUILD_MENU_END