#ifndef NEURAL_NET_H 
#define NEURAL_NET_H 
#include <Arduino.h> 


#define NN_MESSAGE_SIZE 56
#define NN_PARAMETER NN_MESSAGE_SIZE * 6

PROGMEM const float percieve_kernel_back[NN_MESSAGE_SIZE][NN_PARAMETER] = {};
PROGMEM const float percieve_kernel_front[NN_MESSAGE_SIZE][NN_PARAMETER]= {};
PROGMEM const float percieve_kernel_north[NN_MESSAGE_SIZE][NN_PARAMETER]= {};
PROGMEM const float percieve_kernel_south[NN_MESSAGE_SIZE][NN_PARAMETER]= {};
PROGMEM const float percieve_kernel_east[NN_MESSAGE_SIZE][NN_PARAMETER]= {};
PROGMEM const float percieve_kernel_west[NN_MESSAGE_SIZE][NN_PARAMETER]= {};
PROGMEM const float percieve_kernel_self[NN_MESSAGE_SIZE][NN_PARAMETER]= {};
PROGMEM const float percieve_bias[NN_PARAMETER]= {};
PROGMEM const float dmodel_bias1[NN_PARAMETER]= {};
PROGMEM const float dmodel_bias2[NN_MESSAGE_SIZE-1]= {};
PROGMEM const float dmodel_kernel_1[NN_PARAMETER][NN_PARAMETER]= {};
PROGMEM const float dmodel_kernel_2[NN_PARAMETER][NN_MESSAGE_SIZE-1]= {};

#endif
