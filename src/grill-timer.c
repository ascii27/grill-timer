#include <pebble.h>
#include "grill-timer.h"
#include "food-list.h"

#define MAX_GRILL_TIMERS 4

static AppTimer *timer = NULL;

/* List Window */
static Window *list_window;
static Layer *list_items_layer, *delete_item_layer;
static TextLayer *title_layer;
static GBitmap *flame_bitmap;
static int spatula_pos = 0;
static int item_count = 0;
static int highlight_item = 0;
static int current_timer = 0;
static int should_delete = 0;

/* Detail Window */
static Window *detail_window;
static Layer *detail_layer;
static TextLayer *text_layer, *seconds_layer;
static Layer *blink_layer, *detail_graphics_layer;
static GBitmap *flip_image;
static BitmapLayer *flip_layer;
static GrillTimer grill_timers[MAX_GRILL_TIMERS];
static char str_out[7], sec_out[3], tm_str[9], tmp_str[20];

/* Food Menu Window */
static Window *food_list_window;
static SimpleMenuLayer  *food_list_layer;
static SimpleMenuSection food_list_section[1]; 
static SimpleMenuItem	 food_list_item[NUM_FOOD_ITEMS]; 

/* Timers */
static void timer_callback(void *context) {
    if( window_stack_get_top_window() == detail_window ) {

    	layer_mark_dirty(blink_layer);

    	GrillTimer* gtimer = &grill_timers[current_timer];

	if( layer_get_hidden(blink_layer) ) { layer_set_hidden(blink_layer,false); }
	else 		    		    { layer_set_hidden(blink_layer,true); }

	if( gtimer->flip_show ) {
	     gbitmap_destroy(flip_image);
	     
	     if( spatula_pos == 3 ) spatula_pos = 0;
	     else                  ++spatula_pos;
	     
	     switch (spatula_pos) {
	     case 0:
	     flip_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SPATULA_TOP_WHITE);
	     break;
	     case 1:
	     flip_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SPATULA_TURNING_WHITE);
	     break;
	     case 2:
	     flip_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SPATULA_BOTTOM_WHITE);
	     break;
	     case 3:
	     flip_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SPATULA_BOTTOM_WHITE);
	     break;
	     }
	     layer_mark_dirty(bitmap_layer_get_layer(flip_layer));
	} else {
	     layer_remove_from_parent(bitmap_layer_get_layer(flip_layer));
	}
    }
    
    const uint32_t timeout_ms = 500;
    timer = app_timer_register(timeout_ms, timer_callback, NULL);
}


/* Detail Window Functions */
static void detail_select_click_handler(ClickRecognizerRef recognizer, void *context) {

    GrillTimer* gtimer = &grill_timers[current_timer];
    
    switch( gtimer->blinker_pos ) {
	case 0:
	    window_stack_push(food_list_window, true);
	    simple_menu_layer_set_selected_index(food_list_layer,gtimer->food_index,false);
 	    break;
        case 1:
            ++gtimer->hours;
            if( gtimer->hours > 9 ) { gtimer->hours = 0; }
            break;
        case 2:
            gtimer->minutes += 10;
            if( gtimer->minutes >= 60 ) {
                gtimer->minutes -= 60;
            }
            break;
        case 3:
            if( (gtimer->minutes % 10) == 9 ) {
                gtimer->minutes -= 9;
            } else {
                ++gtimer->minutes;
            }
            break;
        case 4:
            if( gtimer->running ) {
                gtimer->running = 0;
                return;
            }
            gtimer->running = 1;
    	    layer_mark_dirty(detail_graphics_layer);
	    return;
            break;
            
    }
    update_timer_display();
}

