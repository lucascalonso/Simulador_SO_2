#ifndef DESPACHANTE_H
#define DESPACHANTE_H

#include "Processo.h"
#include <queue>
#include <vector>
#include "GerenciadorMemoria.h"
#include "globals.h"

typedef struct CPU{
    Processo *P = nullptr;
    int tempo_executando_processo = 0;
}CPU;

extern int tempoAtual;

class GerenciadorMemoria;

class Despachante {
private:
    CPU cpusDisponiveis[4];
    std::queue<Processo*> filaProntos;
    std::queue<Processo*> filaBloqueados;
    std::queue<Processo*> filaAuxiliar;
    GerenciadorMemoria* gerenciadorMemoria;
    int quantum;
    int numCpus;

public:

    Despachante() : cpusDisponiveis() {}
    void imprimirFila(const std::queue<Processo*>& fila, const std::string& nomeFila);
    explicit Despachante(int quantum,int cpusDisponiveis);
    void setGerenciadorMemoria(GerenciadorMemoria* gm);
    void adicionarPronto(Processo* processo);
    void adicionarBloqueado(Processo* processo);
    Processo* recuperarProcessoPorId(int processoId);
    void desbloquear();
    void escalonar();
};
#endif