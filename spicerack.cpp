/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* Ziping Branch Edit */
/* ====================================================================
 * Copyright (c) 1999-2010 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/*
 * spicerack.cpp - Simple pocketsphinx command-line application to test
 *                both continuous listening/silence filtering from microphone, 
 *                thereby controlling the motor.
 *                
 */

/*
 * This is a simple example of pocketsphinx application that uses continuous listening
 * with silence filtering to automatically segment a continuous stream of audio input
 * into utterances that are then decoded.
 * 
 * Remarks:
 *   - Each utterance is ended when a silence segment of at least 1 sec is recognized.
 *   - Single-threaded implementation for portability.
 *   - Uses audio library; can be replaced with an equivalent custom library.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#else
#include <sys/select.h>
#endif

#include <sphinxbase/err.h>
#include <sphinxbase/ad.h>
#include "pocketsphinx/src/libpocketsphinx/kws_search.h"
#include "pocketsphinx/include/pocketsphinx.h"
#include "gpio/GPIO.h"
#include "motor/motor.h"
#include "i2c/I2CDevice.h"
#define TM4CADDR 0x3C

glist_t detected_kws[256] = {0}; // pre-allocate 256 sized array for holding detected keywords
glist_t temp[256] = {0}; // temporary array used when sorting detected_kws array based on prob score 

uint32 kws_num = 0; // a variable that holds the count of keywords spotted

std::string str_spices[6] = {
    "CUMIN SEED",
    "GARLIC POWDER",
    "HAMBURGER SEASONING",
    "BAY LEAVES",
    "PUMPKIN PIE SPICE",
    "MEXICAN OREGANO"
};


void Merger(glist_t arr[], int lo, int  mi, int hi){
    //glist_t *temp = (glist_t*) malloc(sizeof(glist_t)*(hi-lo+1)); //new int[hi-lo+1];//temporary merger array
    int i = lo, j = mi + 1;//i is for left-hand,j is for right-hand
    int k = 0;//k is for the temporary array
    while(i <= mi && j <=hi){
        if(((kws_detection_t*) gnode_ptr(arr[i]))->prob >= ((kws_detection_t*) gnode_ptr(arr[j]))->prob)
            temp[k++] = arr[i++];
        else
            temp[k++] = arr[j++];
    }
    //rest elements of left-half
    while(i <= mi)
        temp[k++] = arr[i++];
    //rest elements of right-half
    while(j <= hi)
        temp[k++] = arr[j++];
    //copy the mergered temporary array to the original array
    for(k = 0, i = lo; i <= hi; ++i, ++k)
        arr[i] = temp[k];
	
	//if(temp!=NULL)
    //free(temp);
}
void MergeSortHelper(glist_t arr[], int lo, int hi){
    int mid;
    if(lo < hi){
        mid = (lo + hi) >> 1;
        MergeSortHelper(arr, lo, mid);
        MergeSortHelper(arr, mid+1, hi);
        Merger(arr, lo, mid, hi);
    }
}
// function that sorts an array based on probability size
void MergeSort(glist_t arr[], int arr_size){
    MergeSortHelper(arr, 0, arr_size-1);
}

static const arg_t cont_args_def[] = {
    POCKETSPHINX_OPTIONS,
    /* Argument file. */
    {"-argfile",
     ARG_STRING,
     NULL,
     "Argument file giving extra arguments."},
    {"-adcdev",
     ARG_STRING,
     NULL,
     "Name of audio device to use for input."},
    {"-infile",
     ARG_STRING,
     NULL,
     "Audio file to transcribe."},
    {"-inmic",
     ARG_BOOLEAN,
     "no",
     "Transcribe audio from microphone."},
    {"-time",
     ARG_BOOLEAN,
     "no",
     "Print word times in file transcription."},
    CMDLN_EMPTY_OPTION,
	{"-kws",
	  ARG_STRING,
	  NULL,
	  "File for keyphrases"
	}
};

static ps_decoder_t *ps;
static cmd_ln_t *config;
static FILE *rawfd;

static void




print_word_times()
{
    int frame_rate = cmd_ln_int32_r(config, "-frate");
    ps_seg_t *iter = ps_seg_iter(ps);
    while (iter != NULL) {
        int32 sf, ef, pprob;
        float conf;

        ps_seg_frames(iter, &sf, &ef);
        pprob = ps_seg_prob(iter, NULL, NULL, NULL);
        conf = logmath_exp(ps_get_logmath(ps), pprob);
        printf("%s %.3f %.3f %f\n", ps_seg_word(iter), ((float)sf / frame_rate),
               ((float) ef / frame_rate), conf);
        iter = ps_seg_next(iter);
    }
}