static void set_blinker_pos( GrillTimer* gtimer ) {
       switch( gtimer->blinker_pos ) {
	    case 0:
                layer_set_frame(blink_layer, (GRect){ .origin = {48, 0}, .size = {115,28}});
		break;
            case 1:
                layer_set_frame(blink_layer, (GRect){ .origin = {25, 80}, .size = {21,30}});
                break;
            case 2:
                layer_set_frame(blink_layer, (GRect){ .origin = {65, 80}, .size = {21,30}});
                break;
            case 3:
                layer_set_frame(blink_layer, (GRect){ .origin = {88, 80}, .size = {20,30}});
                break;
	    case 4:
                layer_set_frame(blink_layer, (GRect){ .origin = {0, 0}, .size = {0,0}});
		break;
        }
}

static void detail_up_click_handler(ClickRecognizerRef recognizer, void *context) {
    GrillTimer* gtimer = &grill_timers[current_timer];

    --gtimer->blinker_pos;
    if( gtimer->blinker_pos < 0) gtimer->blinker_pos = 4;
    set_blinker_pos(gtimer);
    layer_mark_dirty(detail_graphics_layer);
}

static void detail_down_click_handler(ClickRecognizerRef recognizer, void *context) {
    GrillTimer* gtimer = &grill_timers[current_timer];
    ++gtimer->blinker_pos;
    if( gtimer->blinker_pos > 4) gtimer->blinker_pos = 0;
    set_blinker_pos(gtimer);
    layer_mark_dirty(detail_graphics_layer);
}

static void detail_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, detail_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, detail_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, detail_down_click_handler);
}


static void update_timer_display() {
    if( window_stack_get_top_window() == detail_window ) {
    	GrillTimer* gtimer = &grill_timers[current_timer];

	    snprintf(str_out,6,"%d:%02d",gtimer->hours, gtimer->minutes);
	    snprintf(sec_out,3,"%02d",gtimer->seconds);
	    text_layer_set_text(text_layer,str_out);
	    text_layer_set_text(seconds_layer,sec_out);
    }
}

static void second_callback( struct tm *tick_time, TimeUnits units_changed ) {
  layer_mark_dirty(list_items_layer);

  int i = 0;
  for( ; i < item_count; i++ ) {
  	GrillTimer* gtimer = &grill_timers[i];

	  if( gtimer->running ) { 
	    if ( gtimer->seconds == 0 ) {
		if( gtimer->minutes == 0 ) {
		    if( gtimer->hours == 0 ) {
			gtimer->running = 0;
			vibes_double_pulse();
			return;
		    }
		    gtimer->minutes = 59;
		    --gtimer->hours;
		}
		gtimer->seconds = 60;
		--gtimer->minutes;
	    }
	    
	    --gtimer->seconds;
	    
	    update_timer_display();

	    int time = (( gtimer->hours * 3600 ) + (gtimer->minutes * 60) + gtimer->seconds);
	    if( time != 0 && ( time % gtimer->flip_time ) == 0 ) {
		gtimer->flip_show = 1;
		vibes_short_pulse();

    		if( window_stack_get_top_window() != detail_window ) {
		    current_timer = i;
	  	    window_stack_push(detail_window, true);
  	    	    layer_add_child(window_get_root_layer(detail_window), bitmap_layer_get_layer(flip_layer));
	    	    update_timer_display();
		} else if (current_timer == i) {
  	    	    layer_add_child(window_get_root_layer(detail_window), bitmap_layer_get_layer(flip_layer));
		}
	    } 
	  }
  }
}

static void update_flip_layer(Layer *layer, GContext* ctx) {

}

