/************************************************************
Проект:     Программная реализация приемного УПС
Файл:       receiver.c
Автор:      Куликов А.А.
Содержание: Функции для моделирования работы приемного УПС
История:    28.02.2016 - базовая версия
*************************************************************/

#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#include <io.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "wave.h"

#define SPEED 1200
#define STEP 0
#define SAMPLE_RATE 44100

/*--------------------------------
Функция чтения заголовка WAV файла
---------------------------------*/
long wave_header(struct HEADER *header, FILE *ptr) {

	unsigned char buffer4[4];
	unsigned char buffer2[2];

	/* Чтение служебной информации из полей заголовка WAV файла */

	fread(header->riff, sizeof(header->riff), 1, ptr);

	fread(buffer4, sizeof(buffer4), 1, ptr);

	/* Конвертирование little endian в big endian */
	header->overall_size = buffer4[0] |
		(buffer4[1] << 8) |
		(buffer4[2] << 16) |
		(buffer4[3] << 24);

	fread(header->wave, sizeof(header->wave), 1, ptr);

	fread(header->fmt_chunk_marker, sizeof(header->fmt_chunk_marker), 1, ptr);

	fread(buffer4, sizeof(buffer4), 1, ptr);

	/* Конвертирование little endian в big endian */
	header->length_of_fmt = buffer4[0] |
		(buffer4[1] << 8) |
		(buffer4[2] << 16) |
		(buffer4[3] << 24);

	fread(buffer2, sizeof(buffer2), 1, ptr);

	header->format_type = buffer2[0] | (buffer2[1] << 8);
	char format_name[10] = "";
	if (header->format_type == 1)
		strcpy(format_name, "PCM");
	else if (header->format_type == 6)
		strcpy(format_name, "A-law");
	else if (header->format_type == 7)
		strcpy(format_name, "Mu-law");

	fread(buffer2, sizeof(buffer2), 1, ptr);

	header->channels = buffer2[0] | (buffer2[1] << 8);

	fread(buffer4, sizeof(buffer4), 1, ptr);

	header->sample_rate = buffer4[0] |
		(buffer4[1] << 8) |
		(buffer4[2] << 16) |
		(buffer4[3] << 24);


	fread(buffer4, sizeof(buffer4), 1, ptr);

	header->byterate = buffer4[0] |
		(buffer4[1] << 8) |
		(buffer4[2] << 16) |
		(buffer4[3] << 24);

	fread(buffer2, sizeof(buffer2), 1, ptr);

	header->block_align = buffer2[0] |
		(buffer2[1] << 8);

	fread(buffer2, sizeof(buffer2), 1, ptr);

	header->bits_per_sample = buffer2[0] |
		(buffer2[1] << 8);

	fread(header->data_chunk_header, sizeof(header->data_chunk_header), 1, ptr);

	fread(buffer4, sizeof(buffer4), 1, ptr);

	header->data_size = buffer4[0] |
		(buffer4[1] << 8) |
		(buffer4[2] << 16) |
		(buffer4[3] << 24);

	/* Вычисление количества сэмплов в WAV файле */
	unsigned int num_samples = (8 * header->data_size) / (header->channels * header->bits_per_sample);
	return num_samples;
}

