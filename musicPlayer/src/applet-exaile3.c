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
#include <stdio.h>
#include <glib/gi18n.h>
#include <cairo-dock.h>

#include "applet-struct.h"
#include "applet-musicplayer.h"
#include "applet-mpris.h"
#include "applet-exaile3.h"

/* On enregistre notre lecteur.
 * Par matttbe
 */
void cd_musicplayer_register_exaile3_handler (void)
{
	MusicPlayerHandeler *pExaile3 = cd_mpris_new_handler ();
	pExaile3->cMprisService = "org.mpris.exaile";
	pExaile3->appclass = "exaile.py";  // les classes sont passees en minuscule par le dock.
	pExaile3->launch = "exaile";
	pExaile3->name = "Exaile 0.3";
	pExaile3->iPlayer = MP_EXAILE3;
	cd_musicplayer_register_my_handler (pExaile3, "Exaile 0.3");
}