static void update_detail_graphics_layer(Layer *layer, GContext* ctx) {
    graphics_context_set_stroke_color(ctx, GColorBlack);
    GrillTimer* gtimer = &grill_timers[current_timer];

    if( gtimer->blinker_pos == 4 ) {
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_context_set_text_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, (GRect) { .origin = { 20, 114 }, .size = { 105, 32 } }, 9, GCornersAll);
    } else {
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_context_set_text_color(ctx, GColorBlack);
	graphics_draw_round_rect(ctx, (GRect) { .origin = { 20, 114 }, .size = { 105, 32 } }, 9);
    }


    char* str = "Start";
    if( gtimer->running ) str = "Stop";

    graphics_draw_text(ctx, str, 
		fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK), 
		(GRect) { .origin = { 33, 110 }, .size = { 115,35 } },
		GTextOverflowModeTrailingEllipsis,
		GTextAlignmentLeft,
		NULL);

    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_text_color(ctx, GColorBlack);

    graphics_draw_text(ctx, "Food:", 
		fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), 
		(GRect) { .origin = { 3, 0 }, .size = { 55,25 } },
		GTextOverflowModeTrailingEllipsis,
		GTextAlignmentLeft,
		NULL);

    graphics_draw_text(ctx, gtimer->name, 
		fonts_get_system_font(FONT_KEY_GOTHIC_24), 
		(GRect) { .origin = { 48, 0 }, .size = { 90,28 } },
		GTextOverflowModeTrailingEllipsis,
		GTextAlignmentLeft,
		NULL);

    graphics_draw_text(ctx, "Heat:", 
		fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), 
		(GRect) { .origin = { 4, 23 }, .size = { 55,28 } },
		GTextOverflowModeTrailingEllipsis,
		GTextAlignmentLeft,
		NULL);

    graphics_draw_text(ctx, heat_type[gtimer->heat], 
		fonts_get_system_font(FONT_KEY_GOTHIC_24), 
		(GRect) { .origin = { 48, 23 }, .size = { 130,28 } },
		GTextOverflowModeTrailingEllipsis,
		GTextAlignmentLeft,
		NULL);

    if( gtimer->sear_minutes > 0 ) {
	    graphics_draw_text(ctx, "Sear:", 
			fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), 
			(GRect) { .origin = { 5, 46 }, .size = { 55,28 } },
			GTextOverflowModeTrailingEllipsis,
			GTextAlignmentLeft,
			NULL);

	    snprintf(tmp_str,10,"%d mins.",gtimer->sear_minutes);

	    graphics_draw_text(ctx, tmp_str, 
			fonts_get_system_font(FONT_KEY_GOTHIC_24), 
			(GRect) { .origin = { 48, 46 }, .size = { 120,28 } },
			GTextOverflowModeTrailingEllipsis,
			GTextAlignmentLeft,
			NULL);
    }

}

static void update_blink_layer(Layer *layer, GContext* ctx) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, (GRect){ .origin = {0, 0}, .size = {110,30}}, 0, GCornerNone);
}

static void detail_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  text_layer = text_layer_create((GRect) { .origin = { 25, 72 }, .size = { bounds.size.w, 50 } });
  
  GFont custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MONOTYPEWRITER_35));
  text_layer_set_font(text_layer, custom_font);
  text_layer_set_text_alignment(text_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  seconds_layer = text_layer_create((GRect) { .origin = { 107, 87 }, .size = { 30, 40 } });
  GFont sec_custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MONOTYPEWRITER_20));
  text_layer_set_font(seconds_layer, sec_custom_font);
  text_layer_set_text_alignment(seconds_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(seconds_layer));
  
  update_timer_display();
  
  detail_graphics_layer = layer_create(bounds);
  layer_set_update_proc(detail_graphics_layer, update_detail_graphics_layer);
  layer_add_child(window_layer, detail_graphics_layer);

  blink_layer = layer_create(bounds);
  layer_set_update_proc(blink_layer, update_blink_layer);
  layer_set_clips(blink_layer,true);
  layer_set_frame(blink_layer, (GRect){ .origin = {48, 0}, .size = {115,28}});
  layer_add_child(window_layer, blink_layer);

  flip_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SPATULA_TOP_WHITE);
  flip_layer = bitmap_layer_create((GRect) { .origin = {0,40}, .size = { bounds.size.w, 50 } });
  bitmap_layer_set_bitmap(flip_layer, flip_image);
}

static void detail_window_unload(Window *window) {
  text_layer_destroy(text_layer);
  bitmap_layer_destroy(flip_layer);
  gbitmap_destroy(flip_image);
  layer_destroy(blink_layer);
  layer_destroy(detail_graphics_layer);

}

static void detail_window_appear(Window *window) {
  if( grill_timers[current_timer].flip_show ) {
  	layer_add_child(window_get_root_layer(detail_window), bitmap_layer_get_layer(flip_layer));
  }

}

