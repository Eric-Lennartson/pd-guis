#include "m_pd.h"
#include <string.h>
#include <stdlib.h>

#define NULLSYM &s_ // not sure if this is very accurate, but that's what I'm using it for
#define advance() strtok(NULL, ";")

static t_class *plist_class;

// a param is a node in the preset linked list.
typedef struct _param {
    t_symbol *name;
    t_float value;
    struct _param *next;
} t_param;

// a preset holds a list of parameters
typedef struct _preset {
    t_symbol *name;
    t_param *preset_head;
    struct _preset *next;
} t_preset;

// A plist holds a list of presets
typedef struct _preset_list {
    t_object obj;
    t_preset *plist_head;
    t_symbol *name, *filename, *rcv;
    t_symbol *last_read_filename; // for saving the last file we read before we read in the new one.
    t_canvas *cnv; // necessary to read and write files
    t_outlet *out;
} t_preset_list;

#ifdef DEBUG
    static void preset_print(t_preset *this);
    static void plist_print(t_preset_list *this);
#endif

static inline char *rm_newline(char *str) { return (str[0] == '\n') ? ++str : str; }

static t_param *param_new(t_symbol *name, t_float value)
{
    t_param *p = getbytes(sizeof(t_param));
    p->name = name;
    p->value = value;
    p->next = NULL;
    return p;
}

static t_param *find_param(t_preset *this, t_symbol *name)
{
    t_param *tmp = this->preset_head;

    while(tmp != NULL) {
        if(name == tmp->name) {
            return tmp;
        }
        tmp = tmp->next;
    }
    return NULL;
}

static t_preset *find_preset(t_preset_list *this, t_symbol *name)
{
    t_preset *tmp = this->plist_head;

    while(tmp != NULL) {
        if(name == tmp->name) {
            return tmp;
        }
        tmp = tmp->next;
    }
    return NULL;
}

static void param_free(t_param *p) {
    p->next = NULL;
    freebytes(p, sizeof(t_param));
}

// clear removes all the params from the preset
static void preset_clear(t_preset *p) {
    t_param *tmp = p->preset_head;
    t_param *next = p->preset_head->next;

    while(tmp != NULL) {
        param_free(tmp);
        tmp = next;
        next = next->next;
    }
    p->preset_head = NULL;
}

static void preset_free(t_preset *p) {
    preset_clear(p);
    p->next = NULL;
    freebytes(p, sizeof(t_preset));
}

static void find_and_rm_param(t_preset *this, t_symbol *name)
{
    t_param *tmp = this->preset_head;
    t_param *prev = this->preset_head;

    while(tmp != NULL) {
        if(name == tmp->name) {
            if(prev->next == tmp->next) { // if we're removing the head
                this->preset_head = tmp->next; // if it's only one long, we're pointing to NULL anyways
                param_free(tmp);
                break;
            } else {
                prev->next = tmp->next;
                param_free(tmp);
                break;
            }
        }
        prev = tmp;
        tmp = tmp->next;
    }
    #ifdef DEBUG
        preset_print(this);
    #endif
}

static void find_and_rm_preset(t_preset_list *this, t_symbol *name)
{
    t_preset *tmp = this->plist_head;
    t_preset *prev = this->plist_head;

    while(tmp != NULL)
    {
        if(name == tmp->name)
        {
            if(prev->next == tmp->next) { // if we're removing the head
                this->plist_head = tmp->next; // if it's only one long, we're pointing to NULL anyways
                preset_free(tmp);
                break;
            } else {
                prev->next = tmp->next;
                preset_free(tmp);
                break;
            }
        }
        prev = tmp;
        tmp = tmp->next;
    }
    #ifdef DEBUG
        plist_print(this);
    #endif
}

// adds a param after the head, order doesn't matter and this is easiest
static void add_param(t_preset *this, t_param *new_param)
{
    if(this->preset_head != NULL) {
        new_param->next = this->preset_head->next;
        this->preset_head->next = new_param;
    } else {
        this->preset_head = new_param;
    }
}

// not very DRY, but doesn't matter to much?
// if I come back to this and decide to expand the functionality, I'll refactor this
static void add_preset(t_preset_list *this, t_preset *new_node)
{
    if(new_node != NULL)
    {
        if(this->plist_head != NULL) {
            new_node->next = this->plist_head->next;
            this->plist_head->next = new_node;
        } else {
            this->plist_head = new_node;
        }
    } else {
        pd_error(this, "[plist] new_node is equal to NULL");
    }
}

