#ifndef PROCESSO_H
#define PROCESSO_H
#include <string>


enum Estado{
    PRONTO,
    EXECUTANDO,
    EM_ESPERA,
    BLOQUEADO,
    TERMINADO
};

class Processo {
private:
    int id;
    int ram;
    int prioridade;
    int duracaoCpu1;
    int duracaoCpu2;
    int duracaoIo;
    int tempoRestante;
    int tempoRestanteIO;
    Estado estado;

public:
    Processo(int id, int duracaoCpu1, int duracaoIo, int duracaoCpu2, int ram);

    int getId() const;
    int getRam() const;
    int getDuracaoCpu1() const;
    int getDuracaoCpu2() const;
    int getDuracaoIo() const;
    int getTempoRestante() const {return tempoRestante;};
    void reduzirTempo(int quantum) {tempoRestante -= quantum;};
    void alterarEstado(Estado novoEstado);
    void atualizarTempoCpu1(int tempo);
    void atualizarTempoCpu2(int tempo);
    void atualizarTempoIo(int tempo);
    std::string getEstadoString() const;
    void exibirInformacoes() const;
    void executarCpu(int tempo);
    void executarIo(int tempo);
};
#endif