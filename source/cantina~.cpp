//
// Created by piptouque on 23/04/2020.
//

#include "../include/cantina~.hpp"

#include <algorithm>
#include <memory>
#include <cmath>

/******** utility *******/

/******** pd stuff ********/

static t_class *cantina_tilde_class;

typedef struct
{
    t_object x_obj;
    t_sample f;
    /* the first inlet is implicitly stored by pd */
    t_inlet *x_in_notes;
    t_inlet *x_in_controls;
    /* outlets */
    /** signals **/
    t_outlet *x_out_seed;
    t_outlet **x_out_harmonics;
    /** midi-related stuff **/
    t_outlet *x_out_notes;
    t_outlet *x_out_controls;
    /* values */
    /* intern */
    std::unique_ptr<cant::Cantina> cant;
    std::vector<t_sample*> s_vec_harmonics;
} t_cantina_tilde;

void* cantina_tilde_new(const t_symbol *s, const int argc, t_atom *argv)
{
    auto *x = (t_cantina_tilde*) pd_new(cantina_tilde_class);
    /* number of harmonics */
    t_float n_arg = 0;
    if(argc)
    {
            n_arg = atom_getfloat(argv);
    }
    const std::size_t numberHarmonics = std::max<std::size_t>(0, std::floor(n_arg));
    try
    {
        x->cant = std::make_unique<cant::Cantina>(
                numberHarmonics,
                sys_getsr() /* sample rate */
        );
    }
    catch (const cant::CantinaException& e)
    {
        std::cerr << e.what() << std::endl;
    }
    /* inlet */
    /* first one managed automatically */
    x->x_in_notes = inlet_new(
            &x->x_obj,
            &x->x_obj.ob_pd,
            &s_list,
            gensym("notes")
            );
    x->x_in_controls = inlet_new(
            &x->x_obj,
            &x->x_obj.ob_pd,
            &s_list,
            gensym("controls")
            );
    /* outlets */
    /** signals **/
    x->x_out_seed = outlet_new(&x->x_obj, &s_signal);
    /** midi (here because not dynamic) **/
    x->x_out_notes = outlet_new(&x->x_obj, &s_list);
    x->x_out_controls = outlet_new(&x->x_obj, &s_list);
    /** signals again **/
    x->x_out_harmonics = (t_outlet**)getbytes(x->cant->getNumberHarmonics() * sizeof(t_outlet*));
    if(!x->x_out_harmonics)
    {
        post("cantina~: failed to allocate outlets.");
    }
    for(std::size_t i=0; i < x->cant->getNumberHarmonics(); ++i)
    {
        x->x_out_harmonics[i] = outlet_new(&x->x_obj, &s_signal);
    }
    return (void*)x;
}

void cantina_tilde_free(t_cantina_tilde *x)
{

    /* inlets */
    inlet_free(x->x_in_notes);
    inlet_free(x->x_in_controls);
    /* outlets */
    /** signals **/
    outlet_free(x->x_out_seed);
    for(std::size_t i=0; i < x->cant->getNumberHarmonics(); ++i)
    {
        outlet_free(x->x_out_harmonics[i]);
    }
    freebytes(x->x_out_harmonics, x->cant->getNumberHarmonics() * sizeof(t_outlet*));
    /** midi **/
    outlet_free(x->x_out_notes);
    outlet_free(x->x_out_controls);
    x->cant.reset();
}

int get_vec_size(const t_cantina_tilde *x)
{
    return 4 + (int) x->cant->getNumberHarmonics();
}

t_int* cantina_tilde_perform(t_int *w)
{
    auto *x = (t_cantina_tilde*)(w[1]);
    size_t blockSize = (size_t)(w[2]);
    auto *in = (t_sample*)(w[3]);
    auto *out_seed = (t_sample*)(w[4]);
    auto **out_harmonics = (t_sample**)(&w[5]);
    /* for now, no computation on the first outlet
     * -> seed bypass
     */
    /** DOING STUFF **/
    for(int i=0; i < (int) x->cant->getNumberHarmonics(); ++i)
    {
        /* resetting samples in outlet before filling it again */
        std::fill(out_harmonics[i], out_harmonics[i] + blockSize, 0.);
    }
    /** CANT **/
    try
    {
        x->cant->perform(in, out_seed, out_harmonics, blockSize);
    }
    catch (const cant::CantinaException& e)
    {
        std::cerr << e.what() << std::endl;
    }
    t_int size = get_vec_size(x);
    return (w + size + 1);
}


