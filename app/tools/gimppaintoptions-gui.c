/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-symmetry.h"
#include "core/gimpsymmetry.h"
#include "core/gimptoolinfo.h"

#include "paint/gimppaintoptions.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpspinscale.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-constructors.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpairbrushtool.h"
#include "gimpclonetool.h"
#include "gimpconvolvetool.h"
#include "gimpdodgeburntool.h"
#include "gimperasertool.h"
#include "gimphealtool.h"
#include "gimpinktool.h"
#ifdef HAVE_LIBMYPAINT
#include "gimpmybrushtool.h"
#endif
#include "gimppaintoptions-gui.h"
#include "gimppenciltool.h"
#include "gimpperspectiveclonetool.h"
#include "gimpsmudgetool.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


static void gimp_paint_options_gui_reset_size  (GtkWidget        *button,
                                                GimpPaintOptions *paint_options);
static void gimp_paint_options_gui_reset_aspect_ratio
                                               (GtkWidget        *button,
                                                GimpPaintOptions *paint_options);
static void gimp_paint_options_gui_reset_angle (GtkWidget        *button,
                                                GimpPaintOptions *paint_options);
static void gimp_paint_options_gui_reset_spacing
                                               (GtkWidget        *button,
                                                GimpPaintOptions *paint_options);
static void gimp_paint_options_gui_reset_hardness
                                               (GtkWidget        *button,
                                                GimpPaintOptions *paint_options);
static void gimp_paint_options_gui_reset_force (GtkWidget        *button,
                                                GimpPaintOptions *paint_options);

static GtkWidget * dynamics_options_gui        (GimpPaintOptions *paint_options,
                                                GType             tool_type);
static GtkWidget * jitter_options_gui          (GimpPaintOptions *paint_options,
                                                GType             tool_type);
static GtkWidget * smoothing_options_gui       (GimpPaintOptions *paint_options,
                                                GType             tool_type);

static GtkWidget * gimp_paint_options_gui_scale_with_buttons
                                               (GObject      *config,
                                                gchar        *prop_name,
                                                gchar        *prop_descr,
                                                gchar        *link_prop_name,
                                                gchar        *reset_tooltip,
                                                gdouble       step_increment,
                                                gdouble       page_increment,
                                                gint          digits,
                                                gdouble       scale_min,
                                                gdouble       scale_max,
                                                gdouble       factor,
                                                gdouble       gamma,
                                                GCallback     reset_callback,
                                                GtkSizeGroup *link_group);

static void
         gimp_paint_options_symmetry_update_cb (GimpSymmetry     *sym,
                                                GimpImage        *image,
                                                GtkWidget        *frame);
static void
          gimp_paint_options_symmetry_callback (GimpPaintOptions *options,
                                                GParamSpec       *pspec,
                                                GtkWidget        *frame);
static void     gimp_paint_options_symmetry_ui (GimpSymmetry     *sym,
                                                GtkWidget        *frame);


/*  public functions  */

