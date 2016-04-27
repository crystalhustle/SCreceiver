/****************************************************************
Проект:     Программная реализация приемного УПС
Файл:       main.c
Автор:      Куликов А.А.
Содержание: Основной код программы, реализующей приемный УПС
История:    28.02.2016 - базовая версия
*****************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <conio.h>
#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "wave.h"
#include "functions.h"

int main() {
	SetConsoleCP(1251);// установка кодовой страницы win-cp 1251 в поток ввода
	SetConsoleOutputCP(1251); // установка кодовой страницы win-cp 1251 в поток вывода
	struct HEADER header;
	char *in_filename = (char*)malloc(sizeof(char) * 1024);
	char *out_filename = (char*)malloc(sizeof(char) * 1024);

	/* ввод имени WAV файла для обработки
	и имени файла для записи результата */
	printf("Enter input filename:\n");
	fflush(stdin);
	gets(in_filename);
	//scanf("%s", in_filename);
	printf("Enter output filename:\n");
	fflush(stdin);
	gets(out_filename);
	//scanf("%s", out_filename);
	

	/* открытие файлов для чтения или для записи */
	printf("Opening  files..\n");
	FILE *fin = fopen(in_filename, "rb");
	FILE *fout = fopen(out_filename, "wb+");
	if (!fin || !fout) {
		printf("Error!\n");
		_getch();
		exit(1);
	}

	/* чтение заголовка WAV файла */
	unsigned int num_samples = wave_header(&header, fin);
	unsigned int size_of_each_sample = header.block_align;
	unsigned int sample_rate = header.sample_rate;

	/* чтение сэмплов из WAV файла, если он имеет ИК кодирование */
	if (header.format_type == 1) { // ИК
		printf("Dump sample data...\n");
		int *dump = (int*)calloc(num_samples, sizeof(int));
		data_read(size_of_each_sample, dump, num_samples, fin, header);
		/* ввод частоты "0" и "1" */
		unsigned int f0;
		unsigned int f1;
		/* printf("Enter f0: \n");
		fflush(stdin);
		scanf("%d", &f0);
		printf("Enter f1: \n");
		fflush(stdin);
		scanf("%d", &f1); */

		f0 = 1300;
		f1 = 2100;

		/* демодуляция данных WAV файла */
		printf("Sync and Demode sample data...\n");
		double time = clock();
		int packet_count = receiver(num_samples, dump, sample_rate, f0, f1, fout);
		printf("Time: %f second\n", (clock() - time) / CLOCKS_PER_SEC);
		free(dump);

		/* проверка результата путем сравнения 
		с эталонным выходным файлом */
		printf("Check file? Y/N\n");
		char c = 'n';
		c = _getch();
		if (c == 'Y' || c == 'y') {
			printf("Enter reference filename:\n");
			char *check_filename = (char*)malloc(sizeof(char) * 1024);
			fflush(stdin);
			gets(check_filename);
			FILE *fcheck = fopen(check_filename, "rb");
			if (!fcheck) {
				printf("Error!\n");
				exit(1);
			}
			comparison_files(fout, packet_count, fcheck);
			fclose(fcheck);
			free(check_filename);
		}
	}

	printf("Closing files..\n");
	fclose(fin);
	fclose(fout);

	free(in_filename);
	free(out_filename);

	_getch();
	return 0;
}