/* Form definition file generated by fdesign */

#include <stdlib.h>
#include "stats.h"


/***************************************
 ***************************************/

FD_stats_form *
create_form_stats_form( void )
{
  FL_OBJECT *obj;
  FD_stats_form *fdui = fl_malloc( sizeof *fdui );

  fdui->vdata = fdui->cdata = NULL;
  fdui->ldata = 0;

  fdui->stats_form = fl_bgn_form( FL_NO_BOX, 1115, 900 );

  obj = fl_add_box( FL_UP_BOX, 0, 0, 1115, 900, "" );

  //fdui->stats_text = obj = fl_add_text( FL_NORMAL_TEXT, 60, 50, 1000, 810, "test" );
  //fl_set_object_lsize( obj, FL_TINY_SIZE );
  fdui->stats_text = obj = fl_add_browser( FL_NORMAL_BROWSER, 60, 50, 1000, 810, "test" );
  fl_set_browser_fontsize(obj,FL_TINY_SIZE);

  fdui->stats_button = obj = fl_add_button( FL_PUSH_BUTTON, 60, 10, 130, 30, "Reset Stats" );
  fl_set_object_lalign( obj, FL_ALIGN_CENTER );
  fl_set_object_color( obj, FL_GREEN, FL_GREEN);
  fl_set_object_callback( obj, reset_stats, 0 );


  fl_end_form( );

  fdui->stats_form->fdui = fdui;

  return fdui;
}