GtkWidget *
gimp_paint_options_gui (GimpToolOptions *tool_options)
{
  GObject          *config  = G_OBJECT (tool_options);
  GimpPaintOptions *options = GIMP_PAINT_OPTIONS (tool_options);
  GtkWidget        *vbox    = gimp_tool_options_gui (tool_options);
  GtkWidget        *menu;
  GtkWidget        *scale;
  GType             tool_type;

  tool_type = tool_options->tool_info->tool_type;

  /*  the paint mode menu  */
  menu = gimp_prop_paint_mode_menu_new (config, "paint-mode", TRUE, FALSE);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (menu), _("Mode"));
  g_object_set (menu, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  if (tool_type == GIMP_TYPE_ERASER_TOOL     ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL   ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL ||
      tool_type == GIMP_TYPE_HEAL_TOOL       ||
#ifdef HAVE_LIBMYPAINT
      tool_type == GIMP_TYPE_MYBRUSH_TOOL    ||
#endif
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      gtk_widget_set_sensitive (menu, FALSE);
    }

  /*  the opacity scale  */
  scale = gimp_prop_spin_scale_new (config, "opacity",
                                    _("Opacity"),
                                    0.01, 0.1, 0);
  gimp_prop_widget_set_factor (scale, 100.0, 0.0, 0.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  temp debug foo  */
  if (g_type_is_a (tool_type, GIMP_TYPE_PAINT_TOOL)
#ifdef HAVE_LIBMYPAINT
      && tool_type != GIMP_TYPE_MYBRUSH_TOOL
#endif
      )
    {
      GtkWidget *button;

      button = gimp_prop_check_button_new (config, "use-applicator",
                                           "Use GimpApplicator");
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  /*  the brush  */
  if (g_type_is_a (tool_type, GIMP_TYPE_BRUSH_TOOL))
    {
      GtkSizeGroup *link_group;
      GtkWidget    *button;
      GtkWidget    *frame;
      GtkWidget    *hbox;

      button = gimp_prop_brush_box_new (NULL, GIMP_CONTEXT (tool_options),
                                        _("Brush"), 2,
                                        "brush-view-type", "brush-view-size",
                                        "gimp-brush-editor");
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      link_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

      hbox = gimp_paint_options_gui_scale_with_buttons
        (config, "brush-size", _("Size"), "brush-link-size",
         _("Reset size to brush's native size"),
         1.0, 10.0, 2, 1.0, 1000.0, 1.0, 1.7,
         G_CALLBACK (gimp_paint_options_gui_reset_size), link_group);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      hbox = gimp_paint_options_gui_scale_with_buttons
        (config, "brush-aspect-ratio", _("Aspect Ratio"), "brush-link-aspect-ratio",
         _("Reset aspect ratio to brush's native"),
         0.1, 1.0, 2, -20.0, 20.0, 1.0, 1.0,
         G_CALLBACK (gimp_paint_options_gui_reset_aspect_ratio), link_group);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      hbox = gimp_paint_options_gui_scale_with_buttons
        (config, "brush-angle", _("Angle"), "brush-link-angle",
         _("Reset angle to zero"),
         0.1, 1.0, 2, -180.0, 180.0, 1.0, 1.0,
         G_CALLBACK (gimp_paint_options_gui_reset_angle), link_group);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      hbox = gimp_paint_options_gui_scale_with_buttons
        (config, "brush-spacing", _("Spacing"), "brush-link-spacing",
         _("Reset spacing to brush's native spacing"),
         0.1, 1.0, 1, 1.0, 200.0, 100.0, 1.7,
         G_CALLBACK (gimp_paint_options_gui_reset_spacing), link_group);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      hbox = gimp_paint_options_gui_scale_with_buttons
        (config, "brush-hardness", _("Hardness"), "brush-link-hardness",
         _("Reset hardness to default"),
         0.1, 1.0, 1, 0.0, 100.0, 100.0, 1.0,
         G_CALLBACK (gimp_paint_options_gui_reset_hardness), link_group);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      hbox = gimp_paint_options_gui_scale_with_buttons
        (config, "brush-force", _("Force"), NULL,
         _("Reset force to default"),
         0.1, 1.0, 1, 0.0, 100.0, 100.0, 1.0,
         G_CALLBACK (gimp_paint_options_gui_reset_force), link_group);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      g_object_unref (link_group);

      button = gimp_prop_dynamics_box_new (NULL, GIMP_CONTEXT (tool_options),
                                           _("Dynamics"), 2,
                                           "dynamics-view-type",
                                           "dynamics-view-size",
                                           "gimp-dynamics-editor");
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      frame = dynamics_options_gui (options, tool_type);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      frame = jitter_options_gui (options, tool_type);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }

  if (g_type_is_a (tool_type, GIMP_TYPE_PAINT_TOOL))
    {
      GtkWidget *frame;
      GimpGuiConfig *guiconfig;

      guiconfig = GIMP_GUI_CONFIG (tool_options->tool_info->gimp->config);
      if (guiconfig->playground_symmetry)
        {
          /* Symmetry Painting */
          GtkListStore *store;
          GtkTreeIter   iter;
          GList        *syms;

          store = gimp_int_store_new ();

          syms = gimp_image_symmetry_list ();
          for (syms = gimp_image_symmetry_list (); syms; syms = g_list_next (syms))
            {
              GimpSymmetryClass *klass;
              GType                 type;

              type = (GType) syms->data;
              klass = g_type_class_ref (type);

              gtk_list_store_prepend (store, &iter);
              gtk_list_store_set (store, &iter,
                                  GIMP_INT_STORE_LABEL,
                                  klass->label,
                                  GIMP_INT_STORE_VALUE,
                                  syms->data,
                                  -1);
              g_type_class_unref (klass);
            }
          gtk_list_store_prepend (store, &iter);
          gtk_list_store_set (store, &iter,
                              GIMP_INT_STORE_LABEL, _("None"),
                              GIMP_INT_STORE_VALUE, G_TYPE_NONE,
                              -1);
          menu = gimp_prop_int_combo_box_new (config, "symmetry",
                                              GIMP_INT_STORE (store));
          g_object_unref (store);

          gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (menu), _("Symmetry"));
          gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (menu), G_TYPE_NONE);
          g_object_set (menu, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

          gtk_box_pack_start (GTK_BOX (vbox), menu, FALSE, FALSE, 0);
          gtk_widget_show (menu);

          frame = gimp_frame_new ("");
          gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
          g_signal_connect (options, "notify::symmetry",
                            G_CALLBACK (gimp_paint_options_symmetry_callback),
                            frame);

        }

      /*  the "smooth stroke" options  */
      frame = smoothing_options_gui (options, tool_type);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }

  /*  the "Link size to zoom" toggle  */
  if (g_type_is_a (tool_type, GIMP_TYPE_BRUSH_TOOL))
    {
      GtkWidget *button;

      button = gimp_prop_check_button_new (config,
                                           "brush-zoom",
                                           _("Lock brush size to zoom"));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  /*  the "incremental" toggle  */
  if (tool_type == GIMP_TYPE_PENCIL_TOOL     ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_type == GIMP_TYPE_ERASER_TOOL)
    {
      GtkWidget *button;

      button = gimp_prop_enum_check_button_new (config,
                                                "application-mode",
                                                _("Incremental"),
                                                GIMP_PAINT_CONSTANT,
                                                GIMP_PAINT_INCREMENTAL);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  /* the "hard edge" toggle */
  if (tool_type == GIMP_TYPE_ERASER_TOOL            ||
      tool_type == GIMP_TYPE_CLONE_TOOL             ||
      tool_type == GIMP_TYPE_HEAL_TOOL              ||
      tool_type == GIMP_TYPE_PERSPECTIVE_CLONE_TOOL ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL          ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL        ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      GtkWidget *button;

      button = gimp_prop_check_button_new (config, "hard", _("Hard edge"));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  return vbox;
}


/*  private functions  */

static GtkWidget *
dynamics_options_gui (GimpPaintOptions *paint_options,
                      GType             tool_type)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame;
  GtkWidget *inner_frame;
  GtkWidget *scale;
  GtkWidget *menu;
  GtkWidget *combo;
  GtkWidget *checkbox;
  GtkWidget *vbox;
  GtkWidget *inner_vbox;
  GtkWidget *hbox;
  GtkWidget *box;

  frame = gimp_prop_expander_new (config, "dynamics-expanded",
                                  _("Dynamics Options"));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  inner_frame = gimp_frame_new (_("Fade Options"));
  gtk_box_pack_start (GTK_BOX (vbox), inner_frame, FALSE, FALSE, 0);
  gtk_widget_show (inner_frame);

  inner_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (inner_frame), inner_vbox);
  gtk_widget_show (inner_vbox);

  /*  the fade-out scale & unitmenu  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (inner_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  scale = gimp_prop_spin_scale_new (config, "fade-length",
                                    _("Fade length"), 1.0, 50.0, 0);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 1.0, 1000.0);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

  menu = gimp_prop_unit_combo_box_new (config, "fade-unit");
  gtk_box_pack_start (GTK_BOX (hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

#if 0
  /* FIXME pixel digits */
  g_object_set_data (G_OBJECT (menu), "set_digits", spinbutton);
  gimp_unit_menu_set_pixel_digits (GIMP_UNIT_MENU (menu), 0);
#endif

  /*  the repeat type  */
  combo = gimp_prop_enum_combo_box_new (config, "fade-repeat", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Repeat"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (inner_vbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  checkbox = gimp_prop_check_button_new (config, "fade-reverse",
                                         _("Reverse"));
  gtk_box_pack_start (GTK_BOX (inner_vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show (checkbox);

  /* Color UI */
  if (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL))
    {
      inner_frame = gimp_frame_new (_("Color Options"));
      gtk_box_pack_start (GTK_BOX (vbox), inner_frame, FALSE, FALSE, 0);
      gtk_widget_show (inner_frame);

      box = gimp_prop_gradient_box_new (NULL, GIMP_CONTEXT (config),
                                        _("Gradient"), 2,
                                        "gradient-view-type",
                                        "gradient-view-size",
                                        "gradient-reverse",
                                        "gimp-gradient-editor");
      gtk_container_add (GTK_CONTAINER (inner_frame), box);
      gtk_widget_show (box);
    }

  return frame;
}

static GtkWidget *
jitter_options_gui (GimpPaintOptions *paint_options,
                    GType             tool_type)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame;
  GtkWidget *scale;

  scale = gimp_prop_spin_scale_new (config, "jitter-amount",
                                    _("Amount"),
                                    0.01, 1.0, 2);

  frame = gimp_prop_expanding_frame_new (config, "use-jitter",
                                         _("Apply Jitter"),
                                         scale, NULL);

  return frame;
}

static GtkWidget *
smoothing_options_gui (GimpPaintOptions *paint_options,
                       GType             tool_type)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *scale;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  frame = gimp_prop_expanding_frame_new (config, "use-smoothing",
                                         _("Smooth stroke"),
                                         vbox, NULL);

  scale = gimp_prop_spin_scale_new (config, "smoothing-quality",
                                    _("Quality"),
                                    1, 10, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_spin_scale_new (config, "smoothing-factor",
                                    _("Weight"),
                                    1, 10, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  return frame;
}

static void
gimp_paint_options_gui_reset_size (GtkWidget        *button,
                                   GimpPaintOptions *paint_options)
{
  GimpBrush *brush = gimp_context_get_brush (GIMP_CONTEXT (paint_options));

  if (brush)
    gimp_paint_options_set_default_brush_size (paint_options, brush);
}

static void
gimp_paint_options_gui_reset_aspect_ratio (GtkWidget        *button,
                                           GimpPaintOptions *paint_options)
{
  g_object_set (paint_options,
                "brush-aspect-ratio", 0.0,
                NULL);
}

static void
gimp_paint_options_gui_reset_angle (GtkWidget        *button,
                                    GimpPaintOptions *paint_options)
{
  g_object_set (paint_options,
                "brush-angle", 0.0,
                NULL);
}

static void
gimp_paint_options_gui_reset_spacing (GtkWidget        *button,
                                      GimpPaintOptions *paint_options)
{
  GimpBrush *brush = gimp_context_get_brush (GIMP_CONTEXT (paint_options));

  if (brush)
    gimp_paint_options_set_default_brush_spacing (paint_options, brush);
}

static void
gimp_paint_options_gui_reset_hardness (GtkWidget        *button,
                                       GimpPaintOptions *paint_options)
{
  GimpBrush *brush = gimp_context_get_brush (GIMP_CONTEXT (paint_options));

  if (brush)
    gimp_paint_options_set_default_brush_hardness (paint_options, brush);
}

static void
gimp_paint_options_gui_reset_force (GtkWidget        *button,
                                    GimpPaintOptions *paint_options)
{
  g_object_set (paint_options,
                "brush-force", 0.5,
                NULL);
}

static GtkWidget *
gimp_paint_options_gui_scale_with_buttons (GObject      *config,
                                           gchar        *prop_name,
                                           gchar        *prop_descr,
                                           gchar        *link_prop_name,
                                           gchar        *reset_tooltip,
                                           gdouble       step_increment,
                                           gdouble       page_increment,
                                           gint          digits,
                                           gdouble       scale_min,
                                           gdouble       scale_max,
                                           gdouble       factor,
                                           gdouble       gamma,
                                           GCallback     reset_callback,
                                           GtkSizeGroup *link_group)
{
  GtkWidget *scale;
  GtkWidget *hbox;
  GtkWidget *button;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  scale = gimp_prop_spin_scale_new (config, prop_name,
                                    prop_descr,
                                    step_increment, page_increment, digits);
  gimp_prop_widget_set_factor (scale, factor,
                               step_increment, page_increment, digits);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale),
                                    scale_min, scale_max);
  gimp_spin_scale_set_gamma (GIMP_SPIN_SCALE (scale), gamma);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

  button = gimp_icon_button_new (GIMP_STOCK_RESET, NULL);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_image_set_from_icon_name (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (button))),
                                GIMP_STOCK_RESET, GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    reset_callback,
                    config);

  gimp_help_set_help_data (button,
                           reset_tooltip, NULL);

  if (link_prop_name)
    {
      GtkWidget *image;

      button = gtk_toggle_button_new ();
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

      image = gtk_image_new_from_icon_name (GIMP_STOCK_LINKED,
                                            GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);

      g_object_bind_property (config, link_prop_name,
                              button, "active",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
    }
  else
    {
      button = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    }

  gtk_size_group_add_widget (link_group, button);

  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Link to brush default"), NULL);

  return hbox;
}

