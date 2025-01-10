#ifndef PROCESSO_H
#define PROCESSO_H
#include <string>
#include "globals.h"


enum Estado{
    PRONTO,
    EXECUTANDO,
    BLOQUEADO,
    PRONTO_SUSPENSO,
    BLOQUEADO_SUSPENSO,
    TERMINADO,
    PRONTO_FIM_QUANTUM
};

class Processo {
private:
    int id;
    int ram;
    int duracaoCpu1;
    int duracaoCpu2;
    int duracaoIo;
    int duracaoIoTotal;
    int tempoRestanteCpu;
    bool fezIo;
    Estado estado;

public:
    Processo(int id, int duracaoCpu1, int duracaoIo, int duracaoCpu2, int ram);

    int tempoChegada;
    int tempoTermino;
    int turnAroundTime;
    int getId() const;
    int getRam() const;
    bool getFezIo();
    int getDuracaoCpu1() const;
    int getDuracaoCpu2() const;
    int getDuracaoIo();
    int getDuracaoIoTotal();
    int getTempoRestanteCpu();
    void alterarEstado(Estado novoEstado);
    void atualizarTempoCpu1(int tempo);
    void atualizarTempoCpu2(int tempo);
    void atualizarTempoIo(int tempo);
    std::string getEstadoString() const;
    std::string getCor(int idProcesso);
    void executarCpu();
    bool checarTerminoDaFase();
    void setTempoRestanteCpu(int tempo);
    void decrementaTempoRestanteIo();
    void setTempoChegada(int tempo);
    int getTempoChegada();
    void setFezIo();
};
#endif