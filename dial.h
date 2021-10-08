#ifndef DIAL_H
#define DIAL_H

#define TTIP_LR_MARGIN 7
#define TTIP_TB_MARGIN 2
#define TTIP_EDGE_OFFSET 4
#define TTIP_TIP_OFFSET 5
#define TTIP_SIZE 20

#define MIN_SIZE 20
#define MAX_SIZE 300

#define MIN_TICKS 3
#define MAX_TICKS 50

#define MIN_TICK_WIDTH 2
#define MAX_TICK_WIDTH 10

#define MIN_START_ANGLE 91
#define MAX_START_ANGLE 269
#define MIN_END_ANGLE -89
#define MAX_END_ANGLE 89

// more code copied from iemguis
// #define IS_A_FLOAT(atom,index) ((atom+index)->a_type == A_FLOAT)
// #define IS_A_SYMBOL(atom,index) ((atom+index)->a_type == A_SYMBOL)

// in addition to a generic position it is also used to map the starting position to 0 for infinite.
// thus the values that are assigned here. see dial_map_output to see why.
enum positions {BOT = 1, LEFT = 2, TOP = 3, RIGHT = 4};
enum drag_type {H_DRAG, V_DRAG, HV_DRAG, R_DRAG};

#define FLT_EPSILON (1.19209290E-07F)

typedef struct _dial
{
    t_object obj; // the actual pd object
    t_glist *glist; // our owning canvas
    struct _edit_proxy *e_proxy;
    struct _key_listener* listener; // key detection
    struct _tooltip* ttip;
    t_symbol *bindname;
    t_symbol *label;
    int label_position;
    bool show_ttip;
    bool receive_input;
    t_float min, max;
    bool move_on_click;
    int drag_mode;
    int start_position;
    t_float angle; // current angle
    int start_angle, end_angle; // when not infinite start and end of track
    int inf_start_angle; // angle of the starting position when infinite. (seems ugly, how to remove?)
    int margin; // offset from edge of dial
    int mousex, mousey;
    t_symbol *hotkey;
    t_symbol *send, *send_unexpanded;
    t_symbol *rcv, *rcv_unexpanded;
    bool send_state, rcv_state; // can we rcv or send
    bool discrete; // whether or not to snap to values
    int n_ticks;
    int edit_select; // selection via edit mode
    bool infinite;
    bool fine_tune, macro_tune; // control movement speed
    bool dialog_open;
    t_float fine_scale_factor, macro_scale_factor, arrow_scale_factor;
    int xpos, ypos; // topleft coords of dial
    int size;
    int zoom_factor;
    int editmode;
    // int bg_color, track_color, ticks_color, thumb_color, thumb_highlight_color, active_color;
} t_dial;

// we need edit proxy so that we can listen to the messages that are sent to pd itself
typedef struct _edit_proxy
{
    t_object obj;
    t_symbol *bindname;
    t_dial *owner;
} t_edit_proxy;

typedef struct _key_listener
{
    t_object obj;
    t_dial *owner;
} t_key_listener;

// later find a good size for tooltip
typedef struct _tooltip {
    t_object obj;
    t_dial *owner;
    int position;
    char *value;
    int width;
    bool visible;
} t_tooltip;

// Edit Proxy Functions
static t_edit_proxy *edit_proxy_new(t_dial *x, t_symbol *s);
static void edit_proxy_free(t_edit_proxy *p);
static void edit_proxy_any(t_edit_proxy *p, t_symbol *s, int ac, t_atom *av);

// Key Detector Functions
static t_key_listener* key_listener_new(t_dial *dial);
static void key_listener_free(t_key_listener *this);
static void key_listener_list(t_key_listener* listener, t_symbol *s, int argc, t_atom *argv);

// Tooltip Functions
static t_tooltip* tooltip_new(t_dial* dial, int position);
static void tooltip_free(t_tooltip *this);
static void tooltip_text_pos(t_tooltip *this, t_glist *glist, const char* text, int *xp, int *yp);
static void tooltip_erase(t_tooltip *this);
static void tooltip_draw(t_tooltip *this, struct _glist *glist);

