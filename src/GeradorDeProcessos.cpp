#include "../include/GeradorDeProcessos.h"
#include "../include/globals.h"

GeradorDeProcessos::GeradorDeProcessos(double lambda, int seed)
    : lambda(lambda), ultimoId(0), generator(seed), expDistrib(lambda),
      distDuracao(1, 10), distRam(128, 8192) {}

      //lambda é a taxa de chegada de processos 0.5 = 1 processo a cada 2 u.t
      //tempo de chegada em um processo Poisson
      //distDuracao para gerar duracaoCpu1, duracaoCpu2 e duracaoIo entre 1 e 10
      //distRam para gerar memória necessária entre 128 e 8192

Processo* GeradorDeProcessos::gerarProximoProcesso() {
    int duracaoCpu1 = distDuracao(generator);
    int duracaoIo = distDuracao(generator);
    int duracaoCpu2 = distDuracao(generator);
    int ram = distRam(generator);

    return new Processo(++ultimoId, duracaoCpu1, duracaoIo, duracaoCpu2, ram);
}

int GeradorDeProcessos::tempoParaProximo() {
    return static_cast<int>(expDistrib(generator));
}