static void detail_window_disappear(Window *window) {
}

/* List Window */
static void update_list_layer(Layer *layer, GContext* ctx) {
    GRect bounds = layer_get_bounds(layer);

    graphics_draw_bitmap_in_rect(ctx, flame_bitmap,(GRect) { .origin = { 7, 8 }, .size = { 14, 20 } });
    graphics_draw_bitmap_in_rect(ctx, flame_bitmap,(GRect) { .origin = { 124, 8 }, .size = { 14, 20 } });

    graphics_context_set_stroke_color(ctx, GColorBlack);

    if( item_count != MAX_GRILL_TIMERS ) {

	    if( highlight_item == 0 ) {
		graphics_fill_rect(ctx, (GRect) { .origin = { 33, 32 }, .size = { 80, 22 } }, 3, GCornersAll);
		graphics_context_set_fill_color(ctx, GColorBlack);
		graphics_context_set_text_color(ctx, GColorWhite);
	    } else {
		graphics_draw_round_rect(ctx, (GRect) { .origin = { 33, 32 }, .size = { 80, 22 } }, 3);
		graphics_context_set_fill_color(ctx, GColorWhite);
		graphics_context_set_text_color(ctx, GColorBlack);
	    }

	    graphics_draw_text(ctx, "Add Timer", 
			fonts_get_system_font(FONT_KEY_GOTHIC_24), 
			(GRect) { .origin = { 40, 26 }, .size = { 80, 25 } },
			GTextOverflowModeTrailingEllipsis,
			GTextAlignmentLeft,
			NULL);
    }

    graphics_draw_line(ctx, (GPoint){ .x = 0, .y=57}, (GPoint) {.x=bounds.size.w, .y=57});

    int i = 0;
    for(; i < item_count; i++ ) {
        graphics_draw_line(ctx, (GPoint){ .x = 0, .y=(81+(i * 25))}, (GPoint) {.x=bounds.size.w, .y=(81+(i * 25))});
	if( i == (highlight_item - 1) ) {
        	graphics_context_set_fill_color(ctx, GColorBlack);
        	graphics_context_set_text_color(ctx, GColorWhite);
		graphics_fill_rect(ctx, (GRect){ .origin = {0,(56+(i * 25))}, .size = { bounds.size.w, 25 }}, 0, GCornerNone);
	} else {
        	graphics_context_set_fill_color(ctx, GColorWhite);
        	graphics_context_set_text_color(ctx, GColorBlack);
	}

	graphics_draw_text(ctx, grill_timers[i].name, 
			fonts_get_system_font(FONT_KEY_GOTHIC_18), 
			(GRect) { .origin = { 0, 56+(i * 25) }, .size = { 80, 21 } },
			GTextOverflowModeTrailingEllipsis,
			GTextAlignmentLeft,
			NULL);

	if( grill_timers[i].hours == 0 && grill_timers[i].minutes == 0 && grill_timers[i].seconds == 0 ) {
		snprintf(tm_str,9,"Ready!");
	} else if( grill_timers[i].flip_show == 1 ) {
		snprintf(tm_str,9,"Flip!");
	} else {
		snprintf(tm_str,9,"%d:%02d:%02d", grill_timers[i].hours, grill_timers[i].minutes, grill_timers[i].seconds);
	}
	graphics_draw_text(ctx, tm_str, 
			fonts_get_system_font(FONT_KEY_GOTHIC_18), 
			(GRect) { .origin = { 0, 56+(i * 25) }, .size = { (bounds.size.w - 5), 21 } },
			GTextOverflowModeTrailingEllipsis,
			GTextAlignmentRight,
			NULL);

    }
}

static void remove_list_item(int timer_to_delete) {
	int i = 0, j = 0;
	for(; i < item_count; i++ ) {
		if( i != timer_to_delete ) {
			grill_timers[j] = grill_timers[i];
			++j;
		}	
	}

	--item_count;
}

