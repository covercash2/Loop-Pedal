//
//  soundloop.c
//  
//
//  Created by Chris Overcash on 4/12/13.
//
//

#include <stdio.h>
#include <assert.h>

// __android_log_print(ANDROID_LOG_INFO, char* tag, char* log)
#include <android/log.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#define FILE "/file"

#define TAG "soundloop.c"

/* 0 for no output. 1 for function logs. 2+ for verbose mode */
#define TEST 1

/* audio storage size in 16 bit words
 * sample rate = 44100 Hz = 44.1 kHz (kHz = words/sec)
 * (2584 buffers) (512 words) = 1323008 words
 * (1323008 words) / (44100 words/sec) = 30.0001 (pretty much 30 sec)
 * (1323008 words) (2 B/word) = 2646016 Bytes ~~ 2 MB
 *
 * Audio File size = 2646016 B ~~ 2 MB
 * Audio File length = 30 sec
 *
 * These values represent maximums and can be truncated for shorter tracks
 *
 */
#define AUDIO_DATA_STORAGE_SIZE 1323008 //
// 2584 buffers. 16 bit words
#define AUDIO_DATA_BUFFER_SIZE 512

#define MAX_NUMBER_INTERFACES 5
#define MAX_NUMBER_OUTPUT_DEVICES 6

#define MAX_NUMBER_INPUT_DEVICES 3
#define POSITION_UPDATE_PERIOD 1000 /* 1 sec */

/* engine interfaces */
static SLObjectItf engineObject = NULL;
static SLEngineItf engineItf;

/* output mix interfaces */
static SLObjectItf outputMixObject = NULL;

/* buffer queue player interfaces */
static SLObjectItf 						player = NULL;
static SLPlayItf 						playItf;
static SLAndroidSimpleBufferQueueItf	playerBufferQueueItf;
static SLAndroidSimpleBufferQueueItf	recordBufferQueueItf;
static SLVolumeItf						volumeItf;

/* recorder interfaces */
static SLObjectItf 						recorder = NULL;
static SLRecordItf 						recordItf;
static SLAudioIODeviceCapabilitiesItf 	AudioIODeviceCapabilitiesItf;

/* arrays to pass parameters to functions 
 * iidArray contains a list of interface IDs
 * 	to be passed to functions as definitions
 * required is a list of valid bits describing
 *	iidArray in terms of:
 *		SL_BOOLEAN_TRUE		or
 *		SL_BOOLEAN_FALSE
 */
static SLboolean 		required[MAX_NUMBER_INTERFACES];
static SLInterfaceID 	iidArray[MAX_NUMBER_INTERFACES];
//
///* recording interfaces */
//static SLObjectItf		recorder;
//static SLRecordItf		recordItf;


static int numBuffers = 0;


void Log(char* msg) 
{
	if (TEST > 0) {
		// __android_log_print(ANDROID_LOG_INFO, char* tag, char* log)
		__android_log_print(ANDROID_LOG_INFO, TAG, msg);
	}
}

/* checks for errors in the SLresult */
void Check(SLresult result)
{
//     if (result != SL_RESULT_SUCCESS)
//     {
// 		__android_log_print(ANDROID_LOG_INFO, TAG, "bad result");
//     }
	assert(result == SL_RESULT_SUCCESS);

}


/*
 * Contains information about an audio track
 */
typedef struct CallbackCntxt_ {
    SLint16* pDataBase;     // base address of audio data
    SLint16* pData;         // current address
    SLuint32 size;			// the size of the recording
} CallbackCntxt;

CallbackCntxt playerCntxt;

CallbackCntxt recordCntxt;

void logCntxt(CallbackCntxt cntxt, char* message) 
{
	Log(message);

	if (TEST > 1) {
	char msg[150];
		sprintf(msg, 
			"pDataBase %p size %x pData %p buffer size %x numBuffers %d",
			cntxt.pDataBase, cntxt.size, cntxt.pData, AUDIO_DATA_BUFFER_SIZE, 
			numBuffers);
	
		Log(msg);
	}
}

/* Local storage for audio data */
SLint16 pcmData[AUDIO_DATA_STORAGE_SIZE];

/* buffer queue callback
 * called when PlayerBufferQueue finishes */
