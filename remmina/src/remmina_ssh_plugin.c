/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2017 Antenore Gatta, Giovanni Panozzo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU General Public License in all respects
 *  for all of the code used other than OpenSSL. *  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so. *  If you
 *  do not wish to do so, delete this exception statement from your
 *  version. *  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 *
 */

#include "config.h"
#include "remmina/remmina_trace_calls.h"

#if defined (HAVE_LIBSSH) && defined (HAVE_LIBVTE)

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <vte/vte.h>
#include <locale.h>
#include <langinfo.h>
#include "remmina_public.h"
#include "remmina_plugin_manager.h"
#include "remmina_ssh.h"
#include "remmina_protocol_widget.h"
#include "remmina_pref.h"
#include "remmina_ssh_plugin.h"
#include "remmina_masterthread_exec.h"

#define REMMINA_PLUGIN_SSH_FEATURE_TOOL_COPY  1
#define REMMINA_PLUGIN_SSH_FEATURE_TOOL_PASTE 2
#define REMMINA_PLUGIN_SSH_FEATURE_TOOL_SELECT_ALL 3

#define GET_PLUGIN_DATA(gp) (RemminaPluginSshData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

/* Palette colors taken from sakura */
#define PALETTE_SIZE 16

/* 16 color palettes in GdkRGBA format (red, green, blue, alpha)
 * Text displayed in the first 8 colors (0-7) is meek (uses thin strokes).
 * Text displayed in the second 8 colors (8-15) is bold (uses thick strokes). */

const GdkRGBA gruvbox_palette[PALETTE_SIZE] = {
        {0.156863, 0.156863, 0.156863, 1.000000},
        {0.800000, 0.141176, 0.113725, 1.000000},
        {0.596078, 0.592157, 0.101961, 1.000000},
        {0.843137, 0.600000, 0.129412, 1.000000},
        {0.270588, 0.521569, 0.533333, 1.000000},
        {0.694118, 0.384314, 0.525490, 1.000000},
        {0.407843, 0.615686, 0.415686, 1.000000},
        {0.658824, 0.600000, 0.517647, 1.000000},
        {0.572549, 0.513725, 0.454902, 1.000000},
        {0.984314, 0.286275, 0.203922, 1.000000},
        {0.721569, 0.733333, 0.149020, 1.000000},
        {0.980392, 0.741176, 0.184314, 1.000000},
        {0.513725, 0.647059, 0.596078, 1.000000},
        {0.827451, 0.525490, 0.607843, 1.000000},
        {0.556863, 0.752941, 0.486275, 1.000000},
        {0.921569, 0.858824, 0.698039, 1.000000},
};

const GdkRGBA tango_palette[PALETTE_SIZE] = {
	{0,        0,        0,        1},
	{0.8,      0,        0,        1},
	{0.305882, 0.603922, 0.023529, 1},
	{0.768627, 0.627451, 0,        1},
	{0.203922, 0.396078, 0.643137, 1},
	{0.458824, 0.313725, 0.482353, 1},
	{0.0235294,0.596078, 0.603922, 1},
	{0.827451, 0.843137, 0.811765, 1},
	{0.333333, 0.341176, 0.32549,  1},
	{0.937255, 0.160784, 0.160784, 1},
	{0.541176, 0.886275, 0.203922, 1},
	{0.988235, 0.913725, 0.309804, 1},
	{0.447059, 0.623529, 0.811765, 1},
	{0.678431, 0.498039, 0.658824, 1},
	{0.203922, 0.886275, 0.886275, 1},
	{0.933333, 0.933333, 0.92549,  1}
};

const GdkRGBA linux_palette[PALETTE_SIZE] = {
	{0,        0,        0,        1},
	{0.666667, 0,        0,        1},
	{0,        0.666667, 0,        1},
	{0.666667, 0.333333, 0,        1},
	{0,        0,        0.666667, 1},
	{0.666667, 0,        0.666667, 1},
	{0,        0.666667, 0.666667, 1},
	{0.666667, 0.666667, 0.666667, 1},
	{0.333333, 0.333333, 0.333333, 1},
	{1,        0.333333, 0.333333, 1},
	{0.333333, 1,        0.333333, 1},
	{1,        1,        0.333333, 1},
	{0.333333, 0.333333, 1,        1},
	{1,        0.333333, 1,        1},
	{0.333333, 1,        1,        1},
	{1,        1,        1,        1}
};

