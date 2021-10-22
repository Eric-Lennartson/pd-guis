#include "m_pd.h"

static t_class *preset_class;

typedef struct _preset {
    t_object obj;
} t_preset;

static void preset_store(t_float which) {
    post("store called. %g", which);
}
static void preset_float(float which) {
    post("float called. %g", which);
}
static void preset_clear(float which) {
    post("clear called. %g", which);
}
static void preset_clearall() {
    post("clearall called.");
}
static void preset_save() {
    post("save called.");
}
static void preset_free() {
    post("free called.");
}

static void *preset_new(void) {
    t_preset *this = (t_preset *)pd_new(preset_class);

    post("made a new preset obj!");

    return this;
}

void preset_setup(void)
{
    preset_class = class_new(gensym("preset"), (t_newmethod)preset_new,
                             (t_method)preset_free, sizeof(t_preset), CLASS_DEFAULT, 0);

    class_addmethod(preset_class, (t_method)preset_store, gensym("store"), A_DEFFLOAT, 0);
    class_addmethod(preset_class, (t_method)preset_clear, gensym("clear"), A_DEFFLOAT, 0);
    class_addmethod(preset_class, (t_method)preset_clearall, gensym("clearall"), 0);
    class_addfloat(preset_class, preset_float);

    class_sethelpsymbol(preset_class, gensym("preset"));
    class_setsavefn(preset_class, preset_save);
}