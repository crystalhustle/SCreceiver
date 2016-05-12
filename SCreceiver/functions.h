/********************************************************************************
Проект:     Программная реализация приемного УПС
Файл:       functons.h
Автор:      Куликов А.А.
Содержание: Заголовки функций из файла receiver.c
История:    28.02.2016 - базовая версия
*********************************************************************************/

long wave_header(struct HEADER *header, FILE *ptr);
void data_read(unsigned int size_of_each_sample, int *dump, unsigned int num_samples, FILE *ptr, struct HEADER header);
int receiver(unsigned int num_samples, int *dump, unsigned int sample_rate, unsigned int f0, unsigned int f1, unsigned int speed, FILE *ptr);
void comparison_files(FILE *ptr1, int packet_count, FILE *ptr2);