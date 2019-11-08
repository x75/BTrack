/*
 * HOWTO write an External for Pure data
 * (c) 2001-2006 IOhannes m zmölnig zmoelnig[AT]iem.at
 *
 * this is the source-code for the fourth example in the HOWTO
 * it creates a simple dsp-object:
 * 2 input signals are mixed into 1 output signal
 * the mixing-factor can be set via the 3rd inlet
 *
 * for legal issues please see the file LICENSE.txt
 */


/**
 * include the interface to Pd 
 */
#include "m_pd.h"

//===========================================================================
// BTrack includes
// #include "../../src/BTrack_if.h"
#include "../../src/BTrack.h"
#include "../../src/OnsetDetectionFunction.h"

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
extern "C" {
#endif

/**
 * define a new "class" 
 */
static t_class *btrack_tilde_class;

void btrack_tilde_setup(void);

/**
 * this is the dataspace of our new object
 * the first element is the mandatory "t_object"
 * f_pan denotes the mixing-factor
 * "f" is a dummy and is used to be able to send floats AS signals.
 */
typedef struct _btrack_tilde {
  t_object  x_obj;
  t_sample f_pan;
  t_sample f;
  
  t_inlet *x_in2;
  t_inlet *x_in3;
  t_outlet*x_out;
  
  // An instance of the BTrack beat tracker
  BTrack *b;
  float *kubi;
  // BTHandle b;
  // Indicates whether the beat tracker should output beats
  bool should_output_beats;
  // the time of the last bang received in milliseconds
  long time_of_last_bang_ms;
  // a count in counter
  long count_in;
  // the recent tempi observed during count ins
  double           count_in_tempi[3];
  // An outlet for beats
  t_outlet            *beat_outlet;
  // An outlet for tempo estimates
  t_outlet          *tempo_outlet;
  
} t_btrack_tilde;

void btrack_tilde_process(t_btrack_tilde *x,double* audioFrame);
void outlet_beat(t_btrack_tilde *x);
  
/**
 * this is the core of the object
 * this perform-routine is called for each signal block
 * the name of this function is arbitrary and is registered to Pd in the 
 * btrack_tilde_dsp() function, each time the DSP is turned on
 *
 * the argument to this function is just a pointer within an array
 * we have to know for ourselves how many elements inthis array are
 * reserved for us (hint: we declare the number of used elements in the
 * btrack_tilde_dsp() at registration
 *
 * since all elements are of type "t_int" we have to cast them to whatever
 * we think is apropriate; "apropriate" is how we registered this function
 * in btrack_tilde_dsp()
 */
t_int *btrack_tilde_perform(t_int *w)
{
  /* the first element is a pointer to the dataspace of this object */
  t_btrack_tilde *x = (t_btrack_tilde *)(w[1]);

  // t_float *inL = (t_float *)(w[2]);
  // int n = (int)w[3];
  
  // double audioFrame[n];
  
  // for (int i = 0;i < n;i++)
  //   {
  //     audioFrame[i] = (double) inL[i];
  //   }
  
  // btrack_tilde_process(x,audioFrame);
		
  // // you have to return the NEXT pointer in the array OR MAX WILL CRASH
  // return w + 4;
  
  /* here is a pointer to the t_sample arrays that hold the 2 input signals */
  t_sample  *in1 =    (t_sample *)(w[2]);
  t_sample  *in2 =    (t_sample *)(w[3]);
  /* here comes the signalblock that will hold the output signal */
  t_sample  *out =    (t_sample *)(w[4]);
  /* all signalblocks are of the same length */
  int          n =           (int)(w[5]);
  /* get (and clip) the mixing-factor */
  t_sample f_pan = (x->f_pan<0)?0.0:(x->f_pan>1)?1.0:x->f_pan;
  /* just a counter */
  int i;

  /* this is the main routine: 
   * mix the 2 input signals into the output signal
   */
  for(i=0; i<n; i++)
    {
      out[i]=in1[i]*(1-f_pan)+in2[i]*f_pan;
    }

  /* return a pointer to the dataspace for the next dsp-object */
  return (w+6);
}


// //===========================================================================
// void btrack_tilde_process(t_btrack_tilde *x,double* audioFrame)
// {
//   // process the audio frame
//   x->b->processAudioFrame(audioFrame);
    
    
//   // if there is a beat in this frame
//   if (x->b->beatDueInCurrentFrame())
//     {
//       // outlet a beat
//       // defer_low((t_object *)x, (t_method)outlet_beat, NULL, 0, NULL);
//       outlet_beat(x, NULL, 0, NULL);
//     }
// }

// //===========================================================================
// void outlet_beat(t_btrack_tilde *x)
// {
//   if (x->should_output_beats)
//     {
//       // send a bang out of the beat outlet 1
//       outlet_bang(x->beat_outlet);
      
//       // send the tempo out of the tempo outlet 2
//       outlet_float(x->tempo_outlet, (float) x->b->getCurrentTempoEstimate());
//     }
// }
  
/**
 * register a special perform-routine at the dsp-engine
 * this function gets called whenever the DSP is turned ON
 * the name of this function is registered in btrack_tilde_setup()
 */
void btrack_tilde_dsp(t_btrack_tilde *x, t_signal **sp)
{
  /* add btrack_tilde_perform() to the DSP-tree;
   * the btrack_tilde_perform() will expect "5" arguments (packed into an
   * t_int-array), which are:
   * the objects data-space, 3 signal vectors (which happen to be
   * 2 input signals and 1 output signal) and the length of the
   * signal vectors (all vectors are of the same length)
   */
  
  // // get hop size and frame size
  // int hopSize = (int) sp[0]->s_n;
  // int frameSize = hopSize*2;
  
  // // initialise the beat tracker
  // x->b->updateHopAndFrameSize(hopSize, frameSize);
  
  // // set up dsp
  // dsp_add(btrack_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
  
  dsp_add(btrack_tilde_perform, 5, x,
          sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

/**
 * this is the "destructor" of the class;
 * it allows us to free dynamically allocated ressources
 */
void btrack_tilde_free(t_btrack_tilde *x)
{
  /* free any ressources associated with the given inlet */
  inlet_free(x->x_in2);
  inlet_free(x->x_in3);

  /* free any ressources associated with the given outlet */
  outlet_free(x->x_out);
}

/**
 * this is the "constructor" of the class
 * the argument is the initial mixing-factor
 */
void *btrack_tilde_new(t_floatarg f)
{
  t_btrack_tilde *x = (t_btrack_tilde *)pd_new(btrack_tilde_class);

  /* save the mixing factor in our dataspace */
  x->f_pan = f;
  
  /* create a new signal-inlet */
  x->x_in2 = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);

  /* create a new passive inlet for the mixing-factor */
  x->x_in3 = floatinlet_new (&x->x_obj, &x->f_pan);

  /* create a new signal-outlet */
  x->x_out = outlet_new(&x->x_obj, &s_signal);

  // // create detection function and beat tracking objects
  // if ( (x->b = new BTrack) == NULL )
  //   {
  //     post( "btrack~: Cannot create BTrack object" );
  //     return NULL;
  //   }
  
  x->b = new BTrack();
  
  // x->b = create_btrack();
  // post((const char*)x->b);

  post("btrack_tilde_new '%s'", (const char*)x->b);
  
  x->kubi = new float(1337);
  post("btrack_tilde_new %d '%f'", x->kubi, *(x->kubi));
  
  return (void *)x;
}


/**
 * define the function-space of the class
 * within a single-object external the name of this function is very special
 */
void btrack_tilde_setup(void) {
  btrack_tilde_class = class_new(gensym("btrack~"),
        (t_newmethod)btrack_tilde_new,
        (t_method)btrack_tilde_free,
	sizeof(t_btrack_tilde),
        CLASS_DEFAULT, 
        A_DEFFLOAT, 0);

  /* whenever the audio-engine is turned on, the "btrack_tilde_dsp()" 
   * function will get called
   */
  class_addmethod(btrack_tilde_class,
		  (t_method)btrack_tilde_dsp, gensym("dsp"), A_CANT, 0);
  /* if no signal is connected to the first inlet, we can as well 
   * connect a number box to it and use it as "signal"
   */
  CLASS_MAINSIGNALIN(btrack_tilde_class, t_btrack_tilde, f);
}

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
}
#endif
