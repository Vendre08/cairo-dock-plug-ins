#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#include "applet-struct.h"
#include "applet-notifications.h"
#include "applet-rame.h"
#include "cairo-dock.h"

CD_APPLET_INCLUDE_MY_VARS


#define RAME_DATA_PIPE "/proc/meminfo"
/*#define RAME_TMP_FILE "/tmp/rame"

static int s_iThreadIsRunning = 0;
static int s_iSidTimerRedraw = 0;

gboolean cd_rame_timer (gpointer data) {
	cd_rame_launch_analyse();
	return TRUE;
}

gpointer cd_rame_threaded_calculation (gpointer data) {
	cd_rame_get_data();
	
	g_atomic_int_set (&s_iThreadIsRunning, 0);
	cd_message ("*** fin du thread rame");
	return NULL;
}

void cd_rame_get_data (void) {
	gchar *cCommand = g_strdup_printf("bash %s/rame", MY_APPLET_SHARE_DATA_DIR);
	system (cCommand);
	g_free (cCommand);
}

static gboolean _cd_rame_check_for_redraw (gpointer data) {
	int iThreadIsRunning = g_atomic_int_get (&s_iThreadIsRunning);
	cd_message ("%s (%d)", __func__, iThreadIsRunning);
	if (! iThreadIsRunning) {
		s_iSidTimerRedraw = 0;
		if (myIcon == NULL) {
			g_print ("annulation du chargement de rame (myIcon == NULL)\n");
			return FALSE;
		}
		
		gboolean bResultOK = cd_rame_getRate();
		
		//\_______________________ On lance le timer si necessaire.
		if (myData.iSidTimer == 0) {
			myData.iSidTimer = g_timeout_add (myConfig.iCheckInterval, (GSourceFunc) cd_rame_timer, NULL);
		}
		return FALSE;
	}
	return TRUE;
}

void cd_rame_launch_analyse (void) {
	cd_message (" ");
	if (g_atomic_int_compare_and_exchange (&s_iThreadIsRunning, 0, 1)) {  //il etait egal a 0, on lui met 1 et on lance le thread.
		cd_message (" ==> lancement du thread de calcul");
		
		GError *erreur = NULL;
		GThread* pThread = g_thread_create ((GThreadFunc) cd_rame_threaded_calculation, NULL, FALSE, &erreur);
		if (erreur != NULL) {
			cd_warning ("Attention : %s", erreur->message);
			g_error_free (erreur);
		}
				
		if (s_iSidTimerRedraw == 0) {
			s_iSidTimerRedraw = g_timeout_add (333, (GSourceFunc) _cd_rame_check_for_redraw, (gpointer) NULL);
		}
		
	}
}


gboolean cd_rame_getRate(void) {
  	gchar *cContent = NULL;
	gsize length=0;
	GError *tmp_erreur = NULL;
	g_file_get_contents(RAME_TMP_FILE, &cContent, &length, &tmp_erreur);
	if (tmp_erreur != NULL) {
		cd_message("Attention : %s\n", tmp_erreur->message);
		g_error_free(tmp_erreur);
		CD_APPLET_SET_NAME_FOR_MY_ICON(myConfig.defaultTitle);
		CD_APPLET_SET_QUICK_INFO_ON_MY_ICON_PRINTF("N/A");
		return FALSE;
	}
	else {
		gchar **cInfopipesList = g_strsplit(cContent, "\n", -1);
		g_free(cContent);
		gchar *cOneInfopipe;
		gchar **recup;
		GList *pList = NULL;
		double *pValue;
    		unsigned long long ramTotal = 0,  ramUsed = 0,  ramFree = 0,  ramShared = 0, ramBuffers = 0, ramCached = 0;
   		unsigned long long swapTotal = 0, swapUsed = 0, swapFree = 0;
    		unsigned int memPercent = 0;
    		unsigned int swapPercent = 0;
		// On recupere dans le fichier les donnees sur la rame
		cOneInfopipe = cInfopipesList[0];		
		if(strcmp(cOneInfopipe,"stop") == 0) {
			cd_error("rame -> Error -> Wrong /tmp/rame file >< !\n");	
			return FALSE;
		}
		recup = g_strsplit(cOneInfopipe,">", -1);
		ramTotal = atoll(recup[0]);
		ramUsed = atoll(recup[1]);
		ramFree = atoll(recup[2]);
		ramShared = atoll(recup[3]);
		ramBuffers = atoll(recup[4]);
		ramCached = atoll(recup[5]);
		
		// On recupere dans le fichier les donnees sur la swap		
		cOneInfopipe = cInfopipesList[1];		
		if(strcmp(cOneInfopipe,"stop") == 0) {
			cd_error("rame -> Error -> Wrong /tmp/rame file middle >< !\n");	
			return FALSE;
		}
		
		recup = g_strsplit(cOneInfopipe,">", -1);		
		swapTotal = atoll(recup[0]);
		swapUsed = atoll(recup[1]);
		swapFree  = atoll(recup[2]);
		
		cOneInfopipe = cInfopipesList[2];				
		if(strcmp(cOneInfopipe,"stop") != 0) {
			cd_error("rame -> Error -> Wrong /tmp/rame file end >< !\n");	
			return FALSE;
		}
		if(ramTotal != 0) 
		{
			memPercent = (unsigned int) (((ramUsed - ramCached) * 100) / ramTotal);
		}
		else
		{
			memPercent = 0;
		}
		if(swapTotal != 0)
		{
			swapPercent = (unsigned int) ((swapUsed * 100) / swapTotal);
		}
		else
		{
			swapPercent = 0;
		}
		if(inDebug == 1) 
		{
		cairo_dock_show_temporary_dialog(
				"ramTotal : %llu ,  ramUsed : %llu ,  ramFree : %llu ,  \n ramShared : %llu , ramBuffers : %llu , ramCached : %llu ,\n \
swapTotalatoll : %llu , swapUsedatoll : %llu , swapFreeatoll : %llu",
				myIcon, myContainer, myConfig.iCheckInterval,
				ramTotal,  ramUsed,  ramFree,  ramShared, ramBuffers, ramCached,
				swapTotal, swapUsed, swapFree);
		}
		else
		{
		cd_debug(
				"ramTotal : %llu ,  ramUsed : %llu ,  ramFree : %llu ,  \n ramShared : %llu , ramBuffers : %llu , ramCached : %llu ,\n \
swapTotalatoll : %llu , swapUsedatoll : %llu , swapFreeatoll : %llu",
				ramTotal,  ramUsed,  ramFree,  ramShared, ramBuffers, ramCached,
				swapTotal, swapUsed, swapFree);
		}
		CD_APPLET_SET_QUICK_INFO_ON_MY_ICON_PRINTF("M:%i%%\nS:%i%%", memPercent, swapPercent);
		pValue = g_new (double, 1);
		*pValue = (double) memPercent / 100;
		pList = g_list_append (pList, pValue);
		pValue = g_new (double, 1);
		*pValue = (double) swapPercent / 100;
		pList = g_list_append (pList, pValue);
		//make_cd_Gauge(myDrawContext,myDock,myIcon,myData.pGauge,(double) memPercent / 100);
		make_cd_Gauge_multiValue(myDrawContext,myDock,myIcon,myData.pGauge,pList);		
		CD_APPLET_REDRAW_MY_ICON
		g_strfreev (cInfopipesList);
	}  
	return TRUE;
}*/


