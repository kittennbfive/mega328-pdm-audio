#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <getopt.h>

/*
This tool converts a wave file into a PDM-data-file to be played through an ATmega328P.

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!

Please read the documentation!

GCC: Optimisation -ftree-vrp enabled by -O2 or -O3 breaks this code! Use -fno-tree-vrp if needed!

version 1 - 29.05.22
*/

typedef struct __attribute__((__packed__))
{
	char chunkID[4]; //must be "RIFF" by specification
	uint32_t ChunkSize; //filesize-8
	char riffType[4]; //must be "WAVE" by specification
} wav_header_t;

typedef struct __attribute__((__packed__))
{
	char fmt_sig[4]; //must be "fmt " by specification
	uint32_t fmt_size; //must be 16 by specification
	uint16_t fmt_tag; //only 0x0001 == PCM supported by this code
	uint16_t channels; //only 1 supported by this code
	uint32_t sample_rate; //only 8000 or 16000 supported by this code
	uint32_t bytes_per_sec;
	uint16_t block_align;
	uint16_t bits_per_sample; //only 16 supported by this code
} wav_fmt_t;

typedef struct __attribute__((__packed__))
{
	char data_sig[4]; //must be "data" by specification
	uint32_t length; //of actual audio-data in bytes
	//audio-data follows immediately
} wav_data_t;


void print_usage_and_exit(void)
{
	printf("usage: pdmconv --osr $osr infile.wav outfile\n\n$osr is the oversampling ratio, higher means better quality.\nInput wave file must be single channel 16 bit signed PCM with 8kHz or 16kHz sampling rate.\nOutput file can be transfered to formated SD-card as a regular file or written as raw image using dd.\nWarning: An existing file will be overwritten!\nPlease read the documentation.\n\n");
	exit(0);
}

