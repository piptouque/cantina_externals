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
    std::unique_ptr<cant::Cantina> cantina;
    std::vector<t_sample*> s_vec_harmonics;
    std::vector<t_int> x_vec_dspargs;
} t_cantina_tilde;

void* cantina_tilde_new(const t_symbol *, const int argc, t_atom *argv)
{
    auto *x = (t_cantina_tilde*)pd_new(cantina_tilde_class);
    /* number of harmonics */
    t_float n_arg = 0;
    if(argc)
    {
            n_arg = atom_getfloat(argv);
    }
    const std::size_t numberHarmonics = std::max<std::size_t>(0, std::floor(n_arg));
    try
    {
        x->cantina = std::make_unique<cant::Cantina>(
                numberHarmonics,
                static_cast<std::size_t>(sys_getsr()) /* sample rate */
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
    x->x_out_harmonics = (t_outlet**)getbytes(x->cantina->getNumberHarmonics() * sizeof(t_outlet*));
    if(!x->x_out_harmonics)
    {
        post("cantina~: failed to allocate outlets.");
    }
    for(std::size_t i=0; i < x->cantina->getNumberHarmonics(); ++i)
    {
        x->x_out_harmonics[i] = outlet_new(&x->x_obj, &s_signal);
    }
    /** dsp args vector **/
    x->x_vec_dspargs = std::vector<t_int>(4 + x->cantina->getNumberHarmonics());
    return static_cast<void*>(x);
}

void cantina_tilde_free(t_cantina_tilde *x)
{

    /* inlets */
    inlet_free(x->x_in_notes);
    inlet_free(x->x_in_controls);
    /* outlets */
    /** signals **/
    outlet_free(x->x_out_seed);
    for(std::size_t i=0; i < x->cantina->getNumberHarmonics(); ++i)
    {
        outlet_free(x->x_out_harmonics[i]);
    }
    freebytes(x->x_out_harmonics, x->cantina->getNumberHarmonics() * sizeof(t_outlet*));
    /** midi **/
    outlet_free(x->x_out_notes);
    outlet_free(x->x_out_controls);
    x->cantina.reset();
}

t_int* cantina_tilde_perform(t_int *w)
{
    auto *x              = reinterpret_cast<t_cantina_tilde*>(w[1]);
    auto blockSize       = static_cast<std::size_t>          (w[2]);
    auto *in             = reinterpret_cast<t_sample*>       (w[3]);
    auto *out_seed       = reinterpret_cast<t_sample*>       (w[4]);
    auto **out_harmonics = reinterpret_cast<t_sample**>      (&w[5]);
    /* for now, no computation on the first outlet
     * -> seed bypass
     */
    /** DOING STUFF **/
    for(int i=0; i < static_cast<int>(x->cantina->getNumberHarmonics()); ++i)
    {
        /* resetting samples in outlet before filling it again */
        std::fill(out_harmonics[i], out_harmonics[i] + blockSize, 0.);
    }
    /** CANT **/
    try
    {
        x->cantina->perform(in, out_seed, out_harmonics, blockSize);
    }
    catch (const cant::CantinaException& e)
    {
        std::cerr << e.what() << std::endl;
    }
    const t_int size = x->x_vec_dspargs.size();
    return (w + size + 1);
}


void fill_vec_dspargs(t_cantina_tilde *x, t_signal **sp)
{
    auto& vec = x->x_vec_dspargs;
    vec.at(0) = reinterpret_cast<t_int>(x);                 // x
    vec.at(1) = static_cast<t_int>(sp[0]->s_n);             // blockSize
    vec.at(2) = reinterpret_cast<t_int>(sp[0]->s_vec);      // in
    vec.at(3) = reinterpret_cast<t_int>(sp[1]->s_vec);      // out seed
    for(std::size_t i=0; i< x->cantina->getNumberHarmonics(); ++i)
    {
        vec.at(4 + i) = reinterpret_cast<t_int>(sp[2 + i]->s_vec); // out harmonics
    }
}

void cantina_tilde_dsp(t_cantina_tilde *x, t_signal **sp)
{
    fill_vec_dspargs(x, sp);
    dsp_addv(cantina_tilde_perform,
            static_cast<int>(x->x_vec_dspargs.size()),
            x->x_vec_dspargs.data()
            );
}

void cantina_tilde_envelope(t_symbol *, int argc, t_atom *argv)
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

void cantina_tilde_notes(t_cantina_tilde *x, t_symbol *, int argc, t_atom *argv)
{
    if(argc < 2)
    {
        post("Wrong format for note input: expected tone and velocity.");
        return;
    }
    /* voices of the poly object start at 1 */
    const auto tone     = static_cast<cant::pan::tone_i8>(atom_getfloat(argv));
    const auto velocity = static_cast<cant::pan::vel_i8>(atom_getfloat(argv + 1));
    const cant::pan::id_u8 channel = 1;
    const auto data = cant::pan::MidiNoteInputData(channel, tone, velocity);
    try
    {
        x->cantina->receiveNote(data);
    }
    catch (const cant::CantinaException& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

void cantina_tilde_controls(t_cantina_tilde *x, t_symbol *, int argc, t_atom *argv)
{
    if(argc < 2)
    {
        post("Mais non c'est pas ça !!");
        return;
    }
    /*
     * Alright, so in my former set-up pd is always one step behind when it comes to controls,
     * -- I used to send resp. controllerId to left and value to right inlets of [pack]
     * -- It's because of the [ctlin], I think the output bang from right to left,
     * -- so when I switch the order, when inputting them to the [pack],
     * -- The hot inlet bangs first and sends the oudated control.
     * I found out when using the pedal, which goes from 0 to 127 and back again,
     * and that would be more of problem than, say, which the 'potards'.
     * So I'll just switch value first and controller id second, then.
     * Awesome!
     */
    const cant::pan::id_u8 channel = 1;
    const auto value = static_cast<cant::pan::id_u8>(atom_getfloat(argv));
    const auto controllerId = static_cast<cant::pan::id_u8>(atom_getfloat(argv + 1));
    const auto data = cant::pan::MidiControlInputData(channel, controllerId, value);
    try
    {
        x->cantina->receiveControl(data);
    }
    catch (const cant::CantinaException& e)
    {
       std::cerr << e.what() << std::endl;
    }
}

void cantina_tilde_controllers(t_cantina_tilde *x, t_symbol *, int argc, t_atom *argv)
{
    if(argc < 3)
    {
        post("Wrong format for controller definition: expected type, channel, and controller id.");
        return;
    }
    char buf[20];
    atom_string(argv, buf, 20);
    const auto type = std::string(buf);
    const auto channelId  = static_cast<cant::pan::id_u8>(atom_getfloat(argv + 1));
    const auto controllerId = static_cast<cant::pan::id_u8>(atom_getfloat(argv + 2));
    try
    {
        x->cantina->setController(type, channelId, {controllerId });
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
                    A_CANT,
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
    post("Cant version : %.4g", cant::CANTINA_VERSION);
    post("Cant brew    : %s", cant::CANTINA_BREW);
    post("~ tut-tut-tut-tut-tulut-tut ~");
}