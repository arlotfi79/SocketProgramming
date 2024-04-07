#ifndef CONGESTION_CONTROL_H
#define CONGESTION_CONTROL_H

#include <stdio.h>

double adjust_sending_rate(double lambda, double epsilon, double gamma, double beta, unsigned short bufferstate);
int prepare_feedback(int Q_t, int targetbuf, int buffersize);

long get_file_size(FILE *file);
int calculate_expected_number_of_packets(const char *filename, int blocksize);


#endif //CONGESTION_CONTROL_H
