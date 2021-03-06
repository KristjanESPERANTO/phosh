/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxpcpu.org>
 */

#define G_LOG_DOMAIN "phosh-feedback-manager"

#include "feedback-manager.h"
#include "shell.h"

#define LIBFEEDBACK_USE_UNSTABLE_API
#include <libfeedback.h>

/**
 * SECTION:phosh-feedback-manager
 * @short_description: Sends and configures user feedback
 * @Title: PhoshFeedbackManager
 */

/* TODO: proper icons */
#define PHOSH_FEEDBACK_ICON_FULL "preferences-system-notifications-symbolic"
#define PHOSH_FEEDBACK_ICON_SILENT "notifications-disabled-symbolic"

enum {
  PHOSH_FEEDBACK_MANAGER_PROP_0,
  PHOSH_FEEDBACK_MANAGER_PROP_ICON_NAME,
  PHOSH_FEEDBACK_MANAGER_PROP_LAST_PROP
};
static GParamSpec *props[PHOSH_FEEDBACK_MANAGER_PROP_LAST_PROP];

struct _PhoshFeedbackManager {
  GObject parent;

  const char *profile;
  const char *icon_name;
  gboolean inited;
};

G_DEFINE_TYPE (PhoshFeedbackManager, phosh_feedback_manager, G_TYPE_OBJECT);

static void
phosh_feedback_manager_get_property (GObject *object,
                                     guint property_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
  PhoshFeedbackManager *self = PHOSH_FEEDBACK_MANAGER (object);

  switch (property_id) {
  case PHOSH_FEEDBACK_MANAGER_PROP_ICON_NAME:
    g_value_set_string (value, self->icon_name);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
phosh_feedback_manager_update (PhoshFeedbackManager *self)
{
  const char *old;

  old = self->icon_name;
  self->profile = lfb_get_feedback_profile ();
  if (g_strcmp0 (self->profile, "quiet") && g_strcmp0 (self->profile, "silent"))
    self->icon_name = PHOSH_FEEDBACK_ICON_FULL;
  else
    self->icon_name = PHOSH_FEEDBACK_ICON_SILENT;

  g_debug("Feedback profile set to: '%s', icon '%s'\n", self->profile,  self->icon_name);
  if (old != self->icon_name)
    g_object_notify_by_pspec (G_OBJECT (self), props[PHOSH_FEEDBACK_MANAGER_PROP_ICON_NAME]);
}

static void
on_profile_changed (PhoshFeedbackManager *self, GParamSpec *psepc, LfbGdbusFeedback *proxy)
{
  g_return_if_fail (PHOSH_IS_FEEDBACK_MANAGER (self));

  phosh_feedback_manager_update (self);
}

static void
phosh_feedback_manager_constructed (GObject *object)
{
  PhoshFeedbackManager *self = PHOSH_FEEDBACK_MANAGER (object);
  g_autoptr(GError) error = NULL;

  if (lfb_init (PHOSH_APP_ID, &error)) {
    g_debug ("Libfeedback inited");
    self->inited = TRUE;
  } else {
    g_warning ("Failed to init libfeedback: %s", error->message);
  }

  g_signal_connect_swapped (lfb_get_proxy (),
                            "notify::profile",
                            (GCallback)on_profile_changed,
                            self);
  phosh_feedback_manager_update (self);
}

static void
phosh_feedback_manager_finalize (GObject *object)
{
  PhoshFeedbackManager *self = PHOSH_FEEDBACK_MANAGER (object);

  if (self->inited) {
    g_signal_handlers_disconnect_by_data (lfb_get_proxy (), self);
    lfb_uninit ();
    self->inited = FALSE;
  }
  G_OBJECT_CLASS (phosh_feedback_manager_parent_class)->finalize (object);
}

static void
phosh_feedback_manager_class_init (PhoshFeedbackManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phosh_feedback_manager_constructed;
  object_class->finalize = phosh_feedback_manager_finalize;

  object_class->get_property = phosh_feedback_manager_get_property;

  props[PHOSH_FEEDBACK_MANAGER_PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         "icon name",
                         "The feedback icon name",
                         PHOSH_FEEDBACK_ICON_FULL,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_properties (object_class, PHOSH_FEEDBACK_MANAGER_PROP_LAST_PROP, props);
}


static void
phosh_feedback_manager_init (PhoshFeedbackManager *self)
{
}

PhoshFeedbackManager *
phosh_feedback_manager_new (void)
{
  return g_object_new (PHOSH_TYPE_FEEDBACK_MANAGER, NULL);
}

const gchar*
phosh_feedback_manager_get_icon_name (PhoshFeedbackManager *self)
{
  g_return_val_if_fail (PHOSH_IS_FEEDBACK_MANAGER (self), NULL);

  return self->icon_name;
}

const gchar*
phosh_feedback_manager_get_profile (PhoshFeedbackManager *self)
{
  g_return_val_if_fail (PHOSH_IS_FEEDBACK_MANAGER (self), NULL);

  return self->profile;
}

void
phosh_feedback_manager_toggle (PhoshFeedbackManager *self)
{
  const char *profile = "silent";

  if (g_strcmp0 (lfb_get_feedback_profile (), "full"))
    profile = "full";

  g_debug ("Setting feedback profile to %s", profile);
  lfb_set_feedback_profile (profile);
}