/*--------------------------------
Функция чтения сэмплов WAV файла
---------------------------------*/
void data_read(unsigned int size_of_each_sample, int *dump, unsigned int num_samples, FILE *ptr, struct HEADER header) {
	unsigned int i = 0;
	char *data_buffer = (char*)calloc(size_of_each_sample, sizeof(char));
	int  size_is_correct = 1;

	/* Проверка размера сэмпла */
	long bytes_in_each_channel = (size_of_each_sample / header.channels);
	if ((bytes_in_each_channel  * header.channels) != size_of_each_sample) {
		printf("Error: %ld x %ud <> %ld\n", bytes_in_each_channel, header.channels, size_of_each_sample);
		size_is_correct = 0;
	}

	if (size_is_correct) {
		/* Определение допустимого диапозона значений сэмплов */
		long low_limit = 0l;
		long high_limit = 0l;

		switch (header.bits_per_sample) {
		case 8:
			low_limit = -128;
			high_limit = 127;
			break;
		case 16:
			low_limit = -32768;
			high_limit = 32767;
			break;
		case 32:
			low_limit = -2147483647 - 1;
			high_limit = 2147483647;
			break;
		}

		for (i = 0; i < num_samples; i++) {
			int read = fread(data_buffer, size_of_each_sample * sizeof(char), 1, ptr);
			if (read == 1) {

				/* Сохранение данных из WAV файла */
				unsigned int  xchannels = 0;
				int data_in_channel = 0;

				for (xchannels = 0; xchannels < header.channels; xchannels++) {
					/* Конвертирование little endian в big endian */
					if (bytes_in_each_channel == 4) {
						data_in_channel = data_buffer[0 + xchannels * 4] |
							(data_buffer[1 + xchannels * 4] << 8) |
							(data_buffer[2 + xchannels * 4] << 16) |
							(data_buffer[3 + xchannels * 4] << 24);
					}
					else if (bytes_in_each_channel == 2) {
						data_in_channel = data_buffer[0 + xchannels * 2];
						data_in_channel &= 0x000000ff;
						data_in_channel |= (data_buffer[1 + xchannels * 2] << 8);
					}
					else if (bytes_in_each_channel == 1) {
						data_in_channel = data_buffer[0 + xchannels];
					}

					/* Проверка значения по границам диапозона допустимых значений */
					if (data_in_channel < low_limit || data_in_channel > high_limit)
						printf("**value out of range\n");

				}
				dump[i] = data_in_channel;
			}
			else {
				printf("Error reading file. %d bytes\n", read);
				break;
			}
		}
	}
	free(data_buffer);
}