int main(int argc, char **argv)
{
	const struct option optiontable[]=
	{
		{ "osr",		required_argument,	NULL,	0 }, //mandatory!
		{ "version",	no_argument,		NULL, 	100 },
		{ "help",		no_argument,		NULL, 	101 },
		{ "usage",		no_argument,		NULL, 	101 },
		{ NULL, 0, NULL, 0 }
	};

	int optionindex;
	int opt;

	uint8_t oversampling_ratio=0;

	bool only_print_version=false;

	printf("\nThis is pdmconv version 1 (c) 2022 by kittennbfive\nThis tool is released under AGPLv3+ and comes WITHOUT ANY WARRANTY!\n\n");

	if(argc==1)
		print_usage_and_exit();

	while((opt=getopt_long(argc, argv, "", optiontable, &optionindex))!=-1)
	{
		switch(opt)
		{
			case '?': print_usage_and_exit(); break;
			case 0: oversampling_ratio=atoi(optarg); break;
			case 100: only_print_version=true; break;
			case 101: print_usage_and_exit(); break;

			default: errx(1, "don't know how to handle %d returned by getopt_long", opt); break;
		}
	}

	if(only_print_version)
		return 0;

	if(oversampling_ratio==0)
		errx(1, "invalid value or missing argument --osr");

	if(argc!=5)
		errx(1, "missing input and/or output file name");

	char input_file_name[50], output_file_name[50];

	strncpy(input_file_name, argv[optind], 49);

	strncpy(output_file_name, argv[optind+1], 49);

	printf("Input file: \"%s\"\nOutput file: \"%s\"\n\n", input_file_name, output_file_name);

	FILE * inp=fopen(input_file_name, "rb");
	if(!inp)
		err(1, "opening input file %s failed", input_file_name);

	wav_header_t header;
	if(fread(&header, sizeof(wav_header_t), 1, inp)!=1)
		err(1, "reading file header failed");

	if(memcmp(header.chunkID, "RIFF", 4))
		errx(1, "not a wav file, invalid chunkID");

	if(memcmp(header.riffType, "WAVE", 4))
		errx(1, "not a wav file, invalid RIFFtype");

	wav_fmt_t fmt;
	if(fread(&fmt, sizeof(wav_fmt_t), 1, inp)!=1)
		err(1, "reading fmt block failed");

	if(memcmp(fmt.fmt_sig, "fmt ", 4))
		errx(1, "fmt block has invalid signature");

	if(fmt.fmt_size!=16)
		errx(1, "fmt block has wrong size");

	if(fmt.fmt_tag!=0x0001)
		errx(1, "wrong data format, only plain PCM supported");

	if(fmt.channels!=1)
		errx(1, "too many channels in file, only mono supported");

	if(fmt.sample_rate!=8000 && fmt.sample_rate!=16000)
		errx(1, "wrong sample rate, only 8kHz or 16kHz supported");

	uint16_t sample_rate=fmt.sample_rate;

	if(fmt.bits_per_sample!=16)
		errx(1, "wrong number of bits per sample, only 16 bits supported");

	wav_data_t wav_data;
	if(fread(&wav_data, sizeof(wav_data_t), 1, inp)!=1)
		err(1, "reading header of wave data block failed");

	if(memcmp(wav_data.data_sig, "data", 4))
		errx(1, "header of wave data block has invalid signature");

	uint32_t nb_samples=wav_data.length/(16/8);

	printf("Input file contains %u bytes or %u samples of audio data at sampling rate %ukHz.\n", wav_data.length, nb_samples, sample_rate/1000);

	uint32_t nb_samples_output=2*oversampling_ratio*nb_samples;
	uint32_t nb_bytes_output=nb_samples_output/8;

	printf("Output file will contain %u samples and be about %.3fMB or %u sectors (512B each).\n\n", nb_samples_output, (float)nb_bytes_output/1024/1024, nb_bytes_output/512);

	float ubbr_value_float=20E6/(2*sample_rate*oversampling_ratio*2)-1;
	uint16_t ubbr_value=(uint16_t)(ubbr_value_float+0.5);

	printf("AVR configuration assuming a 20MHz crystal: UBBR %.2f -> %u\n\n", ubbr_value_float, ubbr_value);

	int16_t * data=malloc(nb_samples*sizeof(int16_t));
	if(!data)
		err(1, "malloc for input audio data failed");

	uint8_t * data_out=malloc(nb_samples_output/8*sizeof(uint8_t));
	if(!data_out)
		err(1, "malloc for output data failed");

	if(fread(data, sizeof(int16_t), nb_samples, inp)!=nb_samples)
		err(1, "reading audio data from %s failed", input_file_name);

	fclose(inp);

	FILE * out=fopen(output_file_name, "wb");
	if(!out)
		err(1, "creating output file %s failed", output_file_name);
	
	int32_t integrator1=0, integrator2=0;
	int16_t sample;
	int16_t output=0;

	uint8_t packed=0;
	uint8_t bitcount=0;

	printf("converting data and writing output file..."); fflush(stdout);

	uint32_t index_in, counter_in, counter_out, index_out;
	for(index_in=0,counter_in=0,counter_out=0,index_out=0; counter_out<(2*oversampling_ratio*nb_samples+sample_rate*oversampling_ratio); counter_in++,counter_out++)
	{
		//wait for sample_rate*oversampling_ratio output samples before starting the actual conversion to remove nasty audio glitch at the beginning of created file
		if(counter_out>sample_rate*oversampling_ratio)
		{
			sample=data[index_in];
			if(counter_in>=2*oversampling_ratio)
			{
				counter_in=0;
				index_in++;
			}
		}
		else
			sample=0;

		integrator2+=integrator1;

		if(integrator2>0)
			output=-32768;
		else
			output=32767;

		integrator1+=((int32_t)sample-output);

		integrator2-=(int32_t)output;

		//second part to avoid nasty audio glitch
		if(counter_out<sample_rate*oversampling_ratio)
			continue;

		packed<<=1;

		packed|=(output>0?1:0);

		bitcount++;

		if(bitcount==8)
		{
			data_out[index_out++]=packed;
			bitcount=0;
			packed=0;
		}
	}

	if(fwrite(data_out, sizeof(uint8_t), nb_bytes_output, out)!=nb_bytes_output)
		err(1, "writing output file %s failed", output_file_name);

	fclose(out);

	free(data);
	free(data_out);

	printf(" all done!\n\n");

	printf("Copy output file to SD-card (as a file or an image).\nDon't forget adjusting UBBR, file name and size for raw-access inside your code.\n\n");

	return 0;
}

