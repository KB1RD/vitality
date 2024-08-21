/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "distortion.h"

#include "synth_constants.h"

#include <climits>

#include <sst/waveshapers.h>

namespace vital {

  namespace {
    sst::waveshapers::QuadWaveshaperPtr sstInit(int type, sst::waveshapers::QuadWaveshaperState& state) {
      const sst::waveshapers::WaveshaperType ntype = static_cast<sst::waveshapers::WaveshaperType>(type);

      float R[sst::waveshapers::n_waveshaper_registers];
      sst::waveshapers::initializeWaveshaperRegister(ntype, R);

      for (int i = 0; i < sst::waveshapers::n_waveshaper_registers; ++i)
      {
          state.R[i] = _mm_set1_ps(R[i]);
      }

      state.init = _mm_cmpneq_ps(_mm_setzero_ps(), _mm_setzero_ps());

      return sst::waveshapers::GetQuadWaveshaper(ntype);
    }
    poly_float sstProcess(
      poly_float in,
      poly_float drive,
      sst::waveshapers::QuadWaveshaperPtr ws,
      sst::waveshapers::QuadWaveshaperState& state
    ) {
#if VITAL_AVX2
      #error "Converting AVX2 to Surge SSE2 floats not supported yet. Fix when vital supports AVX2"
#elif VITAL_SSE2
      return ws(&state, in.value, drive.value);
#elif VITAL_NEON
      // Directly using intel intrinsics TBH doesn't make sense to me because GCC does better optimization natively
      // TODO: File an upstream issue in surge and encourage use of a typedef __attribute__ ((vector_size (n))) instead
      #error "Surge waveshapers don't support NEON. File an upstream issue"
#endif
    }

    force_inline int compactAudio(poly_float* audio_out, const poly_float* audio_in, int num_samples) {
      int num_full = num_samples / 2;
      for (int i = 0; i < num_full; ++i) {
        int in_index = 2 * i;
        audio_out[i] = utils::compactFirstVoices(audio_in[in_index], audio_in[in_index + 1]);
      }

      int num_remaining = num_samples % 2;

      if (num_remaining)
        audio_out[num_full] = audio_in[num_samples - 1];

      return num_full + num_remaining;
    }

    force_inline void expandAudio(poly_float* audio_out, const poly_float* audio_in, int num_samples) {
      int num_full = num_samples / 2;
      if (num_samples % 2)
        audio_out[num_samples - 1] = audio_in[num_full];

      for (int i = num_full - 1; i >= 0; --i) {
        int out_index = 2 * i;
        audio_out[out_index] = audio_in[i];
        audio_out[out_index + 1] = utils::swapVoices(audio_in[i]);
      }
    }
  } // namespace

  poly_float Distortion::getDrivenValue(int type, poly_float value, poly_float drive) {
    // Recreate the distort every time; Fresh state
    sst::waveshapers::QuadWaveshaperState state;
    sst::waveshapers::QuadWaveshaperPtr sst_ws = sstInit(type, state);

    if (sst_ws) {
      // Use the existing bit crush scale from Vital for the "digital" (bit crush) effect
      if (type == (int)vital::constants::DistortionType::wst_digital) {
        return sstProcess(value, drive * 16.0f, sst_ws, state);
      } else {
        return sstProcess(value, drive, sst_ws, state);
      }
    } else {
      // When using wst_none, assume this is a downsample instead
      sst_ws = sstInit((int)vital::constants::DistortionType::wst_digital, state);
      return sstProcess(value, (poly_float(1.001f) - poly_float(kPeriodScale) / drive) * 16.0f, sst_ws, state);
    }
  }

  Distortion::Distortion() : Processor(kNumInputs, kNumOutputs),
                             last_distorted_value_(0.0f), current_samples_(0.0f),
                             type_((int)vital::constants::DistortionType::n_ws_types) { }

  template<poly_float(*scale)(poly_float)>
  void Distortion::processTimeInvariant(int num_samples, const poly_float* audio_in, const poly_float* drive,
                                        poly_float* audio_out) {
    for (int i = 0; i < num_samples; ++i) {
      poly_float current_drive = scale(drive[i]);
      poly_float sample = audio_in[i];
      audio_out[i] = sstProcess(sample, current_drive, sst_ptr_, sst_wss_);
      VITAL_ASSERT(utils::isContained(audio_out[i]));
    }
  }

  void Distortion::processDownSample(int num_samples, const poly_float* audio_in, const poly_float* drive,
                                     poly_float* audio_out) {
    mono_float sample_rate = getSampleRate();
    poly_float current_samples = current_samples_;

    for (int i = 0; i < num_samples; ++i) {
      poly_float current_period = downSampleScale(drive[i]) * sample_rate; 
      current_samples += 1.0f;

      poly_float current_sample = audio_in[i];
      poly_float current_downsample = current_sample & constants::kFirstMask;
      current_downsample += utils::swapVoices(current_downsample);

      poly_mask update = poly_float::greaterThanOrEqual(current_samples, current_period);
      last_distorted_value_ = utils::maskLoad(last_distorted_value_, current_downsample, update);
      current_samples = utils::maskLoad(current_samples, current_samples - current_period, update);
      audio_out[i] = last_distorted_value_;
    }

    current_samples_ = current_samples;
  }

  void Distortion::processWithInput(const poly_float* audio_in, int num_samples) {
    VITAL_ASSERT(checkInputAndOutputSize(num_samples));

    int type = static_cast<int>(input(kType)->at(0)[0]);
    poly_float* audio_out = output(kAudioOut)->buffer;
    poly_float* drive_out = output(kDriveOut)->buffer;

    int compact_samples = compactAudio(audio_out, audio_in, num_samples);
    compactAudio(drive_out, input(kDrive)->source->buffer, num_samples);

    if (type != type_) {
      type_ = type;
      last_distorted_value_ = 0.0f;
      current_samples_ = 0.0f;
      sst_ptr_ = sstInit(type, sst_wss_);
    }

    if (sst_ptr_) {
      // Use the existing bit crush scale from Vital for the "digital" (bit crush) effect
      if (type == (int)vital::constants::DistortionType::wst_digital) {
        processTimeInvariant<bitCrushScale>(compact_samples, audio_out, drive_out, audio_out);
      } else {
        processTimeInvariant<driveDbScale>(compact_samples, audio_out, drive_out, audio_out);
      }
    } else {
      // When using wst_none, assume this is a downsample instead
      processDownSample(compact_samples, audio_out, drive_out, audio_out);
    }

    expandAudio(audio_out, audio_out, num_samples);
  }

  void Distortion::process(int num_samples) {
    VITAL_ASSERT(inputMatchesBufferSize(kAudio));
    processWithInput(input(kAudio)->source->buffer, num_samples);
  }
} // namespace vital