const GdkRGBA solarized_dark_palette[PALETTE_SIZE] = {
	{0.027451, 0.211765, 0.258824, 1},
	{0.862745, 0.196078, 0.184314, 1},
	{0.521569, 0.600000, 0.000000, 1},
	{0.709804, 0.537255, 0.000000, 1},
	{0.149020, 0.545098, 0.823529, 1},
	{0.827451, 0.211765, 0.509804, 1},
	{0.164706, 0.631373, 0.596078, 1},
	{0.933333, 0.909804, 0.835294, 1},
	{0.000000, 0.168627, 0.211765, 1},
	{0.796078, 0.294118, 0.086275, 1},
	{0.345098, 0.431373, 0.458824, 1},
	{0.396078, 0.482353, 0.513725, 1},
	{0.513725, 0.580392, 0.588235, 1},
	{0.423529, 0.443137, 0.768627, 1},
	{0.576471, 0.631373, 0.631373, 1},
	{0.992157, 0.964706, 0.890196, 1}
#if 0
    { 0, 0x0707, 0x3636, 0x4242 }, // 0  base02 black (used as background color)
    { 0, 0xdcdc, 0x3232, 0x2f2f }, // 1  red
    { 0, 0x8585, 0x9999, 0x0000 }, // 2  green
    { 0, 0xb5b5, 0x8989, 0x0000 }, // 3  yellow
    { 0, 0x2626, 0x8b8b, 0xd2d2 }, // 4  blue
    { 0, 0xd3d3, 0x3636, 0x8282 }, // 5  magenta
    { 0, 0x2a2a, 0xa1a1, 0x9898 }, // 6  cyan
    { 0, 0xeeee, 0xe8e8, 0xd5d5 }, // 7  base2 white (used as foreground color)
    { 0, 0x0000, 0x2b2b, 0x3636 }, // 8  base3 bright black
    { 0, 0xcbcb, 0x4b4B, 0x1616 }, // 9  orange
    { 0, 0x5858, 0x6e6e, 0x7575 }, // 10 base01 bright green
    { 0, 0x6565, 0x7b7b, 0x8383 }, // 11 base00 bright yellow
    { 0, 0x8383, 0x9494, 0x9696 }, // 12 base0 brigth blue
    { 0, 0x6c6c, 0x7171, 0xc4c4 }, // 13 violet
    { 0, 0x9393, 0xa1a1, 0xa1a1 }, // 14 base1 cyan
    { 0, 0xfdfd, 0xf6f6, 0xe3e3 }  // 15 base3 white
#endif
};

const GdkRGBA solarized_light_palette[PALETTE_SIZE] = {
	{0.933333, 0.909804, 0.835294, 1},
	{0.862745, 0.196078, 0.184314, 1},
	{0.521569, 0.600000, 0.000000, 1},
	{0.709804, 0.537255, 0.000000, 1},
	{0.149020, 0.545098, 0.823529, 1},
	{0.827451, 0.211765, 0.509804, 1},
	{0.164706, 0.631373, 0.596078, 1},
	{0.027451, 0.211765, 0.258824, 1},
	{0.992157, 0.964706, 0.890196, 1},
	{0.796078, 0.294118, 0.086275, 1},
	{0.576471, 0.631373, 0.631373, 1},
	{0.513725, 0.580392, 0.588235, 1},
	{0.396078, 0.482353, 0.513725, 1},
	{0.423529, 0.443137, 0.768627, 1},
	{0.345098, 0.431373, 0.458824, 1},
	{0.000000, 0.168627, 0.211765, 1}
#if 0
	{ 0, 0xeeee, 0xe8e8, 0xd5d5 }, // 0 S_base2
	{ 0, 0xdcdc, 0x3232, 0x2f2f }, // 1 S_red
	{ 0, 0x8585, 0x9999, 0x0000 }, // 2 S_green
	{ 0, 0xb5b5, 0x8989, 0x0000 }, // 3 S_yellow
	{ 0, 0x2626, 0x8b8b, 0xd2d2 }, // 4 S_blue
	{ 0, 0xd3d3, 0x3636, 0x8282 }, // 5 S_magenta
	{ 0, 0x2a2a, 0xa1a1, 0x9898 }, // 6 S_cyan
	{ 0, 0x0707, 0x3636, 0x4242 }, // 7 S_base02
	{ 0, 0xfdfd, 0xf6f6, 0xe3e3 }, // 8 S_base3
	{ 0, 0xcbcb, 0x4b4B, 0x1616 }, // 9 S_orange
	{ 0, 0x9393, 0xa1a1, 0xa1a1 }, // 10 S_base1
	{ 0, 0x8383, 0x9494, 0x9696 }, // 11 S_base0
	{ 0, 0x6565, 0x7b7b, 0x8383 }, // 12 S_base00
	{ 0, 0x6c6c, 0x7171, 0xc4c4 }, // 13 S_violet
	{ 0, 0x5858, 0x6e6e, 0x7575 }, // 14 S_base01
	{ 0, 0x0000, 0x2b2b, 0x3636 } // 15 S_base03
#endif
};