#define goto_next_line \
	str = strchr (str, '\n'); \
	if (str == NULL) { \
		myData.bAcquisitionOK = FALSE; \
		return; \
	} \
	str ++;
#define get_value(iValue) \
	str = strchr (str, ':'); \
	if (str == NULL) { \
		myData.bAcquisitionOK = FALSE; \
		g_free (cContent); \
		return; \
	} \
	str ++; \
	while (*str == ' ') \
		str ++; \
	iValue = atoi (str);
void cd_rame_read_data (void)
{
	g_timer_stop (myData.pClock);
	double fTimeElapsed = g_timer_elapsed (myData.pClock, NULL);
	g_return_if_fail (fTimeElapsed != 0);
	g_timer_start (myData.pClock);
	
	gchar *cContent = NULL;
	gsize length=0;
	GError *erreur = NULL;
	g_file_get_contents (RAME_DATA_PIPE, &cContent, &length, &erreur);
	if (erreur != NULL)
	{
		cd_warning("Attention : %s", erreur->message);
		g_error_free(erreur);
		erreur = NULL;
		myData.bAcquisitionOK = FALSE;
	}
	else
	{
		int iNumLine = 1;
		gchar *str = cContent;
		
		get_value (myData.ramTotal)  // MemTotal:
		
		goto_next_line
		get_value (myData.ramFree)  // MemFree:
		
		myData.ramUsed = myData.ramTotal - myData.ramFree;
		
		goto_next_line
		get_value (myData.ramBuffers)  // Buffers:
		
		goto_next_line
		get_value (myData.ramCached)  // Cached:
		
		goto_next_line  // SwapCached:
		goto_next_line  // Active:
		goto_next_line  // Inactive:
		goto_next_line  // HighTotal:
		goto_next_line  // HighFree:
		goto_next_line  // LowTotal:
		goto_next_line  // LowFree:
		
		goto_next_line
		get_value (myData.swapTotal)  // SwapTotal:
		
		goto_next_line
		get_value (myData.swapFree)  // SwapFree:
		
		myData.swapUsed = myData.swapTotal - myData.swapFree;
		
		g_free (cContent);
		myData.bAcquisitionOK = TRUE;
	}
}