void BufferQueuePlayerCallback(
	SLAndroidSimpleBufferQueueItf queueItf, void *pContext)
{

	SLresult result;
	
	logCntxt(playerCntxt, "player callback");
	
	// if the pointer points to data before the end of the sound byte
 	if(playerCntxt.pData < (playerCntxt.pDataBase + playerCntxt.size)) {
 		// enqueue the next buffer
		result = (*playerBufferQueueItf)->Enqueue(playerBufferQueueItf, 
			(void*) playerCntxt.pData, (2 * AUDIO_DATA_BUFFER_SIZE)); 
										/* Size given in bytes. */
 		Check(result); 
		/* Increase data pointer by buffer size */ 
 		playerCntxt.pData += AUDIO_DATA_BUFFER_SIZE; 
 	} else {
 		// enqueue the first buffer
 		playerCntxt.pData = playerCntxt.pDataBase;
 		
 		result = (*playerBufferQueueItf)->Enqueue(playerBufferQueueItf,
 			(void*) playerCntxt.pData, (2 * AUDIO_DATA_BUFFER_SIZE));
 										/* size given in bytes. */
 		Check(result);
 		playerCntxt.pData += AUDIO_DATA_BUFFER_SIZE;
 		
 		logCntxt(playerCntxt, "player callback loop");
 	}
	
}

void BufferQueueRecorderCallback(
	SLAndroidSimpleBufferQueueItf queueItf, void *pContext)
{
	SLresult result;
	
	logCntxt(recordCntxt, "record callback");
	
	if (recordCntxt.size < AUDIO_DATA_STORAGE_SIZE)
	{	
		result = (*queueItf)->Enqueue(queueItf, (void*)recordCntxt.pData,
			(2 * AUDIO_DATA_BUFFER_SIZE));
		Check(result);
		recordCntxt.pData += AUDIO_DATA_BUFFER_SIZE;
		recordCntxt.size += AUDIO_DATA_BUFFER_SIZE;
		numBuffers++;
	}
}

void Java_com_capstone_soundloop_AudioEngine_createEngine(JNIEnv* env, jclass clazz)
{
	SLresult result;
	int i;

	SLBufferQueueState 	state;
	
	/*==========================================================================
	 * ENGINE
	 *========================================================================*/
	 
	/*--------------CREATE ENGINE--------------*/
	// Engine options for the engine
	 SLEngineOption EngineOptions[] = {(SLuint32) SL_ENGINEOPTION_THREADSAFE,
	 		(SLuint32) SL_BOOLEAN_TRUE};
	
	// create the engine	 
	result = slCreateEngine(&engineObject, 1, EngineOptions, 0, NULL, NULL);
	Check(result);
	
	// realize the engine
	result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
	Check(result);
	
	// get the engine interface, used to create other objects
	result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, 
		(void*)&engineItf);
	Check(result);
	
	// initialize arrays required and iidArray
	for (i = 0; i < MAX_NUMBER_INTERFACES; i++) {
		required[i] = SL_BOOLEAN_FALSE;
		iidArray[i] = SL_IID_NULL;
	}
	
	Log("engine created");
}



/*==============================================================================
				PLAYER FUNCTIONS
==============================================================================*/

void Java_com_capstone_soundloop_AudioEngine_createPlayer(JNIEnv* env,
	jclass clazz)
{
	
	SLresult result;
	
	SLDataLocator_AndroidSimpleBufferQueue 	bufferQueue;
	SLDataLocator_OutputMix 				locator_outputmix;	
	SLDataSource 							audioSource;
	SLDataSink 								audioSink;
	SLDataFormat_PCM 						pcm;
	
	//create output mix object
	result = (*engineItf)->CreateOutputMix(engineItf, &outputMixObject, 0,
		iidArray, required);
	Check(result);
	
	// realize the output mix
	result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
	Check(result);
	
	/*--------------CONFIGURE AUDIO--------------*/
	// configure audio source
	bufferQueue.locatorType = SL_DATALOCATOR_BUFFERQUEUE;
	bufferQueue.numBuffers = 3; /* just 3 buffers for now */
	
	// configure the audio format
	pcm.formatType = SL_DATAFORMAT_PCM;
	pcm.numChannels = 2;
	pcm.samplesPerSec = SL_SAMPLINGRATE_44_1;
	pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	pcm.containerSize = 16;
	pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
	pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
	
	audioSource.pFormat = (void*)&pcm;
	audioSource.pLocator = (void*)&bufferQueue;
	
	// setup data sink structure
	locator_outputmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
	locator_outputmix.outputMix = outputMixObject;
	audioSink.pLocator = (void*)&locator_outputmix;
	audioSink.pFormat = NULL;
	
	/*--------------CALLBACK CONTEXT--------------*/
	playerCntxt.pDataBase = (void*)&pcmData;
	playerCntxt.pData = playerCntxt.pDataBase;
	playerCntxt.size = sizeof(pcmData);
	
	/*--------------SEEK INTERFACE--------------*/
	// set arrays for seek interface. (PlayerItf is implicit)
	required[0] = SL_BOOLEAN_TRUE;
	iidArray[0] = SL_IID_BUFFERQUEUE;
	required[1] = SL_BOOLEAN_TRUE;
	iidArray[1] = SL_IID_VOLUME;
	
	// create audio player
	result = (*engineItf)->CreateAudioPlayer(engineItf, &player, &audioSource,
		&audioSink, 2, iidArray, required);
	Check(result);
	
	// realize audio player in synchronous mode
	result = (*player)->Realize(player, SL_BOOLEAN_FALSE);
	Check(result);
	
	// get play interface
	result = (*player)->GetInterface(player, SL_IID_PLAY, (void*)&playItf);
	Check(result);
	
	// get volume interface
	result = (*player)->GetInterface(player, SL_IID_VOLUME, (void*)&volumeItf);
	Check(result);
	
	// get seek interface
	result = (*player)->GetInterface(player, SL_IID_BUFFERQUEUE,
		(void*)&playerBufferQueueItf);
	Check(result);
	
	/*--------------CONFIGURE FOR PLAYBACK--------------*/
	// register callback
	result = (*playerBufferQueueItf)->RegisterCallback(playerBufferQueueItf, 
		BufferQueuePlayerCallback, NULL);
	Check(result);
	
	// set volume to -3dB (-300mB)
	result = (*volumeItf)->SetVolumeLevel(volumeItf, -300);
	Check(result);
	
	Log("audio player created");
}


