//===========================================================================
/** @file btrack~.cpp
 *  @brief The btrack~ Pd external
 *  @author Oswald Berthold
 *  @note based on max-external code btrack~.cpp by Adam Stark and external-howto code by IOhannes Zmölnig
 *  @copyright Copyright (C) Jetpack Cognition Lab
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
//===========================================================================

/**
 * include the interface to Pd 
 */
#include "m_pd.h"

//===========================================================================
// BTrack includes
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
    t_sample f;
    // An instance of the BTrack beat tracker
    BTrack *b;
    // Indicates whether the beat tracker should output beats
    bool should_output_beats;
    // the time of the last bang received in milliseconds
    double time_of_last_bang_ms;
    // a count in counter
    long count_in;
    // the recent tempi observed during count ins
    double           count_in_tempi[3];
    // An outlet for beats
    t_outlet            *beat_outlet;
    // An outlet for tempo estimates
    t_outlet          *tempo_outlet;
  } t_btrack_tilde;

  // forward method declarations
  void btrack_tilde_process(t_btrack_tilde *x,double* audioFrame);
  
  void btrack_tilde_on(t_btrack_tilde *x);
  void btrack_tilde_off(t_btrack_tilde *x);
  
  void btrack_tilde_fixtempo(t_btrack_tilde *x, t_floatarg f);
  void btrack_tilde_unfixtempo(t_btrack_tilde *x);
  
  void btrack_tilde_bang(t_btrack_tilde *x);
  void btrack_tilde_countin(t_btrack_tilde *x);
  
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

  t_float *inL = (t_float *)(w[2]);
  int n = (int)w[3];
  
  double audioFrame[n];
  
  for (int i = 0;i < n;i++)
    {
      audioFrame[i] = (double) inL[i];
    }
  
  btrack_tilde_process(x,audioFrame);
		
  /* return a pointer to the dataspace for the next dsp-object */
  return w + 4;
}


//===========================================================================
void btrack_tilde_process(t_btrack_tilde *x,double* audioFrame)
{
  // process the audio frame
  x->b->processAudioFrame(audioFrame);
    
    
  // if there is a beat in this frame
  if (x->b->beatDueInCurrentFrame())
    {
      // outlet a beat
      // defer_low((t_object *)x, (t_method)outlet_beat, NULL, 0, NULL);
      outlet_beat(x);
    }
}

//===========================================================================
void outlet_beat(t_btrack_tilde *x)
{
  if (x->should_output_beats)
    {
      // send a bang out of the beat outlet 1
      outlet_bang(x->beat_outlet);
      
      // send the tempo out of the tempo outlet 2
      outlet_float(x->tempo_outlet, (float) x->b->getCurrentTempoEstimate());
    }
}
  
/**
 * register a special perform-routine at the dsp-engine
 * this function gets called whenever the DSP is turned ON
 * the name of this function is registered in btrack_tilde_setup()
 */