static void preset_print(t_preset *this)
{
    t_param *tmp = this->preset_head;

    while(tmp != NULL) {
        post("%s: %.3f -> ", tmp->name->s_name, tmp->value);
        tmp = tmp->next;
    }
}

static void plist_print(t_preset_list *this)
{
    t_preset *tmp = this->plist_head;

    while(tmp != NULL) {
        post("%s: -> ", tmp->name->s_name);
        tmp = tmp->next;
    }
}

static void plist_update(t_preset_list *this, t_symbol *preset_name, t_symbol *param_name, t_floatarg value)
{
    t_preset *preset = find_preset(this, preset_name);
    if(preset != NULL) {
        t_param *param = find_param(preset, param_name);
        if(param != NULL) {
            param->value = value;
        } else {
            pd_error(this, "param \"%s\" in preset \"%s\" doesn't exist.", param_name->s_name, preset->name->s_name);
        }
    } else {
        pd_error(this, "preset \"%s\" doesn\'t exist", preset_name->s_name);
    }
}

static t_preset *preset_new(t_symbol *name)
{
    t_preset *p = getbytes(sizeof(t_preset));
    p->name = name;
    p->preset_head = NULL;
    p->next = NULL;
    return p;
}

static void plist_add(t_preset_list *this, t_symbol *name)
{
    // don't add presets if they already exist
    t_preset *found = find_preset(this, name);
    if(!found) {
        add_preset(this, preset_new(name));
        #ifdef DEBUG
            plist_print(this);
        #endif
    }
}

static void preset_add(t_preset_list *this, t_symbol *preset_name, t_symbol *param_name, t_floatarg value)
{
    t_preset *preset = find_preset(this, preset_name);
    if(preset) { // does the preset exist?
        t_param *found = find_param(preset, param_name);
        if(!found) // does the param NOT exist?
            add_param(preset, param_new(param_name, value) );
        #ifdef DEBUG
            preset_print(this);
        #endif
    } else { // preset doesn't exist so we'll make one
        plist_add(this, preset_name);
        preset = find_preset(this, preset_name); // if we change the plist_add code to return a pointer to the just added preset, we don't have to find it again
        t_param *found = find_param(preset, param_name);
        if(!found) // does the param NOT exist?
            add_param(preset, param_new(param_name, value) );
        #ifdef DEBUG
            preset_print(this);
        #endif
    }
}

// output the params i.e. read and output each elem of the linked list
static void plist_out(t_preset_list *this, t_preset *p)
{
    if(p)
    {
        t_param *tmp = p->preset_head;
        t_atom param[2];

        while(tmp != NULL)
        {
            SETSYMBOL(param, tmp->name);
            SETFLOAT(param+1, tmp->value);
            // output to the outlet and to any rcvs bound to our name.
            if(this->name->s_thing != NULL) // check that we can be received
                pd_list(this->name->s_thing, &s_list, 2, param);
            outlet_list(this->obj.ob_outlet, &s_list, 2, param);
            tmp = tmp->next;
        }
    } else {
        pd_error(this, "preset doesn\'t exist");
    }
}

static void plist_symbol(t_preset_list *this, t_symbol *name)
{
    t_preset *p = find_preset(this, name);
    if(p)
        plist_out(this, p);
    else
        pd_error(this, "preset \"%s\" doesn\'t exist", name->s_name);
}


static void plist_float(t_preset_list *this, t_floatarg which)
{
    t_preset *p = this->plist_head;

    int i = 0;
    while(p != NULL && i < (int)which) {
        p = p->next;
        i++;
    }
    if(p)
        plist_out(this, p);
    else
        pd_error(this, "plist \"%s\" only has %d preset(s)", this->name->s_name, (int)which);
}

// remove a single param from a preset
static void preset_remove(t_preset_list *this, t_symbol *preset_name, t_symbol* param_name) {
    post("removing param \"%s\" from preset \"%s\"", param_name->s_name, preset_name->s_name);
    t_preset *p = find_preset(this, preset_name);
    if(p)
        find_and_rm_param(p, param_name);
}

// remove a single preset from a plist
static void plist_remove(t_preset_list *this, t_symbol *name) {
    post("removing preset \"%s\" from plist", name->s_name);
    find_and_rm_preset(this, name);
}

// clear a single preset, but leave the node in the plist
static void plist_clear(t_preset_list *this, t_symbol *name)
{
    post("clearing preset \"%s\"", name->s_name);
    t_preset *p = find_preset(this, name);
    if(p != NULL)
        preset_clear(p);
}

