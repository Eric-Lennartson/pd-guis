#include "m_pd.h"
#include "g_canvas.h"
#include "stdbool.h"
#include "math.h"
#include "stdlib.h"
#include "dial.h"
#include "string.h"

static t_class *dial_class, *edit_proxy_class, *key_listener_class, *tooltip_class;
static t_widgetbehavior dial_widgetbehavior;

// some stuff I'm keeping around for now
// static int getcolorarg(int index, int argc, t_atom *argv)
// {
//     if(index < 0 || index >= argc)
//         return 0;
//     if( IS_A_FLOAT(argv,index) )
//         return (int)atom_getfloatarg(index, argc, argv);
//     if( IS_A_SYMBOL(argv,index) )
//     {
//         t_symbol *s = atom_getsymbolarg(index, argc, argv);
//         if ('#' == s->s_name[0]) {
//             int col = (int)strtol(s->s_name+1, 0, 16);
//             return col & 0xFFFFFF;
//         }
//     }
//     return 0;
// }

// START SYMBOL CLEANUP ===========================================================================================
static t_symbol *symbol_unescape(t_symbol *s) {
    return (s == gensym("__EMPTY__")) ? &s_ : symbol_hash2dollar(s);
}

static t_symbol *symbol_escape(t_symbol *s) {
    return (s == &s_) ? gensym("__EMPTY__") : symbol_dollar2hash(s);
}

static t_symbol *symbol_dollar2hash(t_symbol *s)
{
    const char *s1;
    char buf[MAXPDSTRING+1], *s2;
    if (strlen(s->s_name) >= MAXPDSTRING)
        return (s);
    for (s1 = s->s_name, s2 = buf; ; s1++, s2++)
    {
        if (*s1 == '$')
            *s2 = '#';
        else if (!(*s2 = *s1))
            break;
    }
    return(gensym(buf));
}

static t_symbol *symbol_hash2dollar(t_symbol *s)
{
    const char *s1;
    char buf[MAXPDSTRING+1], *s2;
    if (strlen(s->s_name) >= MAXPDSTRING)
        return (s);
    for (s1 = s->s_name, s2 = buf; ; s1++, s2++)
    {
        if (*s1 == '#')
            *s2 = '$';
        else if (!(*s2 = *s1))
            break;
    }
    return(gensym(buf));
}
// END SYMBOL CLEANUP ===========================================================================================
// START UTILITY FUNCTIONS ======================================================================================
static t_float snap_angle(t_dial *this, t_float angle)
{
    if(this->infinite){
        angle = snap(angle, 360, this->n_ticks);
    }
    else {
        int range = abs(this->start_angle) + abs(this->end_angle);
        angle += abs(this->end_angle); // keep it positive
        angle = snap(angle, range, this->n_ticks-1);
        angle -= abs(this->end_angle); // put it pack
    }
    return angle;
}

static t_float calc_angle_delta(t_dial *x, int dx, int dy)
{
    t_float newx = clamp(x->mousex + (int)dx, x->xpos, x->xpos + x->size);
    t_float newy = clamp(x->mousey + (int)dy, x->ypos, x->ypos + x->size);
    newx = map(newx, x->xpos, x->xpos+x->size, -1, 1);
    newy = map(newy, x->ypos, x->ypos+x->size, 1, -1);
    t_float oldx = map(x->mousex, x->xpos, x->xpos+x->size, -1, 1);
    t_float oldy = map(x->mousey, x->ypos, x->ypos+x->size, 1, -1);

    t_float oldangle = to_degrees(atan2f(oldy, oldx));
    t_float newangle = to_degrees(atan2f(newy, newx));

    t_float delta = newangle-oldangle;

    // atan2 returns angles from -180 -> 180
    // this accounts for deltas that don't make sense
    if( fabsf(delta) > 300) {
        int sign = sgn(delta);
        delta = sign < 0 ? delta + 360 : delta - 360;
    }

    x->mousex = clamp(x->mousex + (int)dx, x->xpos, x->xpos + x->size);
    x->mousey = clamp(x->mousey + (int)dy, x->ypos, x->ypos + x->size);

    return delta;
}

static inline int sgn(t_float value) { return (0 < value) - (value < 0); }

static inline t_float clamp(t_float value, t_float min, t_float max) {
    return (value > max) ? max :
           (value < min) ? min : value;
}

static inline t_float to_radians(t_float deg) {
    return deg * (M_PI/180);
}

static inline t_float to_degrees(t_float rad) {
    return rad * (180/M_PI);
}

static inline t_float map(t_float value,  t_float inputMin,  t_float inputMax,  t_float outputMin,  t_float outputMax)
{
    if(fabs(inputMin - inputMax) < FLT_EPSILON) {
        return outputMin;
    } else {
        t_float outVal = ((value - inputMin) / (inputMax - inputMin) * (outputMax - outputMin) + outputMin);
        return outVal;
    }
}

static inline t_float snap(t_float value, t_float numerator, t_float denominator) {
    return round(value * denominator / numerator) * numerator / denominator;
}

static char* value_to_str(t_float value)
{
    // char buf[MAXPDSTRING];
    // sprintf(buf, "%g", value);
    int len = snprintf(NULL, 0, "%g", value);
    char *result = (char*)malloc(len + 1);
    snprintf(result, len + 1, "%g", value);
    return result;
}
// END UTILITY FUNCTIONS ===========================================================================================
// START EDIT PROXY FUNCTIONS ======================================================================================
static t_edit_proxy *edit_proxy_new(t_dial *x, t_symbol *s)
{
    t_edit_proxy *p = (t_edit_proxy *)pd_new(edit_proxy_class);
    p->owner = x;
    p->bindname = s;
    pd_bind(&p->obj.ob_pd, p->bindname);
    return (p);
}

static void edit_proxy_free(t_edit_proxy *this)
{
    this->owner = NULL;
    this->bindname = NULL;
    // calling these causes a crash, why idk, but the ELSE library doesn't even call it's own free fn.
    // https://github.com/porres/pd-else/blob/master/Classes/Source/pad.c
    // pd_unbind(&this->obj.ob_pd, this->bindname);
    // pd_free(&this->obj.ob_pd);
}

static void edit_proxy_any(t_edit_proxy *p, t_symbol *s, int ac, t_atom *av)
{
    int edit = ac = 0;
    if (p->owner)
    {
        if (s == gensym("editmode"))
            edit = (int)(av->a_w.w_float);
        else if (s == gensym("obj") || s == gensym("msg") || s == gensym("floatatom") || s == gensym("symbolatom")
                 || s == gensym("text") || s == gensym("bng") || s == gensym("toggle") || s == gensym("numbox") ||
                 s == gensym("vslider") || s == gensym("hslider") || s == gensym("vradio") || s == gensym("hradio")
                 || s == gensym("vumeter") || s == gensym("mycnv") || s == gensym("selectall")) {
            edit = 1;
        }
        else
            return;
        if (p->owner->editmode != edit)
        {
            p->owner->editmode = edit;
            if (edit)
                dial_draw_iolet(p->owner);
            else
            {
                t_canvas *cnv = glist_getcanvas(p->owner->glist);
                sys_vgui(".x%lx.c delete %lx_in\n", cnv, p->owner);
                sys_vgui(".x%lx.c delete %lx_out\n", cnv, p->owner);
            }
        }
    }
}
// END EDIT PROXY FUNCTIONS ========================================================================================
// START KEY DETECTOR FUNCTIONS ====================================================================================
static t_key_listener* key_listener_new(t_dial *dial)
{
    t_key_listener *x = (t_key_listener *)pd_new(key_listener_class);
    pd_bind(&x->obj.ob_pd, gensym("#keyname"));
    x->owner = dial;
    return (x);
}

static void key_listener_free(t_key_listener *this) {
    this->owner = NULL;
    pd_unbind(&this->obj.ob_pd, gensym("#keyname"));
    pd_free(&this->obj.ob_pd);
}

