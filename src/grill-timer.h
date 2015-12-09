/*FUNCTIONS*/

static void timer_callback(void *);

/* List Window */
static void list_select_click_handler(ClickRecognizerRef , void *);
static void list_up_click_handler(ClickRecognizerRef , void *);
static void list_down_click_handler(ClickRecognizerRef , void *);
static void list_click_config_provider(void *);

/* Detail Window */
static void update_timer_display();
static void second_callback( struct tm *, TimeUnits );
static void update_blink_layer(Layer *, GContext* );
static void detail_select_click_handler(ClickRecognizerRef , void *);
static void detail_up_click_handler(ClickRecognizerRef , void *);
static void detail_down_click_handler(ClickRecognizerRef , void *);
static void detail_click_config_provider(void *);


/* Structs */

typedef struct {
	char* 	name;
	int	flip_count;
	int	food_index;
	int	heat;
	int	sear_minutes;
	int 	hours;
	int 	minutes;
	int 	seconds;
	int 	running;
	int 	blink;	
	int 	blinker_pos;
	int	flip_show;
	int	flip_time;
} GrillTimer;

static char* heat_type[] = {
	"Indirect/Low",
	"Indirect/Med.",
	"Indirect/High",
	"Direct/Low",
	"Direct/Med.",
	"Direct/High"
};
