#ifndef CONGESTION_CONTROL_H
#define CONGESTION_CONTROL_H

double adjust_sending_rate(double lambda, double epsilon, double gamma, double beta, unsigned short bufferstate);
void send_feedback(int sockfd, struct addrinfo *server_info, int Q_t, int targetbuf, int buffersize);

#endif //CONGESTION_CONTROL_H
