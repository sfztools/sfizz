// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "st_audiofile.h"
#if !defined(ST_AUDIO_FILE_USE_SNDFILE)
#include "st_audiofile_libs.h"
#include <stdlib.h>

struct st_audio_file {
    int type;
    union {
        drwav *wav;
        drflac *flac;
    };
};

enum {
    st_audio_file_null = -1,
};

st_audio_file* st_open_file(const char* filename)
{
    st_audio_file* af = (st_audio_file*)malloc(sizeof(st_audio_file));
    if (!af)
        return NULL;

    af->type = st_audio_file_null;

    if (af->type == st_audio_file_null) {
        af->wav = (drwav*)malloc(sizeof(drwav));
        if (!af->wav) {
            free(af);
            return NULL;
        }
        if (!drwav_init_file(af->wav, filename, NULL))
            free(af->wav);
        else
            af->type = st_audio_file_wav;
    }

    if (af->type == st_audio_file_null) {
        af->flac = drflac_open_file(filename, NULL);
        if (af->flac)
            af->type = st_audio_file_flac;
    }

    if (af->type == st_audio_file_null) {
        free(af);
        af = NULL;
    }

    return af;
}

#if defined(_WIN32)
st_audio_file* st_open_file_w(const wchar_t* filename)
{
    st_audio_file* af = (st_audio_file*)malloc(sizeof(st_audio_file));
    if (!af)
        return NULL;

    af->type = st_audio_file_null;

    if (af->type == st_audio_file_null) {
        af->wav = (drwav*)malloc(sizeof(drwav));
        if (!af->wav) {
            free(af);
            return NULL;
        }
        if (!drwav_init_file_w(af->wav, filename, NULL))
            free(af->wav);
        else
            af->type = st_audio_file_wav;
    }

    if (af->type == st_audio_file_null) {
        af->flac = drflac_open_file_w(filename, NULL);
        if (af->flac)
            af->type = st_audio_file_flac;
    }

    if (af->type == st_audio_file_null) {
        free(af);
        af = NULL;
    }

    return af;
}
#endif

void st_close(st_audio_file* af)
{
    switch (af->type) {
    case st_audio_file_wav:
        drwav_uninit(af->wav);
        free(af->wav);
        break;
    case st_audio_file_flac:
        drflac_close(af->flac);
        break;
    }

    af->type = st_audio_file_null;
}

int st_get_type(st_audio_file* af)
{
    return af->type;
}

uint32_t st_get_channels(st_audio_file* af)
{
    uint32_t channels = 0;

    switch (af->type) {
    case st_audio_file_wav:
        channels = af->wav->channels;
        break;
    case st_audio_file_flac:
        channels = af->flac->channels;
        break;
    }

    return channels;
}

float st_get_sample_rate(st_audio_file* af)
{
    float sample_rate = 0;

    switch (af->type) {
    case st_audio_file_wav:
        sample_rate = af->wav->sampleRate;
        break;
    case st_audio_file_flac:
        sample_rate = af->flac->sampleRate;
        break;
    }

    return sample_rate;
}

uint64_t st_get_frame_count(st_audio_file* af)
{
    uint64_t frames = 0;

    switch (af->type) {
    case st_audio_file_wav:
        frames = af->wav->totalPCMFrameCount;
        break;
    case st_audio_file_flac:
        frames = af->flac->totalPCMFrameCount;
        break;
    }

    return frames;
}

bool st_seek(st_audio_file* af, uint64_t frame)
{
    bool success = false;

    switch (af->type) {
    case st_audio_file_wav:
        success = drwav_seek_to_pcm_frame(af->wav, frame);
        break;
    case st_audio_file_flac:
        success = drflac_seek_to_pcm_frame(af->flac, frame);
        break;
    }

    return success;
}

uint64_t st_read_s16(st_audio_file* af, int16_t* buffer, uint64_t count)
{
    switch (af->type) {
    case st_audio_file_wav:
        count = drwav_read_pcm_frames_s16(af->wav, count, buffer);
        break;
    case st_audio_file_flac:
        count = drflac_read_pcm_frames_s16(af->flac, count, buffer);
        break;
    }

    return count;
}

uint64_t st_read_f32(st_audio_file* af, float* buffer, uint64_t count)
{
    switch (af->type) {
    case st_audio_file_wav:
        count = drwav_read_pcm_frames_f32(af->wav, count, buffer);
        break;
    case st_audio_file_flac:
        count = drflac_read_pcm_frames_f32(af->flac, count, buffer);
        break;
    }

    return count;
}

#endif // !defined(ST_AUDIO_FILE_USE_SNDFILE)