const GdkRGBA xterm_palette[PALETTE_SIZE] = {
    {0,        0,        0,        1},
    {0.803922, 0,        0,        1},
    {0,        0.803922, 0,        1},
    {0.803922, 0.803922, 0,        1},
    {0.117647, 0.564706, 1,        1},
    {0.803922, 0,        0.803922, 1},
    {0,        0.803922, 0.803922, 1},
    {0.898039, 0.898039, 0.898039, 1},
    {0.298039, 0.298039, 0.298039, 1},
    {1,        0,        0,        1},
    {0,        1,        0,        1},
    {1,        1,        0,        1},
    {0.27451,  0.509804, 0.705882, 1},
    {1,        0,        1,        1},
    {0,        1,        1,        1},
    {1,        1,        1,        1}
};

#if VTE_CHECK_VERSION(0,38,0)
static struct {
	const GdkRGBA *palette;
} remminavte;
#endif

#define DEFAULT_PALETTE "linux_palette"


/***** The SSH plugin implementation *****/
typedef struct _RemminaPluginSshData
{
	RemminaSSHShell *shell;
	GtkWidget *vte;

	const GdkRGBA *palette;

	pthread_t thread;
} RemminaPluginSshData;


static RemminaPluginService *remmina_plugin_service = NULL;

static gpointer
remmina_plugin_ssh_main_thread (gpointer data)
{
	TRACE_CALL("remmina_plugin_ssh_main_thread");
	RemminaProtocolWidget *gp = (RemminaProtocolWidget*) data;
	RemminaPluginSshData *gpdata;
	RemminaFile *remminafile;
	RemminaSSH *ssh;
	RemminaSSHShell *shell = NULL;
	gboolean cont = FALSE;
	gint ret;
	gchar *charset;

	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
	CANCEL_ASYNC

	gpdata = GET_PLUGIN_DATA(gp);

	ssh = g_object_get_data (G_OBJECT(gp), "user-data");
	if (ssh)
	{
		/* Create SSH Shell connection based on existing SSH session */
		shell = remmina_ssh_shell_new_from_ssh (ssh);
		if (remmina_ssh_init_session (REMMINA_SSH (shell)) &&
		        remmina_ssh_auth (REMMINA_SSH (shell), NULL) > 0 &&
		        remmina_ssh_shell_open (shell, (RemminaSSHExitFunc)
		                                remmina_plugin_service->protocol_plugin_close_connection, gp))
		{
			cont = TRUE;
		}
	}
	else
	{
		/* New SSH Shell connection */
		remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);
		remmina_plugin_service->file_set_string (remminafile, "ssh_server",
		        remmina_plugin_service->file_get_string (remminafile, "server"));

		shell = remmina_ssh_shell_new_from_file (remminafile);
		while (1)
		{
			if (!remmina_ssh_init_session (REMMINA_SSH (shell)))
			{
				remmina_plugin_service->protocol_plugin_set_error (gp, "%s", REMMINA_SSH (shell)->error);
				break;
			}

			ret = remmina_ssh_auth_gui (REMMINA_SSH (shell),
			                            REMMINA_INIT_DIALOG (remmina_protocol_widget_get_init_dialog (gp)),
						    remminafile);
			if (ret == 0)
			{
				remmina_plugin_service->protocol_plugin_set_error (gp, "%s", REMMINA_SSH (shell)->error);
			}
			if (ret <= 0) break;

			if (!remmina_ssh_shell_open (shell, (RemminaSSHExitFunc)
			                             remmina_plugin_service->protocol_plugin_close_connection, gp))
			{
				remmina_plugin_service->protocol_plugin_set_error (gp, "%s", REMMINA_SSH (shell)->error);
				break;
			}

			cont = TRUE;
			break;
		}
	}
	if (!cont)
	{
		if (shell) remmina_ssh_shell_free (shell);
		IDLE_ADD ((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, gp);
		return NULL;
	}

	gpdata->shell = shell;

	charset = REMMINA_SSH (shell)->charset;
	remmina_plugin_ssh_vte_terminal_set_encoding_and_pty(VTE_TERMINAL (gpdata->vte), charset, shell->master, shell->slave);
	remmina_plugin_service->protocol_plugin_emit_signal (gp, "connect");

	gpdata->thread = 0;
	return NULL;
}