// clear params from presets, but leave the presets intact
static void plist_clear_all(t_preset_list *this)
{
    t_preset *tmp = this->plist_head;
    while(tmp != NULL) {
        preset_clear(tmp);
        tmp = tmp->next;
    }
}

static void plist_rm_all(t_preset_list *this)
{
    t_preset *current = this->plist_head;
    t_preset *next = NULL;
    while(current != NULL) {
        next = current->next;
        preset_free(current);
        current = next;
    }
    this->plist_head = NULL; // make sure the head points to NULL
}

static void plist_write(t_preset_list *this, t_symbol *file)
{
    if(file != NULL && file != gensym("") && file != NULLSYM)
    {
        char filename[MAXPDSTRING];

            t_preset *preset = this->plist_head;
            t_binbuf *bb = binbuf_new();
            binbuf_clear(bb);

            if(preset == NULL) {
                binbuf_addv(bb, "s", gensym("EMPTY_PRESET"));
                binbuf_addsemi(bb);
            }
            else
            {
                binbuf_addv(bb, "s", gensym("PLIST_START"));
                binbuf_addsemi(bb);
                while(preset != NULL)
                {
                    t_param *param = preset->preset_head;
                    binbuf_addv(bb, "s", gensym("PRESET_START"));
                    binbuf_addsemi(bb);
                    binbuf_addv(bb, "s", gensym("PRESET_NAME"));
                    binbuf_addsemi(bb);
                    binbuf_addv(bb, "s", preset->name);
                    binbuf_addsemi(bb);
                    while(param != NULL)
                    {
                        binbuf_addv(bb, "s", gensym("PARAM_NAME"));
                        binbuf_addsemi(bb);
                        binbuf_addv(bb, "s", param->name);
                        binbuf_addsemi(bb);
                        binbuf_addv(bb, "s", gensym("PARAM_VALUE"));
                        binbuf_addsemi(bb);
                        binbuf_addv(bb, "f", param->value);
                        binbuf_addsemi(bb);
                        param = param->next;
                    }
                    binbuf_addv(bb, "s", gensym("PRESET_END"));
                    binbuf_addsemi(bb);
                    preset = preset->next;
                }
                binbuf_addv(bb, "s", gensym("PLIST_END"));
                binbuf_addsemi(bb);
            }

            //this line is essential. why? I don't know, but it doesn't work without it
            canvas_makefilename(this->cnv, file->s_name, filename, MAXPDSTRING);
            if( binbuf_write(bb, filename, "", 0) )
                pd_error(this, "%s: write failed", filename);

            char *presets;
            int txt_length;
            binbuf_gettext(bb, &presets, &txt_length);
            binbuf_clear(bb);
            binbuf_free(bb);
    }
    else
        pd_error(this, "[write( Bad file name.");
}

static void plist_save(t_preset_list *this)
{
    if(this->last_read_filename != NULL) // have we read in a file before?
        plist_write(this, this->last_read_filename);
    else if(this->filename != NULL) // if not, we save to the default or creation arg filename instead.
        plist_write(this, this->filename);
    else
        pd_error(this, "Can't save plist. Filename don't exist");
}

static void plist_read(t_preset_list *this, t_symbol *file)
{
    post("[plist] reading in file: \"%s\"", file->s_name);
    if(file != NULL && file != gensym("") && file != NULLSYM)
    {
        t_binbuf *bb = binbuf_new();
        binbuf_clear(bb);
        // no calls to canvas_getcurrent, we won't be able to find the files.
        this->last_read_filename = file; //doesn't fail bc symbols never go away?

        plist_rm_all(this); // clear the plist before we add things

        if( binbuf_read_via_canvas(bb, file->s_name, this->cnv, 0) ) {
            pd_error(this, "%s: read failed", file->s_name);
        }
        else
        {   // if the file exists lets read it in
            char *presets;
            int txt_length;
            binbuf_gettext(bb, &presets, &txt_length);

            char *token;
            token = strtok(presets, ";");

            // if it isn't empty
            if(strcmp(token, "EMPTY_PRESET") != 0)
            {
                char *preset_name = NULL, *param_name = NULL;
                t_float value = 0.f;
                while(token != NULL)
                {
                    if(strcmp(token, "\n") == 0) {
                        token = advance();
                    }
                    else
                    {
                        token = rm_newline(token);
                        if(strcmp(token, "PLIST_START") == 0 || strcmp(token, "PRESET_START") == 0) {
                        }
                        else if(strcmp(token, "PRESET_NAME") == 0) {
                            token = advance();
                            token = rm_newline(token);
                            preset_name = token;
                            plist_add(this, gensym(preset_name));
                        }
                        else if(strcmp(token, "PARAM_NAME") == 0) {
                            token = advance();
                            token = rm_newline(token);
                            param_name = token; // we don't have the value yet.
                        }
                        else if(strcmp(token, "PARAM_VALUE") == 0) {
                            token = advance();
                            token = rm_newline(token);
                            value = strtod(token, NULL);
                            preset_add(this, gensym(preset_name), gensym(param_name), value);
                        }
                        else if(strcmp(token, "PLIST_END") == 0) {
                            break;
                        }
                        token = advance();
                    }
                }
                preset_name = param_name = NULL;
                free(preset_name);
                free(param_name);
            }
        }
        binbuf_clear(bb);
        binbuf_free(bb);
    }
    else
        pd_error(this, "[read( Bad file name.");
}