t_int* create_vec_dsp(int *size, t_cantina_tilde *x, t_signal **sp)
{
    *size = get_vec_size(x);
    auto *vec = (t_int *)malloc((*size) * sizeof(t_int));
    vec[0] = (t_int)x;
    vec[1] = (t_int)sp[0]->s_n;
    vec[2] = (t_int)sp[0]->s_vec;
    vec[3] = (t_int)sp[1]->s_vec;
    for(std::size_t i=0; i< x->cant->getNumberHarmonics(); ++i)
    {
        vec[4 + i] = (t_int)sp[2 + i]->s_vec;
    }
    return vec;
}

void free_vec_dsp(t_int* vec) { free(vec); }

void cantina_tilde_dsp(t_cantina_tilde *x, t_signal **sp)
{
    int size;
    t_int* vec = create_vec_dsp(&size, x, sp);
    dsp_addv(cantina_tilde_perform,
            size,
            vec
            );
    free_vec_dsp(vec);
}

void cantina_tilde_envelope(t_symbol *s, int argc, t_atom *argv)
{
    /* todo */
    char type[20];
    switch (argc)
    {
        case 0:
            post("MidiEnvelope method not set.");
            return;
        case 1:
            atom_string(argv + 1, type, 20);
            post(type);
        default:
            break;
    }
}

void cantina_tilde_notes(t_cantina_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if(argc < 2)
    {
        post("Wrong format for note input: expected tone and velocity.");
        return;
    }
    /* voices of the poly object start at 1 */
    const auto tone = (cant::pan::tone_mint)atom_getfloat(argv);
    const auto velocity = (cant::pan::vel_mint)atom_getfloat(argv + 1);
    const cant::pan::byte_m channel = 1;
    const auto data = cant::pan::MidiNoteInputData(channel, tone, velocity);
    try
    {
        x->cant->receiveNote(data);
    }
    catch (const cant::CantinaException& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

void cantina_tilde_controls(t_cantina_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if(argc < 2)
    {
        post("Mais non c'est pas ça !!");
        return;
    }
    const cant::pan::byte_m channel = 1;
    const auto controllerId = (cant::pan::byte_m) atom_getfloat(argv);
    const auto value = (cant::pan::byte_m) atom_getfloat(argv + 1);
    const auto data = cant::pan::MidiControlInputData(channel, controllerId, value);
    try
    {
        x->cant->receiveControl(data);
    }
    catch (const cant::CantinaException& e)
    {
       std::cerr << e.what() << std::endl;
    }
}

void cantina_tilde_controllers(t_cantina_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if(argc < 3)
    {
        post("Wrong format for controller definition: expected type, channel, and controller id.");
        return;
    }
    char buf[20];
    atom_string(argv, buf, 20);
    const auto type = std::string(buf);
    const auto channelId  = (cant::pan::byte_m) atom_getfloat(argv + 1);
    const auto controllerId = (cant::pan::byte_m) atom_getfloat(argv + 2);
    try
    {
        x->cant->setController(type, channelId, { controllerId });
    }
    catch (const cant::CantinaException& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

extern "C" void cantina_tilde_setup(void)
{
    cantina_tilde_class = class_new(gensym("cantina~"),
            (t_newmethod)cantina_tilde_new,
            (t_method)cantina_tilde_free,
            sizeof(t_cantina_tilde),
            CLASS_DEFAULT,
            A_GIMME, 0);

    class_addmethod(cantina_tilde_class,
                    (t_method)cantina_tilde_dsp,
                    gensym("dsp"),
                    A_CANT, // uhuh
                    0);
    class_addmethod(cantina_tilde_class,
                    (t_method)cantina_tilde_envelope,
                    gensym("envelope"),
                    A_GIMME,
                    0
            );
    class_addmethod(cantina_tilde_class,
                    (t_method)cantina_tilde_notes,
                    gensym("notes"),
                    A_GIMME,
                    0
    );
    class_addmethod(cantina_tilde_class,
                    (t_method)cantina_tilde_controls,
                    gensym("controls"),
                    A_GIMME,
                    0
            );
    class_addmethod(cantina_tilde_class,
                    (t_method)cantina_tilde_controllers,
                    gensym("controller"),
                    A_GIMME,
                    0);
    CLASS_MAINSIGNALIN(cantina_tilde_class, t_cantina_tilde, f);
    post("Cant version: %.2g", __CANT_VERSION__);
    post("Cant brew:    %s", __CANT_BREW__);
    post("~ tut-tut-tut-tut-tulut-tut ~");
}