static void list_select_click_handler(ClickRecognizerRef recognizer, void *context) {
	if( !layer_get_hidden(delete_item_layer) ) {
	    layer_set_hidden(delete_item_layer, true);
	    if( should_delete ) {
		    remove_list_item(highlight_item - 1);
		    layer_mark_dirty(list_items_layer);
	    }
	} else {

	    if( highlight_item == 0 ) {
		    if( item_count < MAX_GRILL_TIMERS ) {
			    current_timer = item_count;
			    ++item_count;

			    grill_timers[current_timer].name = "Choose";
			    grill_timers[current_timer].hours = 0;
			    grill_timers[current_timer].minutes = 0;
			    grill_timers[current_timer].seconds = 0;
			    grill_timers[current_timer].running = 0;
			    grill_timers[current_timer].blinker_pos = 0;
			    grill_timers[current_timer].blink = 0;
			    grill_timers[current_timer].flip_count = 0;
			    grill_timers[current_timer].sear_minutes = 0;
			    grill_timers[current_timer].food_index = 0;
			    grill_timers[current_timer].heat = 0;
			    grill_timers[current_timer].flip_show = 0;
			    grill_timers[current_timer].flip_time = 99999;

			    window_stack_push(detail_window, true);
			    window_stack_push(food_list_window, true);
		    }
	    } else {
		  current_timer = (highlight_item - 1);
		  window_stack_push(detail_window, true);
	    }
	}
}

static void list_up_click_handler(ClickRecognizerRef recognizer, void *context) {
	if( !layer_get_hidden(delete_item_layer) ) {
	    should_delete = 1;
	    layer_mark_dirty(delete_item_layer);
	} else {
		if( item_count == MAX_GRILL_TIMERS && highlight_item == 1 ) {
			highlight_item = item_count;
		} else if( highlight_item > 0 ) {
			--highlight_item;
		} else {
			highlight_item = item_count;
		}

	    layer_mark_dirty(list_items_layer);
	}
}

static void list_down_click_handler(ClickRecognizerRef recognizer, void *context) {
	if( !layer_get_hidden(delete_item_layer) ) {
	    should_delete = 0;
	    layer_mark_dirty(delete_item_layer);
	} else {
		if( highlight_item < item_count ) {
			++highlight_item;
		} else if (item_count == MAX_GRILL_TIMERS ) {
			highlight_item = 1;
		} else {
			highlight_item = 0;
		}

	    layer_mark_dirty(list_items_layer);
	}
}

static void update_delete_item_layer(Layer *layer, GContext* ctx) {
        GRect bounds = layer_get_bounds(layer);

        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_fill_rect(ctx, bounds, 0, GCornerNone);
        graphics_draw_rect(ctx, bounds);

        graphics_context_set_text_color(ctx, GColorBlack);
	graphics_draw_text(ctx, "Delete Timer?", 
		fonts_get_system_font(FONT_KEY_GOTHIC_24), 
		(GRect) { .origin = { 15, 0 }, .size = { 100, 24 } },
		GTextOverflowModeTrailingEllipsis,
		GTextAlignmentLeft,
		NULL);

	GFont *font = NULL;
	if( should_delete ) {
		font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD); 
	} else {
		font = fonts_get_system_font(FONT_KEY_GOTHIC_18); 
	}

	graphics_draw_text(ctx, "Yes", 
		font,
		(GRect) { .origin = { 25, 25 }, .size = { 80, 24 } },
		GTextOverflowModeTrailingEllipsis,
		GTextAlignmentLeft,
		NULL);

	if( !should_delete ) {
		font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD); 
	} else {
		font = fonts_get_system_font(FONT_KEY_GOTHIC_18); 
	}

	graphics_draw_text(ctx, "No", 
		font, 
		(GRect) { .origin = { 75, 25 }, .size = { 80, 24 } },
		GTextOverflowModeTrailingEllipsis,
		GTextAlignmentLeft,
		NULL);
}

static void list_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    should_delete = 1;
    layer_set_hidden(delete_item_layer,false);
}