static int
check_wav_header(char *header, int expected_sr)
{
    int sr;

    if (header[34] != 0x10) {
        E_ERROR("Input audio file has [%d] bits per sample instead of 16\n", header[34]);
        return 0;
    }
    if (header[20] != 0x1) {
        E_ERROR("Input audio file has compression [%d] and not required PCM\n", header[20]);
        return 0;
    }
    if (header[22] != 0x1) {
        E_ERROR("Input audio file has [%d] channels, expected single channel mono\n", header[22]);
        return 0;
    }
    sr = ((header[24] & 0xFF) | ((header[25] & 0xFF) << 8) | ((header[26] & 0xFF) << 16) | ((header[27] & 0xFF) << 24));
    if (sr != expected_sr) {
        E_ERROR("Input audio file has sample rate [%d], but decoder expects [%d]\n", sr, expected_sr);
        return 0;
    }
    return 1;
}

/* Sleep for specified msec */
static void
sleep_msec(int32 ms)
{
#if (defined(_WIN32) && !defined(GNUWINCE)) || defined(_WIN32_WCE)
    Sleep(ms);
#else
    /* ------------------- Unix ------------------ */
    struct timeval tmo;

    tmo.tv_sec = 0;
    tmo.tv_usec = ms * 1000;

    select(0, NULL, NULL, NULL, &tmo);
#endif
}

/*
 * Main utterance processing loop:
 *     for (;;) {
 *        start utterance and wait for speech to process
 *        decoding till end-of-utterance silence will be detected
 *        print utterance result;
 *     }
 */


void writeDataToLCD(const char* data, exploringBB::I2CDevice* tm4c123)
{
    tm4c123->write('<');
    for(int i = 0; data[i] != NULL; i++)
    {
        tm4c123->write(data[i]);
    }
    tm4c123->write('>');
}