/*-----------------------------------------------
Функция демодуляции и синхронизации приемного УПС
-------------------------------------------------*/
int receiver(unsigned int num_samples, int *dump, unsigned int sample_rate, unsigned int f0, unsigned int f1, FILE *ptr) {
	long long integral_SINf0;
	long long integral_COSf0;
	long long integral_SINf1;
	long long integral_COSf1;
	long long Energyf0;
	long long Energyf1;
	long long Constant = 0;
	/* вычисление шумового порога */
	long long threshold = (long long)pow((sample_rate / SAMPLE_RATE), 2) * STEP;
	unsigned int sample = 0;
	unsigned int sample_count;
	int count;
	unsigned int bit_count_in_packet = 0;
	unsigned int registr_count = 0;
	int sync_count = 0;
	int start_count = 0;
	int end_count = 0;
	int packet_count = 0;
	/* комбинация для синхронизации */
	char sync[20] = { '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1' };
	char packet_start[12] = { '1', '1', '1', '1', '0', '0', '1', '1', '1', '1', '1', '0' };
	char packet_midl[12] = { '0', '0', '1', '0', '0', '0', '0', '0', '0', '1', '0', '0' };
	char packet_end[12] = { '0', '0', '1', '0', '0', '1', '0', '0', '1', '1', '1', '1' };
	char bit;
	char bits[12];
	/* вычисление длительности одного сэмпла
	и количества сэмплов на один бит */
	double T = 1.0 / sample_rate;
	int N = sample_rate / SPEED;
	double* t = (double*) malloc(N * sizeof(double));
	double* SINf0 = (double*) malloc(N * sizeof(double));
	double* COSf0 = (double*) malloc(N * sizeof(double));
	double* SINf1 = (double*) malloc(N * sizeof(double));
	double* COSf1 = (double*) malloc(N * sizeof(double));
	long long* Filter = (long long*) calloc(N, sizeof(long long));
	/* генерация эталонных сигналов */
	for (int i = 0; i < N; i++) {
		t[i] = (double)i * T;
		SINf0[i] = sin(2 * M_PI * f0 * t[i]);
		COSf0[i] = cos(2 * M_PI * f0 * t[i]);
		SINf1[i] = sin(2 * M_PI * f1 * t[i]);
		COSf1[i] = cos(2 * M_PI * f1 * t[i]);
	}
	free(t);
	for (long long i = 0; i < num_samples; i++) {
		Constant += dump[i];
	}
	Constant /= num_samples;
	printf("Constant: %I64d\n", Constant);
	double time = clock();
	/* пока остается еще как минимум N сэмплов */
	while (sample <= num_samples - N) {
		count = 0;
		while (count < N) {
			integral_SINf0 = 0;
			integral_COSf0 = 0;
			integral_SINf1 = 0;
			integral_COSf1 = 0;
			/* вычисляем взаимную корреляцию с эталонным сигналом текущих N сэмплов */
			for (int j = 0; j < N; j++) {
				integral_SINf0 += (long long)((dump[sample + j] - Constant) * SINf0[j]);
				integral_COSf0 += (long long)((dump[sample + j] - Constant) * COSf0[j]);
				integral_SINf1 += (long long)((dump[sample + j] - Constant) * SINf1[j]);
				integral_COSf1 += (long long)((dump[sample + j] - Constant) * COSf1[j]);
			}
			integral_SINf0 *= integral_SINf0;
			integral_COSf0 *= integral_COSf0;
			integral_SINf1 *= integral_SINf1;
			integral_COSf1 *= integral_COSf1;
			Energyf0 = integral_SINf0 + integral_COSf0;
			Energyf1 = integral_SINf1 + integral_COSf1;
			/* находим разность корреляций м-д "0" и "1" */
			Filter[count] = Energyf1 - Energyf0;
			/* если есть переход через 0, то устанавливаем счетчик принятия решений на середину */
			if (count > 0) {
				if (Filter[count - 1] < 0 && Filter[count] > 0 || Filter[count] < 0 && Filter[count - 1] > 0) {
					Filter[N/2 - 1] = Filter[count];
					count = N/2 - 1;
				}
			}
			else {
				if (Filter[N - 1] < 0 && Filter[count] > 0 || Filter[count] < 0 && Filter[N - 1] > 0) {
					Filter[N/2 - 1] = Filter[count];
					count = N/2 - 1;
				}
			}
			count++;
			sample++;
			if (sample > num_samples - N) {
				break;
			}
		}
		/* если счетчик принятия решений достиг N */
		if (count == N) {
			Filter[0] = 0;
			for (count--; count > N / 2 - 1; count--) {
				Filter[0] += Filter[count];
			}
			/* находим среднее значение разности за последние N/2 выходов */
			Filter[0] /= N / 2;
			/* если средне значение выше порога шума, то принемаем решение */
			if (Filter[0] > 0) {
				bit = '1';
			}
			else {
				bit = '0';
			}

			/* если синхронизация не установлена, то ищем синхропосылку */
			if (sync_count < 20) {
				if (bit == sync[sync_count]) {
					sync_count++;
					if (sync_count == 20) {
						printf("Sync: %3.3f second\n", (clock() - time) / CLOCKS_PER_SEC);
					}
				}
				else 
					sync_count = 0;
			}
			else {
				fprintf(ptr, "%c", bit);
				if (registr_count < 12) {
					bits[registr_count] = bit;
					registr_count++;
				}
				else {
					for (int i = 1; i < 12; i++) {
						bits[i - 1] = bits[i];
					}
					bits[11] = bit;
				}

				if (registr_count == 12) {
					if (start_count < 12 && end_count < 12) {
						for (int i = 0; i < 12; i++) {
							if (bits[i] == packet_start[i])
								start_count++;
							else start_count = 0;
							if (bits[i] == packet_end[i])
								end_count++;
							else end_count = 0;
						}

						if (start_count == 12) {
							sample_count = sample + 2675 * N;
							fseek(ptr, -12, SEEK_CUR);
							fprintf(ptr, "%s", "н111100111110");
							bit_count_in_packet = 12;
						}
						else start_count = 0;

						if (end_count == 12) {
							sample = sample - 256 * N;
							sample_count = sample + 2687 * N;
							fprintf(ptr, "%c", 'н');
							bit_count_in_packet = 0;
						}
						else end_count = 0;
					}
				}
				
				if (start_count == 12 || end_count == 12) {
					
					bit_count_in_packet++;
					//printf("%d\n", bit_count_in_packet);

					if (end_count == 12 && start_count == 0) {
						registr_count = 1;
						bits[0] = bits[11];
						start_count = 12;
					}

					if (registr_count == 12 && start_count == 12 && end_count >= 12) {
						end_count = 12;
						for (int i = 0; i < 12; i++) {
							if (bits[i] == packet_end[i])
								end_count++;
							else end_count = 12;
						}
						if (end_count == 24) {
							end_count = 0;
							sample = sample - 256 * N;
							sample_count = sample + 2687 * N;
							fseek(ptr, -2 * bit_count_in_packet, SEEK_CUR);
							fprintf(ptr, "%c", 'н');
							bit_count_in_packet = 0;
						}
					}
					
					if (sample >= sample_count) {
						sync_count = 0;
						registr_count = 0;
						start_count = 0;
						end_count = 0;
						bit_count_in_packet = 0;
						time = clock();
						fprintf(ptr, "%c", 'к');
						packet_count++;
					}
				}
			}
		}
	}
	if (sync_count < 10) {
		printf("Bad Sync...\n");
	}
	free(Filter);
	free(SINf0);
	free(SINf1);
	free(COSf0);
	free(COSf1);
	printf("Packet: %d\nOrigin bits: %d\n", packet_count, num_samples/N);
	return packet_count;
}