static void list_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, list_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, list_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, list_down_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 1500, list_long_click_handler, NULL); 
}

static void list_window_appear(Window *window) {
  if( grill_timers[current_timer].flip_show ) {
  	grill_timers[current_timer].flip_show = 0;
  }
}

static void list_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    
    title_layer = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, 50 } });
    text_layer_set_text(title_layer,"Grill Timer");
    text_layer_set_font(title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(title_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(title_layer));

    list_items_layer = layer_create(bounds);
    layer_set_update_proc(list_items_layer, update_list_layer);
    layer_add_child(window_layer, list_items_layer);

    delete_item_layer = layer_create((GRect) { .origin = { 10, 40 }, .size = { (bounds.size.w -20), 50 } });
    layer_set_update_proc(delete_item_layer, update_delete_item_layer);
    layer_add_child(window_layer, delete_item_layer);
    layer_set_hidden(delete_item_layer,true);

    flame_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FLAME);
}

static void list_window_unload(Window *window) {
  layer_destroy(list_items_layer);
  gbitmap_destroy(flame_bitmap);
}

/* Food Window */
static void food_list_select_callback(int index, void *ctx) {
	grill_timers[current_timer].name = food_items[index].name;
	grill_timers[current_timer].hours = food_items[index].hours;
        grill_timers[current_timer].minutes = food_items[index].minutes;
        grill_timers[current_timer].flip_count = food_items[index].flips;
        grill_timers[current_timer].sear_minutes = food_items[index].sear_minutes;
        grill_timers[current_timer].heat = food_items[index].heat;
        grill_timers[current_timer].food_index = index;

	if( food_items[index].flips > 0 ) {
		grill_timers[current_timer].flip_time = ((( food_items[index].hours * 3600 ) + (food_items[index].minutes * 60)) / (food_items[index].flips+1));
	}

	window_stack_pop(true);
	layer_mark_dirty(detail_graphics_layer);
	update_timer_display();
}

static void food_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    food_list_section[0].num_items = NUM_FOOD_ITEMS;
    food_list_section[0].title = NULL;

    int f = 0;
    for(; f < NUM_FOOD_ITEMS; f++ ) {
	    food_list_item[f].callback = food_list_select_callback;
	    food_list_item[f].icon = NULL;
	    food_list_item[f].title = food_items[f].name;
	    food_list_item[f].subtitle = food_items[f].sub_name;
    } 

    food_list_section[0].items = food_list_item;
    
    food_list_layer = simple_menu_layer_create( bounds, window, food_list_section, 1, NULL);
    layer_add_child(window_layer, simple_menu_layer_get_layer(food_list_layer));
}

static void food_window_unload(Window *window) {
  layer_destroy(simple_menu_layer_get_layer(food_list_layer));
}


/* Initialization */
static void init(void) {
  //Food List Window
  food_list_window = window_create();
  //window_set_click_config_provider(food_list_window, food_click_config_provider);
  window_set_window_handlers(food_list_window, (WindowHandlers) {
    .load = food_window_load,
    .unload = food_window_unload,
  });

  //Detail Window
  detail_window = window_create();
  window_set_click_config_provider(detail_window, detail_click_config_provider);
  window_set_window_handlers(detail_window, (WindowHandlers) {
    .load = detail_window_load,
    .unload = detail_window_unload,
    .disappear = detail_window_disappear,
    .appear = detail_window_appear,
  });

  //List Window
  list_window = window_create();
  window_set_click_config_provider(list_window, list_click_config_provider);
  window_set_window_handlers(list_window, (WindowHandlers) {
        .load = list_window_load,
        .unload = list_window_unload,
    	.appear = list_window_appear,
  });
    
  window_stack_push(list_window, false);

  tick_timer_service_subscribe( SECOND_UNIT, second_callback );

  const uint32_t timeout_ms = 500;
  timer = app_timer_register(timeout_ms, timer_callback, NULL);
}

static void deinit(void) {
  window_destroy(list_window);
  window_destroy(detail_window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", list_window);

  app_event_loop();
  deinit();
}
