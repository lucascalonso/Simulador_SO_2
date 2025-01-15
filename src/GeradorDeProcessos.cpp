#include "../include/GeradorDeProcessos.h"
#include "../include/globals.h"
#include "../include/Despachante.h"
#include <vector>
#include <iostream> // Adicionar para debug

GeradorDeProcessos::GeradorDeProcessos(double lambda, int seed)
    : lambda(lambda), ultimoId(0), generator(seed), distDuracao(1, 10), distRam(128, 8192) {}

Processo* GeradorDeProcessos::gerarProcesso() {
    int duracaoCpu1 = distDuracao(generator);
    int duracaoIo = distDuracao(generator);
    int duracaoCpu2 = distDuracao(generator);
    int ram = distRam(generator);
    Processo *novoProcesso = new Processo(++ultimoId, duracaoCpu1, duracaoIo, duracaoCpu2, ram);
    return novoProcesso;
}

std::vector<Processo*> GeradorDeProcessos::gerarProcessos() {
    std::vector<Processo*> novosProcessos;

    //Distribuição de Poisson para o número de chegadas no intervalo
    std::poisson_distribution<int> poissonDistrib(lambda);

    //Número de processos que chegam neste intervalo
    int numeroDeProcessos = poissonDistrib(generator);

    for (int i = 0; i < numeroDeProcessos; ++i) {
        Processo* novoProcesso = gerarProcesso();
        novosProcessos.push_back(novoProcesso);
    }

    return novosProcessos;
}
