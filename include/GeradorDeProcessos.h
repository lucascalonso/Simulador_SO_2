#ifndef GERADOR_DE_PROCESSOS_H
#define GERADOR_DE_PROCESSOS_H

#include "Processo.h"
#include <random>

class GeradorDeProcessos {
private:
    double lambda;
    int ultimoId;
    std::default_random_engine generator;
    std::exponential_distribution<double> expDistrib;
    std::uniform_int_distribution<int> distDuracao;
    std::uniform_int_distribution<int> distRam;

public:
    GeradorDeProcessos(double lambda, int seed = std::time(nullptr));

    std::vector<Processo*>gerarProcessos();
    Processo* gerarProximoProcesso();
};

#endif