// Drawing Functions
static void dial_draw(t_dial *x, t_glist *glist);
static void dial_draw_iolet(t_dial *x); // todo refactor this and the edit proxy
static void dial_erase(t_dial *x, t_glist *glist);
static void dial_update(t_dial *x, t_glist *glist);
static void dial_zoom(t_dial *x, t_floatarg zoom);

// widgetbehavior
static void dial_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2);
static void dial_displace(t_gobj *z, t_glist *glist, int dx, int dy);
static void dial_delete(t_gobj *z, t_glist *glist);
static void dial_vis(t_gobj *z, t_glist *glist, int vis);
static void dial_select(t_gobj *z, t_glist *glist, int sel);
static int dial_click(t_gobj *z, struct _glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit);

// motion and io handling
static t_float dial_map_output(t_dial *this); // map angle to the specified range and return the actual value
static void dial_output(t_dial *this); // output the value
static t_float dial_snap_angle(t_dial *this);
static void dial_mouserelease(t_dial *x);
static void dial_motion(t_dial *x, t_floatarg dx, t_floatarg dy);
static void dial_arrow_motion(t_dial *x, t_symbol arrow);

// message handling
static void dial_float(t_dial *this, t_floatarg value);
static void dial_set(t_dial *this, t_floatarg value);
static void dial_bang(t_dial *this);
static void dial_discrete(t_dial *this, t_floatarg flag);
static void dial_ticks(t_dial *this, t_floatarg n_ticks);
static void dial_infinite(t_dial *this, t_floatarg flag);
static void dial_drag(t_dial *this, t_symbol *mode);
static void dial_range(t_dial *this, t_floatarg min, t_floatarg max);
static void dial_send(t_dial *this, t_symbol *s);
static void dial_receive(t_dial *this, t_symbol *s);
static void dial_size(t_dial *x, t_floatarg f);
static void dial_start_position(t_dial *this, t_symbol *start_position);
static void dial_label(t_dial *this, t_symbol *label);
static void dial_hotkey(t_dial *this, t_symbol *hotkey);

// Dial Functions
static void dial_free(t_dial *x);
static void *dial_new(t_symbol *s, int argc, t_atom *argv);
static void dial_set_start_position(t_dial *this, int start_position);
static void dial_init_send_rcv(t_dial *this, t_symbol *s, t_symbol *r);
static void dial_save(t_gobj *z, t_binbuf *b);
static void dial_properties(t_gobj *z, t_glist *owner); // create and send dial properties to the tcl properties dialog
static void dial_set_param(t_dial *this, t_symbol *unused, int argc, t_atom *argv); // receive messages back from the dialog
void dial_setup(void);

// symbol cleanup functions
// hash2dollar is from iemgui's raute2dollar
static t_symbol *symbol_unescape(t_symbol *s);
static t_symbol *symbol_escape(t_symbol *s);
static t_symbol *symbol_hash2dollar(t_symbol *s);
static t_symbol *symbol_dollar2hash(t_symbol *s);

// dealing with colors
// where should this go? utility or somewhere else...
// static int getcolorarg(int index, int argc, t_atom *argv);

// Utility Functions
// todo figure out a way to clean up snap_angle and dial_snap_angle
static t_float snap_angle(t_dial *this, t_float angle);
static t_float calc_angle_delta(t_dial *this, int dx, int dy);
static inline int sgn(t_float value);
static inline t_float clamp(t_float value, t_float min, t_float max);
static inline t_float to_radians(t_float deg);
static inline t_float to_degrees(t_float rad);
static inline t_float map(t_float value,  t_float inputMin,  t_float inputMax,  t_float outputMin,  t_float outputMax);
static inline t_float snap(t_float value, t_float numerator, t_float denominator);
static char* value_to_str(t_float value);

#endif