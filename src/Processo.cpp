#include "../include/Processo.h"
#include "../include/GerenciadorMemoria.h"
#include <iostream>

Processo::Processo(int id, int duracaoCpu1, int duracaoIo, int duracaoCpu2, int ram)
    : id(id), duracaoCpu1(duracaoCpu1), duracaoIo(duracaoIo), duracaoCpu2(duracaoCpu2), ram(ram), estado(Estado::PRONTO)
{   
    tempoRestante = duracaoCpu1 + duracaoIo + duracaoCpu2;
}


std::string Processo::getEstadoString() const{
    switch(estado){
        case PRONTO: return "PRONTO";
        case EXECUTANDO: return "EXECUTANDO";
        case BLOQUEADO: return "BLOQUEADO";
        case EM_ESPERA: return "EM ESPERA";
        case TERMINADO: return "TERMINADO";
        default: return "DESCONHECIDO";
    }
}

void Processo::executarCpu(int tempo) {
    
    tempoRestante -= tempo;
    if (tempoRestante <= 0) {
        alterarEstado(Estado::TERMINADO);
    } else if (estado == Estado::PRONTO) {
        alterarEstado(Estado::EXECUTANDO);
    }
}


void Processo::executarIo(int tempo) {
    if (estado == Estado::BLOQUEADO) {
        duracaoIo = std::max(0, duracaoIo - tempo);
        std::cout << "Processo #" << id << " executando I/O por " << tempo << " unidades. Restante: " << duracaoIo << "\n";
    }
}

int Processo::getRam() const {return ram;}
int Processo::getId() const { return id; }
int Processo::getDuracaoCpu1() const { return duracaoCpu1; }
int Processo::getDuracaoCpu2() const { return duracaoCpu2; }
int Processo::getDuracaoIo() const { return duracaoIo; }

void Processo::alterarEstado(Estado novoEstado) {
    estado = novoEstado;
}

void Processo::atualizarTempoCpu1(int tempo) {
    duracaoCpu1 = std::max(0, duracaoCpu1 - tempo);
}

void Processo::atualizarTempoCpu2(int tempo) {
    duracaoCpu2 = std::max(0, duracaoCpu2 - tempo);
}

void Processo::atualizarTempoIo(int tempo) {
    duracaoIo = std::max(0, duracaoIo - tempo);
}

bool Processo::checarTermino(){
    if (getTempoRestante() <= 0){
        return true;
    }
    return false;
}