static void
recognize_from_microphone()
{
    ad_rec_t *ad;
    int16 adbuf[4096]; // Buffer for holding audio data from mic
    uint8 utt_started, in_speech; // Variables to determine speech status
    int32 k;
	int32 i = 0;
	int32 score = 0;
	kws_search_t* kws_ps = 0;
	glist_t detection_list = 0;
	const char* keyphrase;

    /* Initialize I2C for LCD display */
   // exploringBB::I2CDevice tm4c123(1, TM4CADDR);
	
    /* Initialize GPIO pins for speech status LED's */
    E_INFO("About to set ready\n");
    exploringBB::GPIO readyLED(27); //p8_17
    E_INFO("About to set busy\n");
    exploringBB::GPIO busyLED(61); //p8_26
    E_INFO("Setting direction\n");
    while(readyLED.setDirection(exploringBB::OUTPUT) == -1){};
    E_INFO("Setting direction 2\n");
    while(busyLED.setDirection(exploringBB::OUTPUT) == -1){};
    E_INFO("Setting initial state\n");
    while(readyLED.setValue(exploringBB::LOW) == -1) {};
    while(busyLED.setValue(exploringBB::HIGH) == -1) {};
    
    /* Check if Microphone or Speech Rec has failed to start */
    if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
                          (int) cmd_ln_float32_r(config,
                                                 "-samprate"))) == NULL)
        E_FATAL("Failed to open audio device\n");
    if (ad_start_rec(ad) < 0)
        E_FATAL("Failed to start recording\n");

    if (ps_start_utt(ps) < 0)
        E_FATAL("Failed to start utterance\n");
    utt_started = FALSE;
	
    /* Microphone is now listening for commands */
    /* Set LED status lights to reflect speech state */
    E_INFO("Ready to listen....\n");
    while(readyLED.setValue(exploringBB::HIGH) == -1) {};
    while(busyLED.setValue(exploringBB::LOW) == -1) {};

    /* Loop indefinitely, listening for commands on each iteration */
    /* For each iteration, listen for valid commands, and control motor accordingly*/
    while(1) {
	    
	/* Read data from microphone into buffer */
        if ((k = ad_read(ad, adbuf, 4096)) < 0)
            E_FATAL("Failed to read audio\n");
	    
	/* Process Data from microphone using pocketsphinx */
        ps_process_raw(ps, adbuf, k, FALSE, FALSE);
        in_speech = ps_get_in_speech(ps);
	    
	/* The very moment that the mic hears noise */
	/* Set utterance started status to true */
	/* and set status LED lights to reflect */
	/* that the microphone has started to hear something*/
        if (in_speech && !utt_started) {
            utt_started = TRUE;
            E_INFO("I am hearing something...\n");
	    busyLED.setValue(exploringBB::HIGH);
        }
	    
	/* Once the microphone detects a silence after having */
	/* heard something, begin the speech to text processing */
        if (!in_speech && utt_started) {
            /* speech -> silence transition, time to start new utterance  */
            ps_end_utt(ps); // end the speech listening stage, and start speech processing
			
	                /* Retrieve the variable for holding data from keyword spotting */
			kws_ps = ((kws_search_t*) ps->search);

	    	        /* Retrieve the array list of spotted keywords */
			detection_list = kws_ps->detections->detect_list;
			E_INFO("DATA: SOME PHRASES I THINK I HEARD:\n");
			kws_num = 0;
	    
	                /* Place the spotted keywords into an array*/
			/* for easier manipulation*/
			while(detection_list != NULL)
			{
			    detected_kws[kws_num++] = detection_list;
				detection_list = gnode_next(detection_list);
			}
			E_INFO("Sorting phrases\n");
		        /* Sort the spotted phrases based on probability scores */
			MergeSort(detected_kws, kws_num);
			for(i = 0; i < kws_num; i++)
			{
			  keyphrase = ((kws_detection_t*) gnode_ptr(detected_kws[i]))->keyphrase;
			  score = ((kws_detection_t*) gnode_ptr(detected_kws[i]))->prob;
			  E_INFO("DATA:  %-18s || PROB_SCORE: %5d\n", keyphrase, score);
			}
		
		        /* Here is where we can analyze the spotted keyword */
		        /* And control the motor if necessary */
		        /* We will extract the keyword with the highest score */
		        /* From the sorted array: detected_keywords */
            std::string turn_str("...Turning to Sector");

            if(kws_num > 0)
            {
                keyphrase = ((kws_detection_t*) gnode_ptr(detected_kws[0]))->keyphrase;
                

                for(int i = 0; i < SECTORS; i++)
                {
                    if(str_spices[i].compare(keyphrase) == 0)
                    {
                        busyLED.setValue(exploringBB::LOW);
                        readyLED.setValue(exploringBB::LOW);
                        std::string spice_str(keyphrase);
                        turn_str += (" " + spice_str + "\n");
                       // writeDataToLCD(turn_str.c_str(), &tm4c123);
                        E_INFO(turn_str.c_str());
                        turnToSector(i + 1);
                       // writeDataToLCD(spice_str.c_str(), &tm4c123);
                        readyLED.setValue(exploringBB::HIGH);
                        break;
                    }
                }
            }
		        

	    /* Restart the listening process again */
            if (ps_start_utt(ps) < 0)
                E_FATAL("Failed to start utterance\n");
            utt_started = FALSE;
            E_INFO("Ready to listen....\n");
	    busyLED.setValue(exploringBB::LOW);
        }
       // sleep_msec(100);
    }
    ad_close(ad);
}

int
main(int argc, char *argv[])
{

    char const *cfg;
	err_set_debug_level(4);
    printf("Error DEBUG LEVEL %d\n", err_get_debug_level());

    config = cmd_ln_parse_r(NULL, cont_args_def, argc, argv, TRUE);

    /* Handle argument file as -argfile. */
    if (config && (cfg = cmd_ln_str_r(config, "-argfile")) != NULL) {
        config = cmd_ln_parse_file_r(config, cont_args_def, cfg, FALSE);
    }

    if (config == NULL || (cmd_ln_boolean_r(config, "-inmic") == FALSE)) {
	E_INFO("Specify '-inmic yes' to recognize from microphone.\n");
        cmd_ln_free_r(config);
	return 1;
    }

    /* Initialize Speech Recognition */
    ps_default_search_args(config);
    ps = ps_init(config);
    if (ps == NULL) {
        cmd_ln_free_r(config);
        return 1;
    }

    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);
    E_INFO("Initializing MOTOR\n");
    initMotorPin();

    if (cmd_ln_boolean_r(config, "-inmic")) {
        recognize_from_microphone(); // Start listening for commands.
    } 
	
    /* This here was used to debug GPIO manipulation
    E_INFO("About to init led\n");
    exploringBB::GPIO testLED(69);
    E_INFO("ABOUT TO SET DIR\n");
    while(testLED.setDirection(exploringBB::OUTPUT) == -1){}
    E_INFO("ABOUT TO ENTER WHILE LOOP\n");
    while(1)
    {
	    E_INFO("WHILE TIME\n");
	    sleep_msec(1000);
	    testLED.setValue(exploringBB::LOW);
	    sleep_msec(1000);
	    testLED.setValue(exploringBB::HIGH);
    }
    */
    ps_free(ps);
    cmd_ln_free_r(config);

    return 0;
}