void Java_com_capstone_soundloop_AudioEngine_play(JNIEnv* env, jclass clazz)
/* Plays recorded data.
 * Enqueue buffers to be played
 */
{
	SLresult result;
	
	// stop recording
	result = (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_STOPPED);
	Check(result);
	
	
	playerCntxt.pDataBase = (void*)&pcmData;
	playerCntxt.pData = playerCntxt.pDataBase;
	playerCntxt.size = recordCntxt.size;
	
	logCntxt(playerCntxt, "initial play");
	
	// Enqueue buffers
	result = (*playerBufferQueueItf)->Enqueue(playerBufferQueueItf, 
		playerCntxt.pData, (2 * AUDIO_DATA_BUFFER_SIZE));
	Check(result);
	playerCntxt.pData += AUDIO_DATA_BUFFER_SIZE;
	
	logCntxt(playerCntxt, "1st play buffer");
	
	result = (*playerBufferQueueItf)->Enqueue(playerBufferQueueItf, 
		playerCntxt.pData, (2 * AUDIO_DATA_BUFFER_SIZE));
	playerCntxt.pData += AUDIO_DATA_BUFFER_SIZE;	
	Check(result);
	
	result = (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PLAYING);
	Check(result);

}

void Java_com_capstone_soundloop_AudioEngine_stop(JNIEnv* env, jclass clazz)
{
	SLresult result;
	
	Log("stop");
	
	result = (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_STOPPED);
	Check(result);
	
	result = (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_STOPPED);
	Check(result);
}

void Java_com_capstone_soundloop_AudioEngine_stopPlaying(JNIEnv* env, jclass clazz)
{
	SLresult result;

	Log("stopPlaying");

	result = (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_STOPPED);
	Check(result);
}


/*==============================================================================
				RECORDER FUNCTIONS
==============================================================================*/

void Java_com_capstone_soundloop_AudioEngine_createRecorder(JNIEnv* env,
	jclass clazz)
{
	
	SLresult result;
	int i;

	SLDataSource 	audioSource;
	SLDataSink 		audioSink;
	
	SLAudioInputDescriptor 					AudioInputDescriptor;
	SLDeviceVolumeItf 						deviceVolumeItf;
	
	SLDataLocator_URI 	uri;
	SLDataFormat_PCM	pcm;
	
	SLuint32 	inputDeviceIDs[MAX_NUMBER_INPUT_DEVICES];
	SLint32 	numInputs = 0;
	
	/*--------------AUDIO SOURCE--------------*/
	SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, 
			SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
	audioSource.pLocator = (void*)&loc_dev;
	audioSource.pFormat = NULL;
	
	/*--------------DATA SINK--------------*/
	SLDataLocator_AndroidSimpleBufferQueue loc_bq = 
		{SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 4};
	
	// data format	
	pcm.formatType = SL_DATAFORMAT_PCM;
	pcm.numChannels = 2;
	pcm.samplesPerSec = SL_SAMPLINGRATE_44_1;
	pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	pcm.containerSize = 16;
	pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
	pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
	
	audioSink.pLocator = (void*)&loc_bq;
	audioSink.pFormat = (void*)&pcm;
	
	/*--------------CONFIGURE RECORDER--------------*/
	// create audio recorder
	const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
	const SLboolean req[1] = {SL_BOOLEAN_TRUE};
	result = (*engineItf)->CreateAudioRecorder(engineItf, &recorder,
		&audioSource, &audioSink, 1, id, req);
	Check(result);
	
	result = (*recorder)->Realize(recorder, SL_BOOLEAN_FALSE);
	Check(result);
	
	// get the record interface
	result = (*recorder)->GetInterface(recorder, SL_IID_RECORD, 
		&recordItf);
	Check(result);
	
	// get the buffer queue interface
	result = (*recorder)->GetInterface(recorder, 
		SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &recordBufferQueueItf);
	Check(result);
	
	// register callback
	result = (*recordBufferQueueItf)->RegisterCallback(recordBufferQueueItf, 
		BufferQueueRecorderCallback, NULL);
	Check(result);
	
	Log("recorder created");
}