static void
gimp_paint_options_symmetry_update_cb (GimpSymmetry *sym,
                                       GimpImage    *image,
                                       GtkWidget    *frame)
{
  GimpContext *context;

  g_return_if_fail (GIMP_IS_SYMMETRY (sym));

  context = gimp_get_user_context (sym->image->gimp);
  if (image != context->image ||
      sym != gimp_image_get_selected_symmetry (image))
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (sym),
                                            gimp_paint_options_symmetry_update_cb,
                                            frame);
      return;
    }

  gimp_paint_options_symmetry_ui (sym, frame);
}

static void
gimp_paint_options_symmetry_callback (GimpPaintOptions *options,
                                      GParamSpec       *pspec,
                                      GtkWidget        *frame)
{
  GimpSymmetry *sym = NULL;
  GimpContext  *context;
  GimpImage    *image;

  g_return_if_fail (GIMP_IS_PAINT_OPTIONS (options));

  context = gimp_get_user_context (GIMP_CONTEXT (options)->gimp);
  image   = context->image;

  if (image &&
      (sym = gimp_image_get_selected_symmetry (image)))
    {
      g_signal_connect (sym, "update-ui",
                        G_CALLBACK (gimp_paint_options_symmetry_update_cb),
                        frame);
    }

  gimp_paint_options_symmetry_ui (sym, frame);
}