/*---------------------------------------
Функция побайтового сравнения двух файлов
-----------------------------------------*/
void comparison_files(FILE *ptr1, int packet_count, FILE *ptr2) {
	unsigned int size_ptr1;
	unsigned int size_ptr2;
	unsigned int start_file1, end_file1 = 0;
	unsigned int start_file2, end_file2 = 0;
	unsigned int start_error, end_error;
	unsigned int i, j;
	double error_sync = 0;
	double error_packet = 0;
	double error;
	
	/* определяем размер файлов */
	fseek(ptr1, 0, SEEK_END);
	size_ptr1 = ftell(ptr1);

	fseek(ptr2, 0, SEEK_END);
	size_ptr2 = ftell(ptr2);

	printf("Size file_1 = %d bytes\n", size_ptr1);
	printf("Size file_2 = %d bytes\n", size_ptr2);

	char *buf1 = (char*)calloc(size_ptr1, sizeof(char));
	char *buf2 = (char*)calloc(size_ptr2, sizeof(char));

	fseek(ptr1, 0, SEEK_SET);
	fseek(ptr2, 0, SEEK_SET);

	fread(buf1, sizeof(char), size_ptr1, ptr1);
	fread(buf2, sizeof(char), size_ptr2, ptr2);

	j = 0;
	while (buf2[j] != 'н') {
		j++;
	}
	start_file2 = j;

	for (int m = 0; m < packet_count; m++) {
		i = end_file1;
		while (buf1[i] != 'н') {
			i++;
		}
		start_file1 = i;

		for (int k = 1; k < 1329; k++) {
			if (buf1[start_file1 - k] != buf2[start_file2 - k])
				error_sync++;
		}

		i = start_file1 + 1;
		j = start_file2 + 1;
		while (buf2[j] != 'к') {
			if (buf1[i] != 'к') {
				if (buf1[i] != buf2[j]) {
					if (error_packet == 0)
						start_error = j - start_file2;
					error_packet++;
				}
					
				i++;
			}
			j++;
		}
		end_file1 = i;
		end_file2 = j;

		if (error_packet > 0) {
			i = end_file1 - 1;
			j = end_file2 - 1;
			while (buf1[i] == buf2[j]) {
				i--;
				j--;
				if (i <= 0 || j <= 0)
					break;
			}
			end_error = j - start_file2;
			printf("Error(%d): %d - %d bits\n", m + 1, start_error, end_error);
		}
		
		error_packet += abs((end_file2 - start_file2) - (end_file1 - start_file1));
		error = (error_packet + error_sync) / (1328 + (end_file2 - start_file2)) * 100;
		printf("Packet error(%d): %3.3f %%\n", m + 1, error);
		error_packet = 0;
		error_sync = 0;
	}

	free(buf1);
	free(buf2);
}