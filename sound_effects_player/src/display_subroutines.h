/*
 * display_subroutines.h
 *
 * Copyright © 2015 by John Sauter <John_Sauter@systemeyescomputerstore.com>
 *
 * This program is free software; you can redistribute it and/or modify
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DISPLAY_SUBROUTINES_H
#define DISPLAY_SUBROUTINES_H

#include <gtk/gtk.h>

/* Subroutines defined in display_subroutines.c */
void display_update_vu_meter (gpointer * user_data, gint channel,
                              gdouble new_value, gdouble peak_dB,
                              gdouble decay_dB);

guint display_show_message (gchar * message_text, GApplication * app);

void display_remove_message (guint message_id, GApplication * app);

void display_set_operator_text (gchar * text_to_display, GApplication * app);

void display_clear_operator_text (GApplication * app);

#endif /* DISPLAY_SUBROUTINES_H */

/* End of file display_subroutines.h */