static void key_listener_list(t_key_listener* listener, t_symbol *s, int argc, t_atom *argv)
{
    s = NULL; // remove annoying unused arg warnings
    t_dial *x = listener->owner;
    if(!x->editmode && !x->dialog_open)
    {   // don't listen to input if we're in editmode
        t_symbol key = *atom_getsymbolarg(1, argc, argv);
        bool press_state = atom_getfloatarg(0, argc, argv);

        if(x->receive_input) {
            if( 0 == strcmp("Shift_L", key.s_name) )
                x->fine_tune = !x->fine_tune;
            if( 0 == strcmp("Alt_L", key.s_name) || 0 == strcmp("Meta_L", key.s_name))
                x->macro_tune = !x->macro_tune;
            dial_arrow_motion(x, key);
        }

        // update when we release the hotkey
        if(!press_state && 0 == strcmp(x->hotkey->s_name, key.s_name)) {
            x->receive_input = !x->receive_input;

            if(x->receive_input) {
                sys_vgui(".x%lx.c itemconfigure %lxTHUMBHIGHLIGHT -fill [::pdtk_canvas::get_color %s .x%lx]\n",
                    x->glist, x, "dial_thumb_highlight", x->glist);
            } else {
                dial_mouserelease(x);
                x->fine_tune = false;
                x->macro_tune = false;
            }
        }
    }
}
// END KEY DETECTOR FUNCTIONS ======================================================================================
// START TOOLTIP FUNCTIONS =========================================================================================
static t_tooltip* tooltip_new(t_dial* dial, int position)
{
    t_tooltip *this = (t_tooltip *)pd_new(tooltip_class);
    this->owner = dial;
    this->position = position;
    this->value = "0";
    this->width = 0;
    this->visible = false;

    return this;
}

static void tooltip_free(t_tooltip *this) {
    this->owner = NULL;
    this->value = NULL;
    pd_free(&this->obj.ob_pd);
}

static void tooltip_text_pos(t_tooltip *this, t_glist *glist, const char* text, int *xp, int *yp)
{
    // look into [makefilename %s] <- will these shenanigans go away?
    t_dial *owner = this->owner;
    int x1 = owner->xpos, y1 = owner->ypos;

    int txt_width  = (int)strlen(text) * glist_fontwidth(glist);
    this->width = txt_width;
    int txt_height = glist_fontheight(glist);

    switch(this->position)
    {   // there's this fiddly little bit of positioning I have to do,
        // try and figure that out at some point
        case TOP:
        {
            *xp = x1 + txt_width/2 + (owner->size/2 - txt_width/2);
            *yp = y1 - 5 - txt_height/2 - 3; // margin - fontheight/2 - tooltipstuff
            break;
        }
        case RIGHT:
            *xp = x1 + owner->size + txt_width/2 + 5 + 5; // margin + tooltipstuff
            *yp = y1 + txt_height/2 + (owner->size/2 - txt_height/2) + 1;
            break;
        case BOT:
            *xp = x1 + txt_width/2 + (owner->size/2 - txt_width/2);
            *yp = y1 + 5 + owner->size + txt_height/2 + 5; // margin - fontheight/2 - tooltipstuff
            break;
        case LEFT:
            *xp = x1 - txt_width/2 - 5 - 5; // margin + tooltipstuff
            *yp = y1 + txt_height/2 + (owner->size/2 - txt_height/2) + 1;
            break;
    }
}

static void tooltip_erase(t_tooltip *this)
{
    sys_vgui(".x%lx.c delete %lxTOOLTIP\n", this->owner->glist, this->owner);
    sys_vgui(".x%lx.c delete %lxTOOLTIP_VALUE\n", this->owner->glist, this->owner);
    this->visible = false;
}

