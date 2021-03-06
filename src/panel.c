/*
 * Copyright (C) 2018 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 *
 * Somewhat based on maynard's panel which is
 * Copyright (C) 2014 Collabora Ltd. *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#define G_LOG_DOMAIN "phosh-panel"

#include "config.h"

#include "panel.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>
#include <libgnome-desktop/gnome-xkb-info.h>

#include <glib/gi18n.h>

#define _(String) gettext (String)

enum {
  SETTINGS_ACTIVATED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

typedef struct {
  GtkWidget *btn_top_panel;
  GtkWidget *lbl_clock;
  GtkWidget *lbl_lang;
  gint height;

  GnomeWallClock *wall_clock;
  GnomeXkbInfo *xkbinfo;
  GSettings *input_settings;
  GdkSeat *seat;
} PhoshPanelPrivate;

typedef struct _PhoshPanel
{
  PhoshLayerSurface parent;
} PhoshPanel;

G_DEFINE_TYPE_WITH_PRIVATE (PhoshPanel, phosh_panel, PHOSH_TYPE_LAYER_SURFACE)


static void
top_panel_clicked_cb (PhoshPanel *self, GtkButton *btn)
{
  g_return_if_fail (PHOSH_IS_PANEL (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));
  g_signal_emit(self, signals[SETTINGS_ACTIVATED], 0);
}


static void
wall_clock_notify_cb (PhoshPanel *self,
                      GParamSpec *pspec,
                      GnomeWallClock *wall_clock)
{
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);
  const gchar *str;

  g_return_if_fail (PHOSH_IS_PANEL (self));
  g_return_if_fail (GNOME_IS_WALL_CLOCK (wall_clock));

  str = gnome_wall_clock_get_clock(wall_clock);
  gtk_label_set_text (GTK_LABEL (priv->lbl_clock), str);
}


static void
size_allocated_cb (PhoshPanel *self, gpointer unused)
{
  gint width;
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);

  gtk_window_get_size (GTK_WINDOW (self), &width, &priv->height);
}


static gboolean
needs_keyboard_label (PhoshPanel *self)
{
  PhoshPanelPrivate *priv;
  GList *slaves;
  g_autoptr(GVariant) sources = NULL;

  priv = phosh_panel_get_instance_private (self);
  g_return_val_if_fail (GDK_IS_SEAT (priv->seat), FALSE);
  g_return_val_if_fail (G_IS_SETTINGS (priv->input_settings), FALSE);

  sources = g_settings_get_value(priv->input_settings, "sources");
  if (g_variant_n_children (sources) < 2)
    return FALSE;

  slaves = gdk_seat_get_slaves (priv->seat, GDK_SEAT_CAPABILITY_KEYBOARD);
  if (!slaves)
    return FALSE;

  g_list_free (slaves);
  return TRUE;
}


static void
on_seat_device_changed (PhoshPanel *self, GdkDevice  *device, GdkSeat *seat)
{
  gboolean visible;
  PhoshPanelPrivate *priv;

  g_return_if_fail (PHOSH_IS_PANEL (self));
  g_return_if_fail (GDK_IS_SEAT (seat));

  priv = phosh_panel_get_instance_private (self);
  visible = needs_keyboard_label (self);
  gtk_widget_set_visible (priv->lbl_lang, visible);
}


static void
on_input_setting_changed (PhoshPanel  *self,
                          const gchar *key,
                          GSettings   *settings)
{
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);
  g_autoptr(GVariant) sources = NULL;
  GVariantIter iter;
  g_autofree gchar *id = NULL;
  g_autofree gchar *type = NULL;
  const gchar *name;

  if (!needs_keyboard_label (self)) {
    gtk_widget_hide (priv->lbl_lang);
    return;
  }

  sources = g_settings_get_value(settings, "sources");
  g_variant_iter_init (&iter, sources);
  g_variant_iter_next (&iter, "(ss)", &type, &id);

  if (g_strcmp0 (type, "xkb")) {
    g_debug ("Not a xkb layout: '%s' - ignoring", id);
    return;
  }

  if (!gnome_xkb_info_get_layout_info (priv->xkbinfo, id,
                                       NULL, &name, NULL, NULL)) {
    g_debug ("Failed to get layout info for %s", id);
    name = id;
  }
  g_debug ("Layout is %s", name);
  gtk_label_set_text (GTK_LABEL (priv->lbl_lang), name);
  gtk_widget_show (priv->lbl_lang);
}


static void
phosh_panel_constructed (GObject *object)
{
  PhoshPanel *self = PHOSH_PANEL (object);
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);
  GdkDisplay *display = gdk_display_get_default ();

  G_OBJECT_CLASS (phosh_panel_parent_class)->constructed (object);

  priv->wall_clock = gnome_wall_clock_new ();

  g_signal_connect_object (priv->wall_clock,
                           "notify::clock",
                           G_CALLBACK (wall_clock_notify_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (priv->btn_top_panel,
                           "clicked",
                           G_CALLBACK (top_panel_clicked_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect (self,
                    "size-allocate",
                    G_CALLBACK (size_allocated_cb),
                    NULL);

  gtk_window_set_title (GTK_WINDOW (self), "phosh panel");
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "phosh-panel");

  /* Button properites */
  gtk_style_context_remove_class (gtk_widget_get_style_context (priv->btn_top_panel),
                                  "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (priv->btn_top_panel),
                                  "image-button");

  wall_clock_notify_cb (self, NULL, priv->wall_clock);

  /* language indicator */
  if (display) {
    priv->input_settings = g_settings_new ("org.gnome.desktop.input-sources");
    priv->xkbinfo = gnome_xkb_info_new ();
    priv->seat = gdk_display_get_default_seat (display);
    g_object_connect (priv->seat,
                      "swapped_signal::device-added", G_CALLBACK (on_seat_device_changed), self,
                      "swapped_signal::device-removed", G_CALLBACK (on_seat_device_changed), self,
                      NULL);
    g_signal_connect_swapped (priv->input_settings,
                              "changed::sources", G_CALLBACK (on_input_setting_changed),
                              self);
    on_input_setting_changed (self, NULL, priv->input_settings);
  }
}


