#ifndef DESPACHANTE_H
#define DESPACHANTE_H

#include "Processo.h"
#include <queue>
#include <vector>
#include <map>
#include "GerenciadorMemoria.h"

class GerenciadorMemoria;

class Despachante {
private:
    int cpusNums;
    std::map<int,Processo*> processosPorId;
    std::vector<bool> cpusDisponiveis;
    std::queue<Processo*> filaProntos;
    std::queue<Processo*> filaBloqueados;
    GerenciadorMemoria* gerenciadorMemoria;
    int quantum;

public:
    Despachante() : cpusDisponiveis() {}
    std::queue<Processo*>& getFilaProntos(); 
    std::queue<Processo*>& getFilaBloqueados();
    void imprimirFila(const std::queue<Processo*>& fila, const std::string& nomeFila);
    explicit Despachante(int quantum,int cpusDisponiveis);
    void setGerenciadorMemoria(GerenciadorMemoria* gm);
    void adicionarPronto(Processo* processo);
    void adicionarBloqueado(Processo* processo);
    Processo* recuperarProcessoPorId(int processoId);
    void desbloquear();
    void escalonar(int delay);
    void liberarCPU(int cpuIndex);
    void imprimirStatus();
    void simularPassagemDeTempo();
    std::vector<Processo*>& getProcessos();
};
#endif