// later the values when calling tan
// could probably also be turned into macros
// to be less magic numbery
static void tooltip_draw(t_tooltip *this, t_glist *glist)
{
    t_dial *owner = this->owner;
    owner->xpos = text_xpix(&owner->obj, glist);
    owner->ypos = text_ypix(&owner->obj, glist);

    int text_xpos, text_ypos;
    char* text = value_to_str( dial_map_output(owner) );
    tooltip_text_pos(this, glist, text, &text_xpos, &text_ypos);

    if(this->visible) {
        tooltip_erase(this);
    }

    t_glist *cnv = owner->glist;

    switch(this->position)
    {
        case TOP:
        {
            sys_vgui(".x%lx.c create polygon %d %d %d %d %d %d %d %d %d %d %d %d %d %d -width %d "
                     "-outline [::pdtk_canvas::get_color %s .x%lx] -fill [::pdtk_canvas::get_color %s .x%lx] -tags %lxTOOLTIP\n",
                     cnv,
                     owner->xpos + owner->size/2, owner->ypos - TTIP_EDGE_OFFSET,                                      // triangle tip
                     owner->xpos + owner->size/2 - (int)(( 5/tan(M_PI/6) )-2), owner->ypos - TTIP_EDGE_OFFSET - TTIP_TIP_OFFSET,      // tip left
                     owner->xpos + owner->size/2 - this->width/2 - TTIP_TB_MARGIN, owner->ypos - TTIP_EDGE_OFFSET -TTIP_TIP_OFFSET,  // bot left
                     owner->xpos + owner->size/2 - this->width/2 - TTIP_TB_MARGIN, owner->ypos - TTIP_EDGE_OFFSET - TTIP_SIZE, // top left
                     owner->xpos + owner->size/2 + this->width/2 + TTIP_TB_MARGIN, owner->ypos - TTIP_EDGE_OFFSET - TTIP_SIZE, // top right
                     owner->xpos + owner->size/2 + this->width/2 + TTIP_TB_MARGIN, owner->ypos - TTIP_EDGE_OFFSET -TTIP_TIP_OFFSET,  // bot right
                     owner->xpos + owner->size/2 + (int)(( 5/tan(M_PI/6) )-2), owner->ypos - TTIP_EDGE_OFFSET -TTIP_TIP_OFFSET,      // tip right
                     owner->zoom_factor,
                     "tooltip_border", cnv,
                     "tooltip_fill", cnv, owner);

            sys_vgui(".x%lx.c create text %d %d -text {%s} -justify center \
                -font {{%s} -%d %s} -fill [::pdtk_canvas::get_color %s .x%lx] -tags %lxTOOLTIP_VALUE\n",
                cnv,
                text_xpos, text_ypos,
                text, "menlo", 12, sys_fontweight,
                "tooltip_text", cnv,
                owner);
            break;
        }

        case RIGHT:
        {
            sys_vgui(".x%lx.c create polygon %d %d %d %d %d %d %d %d %d %d %d %d %d %d -width %d "
                     "-outline [::pdtk_canvas::get_color %s .x%lx] -fill [::pdtk_canvas::get_color %s .x%lx] -tags %lxTOOLTIP\n",
                     cnv,
                     owner->xpos + owner->size + TTIP_EDGE_OFFSET, owner->ypos + owner->size/2,                                                           // triangle tip
                     owner->xpos + owner->size + TTIP_EDGE_OFFSET + TTIP_TIP_OFFSET, owner->ypos + owner->size/2 - (int)(5*tan(M_PI/6))-2,                              // tip top
                     owner->xpos + owner->size + TTIP_EDGE_OFFSET + TTIP_TIP_OFFSET, owner->ypos + owner->size/2 - (int)(5*tan(M_PI/6))-2-TTIP_TIP_OFFSET,                            // top left
                     owner->xpos + owner->size + TTIP_EDGE_OFFSET + this->width + TTIP_LR_MARGIN, owner->ypos + owner->size/2 - (int)(5*tan(M_PI/6))-2-TTIP_TIP_OFFSET, // top right
                     owner->xpos + owner->size + TTIP_EDGE_OFFSET + this->width + TTIP_LR_MARGIN, owner->ypos + owner->size/2 + (int)(5*tan(M_PI/6))+2+TTIP_TIP_OFFSET, // bot right
                     owner->xpos + owner->size + TTIP_EDGE_OFFSET + TTIP_TIP_OFFSET, owner->ypos + owner->size/2 + (int)(5*tan(M_PI/6))+2+TTIP_TIP_OFFSET,                            // bot left
                     owner->xpos + owner->size + TTIP_EDGE_OFFSET + TTIP_TIP_OFFSET, owner->ypos + owner->size/2 + (int)(5*tan(M_PI/6))+2,                              // tip bot
                     owner->zoom_factor,
                     "tooltip_border", cnv,
                     "tooltip_fill", cnv, owner);

            sys_vgui(".x%lx.c create text %d %d -text {%s} -justify center \
                -font {{%s} -%d %s} -fill [::pdtk_canvas::get_color %s .x%lx] -tags %lxTOOLTIP_VALUE\n",
                cnv,
                text_xpos, text_ypos,
                text, "menlo", 12, sys_fontweight,
                "tooltip_text", cnv,
                owner);
            break;
        }
        case LEFT:
        {
            sys_vgui(".x%lx.c create polygon %d %d %d %d %d %d %d %d %d %d %d %d %d %d -width %d "
                     "-outline [::pdtk_canvas::get_color %s .x%lx] -fill [::pdtk_canvas::get_color %s .x%lx] -tags %lxTOOLTIP\n",
                     cnv,
                     owner->xpos - TTIP_EDGE_OFFSET, owner->ypos + owner->size/2,                                                           // triangle tip
                     owner->xpos - TTIP_EDGE_OFFSET - TTIP_TIP_OFFSET, owner->ypos + owner->size/2 - (int)(5*tan(M_PI/6))-2,                              // tip top
                     owner->xpos - TTIP_EDGE_OFFSET - TTIP_TIP_OFFSET, owner->ypos + owner->size/2 - (int)(5*tan(M_PI/6))-2-TTIP_TIP_OFFSET,                            // top left
                     owner->xpos - TTIP_EDGE_OFFSET - this->width - TTIP_LR_MARGIN, owner->ypos + owner->size/2 - (int)(5*tan(M_PI/6))-2-TTIP_TIP_OFFSET, // top right
                     owner->xpos - TTIP_EDGE_OFFSET - this->width - TTIP_LR_MARGIN, owner->ypos + owner->size/2 + (int)(5*tan(M_PI/6))+2+TTIP_TIP_OFFSET, // bot right
                     owner->xpos - TTIP_EDGE_OFFSET - TTIP_TIP_OFFSET, owner->ypos + owner->size/2 + (int)(5*tan(M_PI/6))+2+TTIP_TIP_OFFSET,                            // bot left
                     owner->xpos - TTIP_EDGE_OFFSET - TTIP_TIP_OFFSET, owner->ypos + owner->size/2 + (int)(5*tan(M_PI/6))+2,                              // tip bot
                     owner->zoom_factor,
                     "tooltip_border", cnv,
                     "tooltip_fill", cnv, owner);

            sys_vgui(".x%lx.c create text %d %d -text {%s} -justify center \
                -font {{%s} -%d %s} -fill [::pdtk_canvas::get_color %s .x%lx] -tags %lxTOOLTIP_VALUE\n",
                cnv,
                text_xpos, text_ypos,
                text, "menlo", 12, sys_fontweight,
                "tooltip_text", cnv,
                owner);
            break;
        }
        case BOT:
        {
            sys_vgui(".x%lx.c create polygon %d %d %d %d %d %d %d %d %d %d %d %d %d %d -width %d "
                     "-outline [::pdtk_canvas::get_color %s .x%lx] -fill [::pdtk_canvas::get_color %s .x%lx] -tags %lxTOOLTIP\n",
                     cnv,
                     owner->xpos + owner->size/2, owner->ypos + owner->size + TTIP_EDGE_OFFSET,                                             // triangle tip
                     owner->xpos + owner->size/2 - (int)(( 5/tan(to_radians(30.0)) )-2), owner->ypos + owner->size + TTIP_EDGE_OFFSET + TTIP_TIP_OFFSET,  // tip left
                     owner->xpos + owner->size/2 - this->width/2 - TTIP_TB_MARGIN, owner->ypos + owner->size + TTIP_EDGE_OFFSET + TTIP_TIP_OFFSET,        // top left
                     owner->xpos + owner->size/2 - this->width/2 - TTIP_TB_MARGIN, owner->ypos + owner->size + TTIP_EDGE_OFFSET + TTIP_SIZE,       // bot left
                     owner->xpos + owner->size/2 + this->width/2 + TTIP_TB_MARGIN, owner->ypos + owner->size + TTIP_EDGE_OFFSET + TTIP_SIZE,       // bot right
                     owner->xpos + owner->size/2 + this->width/2 + TTIP_TB_MARGIN, owner->ypos + owner->size + TTIP_EDGE_OFFSET + TTIP_TIP_OFFSET,        // top right
                     owner->xpos + owner->size/2 + (int)(( 5/tan(to_radians(30.0)) )-2),  owner->ypos + owner->size + TTIP_EDGE_OFFSET + TTIP_TIP_OFFSET, // tip right
                     owner->zoom_factor,
                     "tooltip_border", cnv,
                     "tooltip_fill", cnv, owner);

            sys_vgui(".x%lx.c create text %d %d -text {%s} -justify center \
                -font {{%s} -%d %s} -fill [::pdtk_canvas::get_color %s .x%lx] -tags %lxTOOLTIP_VALUE\n",
                cnv,
                text_xpos, text_ypos,
                text, "menlo", 12, sys_fontweight,
                "tooltip_text", cnv,
                owner);
            break;
        }
    }
    this->visible = true;
    free(text);
}
// END TOOLTIP FUNCTIONS ===========================================================================================
// START DRAWING FUNCTIONS =========================================================================================
static void dial_draw(t_dial *x, t_glist *glist)
{
    t_canvas *cnv = glist_getcanvas(glist);
    x->xpos = text_xpix(&x->obj, glist);
    x->ypos = text_ypix(&x->obj, glist);

    int track_width, thumb_width, thumb_highlight_width;

    if(x->size < 40) {
        x->margin = 1;
        track_width = 2;
        thumb_width = 2;
        thumb_highlight_width = thumb_width+1;
    } else if(x->size < 60) {
        x->margin = 3;
        track_width = 3;
        thumb_width = 3;
        thumb_highlight_width = thumb_width+1;
    } else if(x->size < 80) {
        x->margin = 3;
        track_width = 3;
        thumb_width = 4;
        thumb_highlight_width = thumb_width+1;
    } else if(80 <= x->size && x->size < 200) {
        x->margin = 5;
        track_width = 5;
        thumb_width = 4;
        thumb_highlight_width = thumb_width+2;
    } else {
        x->margin = 6;
        track_width = 6;
        thumb_width = 4;
        thumb_highlight_width = thumb_width+3;
    }

    sys_vgui(".x%lx.c create oval %d %d %d %d -width %d "
            "-outline [::pdtk_canvas::get_color %s .x%lx] "
            "-fill [::pdtk_canvas::get_color %s .x%lx] "
            "-tags %lxBASE\n",
            cnv,
            x->xpos, x->ypos, // rectangle coords
            x->xpos + x->size * x->zoom_factor,
            x->ypos + x->size * x->zoom_factor,
            x->zoom_factor, // width
            x->edit_select ? "selected" : "dial_background", cnv,
            "dial_background", cnv,
            x);

    // need to make buffer and width size dependent, possibly zoom as well
    if(!x->infinite)
    {
        sys_vgui(".x%lx.c create arc %d %d %d %d -outline [::pdtk_canvas::get_color %s .x%lx] "
                 "-width %d -start %d -extent %d -style arc "
                 "-tags %lxTRACK\n",
                cnv,
                x->xpos + x->margin, x->ypos + x->margin,
                x->xpos + (x->size * x->zoom_factor) - x->margin,
                x->ypos + (x->size * x->zoom_factor) - x->margin,
                "dial_track", cnv,
                track_width,
                x->end_angle,
                x->start_angle + -x->end_angle, // to get proper extent we add the opposite of end angle
                x);
    }
    else
    {
        sys_vgui(".x%lx.c create oval %d %d %d %d -outline [::pdtk_canvas::get_color %s .x%lx] -fill \"\" -width %d -tags %lxTRACK\n",
                cnv,
                x->xpos + x->margin, x->ypos + x->margin,
                x->xpos + (x->size * x->zoom_factor) - x->margin,
                x->ypos + (x->size * x->zoom_factor) - x->margin,
                "dial_track", cnv,
                track_width,
                x);
    }

    if(x->discrete)
    {
        int range, a_start, a_end;
        t_float a_step;
        if(x->infinite) {
            a_step = 360.f / x->n_ticks;
            a_start = 0;
            a_end = 360;
        } else {
            range = abs(x->start_angle) + abs(x->end_angle);
            a_step = (float)range / (float)(x->n_ticks-1);
            a_start = x->end_angle;
            a_end = x->start_angle;
        }

        int tick_width = map(x->n_ticks, MIN_TICKS, MAX_TICKS, MAX_TICK_WIDTH, MIN_TICK_WIDTH);

        for(t_float a = a_start; a <= a_end; a+=a_step)
        {  // using arcs instead circles b/c they don't suffer from int issues
            sys_vgui(".x%lx.c create arc %d %d %d %d -outline [::pdtk_canvas::get_color %s .x%lx] "
                     "-width %d -start %d -extent %d -style arc -tags %lxTICKS\n",
                    cnv,
                    x->xpos + x->margin, x->ypos + x->margin,
                    x->xpos + (x->size * x->zoom_factor) - x->margin,
                    x->ypos + (x->size * x->zoom_factor) - x->margin,
                    "dial_ticks", cnv,
                    track_width,
                    (int)(a - (tick_width/2.f)), tick_width, x); // subtract half of the extent from the angle to make it centered
        }
    }

    if(!x->infinite)
    {
        sys_vgui(".x%lx.c create arc %d %d %d %d -outline [::pdtk_canvas::get_color %s .x%lx] "
                 "-width %d -start %d -extent %d -style arc -tags %lxACTIVE\n",
                 cnv,
                 x->xpos + x->margin, x->ypos + x->margin,
                 x->xpos + (x->size * x->zoom_factor) - x->margin,
                 x->ypos + (x->size * x->zoom_factor) - x->margin,
                 "dial_active", cnv, track_width,
                 (int)x->angle, (int)(x->start_angle-x->angle), x);
    }
    else
    {
        int start_angle = (x->discrete) ? snap_angle(x, x->inf_start_angle) : x->inf_start_angle;

        sys_vgui(".x%lx.c create arc %d %d %d %d -outline [::pdtk_canvas::get_color %s .x%lx] "
                 "-width %d -start %d -extent %d -style arc -tags %lxACTIVE\n",
                 cnv,
                 x->xpos + x->margin, x->ypos + x->margin, // margin + buffer from track
                 x->xpos + (x->size * x->zoom_factor) - x->margin,
                 x->ypos + (x->size * x->zoom_factor) - x->margin,
                 "dial_active", cnv, track_width,
                 start_angle, (int)(start_angle - x->angle), x);
    }

    int label_x = 0, label_y = 0;
    switch(x->label_position) {
        case TOP: {
                label_x = x->xpos;
                label_y = x->ypos - x->margin*2;
            break;
        }
        case BOT: {
                label_x = x->xpos;
                label_y = x->ypos + x->size + x->margin*2;
            break;
        }
        case RIGHT: {
                label_x = x->xpos + x->size + x->margin;
                label_y = x->ypos + x->size/2;
            break;
        }
        case LEFT: {
                label_x = x->xpos - (int)strlen( canvas_realizedollar(x->glist, x->label)->s_name ) * glist_fontwidth(glist) - x->margin;
                label_y = x->ypos + x->size/2;
            break;
        }
    }

    sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w -font {{menlo} -%d %s} "
             "-fill [::pdtk_canvas::get_color %s .x%lx] -tags %lxLABEL\n",
             cnv, label_x, label_y,
             canvas_realizedollar(x->glist, x->label)->s_name,
             12, sys_fontweight, "dial_label", cnv, x);

    // calculate the center
    int x1 = x->xpos + (x->size*x->zoom_factor)/2;
    int y1 = x->ypos + (x->size*x->zoom_factor)/2;

    int radius = (x->size*x->zoom_factor)/2 - x->margin;
    int x2 = (int)(radius*cos( to_radians(x->angle))+x1);
    // reverse radius b/c y coords are reversed in tcl/tk
    int y2 = (int)(-radius*sin( to_radians(x->angle) )+y1);

    sys_vgui(".x%lx.c create line %d %d %d %d -fill \"\" -width %d -smooth true -splinesteps 5 -capstyle round -tags %lxTHUMBHIGHLIGHT\n",
            x->glist, x1, y1, x2, y2, thumb_highlight_width, x);

    sys_vgui(".x%lx.c create line %d %d %d %d -fill [::pdtk_canvas::get_color %s .x%lx] -width %d -smooth true -splinesteps 5 -capstyle round -tags %lxTHUMB\n",
            cnv, x1, y1, x2, y2, "dial_thumb", cnv, thumb_width, x);

    dial_draw_iolet(x);
}
static void dial_draw_iolet(t_dial *x) {
    if (x->editmode) {
        t_canvas *cnv = glist_getcanvas(x->glist);
        int xpos = text_xpix(&x->obj, x->glist);
        int ypos = text_ypix(&x->obj, x->glist);

        if(!x->rcv_state) {
            sys_vgui(".x%lx.c create rectangle %d %d %d %d "
                    "-fill [::pdtk_canvas::get_color %s .x%lx] "
                    "-outline [::pdtk_canvas::get_color %s .x%lx] "
                    "-tags %lx_in\n",
                    cnv,
                    xpos, ypos,
                    xpos + (IOWIDTH * x->zoom_factor),
                    ypos + (IHEIGHT * x->zoom_factor),
                    "dial_iolet", cnv,
                    "dial_iolet_border", cnv,
                    x);
        }

        if(!x->send_state) {
            sys_vgui(".x%lx.c create rectangle %d %d %d %d "
                    "-fill [::pdtk_canvas::get_color %s .x%lx] "
                    "-outline [::pdtk_canvas::get_color %s .x%lx] "
                    "-tags %lx_out\n",
                    cnv,
                    xpos, ypos + x->size,
                    xpos + IOWIDTH * x->zoom_factor,
                    ypos + x->size - IHEIGHT * x->zoom_factor,
                    "dial_iolet", cnv,
                    "dial_iolet_border", cnv,
                    x);
        }
    }
}
static void dial_erase(t_dial *x, t_glist *glist)
{
    t_canvas *cnv = glist_getcanvas(glist);
    sys_vgui(".x%lx.c delete %lxBASE\n", cnv, x);
    sys_vgui(".x%lx.c delete %lxTRACK\n", cnv, x);
    sys_vgui(".x%lx.c delete %lxTHUMB\n", cnv, x);
    sys_vgui(".x%lx.c delete %lxTHUMBHIGHLIGHT\n", cnv, x);
    sys_vgui(".x%lx.c delete %lxACTIVE\n", cnv, x);
    sys_vgui(".x%lx.c delete %lxLABEL\n", cnv, x);
    if(x->discrete) {
        sys_vgui(".x%lx.c delete %lxTICKS\n", cnv, x);
    }
    sys_vgui(".x%lx.c delete %lx_in\n", cnv, x);
    sys_vgui(".x%lx.c delete %lx_out\n", cnv, x);
}
static void dial_update(t_dial *x, t_glist *glist)
{
    if (glist_isvisible(glist) && gobj_shouldvis((t_gobj *)x, glist))
    {
        int xpos = text_xpix(&x->obj, glist);
        int ypos = text_ypix(&x->obj, glist);
        t_canvas *cnv = glist_getcanvas(glist);

        // first find the center of the gui
        int x1 = xpos + (x->size*x->zoom_factor)/2;
        int y1 = ypos + (x->size*x->zoom_factor)/2;

        t_float angle = x->angle;

        if(x->discrete)
            angle = dial_snap_angle(x);

        angle = x->infinite ? angle : clamp(angle, x->end_angle, x->start_angle);

        int radius = (x->size*x->zoom_factor)/2 - x->margin;
        int x2 = (int)(radius*cos( to_radians(angle))+x1);
        // reverse radius b/c y coords are reversed in tcl/tk
        int y2 = (int)(-radius*sin( to_radians(angle) )+y1);

        sys_vgui(".x%lx.c coords %lxTHUMBHIGHLIGHT %d %d %d %d\n",
            cnv, x, x1, y1, x2, y2);
        sys_vgui(".x%lx.c coords %lxTHUMB %d %d %d %d\n",
            cnv, x, x1, y1, x2, y2);

        if(!x->infinite) {
            sys_vgui(".x%lx.c itemconfigure %lxACTIVE -start %d -extent %d\n",
                cnv, x, (int)angle, (int)(x->start_angle - angle));
        }
        else
        {   // use a temporary variable to prevent x's value from changing
            int inf_start_angle = (x->discrete) ? snap_angle(x, x->inf_start_angle) : x->inf_start_angle;

            sys_vgui(".x%lx.c itemconfigure %lxACTIVE -start %d -extent %d\n",
                cnv, x, inf_start_angle, (int)angle - inf_start_angle);
        }

        canvas_fixlinesfor(glist, (t_text *)x);
    }
}
static void dial_zoom(t_dial *x, t_floatarg zoom) {
    post("called");
    x->zoom_factor = (int)zoom;
}
// END DRAWING FUNCTIONS ==========================================================================================
// START WIDGET BEHAVIOR FUNCTIONS ================================================================================
static void dial_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_dial *x = (t_dial *)z;
    *xp1 = text_xpix(&x->obj, glist);
    *yp1 = text_ypix(&x->obj, glist);
    *xp2 = *xp1 + x->size * x->zoom_factor;
    *yp2 = *yp1 + x->size * x->zoom_factor;
}
static void dial_displace(t_gobj *z, t_glist *glist, int dx, int dy)
{
    t_dial *x = (t_dial *)z;
    x->obj.te_xpix += dx, x->obj.te_ypix += dy;
    t_canvas *cnv = glist_getcanvas(glist);
    sys_vgui(".x%lx.c move %lxBASE %d %d\n", cnv, x, dx * x->zoom_factor, dy * x->zoom_factor);
    sys_vgui(".x%lx.c move %lxTRACK %d %d\n", cnv, x, dx * x->zoom_factor, dy * x->zoom_factor);
    sys_vgui(".x%lx.c move %lxTHUMBHIGHLIGHT %d %d\n", cnv, x, dx * x->zoom_factor, dy * x->zoom_factor);
    sys_vgui(".x%lx.c move %lxTHUMB %d %d\n", cnv, x, dx * x->zoom_factor, dy * x->zoom_factor);
    sys_vgui(".x%lx.c move %lxACTIVE %d %d\n", cnv, x, dx * x->zoom_factor, dy * x->zoom_factor);
    sys_vgui(".x%lx.c move %lxLABEL %d %d\n", cnv, x, dx * x->zoom_factor, dy * x->zoom_factor);
    sys_vgui(".x%lx.c move %lx_in %d %d\n", cnv, x, dx * x->zoom_factor, dy * x->zoom_factor);
    sys_vgui(".x%lx.c move %lx_out %d %d\n", cnv, x, dx * x->zoom_factor, dy * x->zoom_factor);
    if(x->discrete) {
        sys_vgui(".x%lx.c move %lxTICKS %d %d\n", cnv, x, dx * x->zoom_factor, dy * x->zoom_factor);
    }
    canvas_fixlinesfor(glist, (t_text *)x);
}
static void dial_delete(t_gobj *z, t_glist *glist) {
    canvas_deletelinesfor(glist, (t_text *)z);
}
static void dial_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_dial *x = (t_dial *)z;
    t_canvas *cnv = glist_getcanvas(glist);
    if(vis)
    {
        dial_draw(x, glist);

        // later is there someway to have bind a keypress to the gui itself?

        // make sure no matter what part we click on we can call mouserelease
        sys_vgui(".x%lx.c bind %lxBASE <ButtonRelease> {pdsend [concat %s _mouserelease \\;]}\n",
            cnv, x, x->bindname->s_name);
        sys_vgui(".x%lx.c bind %lxTRACK <ButtonRelease> {pdsend [concat %s _mouserelease \\;]}\n",
            cnv, x, x->bindname->s_name);
        sys_vgui(".x%lx.c bind %lxACTIVE <ButtonRelease> {pdsend [concat %s _mouserelease \\;]}\n",
            cnv, x, x->bindname->s_name);
        sys_vgui(".x%lx.c bind %lxTHUMB <ButtonRelease> {pdsend [concat %s _mouserelease \\;]}\n",
            cnv, x, x->bindname->s_name);
        sys_vgui(".x%lx.c bind %lxTHUMBHIGHLIGHT <ButtonRelease> {pdsend [concat %s _mouserelease \\;]}\n",
            cnv, x, x->bindname->s_name);
        if(x->discrete) {
            sys_vgui(".x%lx.c bind %lxTICKS <ButtonRelease> {pdsend [concat %s _mouserelease \\;]}\n",
                cnv, x, x->bindname->s_name);
        }
    }
    else {
        dial_erase(x, glist);
    }
}
static void dial_select(t_gobj *z, t_glist *glist, int sel)
{
    t_dial *x = (t_dial *)z;
    t_canvas *cnv = glist_getcanvas(glist);
    if ((x->edit_select = sel))
        sys_vgui(".x%lx.c itemconfigure %lxBASE -outline [::pdtk_canvas::get_color %s .x%lx]\n",
            cnv, x, "selected", cnv);
    else
        sys_vgui(".x%lx.c itemconfigure %lxBASE -outline [::pdtk_canvas::get_color %s .x%lx]\n",
            cnv, x, "dial_background", cnv);
}
    // create a dial_click wrapper function to avoid copy cat code in the key_list function?
