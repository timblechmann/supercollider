//  upsampled audio backend
//  Copyright (C) 2012 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.


#ifndef AUDIO_BACKEND_UPSAMPLED_AUDIO_BACKEND_HPP
#define AUDIO_BACKEND_UPSAMPLED_AUDIO_BACKEND_HPP

#include "audio_backend_common.hpp"

#include "zita-resampler/resampler.h"

namespace nova   {
namespace detail {

template <typename sample_type, typename io_sample_type, bool blocking, bool managed_memory = true>
struct upsampling_audio_backend:
    public audio_backend_base<sample_type, io_sample_type, blocking, managed_memory>
{
    typedef audio_backend_base<sample_type, io_sample_type, blocking, managed_memory> super;

    void set_upsampling_factor(int upsampling_factor)
    {
        upsampling_factor = upsampling_factor;
    }

    void prepare(size_t input_channels, size_t output_channels, size_t frames_per_block)
    {
        using namespace std;

        size_t upsampled_frames_per_block = frames_per_block * upsampling_factor_;
        super::prepare(input_channels, output_channels, upsampled_frames_per_block);

        if (upsampling_factor_ == 1)
            return;

        input_resampler.resize(input_channels);
        output_resampler.resize(output_channels);

        int backend_sr = (int)super::get_samplerate();
        int engine_sr = backend_sr * upsampling_factor_;

        for (Resampler & resampler : input_resampler) {
            int status = resampler.setup(backend_sr, engine_sr, 1, 96);
            if (status)
                throw std::runtime_error("Resampler: upsampling factor not supported");
        }

        for (Resampler & resampler : input_resampler) {
            int status = resampler.setup(engine_sr, backend_sr, 1, 96);
            if (status)
                throw std::runtime_error("Resampler: upsampling factor not supported");
        }
    }

    void fetch_inputs(const float ** inputs, size_t backend_frames, int input_channels)
    {
        if (upsampling_factor_ == 1) {
            super::fetch_inputs(inputs, backend_frames, input_channels);
            return;
        }

        size_t engine_frames = backend_frames * upsampling_factor_;
        for (uint16_t i = 0; i != input_channels; ++i) {
            Resampler & current_resampler = input_resampler[i];
            current_resampler.inp_count = backend_frames;
            current_resampler.out_count = engine_frames;
            current_resampler.inp_data  = inputs[i];
            current_resampler.out_data  = super::input_samples[i].get();

            current_resampler.process();

            inputs[i] += backend_frames;
        }
    }

    void deliver_outputs(float ** outputs, size_t backend_frames, int output_channels)
    {
        if (upsampling_factor_ == 1) {
            super::deliver_outputs(outputs, backend_frames, output_channels);
            return;
        }

        size_t engine_frames = backend_frames * upsampling_factor_;
        for (uint16_t i = 0; i != output_channels; ++i) {
            Resampler & current_resampler = output_resampler[i];
            current_resampler.out_count = backend_frames;
            current_resampler.inp_count = engine_frames;
            current_resampler.out_data  = outputs[i];
            current_resampler.inp_data  = super::output_samples[i].get();

            current_resampler.process();
            outputs[i] += backend_frames;
        }
    }

private:
    int upsampling_factor_;
    std::vector<Resampler> input_resampler, output_resampler;
};


}
}

#endif // AUDIO_BACKEND_UPSAMPLED_AUDIO_BACKEND_HPP
