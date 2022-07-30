// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "st_audiofile.h"
#if defined(ST_AUDIO_FILE_USE_SNDFILE)
#if defined(_WIN32)
#include <windows.h>
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#endif
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>

struct st_audio_file {
    SNDFILE* snd;
    SF_INFO info;
    // Virtual IO data
    const void* data;
    sf_count_t offset;
    sf_count_t size;
};

static sf_count_t sndfile_vio_get_filelen(void *user_data)
{
	const struct st_audio_file* af = user_data;
	return af->size;
}

static sf_count_t sndfile_vio_seek(sf_count_t offset, int whence, void *user_data)
{
	struct st_audio_file* af = user_data;

    sf_count_t new_offset = af->offset;
	switch(whence) {
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            new_offset += offset;
            break;
        case SEEK_END:
            new_offset = af->size + offset;
            break;
	}

	if( ( new_offset >= 0 ) && ( new_offset < af->size ) ) {
		af->offset = new_offset;
	} else {
		return 1;
	}

	return 0;
}

static sf_count_t sndfile_vio_read(void *ptr, sf_count_t count, void *user_data)
{
	struct st_audio_file* af = user_data;
    sf_count_t remaining = af->size - af->offset;
    sf_count_t to_read = remaining > count ? count : remaining;
    memcpy(ptr, af->data + af->offset, to_read);
    af->offset += to_read;
	return to_read;
}

static sf_count_t sndfile_vio_write(const void *ptr, sf_count_t count, void *user_data)
{
	(void)ptr;
	(void)count;
	(void)user_data;
	return -1;
}

static sf_count_t sndfile_vio_tell(void *user_data)
{
	const struct st_audio_file* af = user_data;
	return af->offset;
}


st_audio_file* st_open_memory(const void* memory, size_t length)
{
    st_audio_file* af = (st_audio_file*)malloc(sizeof(st_audio_file));
    if (!af)
        return NULL;

    memset(&af->info, 0, sizeof(SF_INFO));
    af->data = memory;
    af->size = length;
    af->offset = 0;

    SF_VIRTUAL_IO virtual_io = {
        .get_filelen = sndfile_vio_get_filelen,
        .seek = sndfile_vio_seek,
        .read = sndfile_vio_read,
        .write = sndfile_vio_write,
        .tell = sndfile_vio_tell
    };

    af->snd = sf_open_virtual(&virtual_io, SFM_READ, &af->info, af);
    return af;
}

st_audio_file* st_open_file(const char* filename)
{
    st_audio_file* af = (st_audio_file*)malloc(sizeof(st_audio_file));
    if (!af)
        return NULL;

    memset(&af->info, 0, sizeof(SF_INFO));

    af->snd = sf_open(filename, SFM_READ, &af->info);
    if (!af->snd) {
        free(af);
        return NULL;
    }

    return af;
}

#if defined(_WIN32)
st_audio_file* st_open_file_w(const wchar_t* filename)
{
    st_audio_file* af = (st_audio_file*)malloc(sizeof(st_audio_file));
    if (!af)
        return NULL;

    af->snd = sf_wchar_open(filename, SFM_READ, &af->info);
    if (!af->snd) {
        free(af);
        return NULL;
    }

    return af;
}
#endif

void st_close(st_audio_file* af)
{
    if (af->snd)
        sf_close(af->snd);

    free(af);
}

int st_get_type(st_audio_file* af)
{
    int type = st_audio_file_other;

    switch (af->info.format & SF_FORMAT_TYPEMASK) {
    case SF_FORMAT_WAV:
        type = st_audio_file_wav;
        break;
    case SF_FORMAT_FLAC:
        type = st_audio_file_flac;
        break;
    case SF_FORMAT_AIFF:
        type = st_audio_file_aiff;
        break;
    case SF_FORMAT_OGG:
        type = st_audio_file_ogg;
        break;
    }

    return type;
}

uint32_t st_get_channels(st_audio_file* af)
{
    return af->info.channels;
}

float st_get_sample_rate(st_audio_file* af)
{
    return af->info.samplerate;
}

uint64_t st_get_frame_count(st_audio_file* af)
{
    return af->info.frames;
}

bool st_seek(st_audio_file* af, uint64_t frame)
{
    return sf_seek(af->snd, frame, SEEK_SET) != -1;
}

uint64_t st_read_s16(st_audio_file* af, int16_t* buffer, uint64_t count)
{
    return sf_readf_short(af->snd, buffer, count);
}

uint64_t st_read_f32(st_audio_file* af, float* buffer, uint64_t count)
{
    return sf_readf_float(af->snd, buffer, count);
}

SNDFILE* st_get_sndfile_handle(st_audio_file* af)
{
    return af->snd;
}

int st_get_sndfile_format(st_audio_file* af)
{
    return af->info.format;
}

#endif // defined(ST_AUDIO_FILE_USE_SNDFILE)