static int dial_click(t_gobj *z, struct _glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    dbl = 0; // remove unused variable warnings.
    t_dial *this = (t_dial *)z;
    this->xpos = text_xpix(&this->obj, glist);
    this->ypos = text_ypix(&this->obj, glist);

    if(doit)
    {
        this->fine_tune = !this->discrete && shift; // slow moving on shift
        this->macro_tune = !this->discrete && alt; // fast moving on alt/cmd

        this->mousex = xpix;
        this->mousey = ypix;

        if(this->move_on_click && !this->infinite)
        {
            t_float x = map(this->mousex, this->xpos, this->xpos+this->size, -1, 1);
            t_float y = map(this->mousey, this->ypos, this->ypos+this->size, 1, -1);
            t_float new_angle = to_degrees(atan2f(y, x));
            new_angle = (new_angle <= -90) ? new_angle + 360 : new_angle;
            this->angle = clamp(new_angle, this->end_angle, this->start_angle);

            dial_output(this);
            dial_update(this, this->glist);
        }

        if(this->show_ttip)
            tooltip_draw(this->ttip, glist);

        sys_vgui(".x%lx.c itemconfigure %lxTHUMBHIGHLIGHT -fill [::pdtk_canvas::get_color %s .x%lx]\n",
                this->glist, this, "dial_thumb_highlight", this->glist);

        // get the mouse movement and call the motion function
        glist_grab(this->glist, &this->obj.te_g, (t_glistmotionfn)dial_motion, 0, (float)xpix, (float)ypix);
        dial_output(this);
    }

    return (1);
}
// END WIDGET BEHAVIOR FUNCTIONS ==================================================================================
// START MOTION AND IO FUNCTIONS ==================================================================================
static t_float dial_map_output(t_dial *this)
{
    t_float output = 0.f;
    t_float angle = this->angle;
    // later is there a good way to make this a little more readable?
    if(this->infinite) {
        if(this->discrete)
            angle = dial_snap_angle(this);
        // offset by start_position to make start_position output 0.
        output = map(angle + (this->start_position * 90), 0, 360, this->max, this->min);
    } else {
        this->angle = clamp(this->angle, this->end_angle, this->start_angle);
        if(this->discrete)
           angle = dial_snap_angle(this);
        output = map(angle, this->start_angle, this->end_angle, this->min, this->max);
    }

    return output;
}
static void dial_output(t_dial *this)
{
    t_float output = dial_map_output(this);
    outlet_float(this->obj.ob_outlet, output);
    if(this->send_state && this->send->s_thing)
        pd_float(this->send->s_thing, output);
}
static t_float dial_snap_angle(t_dial *this)
{
    t_float angle = this->angle;

    if(this->infinite){
        angle = snap(angle, 360, this->n_ticks);
    }
    else {
        int range = abs(this->start_angle) + abs(this->end_angle);
        angle += abs(this->end_angle); // keep it positive
        angle = snap(angle, range, this->n_ticks-1);
        angle -= abs(this->end_angle); // put it pack
    }
    return angle;
}
static void dial_mouserelease(t_dial *x) {
    sys_vgui(".x%lx.c itemconfigure %lxTHUMBHIGHLIGHT -fill \"\" \n", x->glist, x);
    if(x->show_ttip)
        tooltip_erase(x->ttip);
}
static void dial_motion(t_dial *x, t_floatarg dx, t_floatarg dy)
{
    t_float fine_scale = x->fine_tune ? x->fine_scale_factor : 1.f;
    t_float macro_scale = x->macro_tune ? x->macro_scale_factor : 1.f;
    // move faster when tick count is low magic numbers are constants that I thought worked well
    t_float tick_scale = x->n_ticks <= 9 ? map(x->n_ticks, 3, 9, 2.25f, 1.1f) : 1.f;

    t_float scaling = fine_scale * macro_scale * tick_scale;

    switch (x->drag_mode)
    {
        case V_DRAG: {
            x->angle += dy * scaling; // down -, up +
            break;
        }
        case H_DRAG: {
            x->angle -= dx * scaling; // left -, right +
            break;
        }
        case HV_DRAG: {
            x->angle += dy * scaling;
            x->angle -= dx * scaling;
            break;
        }
        case R_DRAG: {
            x->angle += calc_angle_delta(x, dx, dy) * scaling;
            break;
        }
    }
    dial_output(x);

    if(x->show_ttip)
        tooltip_draw(x->ttip, x->glist);

    dial_update(x, x->glist);
}
static void dial_arrow_motion(t_dial *x, t_symbol arrow) {
    int dir = 0;
    if(0 == strcmp(arrow.s_name, "Left") )
        dir = -1;
    else if(0 == strcmp(arrow.s_name, "Right") )
        dir = 1;

    t_float fine_scale = x->fine_tune ? x->fine_scale_factor : 1.f;
    t_float macro_scale = x->macro_tune ? x->macro_scale_factor : 1.f;
    // move faster when tick count is low (more contstants that I chose)
    t_float tick_scale = x->n_ticks <= 9 ? map(x->n_ticks, 3, 9, 2.25f, 1.1f) : 1.f;

    t_float scaling = fine_scale * macro_scale * tick_scale * x->arrow_scale_factor;

    x->angle -= dir * scaling;

    dial_output(x);

    if(x->show_ttip)
        tooltip_draw(x->ttip, x->glist);

    dial_update(x, x->glist);
}
// END MOTION AND IO FUNCTIONS ====================================================================================
// START MESSAGE HANDLING FUNCTIONS ===============================================================================
static void dial_float(t_dial *this, t_floatarg value)
{
    // dial doesn't keep track of the actual float value
    // it just maps it from the current angle
    // so we have to map a value to an angle and then output it.
    // since this is a reverse mapping, we flip the angles around
    if(this->infinite) {
        this->angle = map(value, this->min, this->max, 360, 0) - (this->start_position * 90);
    }
    else {
        value = clamp(value, this->min, this->max);
        this->angle = map(value, this->min, this->max, this->start_angle, this->end_angle);
    }
    dial_update(this, this->glist);
    dial_output(this);
}
static void dial_set(t_dial *this, t_floatarg value)
{   // same as dial_float but w/o calling dial_output
    // see dial_float for an explanation of the mapping
    if(this->infinite) {
        this->angle = map(value, this->min, this->max, 360, 0) - (this->start_position * 90);
    }
    else {
        value = clamp(value, this->min, this->max);
        this->angle = map(value, this->min, this->max, this->start_angle, this->end_angle);
    }
    dial_update(this, this->glist);
}
static void dial_bang(t_dial *this) { dial_output(this); }
static void dial_discrete(t_dial *this, t_floatarg flag)
{
    if(this->discrete != flag)
    {   // erase first, b/c we need discrete to be in the current state when drawing
        dial_erase(this, this->glist);
        this->discrete = flag;
        dial_draw(this, this->glist);

        if(this->discrete)// if we became true
            dial_snap_angle(this);

        dial_update(this, this->glist);
    }
}
static void dial_ticks(t_dial *this, t_floatarg n_ticks)
{
    n_ticks = (int)n_ticks;
    if(this->n_ticks != n_ticks)
    {
        this->n_ticks = clamp(n_ticks, MIN_TICKS, MAX_TICKS);
        if(this->discrete)
        { // no need to draw if we're not discrete
            dial_erase(this, this->glist);
            dial_draw(this, this->glist);
            //update the thumb to match the new ticks
            dial_snap_angle(this);
            dial_update(this, this->glist);
        }
    }
}
static void dial_infinite(t_dial *this, t_floatarg flag)
{
    if(this->infinite != flag)
    {
        this->infinite = flag;
        dial_erase(this, this->glist);
        dial_set_start_position(this, this->start_position);
        dial_draw(this, this->glist);
    }
}
static void dial_drag(t_dial *this, t_symbol *mode)
{
    if( mode != gensym("") )
    {   // 0 is true for strcmp
        if( mode == gensym("H_DRAG") )
            this->drag_mode = H_DRAG;
        else if( mode == gensym("V_DRAG") )
            this->drag_mode = V_DRAG;
        else if( mode == gensym("HV_DRAG") )
            this->drag_mode = HV_DRAG;
        else if(mode == gensym("R_DRAG") )
            this->drag_mode = R_DRAG;
    }
}
static void dial_range(t_dial *this, t_floatarg min, t_floatarg max)
{
    this->min = min;
    this->max = max;
}
static void dial_send(t_dial *this, t_symbol *s)
{
    t_symbol *send = (s == gensym("__EMPTY__")) ? &s_ : canvas_realizedollar(this->glist, s);
    if(send != canvas_realizedollar(this->glist, this->send) ) {
        this->send = send;
        this->send_unexpanded = s;
        this->send_state = (send != &s_);

        if(this->send_state)
            sys_vgui(".x%lx.c delete %lx_out\n", glist_getcanvas(this->glist), this);
        else
            dial_draw_iolet(this);
    }
}
static void dial_receive(t_dial *this, t_symbol *s)
{
    t_symbol *rcv = (s == gensym("__EMPTY__")) ? &s_ : canvas_realizedollar(this->glist, s);
    if(rcv != this->rcv)
    {
        if(this->rcv != &s_)
            pd_unbind(&this->obj.ob_pd, this->rcv);

        this->rcv = rcv;
        this->rcv_unexpanded = s;
        this->rcv_state = (rcv != &s_);

        if(this->rcv_state) {
            pd_bind(&this->obj.ob_pd, this->rcv);
            sys_vgui(".x%lx.c delete %lx_in\n", glist_getcanvas(this->glist), this);
        } else
            dial_draw_iolet(this);
    }
}
static void dial_size(t_dial *x, t_floatarg f)
{
    int s = (int)clamp(f, MIN_SIZE, MAX_SIZE);
    if (s != x->size)
    {
        x->size = s;
        dial_erase(x, x->glist);
        if (glist_isvisible(x->glist) && gobj_shouldvis((t_gobj *)x, x->glist))
        {
            dial_draw(x, x->glist);
            dial_update(x, x->glist);
            canvas_fixlinesfor(x->glist, (t_text *)x);
        }
    }
}
static void dial_start_position(t_dial *this, t_symbol *start_position)
{
    if(start_position == gensym("bot"))
        dial_set_start_position(this, BOT);
    else if(start_position == gensym("left"))
        dial_set_start_position(this, LEFT);
    else if(start_position == gensym("top"))
        dial_set_start_position(this, TOP);
    else if(start_position == gensym("right"))
        dial_set_start_position(this, RIGHT);

    // this resets the values of the dial
    // BUT I think it's okay b/c you should be deciding the style of the dial and then using it.
    // so it shouldn't matter to much.
    dial_update(this, this->glist);
}
static void dial_label(t_dial *this, t_symbol *label) {
    if(this->label != label)
        this->label = label;
    dial_erase(this, this->glist);
    dial_draw(this, this->glist);
}
static void dial_hotkey(t_dial *this, t_symbol *hotkey) {
    if(this->hotkey != hotkey) {
        if(strlen(hotkey->s_name) > 1) {
            char c = hotkey->s_name[0];
            hotkey = gensym(&c);
        }
        this->hotkey = hotkey;
    }
}
// END MESSAGE HANDLING FUNCTIONS ================================================================================
// START DIAL FUNCTIONS ==========================================================================================
// removed crashing, for unknown reasons
// the commented code in edit_proxy_free was the reason.
static void dial_free(t_dial *x)
{
    tooltip_free(x->ttip);
    key_listener_free(x->listener);
    edit_proxy_free(x->e_proxy);
    pd_unbind(&x->obj.ob_pd, x->bindname);
    gfxstub_deleteforkey(x);
}
static void *dial_new(t_symbol *s, int argc, t_atom *argv)
{
    s = NULL;
    t_dial *x = (t_dial *)pd_new(dial_class);
    t_canvas *cnv = canvas_getcurrent();
    x->glist = (t_glist *)cnv;

    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, ".x%lx", (unsigned long)cnv);
    buf[MAXPDSTRING-1] = 0;
    x->e_proxy = edit_proxy_new(x, gensym(buf));

    sprintf(buf, "#%lx", (long)x);
    x->bindname = gensym(buf);
    pd_bind(&x->obj.ob_pd, x->bindname); // for mouserelease messages

    x->listener = key_listener_new(x);

    // params not set from a_gimme
    x->receive_input = false;
    x->dialog_open = false;
    x->fine_tune = false;
    x->macro_tune = false;
    x->editmode = cnv->gl_edit;
    x->zoom_factor = x->glist->gl_zoom;
    x->xpos = x->ypos = 0;

    // the order of loading args is the order that we save them

    // read in the args
    t_symbol *hotkey = argc > 0 ? atom_getsymbolarg(0, argc, argv) : &s_;
    x->hotkey = hotkey != gensym("__EMPTY__") ? hotkey : &s_;

    t_symbol *send = argc > 1 ? symbol_unescape(atom_getsymbolarg(1, argc, argv)) : &s_;
    t_symbol *rcv = argc > 2 ? symbol_unescape(atom_getsymbolarg(2, argc, argv)) : &s_;

    dial_init_send_rcv(x, send, rcv);

    x->label = argc > 3 ? symbol_unescape(atom_getsymbolarg(3, argc, argv)) : &s_;

    x->size = argc > 4 ? (int)atom_getfloatarg(4, argc, argv) : 60;

    x->start_angle = argc > 5 ? (int)clamp(atom_getfloatarg(5, argc, argv), MIN_START_ANGLE, MAX_START_ANGLE) : 220;
    x->end_angle = argc > 6 ? (int)clamp(atom_getfloatarg(6, argc, argv), MIN_END_ANGLE, MAX_END_ANGLE) : -40;

    x->n_ticks = argc > 7 ? (int)clamp(atom_getfloatarg(7, argc, argv), MIN_TICKS, MAX_TICKS) : 8;

    // base default start position on whether or not it's infinite.
    // move the setting of infinite later
    x->start_position = argc > 8 ? (int)atom_getfloatarg(8, argc, argv) : TOP;
    dial_set_start_position(x, x->start_position);

    x->infinite = argc > 9 ? (int)atom_getfloatarg(9, argc, argv) : false;

    x->discrete = argc > 10 ? (int)atom_getfloatarg(10, argc, argv) : false;
    if(x->discrete)
        x->angle = dial_snap_angle(x);

    x->move_on_click = argc > 11 ? (int)atom_getfloatarg(11, argc, argv) : false;

    x->drag_mode = argc > 12 ? (int)atom_getfloatarg(12, argc, argv) : HV_DRAG;

    x->min = argc > 13 ? atom_getfloatarg(13, argc, argv) : 0.f;
    x->max = argc > 14 ? atom_getfloatarg(14, argc, argv) : 100.f;

    x->show_ttip = argc > 15 ? (int)atom_getfloatarg(15, argc, argv) : true;
    int ttip_pos = argc > 16 ? (int)atom_getfloatarg(16, argc, argv) : RIGHT;
    x->ttip = tooltip_new(x, ttip_pos);

    x->fine_scale_factor = argc > 17 ? atom_getfloatarg(17, argc, argv) : 0.01f;
    x->macro_scale_factor = argc > 18 ? atom_getfloatarg(18, argc, argv) : 2.5f;
    x->arrow_scale_factor = argc > 19 ? atom_getfloatarg(19, argc, argv) : 3.f;

    x->label_position = argc > 20 ? (int)atom_getfloatarg(20, argc, argv) : TOP;

    outlet_new(&x->obj, &s_float);
    return(x);
}
// start position is a value from 1 -> 4
static void dial_set_start_position(t_dial *this, int start_position)
{
    this->start_position = start_position;
    if(!this->infinite)
    {
        switch(this->start_position)
        {
            case LEFT: {
                this->angle = this->start_angle;
                break;
            }
            case TOP: {
                this->angle = 90.f;
                break;
            }
            case RIGHT: {
                this->angle = this->end_angle;
                break;
            }
            default: { // defaults to left
                this->angle = this->start_angle;
                break;
            }
        }
    }
    else
    {
        switch(this->start_position)
        {
            case RIGHT: {
                this->angle = 0.f;
                this->inf_start_angle = 0;
                break;
            }
            case BOT: {
                this->angle = 270.f;
                this->inf_start_angle = 270;
                break;
            }
            case LEFT: {
                this->angle = 180.f;
                this->inf_start_angle = 180;
                break;
            }
            case TOP: {
                this->angle = 90.f;
                this->inf_start_angle = 90;
                break;
            }
            default: { // defaults to top
                this->angle = 90.f;
                this->inf_start_angle = 90;
                break;
            }
        }
    }
}
static void dial_init_send_rcv(t_dial *this, t_symbol *s, t_symbol *r)
{
    // when we save if there was no symbol we save it as __EMPTY__
    t_symbol *send = (s == gensym("__EMPTY__")) ? &s_ : canvas_realizedollar(this->glist, s);
    t_symbol *rcv  = (r == gensym("__EMPTY__")) ? &s_ : canvas_realizedollar(this->glist, r);

    this->send = send;
    this->send_unexpanded = s;
    this->send_state = (send != &s_);

    this->rcv = rcv;
    this->rcv_unexpanded = r;
    this->rcv_state = (rcv != &s_);

    if(this->rcv_state)
        pd_bind(&this->obj.ob_pd, this->rcv);
}
static void dial_save(t_gobj *z, t_binbuf *b)
{
    t_dial *x = (t_dial *)z;

    binbuf_addv(b, "ssiisssssiiiiiiiiiffiifffi", gensym("#X"), gensym("obj"),
                (int)x->obj.te_xpix, (int)x->obj.te_ypix,
                atom_getsymbol(binbuf_getvec(x->obj.te_binbuf)),
                x->hotkey == &s_ ? gensym("__EMPTY__") : x->hotkey, //0
                symbol_escape(x->send_unexpanded),                  //1
                symbol_escape(x->rcv_unexpanded),                   //2
                symbol_escape(x->label),                            //3
                x->size,               //4
                x->start_angle,        //5
                x->end_angle,          //6
                x->n_ticks,            //7
                x->start_position,     //8
                x->infinite,           //9
                x->discrete,           //10
                x->move_on_click,      //11
                x->drag_mode,          //12
                x->min,                //13
                x->max,                //14
                x->show_ttip,          //15
                x->ttip->position,     //16
                x->fine_scale_factor,  //17
                x->macro_scale_factor, //18
                x->arrow_scale_factor, //19
                x->label_position);    //20
    binbuf_addv(b, ";");
}
// create the dialog and send dial properties
static void dial_properties(t_gobj *z, t_glist *owner)
{
    owner = NULL;

    t_dial *this = (t_dial*)z;
    char buffer[MAXPDSTRING];

    this->dialog_open = true;

    sprintf(buffer, "::dial_dialog::dial_dialog %%s %d %d %d %d {%s} %d %d %d %d %d %d %d %g %g "
                    "{%s} {%s} %g %g %g {%s} %d\n",
            this->infinite,
            this->discrete,
            this->move_on_click,
            this->drag_mode,
            this->hotkey->s_name, // no need to escape it on the way in
            this->show_ttip, this->ttip->position,
            this->size,
            this->start_angle, this->end_angle,
            this->start_position,
            this->n_ticks,
            this->min, this->max,
            symbol_escape(this->send_unexpanded)->s_name,
            symbol_escape(this->rcv_unexpanded)->s_name,
            this->fine_scale_factor, this->macro_scale_factor, this->arrow_scale_factor,
            symbol_escape(this->label)->s_name, this->label_position);

    gfxstub_new(&this->obj.ob_pd, this, buffer);
}
// receive dial params from the dialog
static void dial_set_param(t_dial *this, t_symbol *s, int argc, t_atom *argv)
{
    s = NULL;

    this->dialog_open = false;
    this->infinite              = (int)atom_getfloatarg(0, argc, argv);
    this->discrete              = (int)atom_getfloatarg(1, argc, argv);
    this->move_on_click         = (int)atom_getfloatarg(2, argc, argv);
    if(this->infinite) // dialog won't let this happen, but just in case.
        this->move_on_click = false;
    this->drag_mode             = (int)atom_getfloatarg(3, argc, argv);
    this->hotkey                = symbol_unescape( atom_getsymbolarg(4, argc, argv) ); // we do escape on the way out though
    this->show_ttip             = (int)atom_getfloatarg(5, argc, argv);
    this->ttip->position        = (int)atom_getfloatarg(6, argc, argv);
    this->size                  = (int)clamp( atom_getfloatarg(7, argc, argv), MIN_SIZE, MAX_SIZE);
    this->start_angle           = (int)clamp( atom_getfloatarg(8, argc, argv), MIN_START_ANGLE, MAX_START_ANGLE);
    this->end_angle             = (int)clamp( atom_getfloatarg(9, argc, argv), MIN_END_ANGLE, MAX_END_ANGLE);
    this->start_position        = (int)atom_getfloatarg(10, argc, argv);
    this->n_ticks               = (int)clamp( atom_getfloatarg(11, argc, argv), MIN_TICKS, MAX_TICKS);
    this->min                   = atom_getfloatarg(12, argc, argv);
    this->max                   = atom_getfloatarg(13, argc, argv);
    t_symbol *send              = symbol_unescape( atom_getsymbolarg(14, argc, argv) );
    t_symbol *rcv               = symbol_unescape( atom_getsymbolarg(15, argc, argv) );
    this->fine_scale_factor     = atom_getfloatarg(16, argc, argv);
    this->macro_scale_factor    = atom_getfloatarg(17, argc, argv);
    this->arrow_scale_factor    = atom_getfloatarg(18, argc, argv);
    this->label                 = symbol_unescape( atom_getsymbolarg(19, argc, argv) );
    this->label_position        = (int)atom_getfloatarg(20, argc, argv);

    dial_send(this, send);
    dial_receive(this, rcv);

    // seems naive... more thought later
    // issues with start_position, needs more thought / refactoring
    dial_erase(this, this->glist);
    dial_draw(this, this->glist);
    dial_update(this, this->glist);
}
void dial_setup(void)
{
    dial_class = class_new(gensym("dial"), (t_newmethod)dial_new,
                           (t_method)dial_free, sizeof(t_dial), CLASS_DEFAULT, A_GIMME, 0);

    // methods to call with corresponding messages
    class_addmethod(dial_class, (t_method)dial_hotkey, gensym("hotkey"), A_DEFSYMBOL, 0);
    class_addmethod(dial_class, (t_method)dial_label, gensym("label"), A_DEFSYMBOL, 0);
    class_addmethod(dial_class, (t_method)dial_set, gensym("set"), A_DEFFLOAT, 0);
    class_addmethod(dial_class, (t_method)dial_ticks, gensym("ticks"), A_DEFFLOAT, 0);
    class_addmethod(dial_class, (t_method)dial_start_position, gensym("start"), A_DEFSYMBOL, 0);
    class_addmethod(dial_class, (t_method)dial_discrete, gensym("discrete"), A_DEFFLOAT, 0);
    class_addmethod(dial_class, (t_method)dial_infinite, gensym("infinite"), A_DEFFLOAT, 0);
    class_addmethod(dial_class, (t_method)dial_drag, gensym("drag"), A_DEFSYMBOL, 0);
    class_addmethod(dial_class, (t_method)dial_range, gensym("range"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(dial_class, (t_method)dial_size, gensym("size"), A_FLOAT, 0);
    class_addmethod(dial_class, (t_method)dial_send, gensym("send"), A_DEFSYMBOL, 0);
    class_addmethod(dial_class, (t_method)dial_receive, gensym("receive"), A_DEFSYMBOL, 0);
    class_addmethod(dial_class, (t_method)dial_zoom, gensym("zoom"), A_CANT, 0);
    // these methods are called from tcl. I'm using underscore to decrease the change some tries to use them from
    // pd, but afaik, there's no way to make these methods "private"
    class_addmethod(dial_class, (t_method)dial_mouserelease, gensym("_mouserelease"), 0);
    class_addmethod(dial_class, (t_method)dial_set_param, gensym("_param"), A_GIMME, 0);
    class_addfloat(dial_class, dial_float);
    class_addbang(dial_class, dial_bang);

    edit_proxy_class = class_new(0, 0, 0, sizeof(t_edit_proxy), CLASS_NOINLET | CLASS_PD, 0);
    class_addanything(edit_proxy_class, edit_proxy_any);

    key_listener_class = class_new(0, 0, 0, sizeof(t_key_listener), CLASS_NOINLET, 0);
    class_addlist(key_listener_class, key_listener_list);

    tooltip_class = class_new(0, 0, 0, sizeof(t_tooltip), CLASS_NOINLET, 0);

    // set function pointers for widget
    dial_widgetbehavior.w_getrectfn  = dial_getrect; // getrect is needed for showing connections properly
    dial_widgetbehavior.w_displacefn = dial_displace;
    dial_widgetbehavior.w_selectfn   = dial_select;
    dial_widgetbehavior.w_activatefn = NULL;
    dial_widgetbehavior.w_deletefn   = dial_delete;
    dial_widgetbehavior.w_visfn      = dial_vis;
    dial_widgetbehavior.w_clickfn    = dial_click;
    // dial_widgetbehavior.

    class_sethelpsymbol(dial_class, gensym("dial"));
    class_setsavefn(dial_class, dial_save);
    class_setwidget(dial_class, &dial_widgetbehavior);
    class_setpropertiesfn(dial_class, dial_properties);
}
// END DIAL FUNCTIONS ===========================================================================================
