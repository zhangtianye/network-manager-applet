/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* ap-menu-item.c - Class to represent a Wifi access point 
 *
 * Jonathan Blandford <jrb@redhat.com>
 * Dan Williams <dcbw@redhat.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2005 - 2010 Red Hat, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <glib/gi18n.h>
#include <string.h>

#include "mb-menu-item.h"

G_DEFINE_TYPE (NMMbMenuItem, nm_mb_menu_item, GTK_TYPE_IMAGE_MENU_ITEM);

#define NM_MB_MENU_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NM_TYPE_MB_MENU_ITEM, NMMbMenuItemPrivate))

typedef struct {
	GtkWidget *desc;
	char *desc_string;
	GtkWidget *strength;
	guint32    int_strength;
	GtkWidget *detail;
	GtkWidget *hbox;

	gboolean   destroyed;
} NMMbMenuItemPrivate;

static const char *
get_tech_name (guint32 tech)
{
	switch (tech) {
	case MB_TECH_1XRTT:
		return _("CDMA");
	case MB_TECH_EVDO:
		return _("EVDO");
	case MB_TECH_GSM:
		return _("GSM");
	case MB_TECH_GPRS:
		return _("GPRS");
	case MB_TECH_EDGE:
		return _("EDGE");
	case MB_TECH_UMTS:
		return _("UMTS");
	case MB_TECH_HSDPA:
		return _("HSDPA");
	case MB_TECH_HSUPA:
		return _("HSUPA");
	case MB_TECH_HSPA:
		return _("HSPA");
	case MB_TECH_HSPA_PLUS:
		return _("HSPA+");
	case MB_TECH_WIMAX:
		return _("WiMAX");
	case MB_TECH_LTE:
		return _("LTE");
	default:
		break;
	}
	return NULL;
}

GtkWidget *
nm_mb_menu_item_new (const char *connection_name,
                     guint32 strength,
                     const char *provider,
                     gboolean active,
                     guint32 technology,
                     guint32 state,
                     gboolean enabled,
                     NMApplet *applet)
{
	NMMbMenuItem *item;
	NMMbMenuItemPrivate *priv;
	const char *tech_name = NULL;

	item = g_object_new (NM_TYPE_MB_MENU_ITEM, NULL);
	if (!item)
		return NULL;

	priv = NM_MB_MENU_ITEM_GET_PRIVATE (item);
	priv->int_strength = strength;

	/* WiMAX doesn't show tech name */
	if (technology != MB_TECH_WIMAX)
		tech_name = get_tech_name (technology);

	/* Construct the description string */
	switch (state) {
	default:
	case MB_STATE_UNKNOWN:
		priv->desc_string = g_strdup (_("not enabled"));
		break;
	case MB_STATE_IDLE:
		if (connection_name)
			priv->desc_string = g_strdup (connection_name);
		else
			priv->desc_string = g_strdup (_("not registered"));
		break;
	case MB_STATE_HOME:
		if (connection_name) {
			if (provider && tech_name)
				priv->desc_string = g_strdup_printf ("%s (%s %s)", connection_name, provider, tech_name);
			else if (provider || tech_name)
				priv->desc_string = g_strdup_printf ("%s (%s)", connection_name, provider ? provider : tech_name);
			else
				priv->desc_string = g_strdup_printf ("%s", connection_name);
		} else {
			if (provider) {
				if (tech_name)
					priv->desc_string = g_strdup_printf ("%s %s", provider, tech_name);
				else
					priv->desc_string = g_strdup_printf ("%s", provider);
			} else {
				if (tech_name)
					priv->desc_string = g_strdup_printf (_("Home network (%s)"), tech_name);
				else
					priv->desc_string = g_strdup_printf (_("Home network"));
			}
		}
		break;
	case MB_STATE_SEARCHING:
		if (connection_name)
			priv->desc_string = g_strdup (connection_name);
		else
			priv->desc_string = g_strdup (_("searching"));
		break;
	case MB_STATE_DENIED:
		priv->desc_string = g_strdup (_("registration denied"));
		break;
	case MB_STATE_ROAMING:
		if (connection_name) {
			if (tech_name)
				priv->desc_string = g_strdup_printf (_("%s (%s roaming)"), connection_name, tech_name);
			else
				priv->desc_string = g_strdup_printf (_("%s (roaming)"), connection_name);
		} else {
			if (provider) {
				if (tech_name)
					priv->desc_string = g_strdup_printf (_("%s (%s roaming)"), provider, tech_name);
				else
					priv->desc_string = g_strdup_printf (_("%s (roaming)"), provider);
			} else {
				if (tech_name)
					priv->desc_string = g_strdup_printf (_("Roaming network (%s)"), tech_name);
				else
					priv->desc_string = g_strdup_printf (_("Roaming network"));
			}
		}
		break;
	}

	if (enabled && connection_name && active) {
		char *markup;

		gtk_label_set_use_markup (GTK_LABEL (priv->desc), TRUE);
		markup = g_markup_printf_escaped ("<b>%s</b>", priv->desc_string);
		gtk_label_set_markup (GTK_LABEL (priv->desc), markup);
		g_free (markup);
	} else {
		/* Disconnected and disabled states */
		gtk_label_set_use_markup (GTK_LABEL (priv->desc), FALSE);
		gtk_label_set_text (GTK_LABEL (priv->desc), priv->desc_string);
	}

/* Disabling this for indicators only because it won't build otherwise. */
#ifndef ENABLE_INDICATOR
	/* And the strength icon, if we have strength information at all */
	if (enabled && strength) {
		gtk_image_set_from_pixbuf (GTK_IMAGE (priv->strength),
		                           mobile_helper_get_quality_icon (strength, applet));
	}
#endif

	return GTK_WIDGET (item);
}

/*******************************************************/

static void
nm_mb_menu_item_init (NMMbMenuItem *self)
{
	NMMbMenuItemPrivate *priv = NM_MB_MENU_ITEM_GET_PRIVATE (self);

#if GTK_CHECK_VERSION(3,1,6)
        priv->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
#else
	priv->hbox = gtk_hbox_new (FALSE, 6);
#endif
	priv->desc = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->desc), 0.0, 0.5);

	gtk_container_add (GTK_CONTAINER (self), priv->hbox);
	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->desc, TRUE, TRUE, 0);

	priv->strength = gtk_image_new ();
	gtk_box_pack_end (GTK_BOX (priv->hbox), priv->strength, FALSE, TRUE, 0);

	gtk_widget_show (priv->desc);
	gtk_widget_show (priv->strength);
	gtk_widget_show (priv->hbox);
}

static void
dispose (GObject *object)
{
	NMMbMenuItem *self = NM_MB_MENU_ITEM (object);
	NMMbMenuItemPrivate *priv = NM_MB_MENU_ITEM_GET_PRIVATE (self);

	if (priv->destroyed) {
		G_OBJECT_CLASS (nm_mb_menu_item_parent_class)->dispose (object);
		return;
	}
	priv->destroyed = TRUE;

	gtk_widget_destroy (priv->desc);
	gtk_widget_destroy (priv->strength);
	gtk_widget_destroy (priv->hbox);
	g_free (priv->desc_string);

	G_OBJECT_CLASS (nm_mb_menu_item_parent_class)->dispose (object);
}

static void
nm_mb_menu_item_class_init (NMMbMenuItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (NMMbMenuItemPrivate));

	/* virtual methods */
	object_class->dispose = dispose;
}