void remmina_plugin_ssh_vte_terminal_set_encoding_and_pty(VteTerminal *terminal, const char *codeset, int master, int slave)
{
	TRACE_CALL("remmina_plugin_ssh_vte_terminal_set_encoding_and_pty");
	if ( !remmina_masterthread_exec_is_main_thread() )
	{
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
		d->func = FUNC_VTE_TERMINAL_SET_ENCODING_AND_PTY;
		d->p.vte_terminal_set_encoding_and_pty.terminal = terminal;
		d->p.vte_terminal_set_encoding_and_pty.codeset = codeset;
		d->p.vte_terminal_set_encoding_and_pty.master = master;
		remmina_masterthread_exec_and_wait(d);
		g_free(d);
		return;
	}

	setlocale(LC_ALL, "");
	if (codeset && codeset[0] != '\0')
	{
#if VTE_CHECK_VERSION(0,38,0)
		vte_terminal_set_encoding (terminal, codeset, NULL);
#else
		vte_terminal_set_emulation(terminal, "xterm");
		vte_terminal_set_encoding (terminal, codeset);
#endif
	}

	vte_terminal_set_backspace_binding(terminal, VTE_ERASE_ASCII_DELETE);
	vte_terminal_set_delete_binding(terminal, VTE_ERASE_DELETE_SEQUENCE);

#if VTE_CHECK_VERSION(0,38,0)
	/* vte_pty_new_foreig expect master FD, see https://bugzilla.gnome.org/show_bug.cgi?id=765382 */
	vte_terminal_set_pty (terminal, vte_pty_new_foreign_sync(master, NULL, NULL));
#else
	vte_terminal_set_pty (terminal, master);
#endif

}

static gboolean
remmina_plugin_ssh_on_focus_in (GtkWidget *widget, GdkEventFocus *event, RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_ssh_on_focus_in");
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);

	gtk_widget_grab_focus (gpdata->vte);
	return TRUE;
}

static gboolean
remmina_plugin_ssh_on_size_allocate (GtkWidget *widget, GtkAllocation *alloc, RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_ssh_on_size_allocate");
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);
	gint cols, rows;

	if (!gtk_widget_get_mapped (widget)) return FALSE;

		cols = vte_terminal_get_column_count (VTE_TERMINAL (widget));
		rows = vte_terminal_get_row_count (VTE_TERMINAL (widget));

		remmina_ssh_shell_set_size (gpdata->shell, cols, rows);

	return FALSE;
}