/*
 * We need this wrapper function so that when we call plist_read
 * when the object is first created, we don't overwrite the loaded file.
 * All other times read is called from a msg. Thus, this functions existence.
*/
static void plist_read_msg(t_preset_list *this, t_symbol *file) {
    plist_save(this);
    if(file != NULL)
        plist_read(this, file);
}

static void plist_pd_save(t_gobj *z, t_binbuf *b)
{
    t_preset_list *this = (t_preset_list *)z;

    // saving the basic stuff
    binbuf_addv(b, "ssiisss", gensym("#X"), gensym("obj"),
                (int)this->obj.te_xpix, (int)this->obj.te_ypix,
                atom_getsymbol(binbuf_getvec(this->obj.te_binbuf)),
                this->name, this->filename);
    // we don't save filename bc we rebuild it in plist_new
    binbuf_addsemi(b);
    plist_save(this);
}

static void plist_free(t_preset_list *this) {
    plist_rm_all(this);
    outlet_free(this->out);
    pd_unbind(&this->obj.ob_pd, this->rcv);
}

static void *plist_new(t_symbol *name, t_symbol *filename)
{
    t_preset_list *this = (t_preset_list *)pd_new(plist_class);

    this->plist_head = NULL;
    this->name = (name != gensym("") && name != NULLSYM) ? name : gensym("plist");
    this->filename = filename;
    this->last_read_filename = NULL;
    this->out = outlet_new(&this->obj, &s_list);

    // read in the presets if they exit
    this->cnv = canvas_getcurrent(); // only time we get the canvas.
    if( this->filename != gensym("") && this->filename != NULLSYM )
        plist_read(this, this->filename);

    // set up the receive name
    char str[MAXPDSTRING];
    strcpy(str, this->name->s_name);
    strcat(str, "-rcv");
    this->rcv = gensym(str);
    pd_bind(&this->obj.ob_pd, this->rcv);

    return this;
}

void plist_setup(void)
{
    plist_class = class_new(gensym("plist"), (t_newmethod)plist_new,
                           (t_method)plist_free, sizeof(t_preset_list),
                           CLASS_DEFAULT, A_DEFSYMBOL, A_DEFSYMBOL, 0);

    class_addmethod(plist_class, (t_method)plist_add,       gensym("add_preset"), A_DEFSYMBOL, 0);
    class_addmethod(plist_class, (t_method)preset_add,      gensym("add_param"),  A_DEFSYMBOL, A_DEFSYMBOL, A_DEFFLOAT, 0);
    class_addmethod(plist_class, (t_method)plist_update,    gensym("update"),     A_DEFSYMBOL, A_DEFSYMBOL, A_DEFFLOAT, 0);
    class_addmethod(plist_class, (t_method)plist_remove,    gensym("rm_preset"),  A_DEFSYMBOL, 0);
    class_addmethod(plist_class, (t_method)preset_remove,   gensym("rm_param"),   A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(plist_class, (t_method)plist_clear,     gensym("clear"),      A_DEFSYMBOL, 0);
    class_addmethod(plist_class, (t_method)plist_clear_all, gensym("clear_all"),  0);
    class_addmethod(plist_class, (t_method)plist_rm_all,    gensym("rm_all"),     0);
    class_addmethod(plist_class, (t_method)plist_write,     gensym("write"),      A_DEFSYMBOL, 0);
    class_addmethod(plist_class, (t_method)plist_read_msg,  gensym("read"),       A_DEFSYMBOL, 0);
    class_addfloat(plist_class, plist_float);
    class_addsymbol(plist_class, plist_symbol);

    class_sethelpsymbol(plist_class, gensym("plist"));
    class_setsavefn(plist_class, plist_pd_save);
}