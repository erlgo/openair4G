#include <stdlib.h>
#include <stdint.h>

#include <gtk/gtk.h>

#include "rc.h"

#include "buffers.h"

#include "ui_main_screen.h"
#include "ui_tree_view.h"
#include "ui_callbacks.h"
#include "ui_filters.h"

#include "ui_signal_dissect_view.h"

typedef struct
{
    GtkListStore *store;
    GtkTreeModelFilter *filtered;
    guint filtered_last_msg;
    guint filtered_msg_number;
} ui_store_t;

static ui_store_t ui_store;

static gboolean ui_tree_filter_messages(GtkTreeModel *model, GtkTreeIter *iter, ui_store_t *store)
{
    char *msg_number;
    guint number = 0;
    char *message;
    char *origin_task;
    char *destination_task;
    gboolean enabled = FALSE;

    gtk_tree_model_get (model, iter, COL_MSG_NUM, &msg_number, COL_SIGNAL, &message, COL_FROM_TASK, &origin_task,
                        COL_TO_TASK, &destination_task, -1);
    if (msg_number != NULL)
    {
        number = atoi (msg_number);
        enabled = ui_filters_message_enabled (message, origin_task, destination_task);

        if ((enabled) && (ui_store.filtered_last_msg < number))
        {
            ui_store.filtered_last_msg = number;
            ui_store.filtered_msg_number++;
        }
        // g_debug("%x %x %d %s %s %s %d %d", (int) model, (int) iter, number, message, origin_task, destination_task, enabled, ui_store.filtered_msg_number);
    }

    g_free (message);
    g_free (origin_task);
    g_free (destination_task);

    return enabled;
}

static void
ui_tree_view_init_list(GtkWidget *list)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
        "MN", renderer, "text", COL_MSG_NUM, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);

    column = gtk_tree_view_column_new_with_attributes(
        "Signal", renderer, "text", COL_SIGNAL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
    g_signal_connect(G_OBJECT(column), "clicked",
                     G_CALLBACK(ui_callback_on_tree_column_header_click), (gpointer) COL_SIGNAL);

    column = gtk_tree_view_column_new_with_attributes(
        "From", renderer, "text", COL_FROM_TASK, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
    g_signal_connect(G_OBJECT(column), "clicked",
                     G_CALLBACK(ui_callback_on_tree_column_header_click), (gpointer) COL_FROM_TASK);

    column = gtk_tree_view_column_new_with_attributes(
        "To", renderer, "text", COL_TO_TASK, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
    g_signal_connect(G_OBJECT(column), "clicked",
                     G_CALLBACK(ui_callback_on_tree_column_header_click), (gpointer) COL_TO_TASK);

    ui_store.store = gtk_list_store_new(NUM_COLS,
                                       G_TYPE_STRING,
                                       G_TYPE_STRING,
                                       G_TYPE_STRING,
                                       G_TYPE_STRING,
                                       /* HACK: add a reference to the buffer here
                                        * to avoid maintining multiple lists.
                                        * The reference is not displayed
                                        */
                                       G_TYPE_POINTER);

    ui_store.filtered = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (GTK_TREE_MODEL (ui_store.store), NULL));
    gtk_tree_model_filter_set_visible_func (ui_store.filtered,
                                            (GtkTreeModelFilterVisibleFunc) ui_tree_filter_messages,
                                            &ui_store, NULL);

    gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(ui_store.filtered));

    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(list));
}

static void ui_tree_view_add_to_list(GtkWidget *list, const gchar *message_number,
                        const gchar *signal_name, const char *origin_task,
                        const char *to_task, gpointer buffer)
{
    GtkTreeIter iter;

    gtk_list_store_append(ui_store.store, &iter);
    gtk_list_store_set(ui_store.store, &iter,
                       /* Columns */
                       COL_MSG_NUM    , message_number,
                       COL_SIGNAL     , signal_name,
                       COL_FROM_TASK  , origin_task,
                       COL_TO_TASK    , to_task,
                       COL_BUFFER     , buffer,
                       /* End of columns */
                       -1);
}

void ui_tree_view_destroy_list(GtkWidget *list)
{
    g_assert(list != NULL);

    gtk_list_store_clear(ui_store.store);
    ui_store.filtered_last_msg = 0;
    ui_store.filtered_msg_number = 0;

    /* Reset number of messages */
    ui_main_data.nb_message_received = 0;
    ui_main_data.path_last = NULL;
}

int ui_tree_view_create(GtkWidget *window, GtkWidget *vbox)
{
    GtkWidget *hbox;
    GtkTreeSelection *selection;
    GtkWidget *scrolled_window;

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    ui_main_data.signalslist = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_main_data.signalslist), TRUE);

    /* Disable multiple selection */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_main_data.signalslist));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    gtk_container_add(GTK_CONTAINER(scrolled_window),
                      ui_main_data.signalslist);

    ui_tree_view_init_list(ui_main_data.signalslist);
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(ui_main_data.signalslist), TRUE);

    gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 500, -1);
    gtk_box_pack_start(GTK_BOX(hbox), scrolled_window, FALSE, FALSE, 0);
    ui_main_data.text_view = ui_signal_dissect_new(hbox);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);

    /* Connect callback on row selection */
    gtk_tree_selection_set_select_function(selection, ui_callback_on_select_signal,
                                           ui_main_data.text_view, NULL);

    ui_main_data.selection = selection;

    return 0;
}

int ui_tree_view_new_signal_ind(const uint32_t message_number, const char *signal_name,
                                const char *origin_task, const char *to_task, gpointer buffer)
{
    gchar message_number_str[11];

    sprintf(message_number_str, "%u", message_number);

    ui_tree_view_add_to_list(ui_main_data.signalslist, message_number_str, signal_name,
                             origin_task, to_task, (buffer_t *)buffer);

    return RC_OK;
}

void ui_tree_view_select_row(gint row, GtkTreePath **path)
{
    GtkTreePath *path_row;

    path_row = gtk_tree_path_new_from_indices(row, -1);
    /* Select the message in requested row */
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(ui_main_data.signalslist), path_row, NULL, FALSE);
    /* Center the message in the middle of the list if possible */
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(ui_main_data.signalslist), path_row, NULL, TRUE, 0.5, 0.0);

    if (path != NULL)
    {
        *path = path_row;
    }
}

void ui_tree_view_refilter(void)
{
    ui_store.filtered_last_msg = 0;
    ui_store.filtered_msg_number = 0;
    gtk_tree_model_filter_refilter (ui_store.filtered);
}

guint ui_tree_view_get_filtered_number(void)
{
    return ui_store.filtered_msg_number;
}