void cd_rame_update_from_data (void)
{
	if ( ! myData.bAcquisitionOK)
	{
		if (myConfig.iInfoDisplay == CAIRO_DOCK_INFO_ON_LABEL)
			CD_APPLET_SET_NAME_FOR_MY_ICON (myConfig.defaultTitle)
		else if (myConfig.iInfoDisplay == CAIRO_DOCK_INFO_ON_ICON)
			CD_APPLET_SET_QUICK_INFO_ON_MY_ICON_PRINTF("N/A");
		make_cd_Gauge(myDrawContext,myDock,myIcon,myData.pGauge,(double) 0);
		
		cairo_dock_downgrade_frequency_state (myData.pMeasureTimer);
	}
	else
	{
		cairo_dock_set_normal_frequency_state (myData.pMeasureTimer);
		
		double fRamPercent = 100. * (myData.ramUsed - myData.ramCached) / myData.ramTotal;
		double fSwapPercent = 100. * myData.swapUsed / myData.swapTotal;
		if (myConfig.iInfoDisplay != CAIRO_DOCK_INFO_NONE)
		{
			GString *sInfo = g_string_new ("");
			if (myConfig.iInfoDisplay == CAIRO_DOCK_INFO_ON_LABEL)
				g_string_printf (sInfo, "RAM");
			
			g_string_append_printf (sInfo, (fRamPercent < 10 ? "%.1f%%" : "%.0f%%"), fRamPercent);
			if (myConfig.bShowSwap)
			{
				g_string_append_c (sInfo, '\n');
				if (myConfig.iInfoDisplay == CAIRO_DOCK_INFO_ON_LABEL)
					g_string_append_printf (sInfo, "SWAP");
				g_string_append_printf (sInfo, (fSwapPercent < 10 ? "%.1f%%" : "%.0f%%"), fSwapPercent);
			}
			
			if (myConfig.iInfoDisplay == CAIRO_DOCK_INFO_ON_ICON)
			{
				CD_APPLET_SET_QUICK_INFO_ON_MY_ICON (sInfo->str)
			}
			else
			{
				CD_APPLET_SET_NAME_FOR_MY_ICON (sInfo->str)
			}
			g_string_free (sInfo, TRUE);
		}
		
		if (! myConfig.bShowSwap)
			make_cd_Gauge(myDrawContext,myDock,myIcon,myData.pGauge, fRamPercent / 100);
		else
		{
			GList *pList = NULL;  /// un tableau ca serait plus sympa ...
			double *pValue = g_new (double, 1);
			*pValue = (double) fRamPercent / 100;
			pList = g_list_append (pList, pValue);
			pValue = g_new (double, 1);
			*pValue = (double) fSwapPercent / 100;
			pList = g_list_append (pList, pValue);
			make_cd_Gauge_multiValue(myDrawContext,myDock,myIcon,myData.pGauge,pList);
			g_list_foreach (pList, (GFunc) g_free, NULL);
			g_list_free (pList);
		}
	}
}