void btrack_tilde_dsp(t_btrack_tilde *x, t_signal **sp)
{
  /* add btrack_tilde_perform() to the DSP-tree;
   * the btrack_tilde_perform() will expect "3" arguments (packed into an
   * t_int-array), which are:
   * the objects data-space x, 1 signal vectors and the length of the
   * signal vector
   */
  
  // get hop size and frame size
  int hopSize = (int) sp[0]->s_n;
  int frameSize = hopSize*2;
  
  // initialise the beat tracker
  x->b->updateHopAndFrameSize(hopSize, frameSize);
  
  // set up dsp
  dsp_add(btrack_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
  
}

/**
 * this is the "destructor" of the class;
 * it allows us to free dynamically allocated ressources
 */
void btrack_tilde_free(t_btrack_tilde *x)
{
  /* free any ressources associated with the given inlet */

  /* free any ressources associated with the given outlet */
  outlet_free(x->beat_outlet);
  outlet_free(x->tempo_outlet);
}

/**
 * this is the "constructor" of the class
 * the argument is the initial mixing-factor
 */
void *btrack_tilde_new()
{
  t_btrack_tilde *x = (t_btrack_tilde *)pd_new(btrack_tilde_class);

  x->b = new BTrack();
  
  /* create a new signal-outlet */
  x->beat_outlet = outlet_new(&x->x_obj, &s_bang);
  x->tempo_outlet = outlet_new(&x->x_obj, &s_float);

  // initialise variables
  x->should_output_beats = true;
  x->time_of_last_bang_ms = 0.0;
  x->count_in = 4;
  x->count_in_tempi[0] = 120;
  x->count_in_tempi[1] = 120;
  x->count_in_tempi[2] = 120;
  
  return (void *)x;
}

//===========================================================================
void btrack_tilde_on(t_btrack_tilde *x)
{
    x->should_output_beats = true;
}

//===========================================================================
void btrack_tilde_off(t_btrack_tilde *x)
{
    x->should_output_beats = false;
    x->count_in = 4;
}

//===========================================================================
void btrack_tilde_fixtempo(t_btrack_tilde *x, t_floatarg f)
{
    x->b->fixTempo(f);
    post("Tempo fixed to %f BPM",f);
}

//===========================================================================
void btrack_tilde_unfixtempo(t_btrack_tilde *x)
{
    x->b->doNotFixTempo();
    post("Tempo no longer fixed");
}

//===========================================================================
void btrack_tilde_countin(t_btrack_tilde *x)
{
    x->count_in = x->count_in-1;
    
    btrack_tilde_bang(x);
    if (x->count_in == 0)
    {
        x->should_output_beats = 1;
    }
}

//===========================================================================
void btrack_tilde_bang(t_btrack_tilde *x)
{
    double bperiod;
    double tempo;
    double mean_tempo;
    
    // get current time in milliseconds
    double ms = sys_getrealtime(); // systime_ms();
    
    // calculate beat period
    bperiod = ((double) (ms - x->time_of_last_bang_ms)); // /1000.0;
    
    // store time since last bang
    x->time_of_last_bang_ms = ms;
    
    // if beat period is between 0 and 1
    if ((bperiod < 1.0) && (bperiod > 0.0))
    {
        // calculate tempo from beat period
        tempo = (1/bperiod)*60;
        
        double sum = 0;
        
        // move back elements in tempo history and sum remaining elements
        for (int i = 0;i < 2;i++)
        {
            x->count_in_tempi[i] = x->count_in_tempi[i+1];
            sum = sum+x->count_in_tempi[i];
        }
        
        // set final element to be the newly calculated tempo
        x->count_in_tempi[2] = tempo;
        
        // add the new tempo to the sum
        sum = sum+x->count_in_tempi[2];
        
        // calculate the mean tempo by dividing the tempo by 3
        mean_tempo = sum/3;
        
        // set the tempo in the beat tracker
        x->b->setTempo(mean_tempo);
    }
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
        A_NULL, 0);

  /* whenever the audio-engine is turned on, the "btrack_tilde_dsp()" 
   * function will get called
   */
  class_addmethod(btrack_tilde_class,
		  (t_method)btrack_tilde_dsp, gensym("dsp"), A_CANT, 0);
  // handle bang
  class_addbang(btrack_tilde_class, btrack_tilde_bang);
  /* attach messages */
  // on / off
  class_addmethod(btrack_tilde_class, (t_method)btrack_tilde_on, gensym("on"), A_NULL);
  class_addmethod(btrack_tilde_class, (t_method)btrack_tilde_off, gensym("off"), A_NULL);
  // fix / unfix tempo
  class_addmethod(btrack_tilde_class, (t_method)btrack_tilde_fixtempo, gensym("fixtempo"), A_DEFFLOAT, A_NULL);
  class_addmethod(btrack_tilde_class, (t_method)btrack_tilde_unfixtempo, gensym("unfixtempo"), A_NULL);
  
  class_addmethod(btrack_tilde_class, (t_method)btrack_tilde_countin, gensym("countin"), A_NULL);
  
  /* if no signal is connected to the first inlet, we can as well 
   * connect a number box to it and use it as "signal"
   */
  CLASS_MAINSIGNALIN(btrack_tilde_class, t_btrack_tilde, f);
}

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
}
#endif