static void
phosh_panel_dispose (GObject *object)
{
  PhoshPanel *self = PHOSH_PANEL (object);
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);

  g_clear_object (&priv->wall_clock);
  g_clear_object (&priv->xkbinfo);
  g_clear_object (&priv->input_settings);
  priv->seat = NULL;

  G_OBJECT_CLASS (phosh_panel_parent_class)->dispose (object);
}


static void
phosh_panel_class_init (PhoshPanelClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = phosh_panel_constructed;
  object_class->dispose = phosh_panel_dispose;

  signals[SETTINGS_ACTIVATED] = g_signal_new ("settings-activated",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/phosh/ui/top-panel.ui");
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, btn_top_panel);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, lbl_clock);
  gtk_widget_class_bind_template_child_private (widget_class, PhoshPanel, lbl_lang);
}


static void
phosh_panel_init (PhoshPanel *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkWidget *
phosh_panel_new (struct zwlr_layer_shell_v1 *layer_shell,
                 struct wl_output *wl_output)
{
  return g_object_new (PHOSH_TYPE_PANEL,
                       "layer-shell", layer_shell,
                       "wl-output", wl_output,
                       "height", PHOSH_PANEL_HEIGHT,
                       "anchor", ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                       "layer", ZWLR_LAYER_SHELL_V1_LAYER_TOP,
                       "kbd-interactivity", FALSE,
                       "exclusive-zone", PHOSH_PANEL_HEIGHT,
                       "namespace", "phosh",
                       NULL);
}


gint
phosh_panel_get_height (PhoshPanel *self)
{
  PhoshPanelPrivate *priv = phosh_panel_get_instance_private (self);

  return priv->height;
}