void Java_com_capstone_soundloop_AudioEngine_record(JNIEnv* env, jclass clazz)
/* records data from input
 */
{
	Log("record event");

	SLresult result;
	
	// in case already recording, stop recording and clear buffer queue
	result = (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_STOPPED);
	Check(result);
	result = (*playerBufferQueueItf)->Clear(playerBufferQueueItf);
	Check(result);
	
	// buffer not valid for playback
	recordCntxt.size = 0;
	recordCntxt.pDataBase = (void*)&pcmData;
	recordCntxt.pData = recordCntxt.pDataBase;
	
	logCntxt(recordCntxt, "initial record");
	
	// enqueue buffers to be filled
	result = (*recordBufferQueueItf)->Enqueue(recordBufferQueueItf, 
		(void*)recordCntxt.pData, (2 * AUDIO_DATA_BUFFER_SIZE));
	Check(result);
	recordCntxt.pData += AUDIO_DATA_BUFFER_SIZE;
	recordCntxt.size += AUDIO_DATA_BUFFER_SIZE;
	numBuffers++;
	
	logCntxt(recordCntxt, "1st record buffer");
	
	result = (*recordBufferQueueItf)->Enqueue(recordBufferQueueItf, 
		(void*)recordCntxt.pData, (2 * AUDIO_DATA_BUFFER_SIZE));
	Check(result);
	recordCntxt.pData += AUDIO_DATA_BUFFER_SIZE;
	recordCntxt.size += AUDIO_DATA_BUFFER_SIZE;
	numBuffers++;
	
	// start recording
	result = (*recordItf)->SetRecordState(recordItf, 
		SL_RECORDSTATE_RECORDING);
	
}

void Java_com_capstone_soundloop_AudioEngine_stopRecording(JNIEnv* env, jclass clazz)
{
	Log("stopRecording");

	SLresult result;

	result = (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_STOPPED);
	Check(result);
}

void Java_com_capstone_soundloop_AudioEngine_reset(JNIEnv* env, jclass clazz)
{
	Log("reset");

	SLresult result;
	
	numBuffers = 0;
	
	result = (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_STOPPED);
	Check(result);
	
	result = (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_STOPPED);
	Check(result);
	
	result = (*playerBufferQueueItf)->Clear(playerBufferQueueItf);
	Check(result);
	
	result = (*recordBufferQueueItf)->Clear(recordBufferQueueItf);
	Check(result);
	
}

void Java_com_capstone_soundloop_AudioEngine_getPcmData(JNIEnv* env, jclass clazz)
/* get the recorded audio */
{


}

void Java_com_capstone_soundloop_AudioEngine_shutdown(JNIEnv* env, jclass clazz)
/* destroy objects and shutdown */
{
	SLresult result;
	
	if (player != NULL) {
		(*player)->Destroy(player);
		player = NULL;
		playItf = NULL;
		playerBufferQueueItf = NULL;
		volumeItf = NULL;
	}

	if (recorder != NULL) {
		(*recorder)->Destroy(recorder);
		recorder = NULL;
		recordItf = NULL;
		AudioIODeviceCapabilitiesItf = NULL;
	}

	if (outputMixObject != NULL) {
		(*outputMixObject)->Destroy(outputMixObject);
		outputMixObject = NULL;
	}

	if (engineObject != NULL) {
		(*engineObject)->Destroy(engineObject);
		engineObject = NULL;
		engineItf = NULL;
	}

	result = (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_STOPPED);
	Check(result);
	

}



