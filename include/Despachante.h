#ifndef DESPACHANTE_H
#define DESPACHANTE_H

#include "Processo.h"
#include <queue>
#include <vector>
#include "GerenciadorMemoria.h"
#include "globals.h"
#include <unordered_set>

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
    std::queue<Processo*> filaBloqueadosSuspensos;
    std::queue<Processo*> filaProntosSuspensos;
    std::queue<Processo*> filaAuxiliar;
    GerenciadorMemoria* gerenciadorMemoria;
    int quantum;
    int numCpus;

public:

    Despachante() : cpusDisponiveis() {}
    void imprimirFila(const std::queue<Processo*>& fila, const std::string& nomeFila);
    explicit Despachante(int quantum,int cpusDisponiveis);
    void setGerenciadorMemoria(GerenciadorMemoria* gm);
    void tentaAlocarProcesso(Processo* processo);
    void adicionarBloqueado(Processo* processo);
    Processo* recuperarProcessoPorId(int processoId);
    void tentarAlocarProcessosSuspensos();
    void decrementaBloqueadosSuspensos(std::unordered_set<Processo*>& processosAlocadosNoQuantum);
    void decrementaBloqueados(std::unordered_set<Processo*>& processosAlocadosNoQuantum);
    void desbloquearProntosSuspensos();
    void verificaRealocacao();
    bool desalocarBloqueadosParaProntosSuspensos(int memoriaNecessaria);
    void escalonar();
    int desalocarAteNecessario(int memoriaNecessaria, std::vector<Processo*>& processosParaDesalocar);
    void realocarProntosSuspensos(); 

};
#endif