static void
remmina_plugin_ssh_set_vte_pref (RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_ssh_set_vte_pref");
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);

	if (remmina_pref.vte_font && remmina_pref.vte_font[0])
	{
#if !VTE_CHECK_VERSION(0,38,0)
		vte_terminal_set_font_from_string (VTE_TERMINAL (gpdata->vte), remmina_pref.vte_font);
#else
		vte_terminal_set_font (VTE_TERMINAL (gpdata->vte),
		                       pango_font_description_from_string (remmina_pref.vte_font));
#endif
	}
	vte_terminal_set_allow_bold (VTE_TERMINAL (gpdata->vte), remmina_pref.vte_allow_bold_text);
	if (remmina_pref.vte_lines > 0)
	{
		vte_terminal_set_scrollback_lines (VTE_TERMINAL (gpdata->vte), remmina_pref.vte_lines);
	}
}

void
remmina_plugin_ssh_vte_select_all (GtkMenuItem *menuitem, gpointer user_data)
{
	vte_terminal_select_all (VTE_TERMINAL (user_data));
	/* TODO: we should add the vte_terminal_unselect_all as well */
}

void
remmina_plugin_ssh_vte_copy_clipboard (GtkMenuItem *menuitem, gpointer user_data)
{
	vte_terminal_copy_clipboard (VTE_TERMINAL (user_data));
}

void
remmina_plugin_ssh_vte_paste_clipboard (GtkMenuItem *menuitem, gpointer user_data)
{
	vte_terminal_paste_clipboard (VTE_TERMINAL (user_data));
}

/* Send a keystroke to the plugin window */
static void remmina_ssh_keystroke(RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen)
{
	TRACE_CALL("remmina_ssh_keystroke");
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);
	remmina_plugin_service->protocol_plugin_send_keys_signals(gpdata->vte,
		keystrokes, keylen, GDK_KEY_PRESS | GDK_KEY_RELEASE);
	return;
}

gboolean
remmina_ssh_plugin_popup_menu(GtkWidget *widget, GdkEvent *event, GtkWidget *menu) {

	if ((event->type == GDK_BUTTON_PRESS) && (((GdkEventButton*)event)->button == 3)) {

		gtk_menu_popup(GTK_MENU(menu),
				NULL,
				NULL,
				NULL,
				NULL,
				((GdkEventButton*)event)->button,
				gtk_get_current_event_time());
		return TRUE;
	}

	return FALSE;
}

void remmina_plugin_ssh_popup_ui(RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_ssh_popup_ui");
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);
	/* Context menu for slection and clipboard */
	GtkWidget *menu = gtk_menu_new();

	GtkWidget *select_all = gtk_menu_item_new_with_label(_("Select All (Host+a)"));
	GtkWidget *copy = gtk_menu_item_new_with_label(_("Copy (Host+c)"));
	GtkWidget *paste = gtk_menu_item_new_with_label(_("Paste (Host+v)"));

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_all);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), copy);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), paste);

	g_signal_connect (G_OBJECT(gpdata->vte), "button_press_event",
			G_CALLBACK(remmina_ssh_plugin_popup_menu), menu);

	g_signal_connect(G_OBJECT(select_all), "activate",
			G_CALLBACK(remmina_plugin_ssh_vte_select_all), gpdata->vte);
	g_signal_connect(G_OBJECT(copy), "activate",
			G_CALLBACK(remmina_plugin_ssh_vte_copy_clipboard), gpdata->vte);
	g_signal_connect(G_OBJECT(paste), "activate",
			G_CALLBACK(remmina_plugin_ssh_vte_paste_clipboard), gpdata->vte);

	gtk_widget_show_all(menu);
}

static void
remmina_plugin_ssh_init (RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_ssh_init");
	RemminaPluginSshData *gpdata;
	GtkWidget *hbox;
	GtkAdjustment *vadjustment;
	GtkWidget *vscrollbar;
	GtkWidget *vte;
	GtkStyleContext *style_context;
	GdkRGBA foreground_color;
	GdkRGBA background_color;
#if !VTE_CHECK_VERSION(0,38,0)
	GdkColor foreground_gdkcolor;
	GdkColor background_gdkcolor;
#endif /* VTE_CHECK_VERSION(0,38,0) */

	gpdata = g_new0 (RemminaPluginSshData, 1);
	g_object_set_data_full (G_OBJECT(gp), "plugin-data", gpdata, g_free);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER (gp), hbox);
	g_signal_connect(G_OBJECT(hbox), "focus-in-event", G_CALLBACK(remmina_plugin_ssh_on_focus_in), gp);

	vte = vte_terminal_new ();
	gtk_widget_show(vte);
	vte_terminal_set_size (VTE_TERMINAL (vte), 80, 25);
	vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL (vte), TRUE);
	if (remmina_pref.vte_system_colors)
	{
		/* Get default system theme colors */
		style_context = gtk_widget_get_style_context(GTK_WIDGET (vte));
		gtk_style_context_get_color(style_context, GTK_STATE_FLAG_NORMAL, &foreground_color);
		gtk_style_context_get(style_context, GTK_STATE_FLAG_NORMAL, "background-color", &background_color, NULL);
	}
	else
	{
		/* Get customized colors */
		gdk_rgba_parse(&foreground_color, remmina_pref.vte_foreground_color);
		gdk_rgba_parse(&background_color, remmina_pref.vte_background_color);
	}
