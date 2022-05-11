/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// https://github.com/tensorflow/tflite-micro/tree/main/tensorflow/lite/micro/examples/micro_speech

#define LOADDATA    // Load data from files
#define CUSTOMDATA  // Load custom data from file (LOADDATA required)

// Project
#include "config.h"
#include "audio_provider.h"
#include "micro_features/micro_model_settings.h"

#ifndef LOADDATA

#define SAMPLE_BUFFER_SIZE 16

namespace {
bool g_is_audio_initialized = false;
// An internal buffer able to fit 16x our sample size
constexpr int kAudioCaptureBufferSize = SAMPLE_BUFFER_SIZE * 16;
int16_t g_audio_capture_buffer[kAudioCaptureBufferSize];
// A buffer that holds our output
int16_t g_audio_output_buffer[kMaxAudioSampleSize];
// Mark as volatile so we can check in a while loop to see if
// any samples have arrived yet.
volatile int32_t g_latest_audio_timestamp = 0;
}  // namespace

void CaptureSamples() {
	// This is how many samples of new data we have each time this is called
	const int number_of_samples = SAMPLE_BUFFER_SIZE;
	// Calculate what timestamp the last audio sample represents
	const int32_t time_in_ms = g_latest_audio_timestamp + (number_of_samples / (kAudioSampleFrequency / 1000));
	// Determine the index, in the history of all samples, of the last sample
	const int32_t start_sample_offset = g_latest_audio_timestamp * (kAudioSampleFrequency / 1000);
	// Determine the index of this sample in our ring buffer
	const int capture_index = start_sample_offset % kAudioCaptureBufferSize;
	// Read the data to the correct place in our buffer
	// pdm_microphone_read(g_audio_capture_buffer + capture_index, number_of_samples);

	// This is how we let the outside world know that new audio data has arrived.
	g_latest_audio_timestamp = time_in_ms;
}

TfLiteStatus InitAudioRecording(tflite::ErrorReporter* error_reporter) {
	const int sample_buffer_size = SAMPLE_BUFFER_SIZE;

	// Block until we have our first audio sample
	while (!g_latest_audio_timestamp) {
	}

	return kTfLiteOk;
}

TfLiteStatus GetAudioSamples(tflite::ErrorReporter* error_reporter, int start_ms, int duration_ms,
                             int* audio_samples_size, int16_t** audio_samples) {
	// Set everything up to start receiving audio
	if (!g_is_audio_initialized) {
		TfLiteStatus init_status = InitAudioRecording(error_reporter);
		if (init_status != kTfLiteOk) {
			return init_status;
		}
		g_is_audio_initialized = true;
	}
	// This next part should only be called when the main thread notices that the
	// latest audio sample data timestamp has changed, so that there's new data
	// in the capture ring buffer. The ring buffer will eventually wrap around and
	// overwrite the data, but the assumption is that the main thread is checking
	// often enough and the buffer is large enough that this call will be made
	// before that happens.

	// Determine the index, in the history of all samples, of the first
	// sample we want
	const int start_offset = start_ms * (kAudioSampleFrequency / 1000);
	// Determine how many samples we want in total
	const int duration_sample_count = duration_ms * (kAudioSampleFrequency / 1000);
	for (int i = 0; i < duration_sample_count; ++i) {
		// For each sample, transform its index in the history of all samples into
		// its index in g_audio_capture_buffer
		const int capture_index = (start_offset + i) % kAudioCaptureBufferSize;
		// Write the sample to the output buffer
		g_audio_output_buffer[i] = g_audio_capture_buffer[capture_index];
	}

	// Set pointers to provide access to the audio
	*audio_samples_size = kMaxAudioSampleSize;
	*audio_samples = g_audio_output_buffer;

	return kTfLiteOk;
}

int32_t LatestAudioTimestamp() { return g_latest_audio_timestamp; }

#else  // LOADDATA

#ifndef CUSTOMDATA
// Load example test data files
// https://raw.githubusercontent.com/adafruit/Adafruit_TFLite_Micro_Speech/master/examples/micro_speech_mock/audio_provider.cpp

#include "testdata/no_1000ms_audio_data.h"
#include "testdata/yes_1000ms_audio_data.h"
#include "testdata/rec_1000ms_sample_data.h"

namespace {
int16_t g_dummy_audio_data[kMaxAudioSampleSize];
int32_t g_latest_audio_timestamp = 0;
}  // namespace

TfLiteStatus GetAudioSamples(tflite::ErrorReporter* error_reporter, int start_ms, int duration_ms,
                             int* audio_samples_size, int16_t** audio_samples) {
	gpio_put(PICO_DEFAULT_LED_PIN, 1);
	const int yes_start = (0 * kAudioSampleFrequency) / 1000;
	const int yes_end = (1000 * kAudioSampleFrequency) / 1000;

	const int no_start = (2000 * kAudioSampleFrequency) / 1000;
	const int no_end = (3000 * kAudioSampleFrequency) / 1000;

	const int rec_start = (5000 * kAudioSampleFrequency) / 1000;
	const int rec_end = (6000 * kAudioSampleFrequency) / 1000;

	const int wraparound = (8000 * kAudioSampleFrequency) / 1000;
	const int start_sample = (start_ms * kAudioSampleFrequency) / 1000;
	for (int i = 0; i < kMaxAudioSampleSize; ++i) {
		const int sample_index = (start_sample + i) % wraparound;
		int16_t sample;
		if ((sample_index >= yes_start) && (sample_index < yes_end)) {
			sample = g_yes_1000ms_audio_data[sample_index - yes_start];
		} else if ((sample_index >= no_start) && (sample_index < no_end)) {
			sample = g_no_1000ms_audio_data[sample_index - no_start];
		} else if ((sample_index >= rec_start) && (sample_index < rec_end)) {
			sample = g_rec_1000ms_sample_data[sample_index - rec_start];
		} else {
			sample = 0;
		}
		g_dummy_audio_data[i] = sample;
	}
	*audio_samples_size = kMaxAudioSampleSize;
	*audio_samples = g_dummy_audio_data;
	gpio_put(PICO_DEFAULT_LED_PIN, 0);
	return kTfLiteOk;
}

int32_t LatestAudioTimestamp() {
	g_latest_audio_timestamp += 100;
	return g_latest_audio_timestamp;
}

#else  // CUSTOMDATA
// Load custom testdata file

#include "testdata/rec_yes_no_audio_data.h"

namespace {
int16_t g_dummy_audio_data[kMaxAudioSampleSize];
int32_t g_latest_audio_timestamp = 0;
}  // namespace

TfLiteStatus GetAudioSamples(tflite::ErrorReporter* error_reporter, int start_ms, int duration_ms,
                             int* audio_samples_size, int16_t** audio_samples) {
	// gpio_put(PICO_DEFAULT_LED_PIN, 1);

	const int wraparound = g_custom_audio_data_size;
	const int start_sample = (start_ms * kAudioSampleFrequency) / 1000;
	for (int i = 0; i < kMaxAudioSampleSize; ++i) {
		const int sample_index = (start_sample + i) % wraparound;
		int16_t sample;
		sample = g_custom_audio_data[sample_index];
		g_dummy_audio_data[i] = sample;
	}
	*audio_samples_size = kMaxAudioSampleSize;
	*audio_samples = g_dummy_audio_data;

	// gpio_put(PICO_DEFAULT_LED_PIN, 0);
	return kTfLiteOk;
}

int32_t LatestAudioTimestamp() {
	g_latest_audio_timestamp += 100;
	return g_latest_audio_timestamp;
}

#endif  // CUSTOMDATA

#endif  // LOADDATA
