#ifndef DESPACHANTE_H
#define DESPACHANTE_H
#include "../include/Processo.h"
#include <queue>
#include <set>
#include "globals.h"
#include <unordered_set>


typedef struct CPU{
    Processo *P = nullptr;
    Processo* ultimoProcesso = nullptr;
    int tempo_executando_processo = 0;
}CPU;

typedef struct ProcessoComparator {
    bool operator()(Processo* p1, Processo* p2) const {
        return p1->getId() < p2->getId();
    }
}ProcessoComparator;

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
    std::set<Processo*,ProcessoComparator> processosAtuais;
    std::unordered_set<Processo*> processosAlocadosNoQuantum;
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
    bool desalocarBloqueadosParaProntosSuspensos(int memoriaNecessaria);
    void adicionarProcessoNaFilaProntos(Processo* processo);
    void adicionarProcessoNaFilaProntosSuspensos(Processo* processo);
    void escalonar();
    int desalocarAteNecessario(int memoriaNecessaria, std::vector<Processo*>& processosParaDesalocar);
    void realocarProntosSuspensos();
    std::set<Processo*,ProcessoComparator> getProcessosAtuais();
    int getNumCpus(){return numCpus;};
    CPU* getCpusDisponiveis(){return cpusDisponiveis;};
    std::unordered_set<Processo*> getProcessosAlocadosNoQuantum();

};
#endif