#if VTE_CHECK_VERSION(0,38,0)
	/* Set colors to GdkRGBA */
	remminavte.palette = linux_palette;
	vte_terminal_set_colors (VTE_TERMINAL(vte), &foreground_color, &background_color, remminavte.palette, PALETTE_SIZE);
#else
	/* VTE <= 2.90 doesn't support GdkRGBA so we must convert GdkRGBA to GdkColor */
	foreground_gdkcolor.red = (guint16)(foreground_color.red * 0xFFFF);
	foreground_gdkcolor.green = (guint16)(foreground_color.green * 0xFFFF);
	foreground_gdkcolor.blue = (guint16)(foreground_color.blue * 0xFFFF);
	background_gdkcolor.red = (guint16)(background_color.red * 0xFFFF);
	background_gdkcolor.green = (guint16)(background_color.green * 0xFFFF);
	background_gdkcolor.blue = (guint16)(background_color.blue * 0xFFFF);
	/* Set colors to GdkColor */
	vte_terminal_set_colors (VTE_TERMINAL(vte), &foreground_gdkcolor, &background_gdkcolor, NULL, 0);
#endif

	gtk_box_pack_start (GTK_BOX (hbox), vte, TRUE, TRUE, 0);
	gpdata->vte = vte;
	remmina_plugin_ssh_set_vte_pref (gp);
	g_signal_connect(G_OBJECT(vte), "size-allocate", G_CALLBACK(remmina_plugin_ssh_on_size_allocate), gp);

	remmina_plugin_ssh_on_size_allocate(GTK_WIDGET (vte), NULL, gp);

	remmina_plugin_service->protocol_plugin_register_hostkey (gp, vte);

#if VTE_CHECK_VERSION(0, 28, 0) && GTK_CHECK_VERSION(3, 0, 0)
	vadjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (vte));
#else
	vadjustment = vte_terminal_get_adjustment(VTE_TERMINAL(vc->vte.terminal));
#endif

	vscrollbar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, vadjustment);

	gtk_widget_show(vscrollbar);
	gtk_box_pack_start (GTK_BOX (hbox), vscrollbar, FALSE, TRUE, 0);

	remmina_plugin_ssh_popup_ui(gp);
}

static gboolean
remmina_plugin_ssh_open_connection (RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_ssh_open_connection");
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);

	remmina_plugin_service->protocol_plugin_set_expand (gp, TRUE);
	remmina_plugin_service->protocol_plugin_set_width (gp, 640);
	remmina_plugin_service->protocol_plugin_set_height (gp, 480);

	if (pthread_create (&gpdata->thread, NULL, remmina_plugin_ssh_main_thread, gp))
	{
		remmina_plugin_service->protocol_plugin_set_error (gp,
		        "Failed to initialize pthread. Falling back to non-thread mode...");
		gpdata->thread = 0;
		return FALSE;
	}
	else
	{
		return TRUE;
	}
	return TRUE;
}

static gboolean
remmina_plugin_ssh_close_connection (RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_ssh_close_connection");
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);

	if (gpdata->thread)
	{
		pthread_cancel (gpdata->thread);
		if (gpdata->thread) pthread_join (gpdata->thread, NULL);
	}
	if (gpdata->shell)
	{
		remmina_ssh_shell_free (gpdata->shell);
		gpdata->shell = NULL;
	}

	remmina_plugin_service->protocol_plugin_emit_signal (gp, "disconnect");
	return FALSE;
}