static void
gimp_paint_options_symmetry_ui (GimpSymmetry *sym,
                                GtkWidget    *frame)
{
  GimpSymmetryClass  *klass;
  GtkWidget          *vbox;
  GParamSpec        **specs;
  guint               nproperties;
  gint                i;

  /* Clean the old frame */
  gtk_widget_hide (frame);
  gtk_container_foreach (GTK_CONTAINER (frame),
                         (GtkCallback) gtk_widget_destroy, NULL);

  if (! sym)
    return;

  klass = g_type_class_ref (sym->type);
  gtk_frame_set_label (GTK_FRAME (frame),
                       klass->label);
  g_type_class_unref (klass);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  specs = gimp_symmetry_get_settings (sym, &nproperties);

  for (i = 0; i < (gint) nproperties; i++)
    {
      GParamSpec  *spec;
      const gchar *name;
      const gchar *blurb;

      if (specs[i] == NULL)
        {
          GtkWidget *separator;

          separator = gtk_hseparator_new ();
          gtk_box_pack_start (GTK_BOX (vbox), separator,
                              FALSE, FALSE, 0);
          gtk_widget_show (separator);
          continue;
        }
      spec = G_PARAM_SPEC (specs[i]);

      name = g_param_spec_get_name (spec);
      blurb = g_param_spec_get_blurb (spec);

      switch (spec->value_type)
        {
        case G_TYPE_BOOLEAN:
            {
              GtkWidget *checkbox;

              checkbox = gimp_prop_check_button_new (G_OBJECT (sym),
                                                     name,
                                                     blurb);
              gtk_box_pack_start (GTK_BOX (vbox), checkbox,
                                  FALSE, FALSE, 0);
              gtk_widget_show (checkbox);
            }
          break;
        case G_TYPE_DOUBLE:
        case G_TYPE_INT:
        case G_TYPE_UINT:
            {
              GtkWidget *scale;
              gdouble    minimum;
              gdouble    maximum;

              if (spec->value_type == G_TYPE_DOUBLE)
                {
                  minimum = G_PARAM_SPEC_DOUBLE (spec)->minimum;
                  maximum = G_PARAM_SPEC_DOUBLE (spec)->maximum;
                }
              else if (spec->value_type == G_TYPE_INT)
                {
                  minimum = G_PARAM_SPEC_INT (spec)->minimum;
                  maximum = G_PARAM_SPEC_INT (spec)->maximum;
                }
              else
                {
                  minimum = G_PARAM_SPEC_UINT (spec)->minimum;
                  maximum = G_PARAM_SPEC_UINT (spec)->maximum;
                }

              scale = gimp_prop_spin_scale_new (G_OBJECT (sym),
                                                name, blurb,
                                                1.0, 10.0, 1);
              gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale),
                                                minimum,
                                                maximum);
              gtk_box_pack_start (GTK_BOX (vbox), scale, TRUE, TRUE, 0);
              gtk_widget_show (scale);
            }
          break;
        default:
          /* Type of parameter we haven't handled yet. */
          continue;
        }
    }

  g_free (specs);

  /* Finally show the frame. */
  gtk_widget_show (frame);
}
