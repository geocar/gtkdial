/*
 * gtkdial - desktop dialer addon for gtk
 * copyright (c) 2010 Geo Carncross
 * URL: http://github.com/geocar/gtkdial
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gmodule.h>
#include <pcre.h>
#include <string.h>
#include <stdio.h>

static gchar *last_phone_number = 0;
static pcre *regexp_phone;
static pcre_extra *regexp_phone_extra;

static int gtkdial_make_canonical(char *out, const char *in, int inlen)
{
	int c, n=0;

#define ADD(x) do{c=(x);if((!n&&c=='+')||(c>='0'&&c<='9')){n++;if(out){*out=(c);out++;}}}while(0)
	if(*in=='+') {
		while(inlen){ADD(*in);in++;inlen--;}
	} else if (*in == '0' && in[1] == '1' && in[2] == '1') {
		ADD('+');in+=3;inlen-=3;while(inlen > 0){ADD(*in);in++;inlen--;}
	} else if (*in == '1') {
		ADD('+');while(inlen){ADD(*in);in++;inlen--;}
	} else { 
		int i,len = 0;for(i=0;in[i];i++)if(in[i] >= '0' && in[i] <= '9')len++;
		if (len == 10) {
			/* assume usa */
			ADD('+');ADD('1');
			while(inlen){ADD(*in);in++;inlen--;}
		}
	}
	return n;
#undef ADD
}

static void gtkdial_doit(GtkMenuItem *item, gpointer ignored)
{
	gchar *argv[3] = { "dial_helper", last_phone_number, NULL };
	g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
}

static gboolean gtkdial_signal_emission_hook(GSignalInvocationHint *ihint,
		guint n_param_values,
		const GValue *param_values,
		gpointer data)
{
	GObject *p = G_OBJECT(g_value_peek_pointer(param_values+0));
	GtkMenu *m = GTK_MENU(g_value_peek_pointer(param_values+1));
	GtkClipboard *c = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	GObject *o = gtk_clipboard_get_owner(c);
	GtkTextBuffer *b = GTK_IS_TEXT_VIEW(p) ? gtk_text_view_get_buffer(GTK_TEXT_VIEW(p)) : NULL;
	const gchar *text = NULL;
	int out[3];

	if (o && (p == o || (gpointer)b == o)) {
		/* text is local; should be fast */
		text = gtk_clipboard_wait_for_text(c);
	} else if (GTK_IS_ENTRY(p)) {
		text = gtk_entry_get_text(GTK_ENTRY(p));
	} else if (GTK_IS_LABEL(p)) {
		text = gtk_label_get_label(GTK_LABEL(p));
	}

	if (text != NULL && pcre_exec(regexp_phone, regexp_phone_extra, text, strlen(text),
						0, 0, out, 3) && out[1] > out[0]) {
		gchar *label;
		int need, len = out[1] - out[0];
		const char *str = text + out[0];
	
		free(last_phone_number);
		last_phone_number = NULL;
		if ((need = gtkdial_make_canonical(NULL, str, len)) > 1) {
			last_phone_number = malloc(1 + need);
			gtkdial_make_canonical(last_phone_number, str, len);
			last_phone_number[need] = '\0';
		}

		if (last_phone_number != NULL && asprintf(&label, "Dial: %s", last_phone_number)) {
			GtkIconSet *icon_set = gtk_style_lookup_icon_set(gtk_widget_get_style(GTK_WIDGET(m)),
						"call-start");
			GtkImageMenuItem *item;
			GtkImage *icon;

			if (!icon_set) {
				icon = GTK_IMAGE(gtk_image_new_from_file("/usr/share/icons/gnome-human/16x16/actions/call-start.png"));
			} else {
				icon = GTK_IMAGE(gtk_image_new_from_icon_set(icon_set, GTK_ICON_SIZE_MENU));
			}

			item = GTK_IMAGE_MENU_ITEM(gtk_image_menu_item_new());
			gtk_image_menu_item_set_image(item, GTK_WIDGET(icon));
			gtk_menu_item_set_label(GTK_MENU_ITEM(item), label);
			free(label);

			g_signal_connect(G_OBJECT(item), "activate", (void*)gtkdial_doit, NULL);
			gtk_widget_show(GTK_WIDGET(item));
			gtk_menu_prepend(m, GTK_WIDGET(item));
		}
	}

	return TRUE;
}
G_MODULE_EXPORT int gtk_module_init(gint argc, char *argv[])
{
	const char *errptr = 0;
	int offset = 0;

	regexp_phone = pcre_compile("\\+?[0-9\\-\\.\\(\\)\\s]{0,12}\\d{3}[0-9\\-\\.\\(\\)\\s]{0,12}",
			0, &errptr,&offset,NULL);
	if(!regexp_phone) abort();
	regexp_phone_extra = pcre_study(regexp_phone, 0, &errptr);

#define ATTACH(f) \
	g_type_class_ref(f); g_signal_add_emission_hook(g_signal_lookup("populate-popup", f), \
			0, gtkdial_signal_emission_hook, NULL, NULL)
	ATTACH(GTK_TYPE_ENTRY);
	ATTACH(GTK_TYPE_LABEL);
	ATTACH(GTK_TYPE_TEXT_VIEW);
#undef ATTACH
	return TRUE;
}