static gboolean
remmina_plugin_ssh_query_feature (RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL("remmina_plugin_ssh_query_feature");
	return TRUE;
}

static void
remmina_plugin_ssh_call_feature (RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL("remmina_plugin_ssh_call_feature");
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);

	switch (feature->id)
	{
	case REMMINA_PROTOCOL_FEATURE_TOOL_SSH:
		remmina_plugin_service->open_connection (
		    remmina_file_dup_temp_protocol (remmina_plugin_service->protocol_plugin_get_file (gp), "SSH"),
		    NULL, gpdata->shell, NULL);
		return;
	case REMMINA_PROTOCOL_FEATURE_TOOL_SFTP:
		remmina_plugin_service->open_connection (
		    remmina_file_dup_temp_protocol (remmina_plugin_service->protocol_plugin_get_file (gp), "SFTP"),
		    NULL, gpdata->shell, NULL);
		return;
	case REMMINA_PLUGIN_SSH_FEATURE_TOOL_COPY:
		vte_terminal_copy_clipboard (VTE_TERMINAL (gpdata->vte));
		return;
	case REMMINA_PLUGIN_SSH_FEATURE_TOOL_PASTE:
		vte_terminal_paste_clipboard (VTE_TERMINAL (gpdata->vte));
		return;
	case REMMINA_PLUGIN_SSH_FEATURE_TOOL_SELECT_ALL:
		vte_terminal_select_all (VTE_TERMINAL (gpdata->vte));
		return;
	}
}

/* Array for available features.
 * The last element of the array must be REMMINA_PROTOCOL_FEATURE_TYPE_END. */
static RemminaProtocolFeature remmina_plugin_ssh_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_SSH_FEATURE_TOOL_COPY, N_("Copy"), N_("_Copy"), NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_SSH_FEATURE_TOOL_PASTE, N_("Paste"), N_("_Paste"), NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_SSH_FEATURE_TOOL_SELECT_ALL, N_("Select all"), N_("_Select all"), NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END, 0, NULL, NULL, NULL }
};

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Unused pointer
 */
static const RemminaProtocolSetting remmina_ssh_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER, "ssh_server", NULL, FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "ssh_username", N_("User name"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, "ssh_password", N_("User password"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};


/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin_ssh =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,                 // Type
	"SSH",                                        // Name
	N_("SSH - Secure Shell"),                     // Description
	GETTEXT_PACKAGE,                              // Translation domain
	VERSION,                                      // Version number
	"utilities-terminal",                         // Icon for normal connection
	"utilities-terminal",                         // Icon for SSH connection
	remmina_ssh_basic_settings,                   // Array for basic settings
	NULL,                                         // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_SSH,             // SSH settings type
	remmina_plugin_ssh_features,                  // Array for available features
	remmina_plugin_ssh_init,                      // Plugin initialization
	remmina_plugin_ssh_open_connection,           // Plugin open connection
	remmina_plugin_ssh_close_connection,          // Plugin close connection
	remmina_plugin_ssh_query_feature,             // Query for available features
	remmina_plugin_ssh_call_feature,              // Call a feature
	remmina_ssh_keystroke                         // Send a keystroke
};

void
remmina_ssh_plugin_register (void)
{
	TRACE_CALL("remmina_ssh_plugin_register");
	remmina_plugin_ssh_features[0].opt3 = GUINT_TO_POINTER (remmina_pref.vte_shortcutkey_copy);
	remmina_plugin_ssh_features[1].opt3 = GUINT_TO_POINTER (remmina_pref.vte_shortcutkey_paste);
	remmina_plugin_ssh_features[2].opt3 = GUINT_TO_POINTER (remmina_pref.vte_shortcutkey_select_all);

	remmina_plugin_service = &remmina_plugin_manager_service;
	remmina_plugin_service->register_plugin ((RemminaPlugin *) &remmina_plugin_ssh);

	ssh_threads_set_callbacks(ssh_threads_get_pthread());
	ssh_init();

}

#else

void remmina_ssh_plugin_register(void)
{
	TRACE_CALL("remmina_ssh_plugin_register");
}

#endif

