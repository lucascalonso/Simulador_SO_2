#include "../include/Processo.h"
#include "../include/GerenciadorMemoria.h"
#include "../include/globals.h"
#include <iostream>

Processo::Processo(int id, int duracaoCpu1, int duracaoIo, int duracaoCpu2, int ram)
    : id(id), duracaoCpu1(duracaoCpu1), duracaoIo(duracaoIo), duracaoCpu2(duracaoCpu2), ram(ram), estado(Estado::PRONTO), fezIo(false)
{   
    tempoChegada = tempoAtual;

    //Se Processo é CPU bound, seta Io como feito e soma duracaoCpu1 duracaoCpu2
    if(duracaoIo <= 0){
        tempoRestanteCpu = duracaoCpu1+duracaoCpu2; 
        fezIo = true;

    } else tempoRestanteCpu = duracaoCpu1;
        
}

std::string Processo::getEstadoString() const{
    switch(estado){
        case PRONTO: return "PRONTO";
        case EXECUTANDO: return "EXECUTANDO";
        case BLOQUEADO: return "BLOQUEADO";
        case TERMINADO: return "TERMINADO";
        case PRONTO_SUSPENSO: return "PRONTO_SUSPENSO";
        case BLOQUEADO_SUSPENSO: return "BLOQUEADO_SUSPENSO";
        default: return "DESCONHECIDO";
    }
}

bool Processo::checarTerminoDaFase(){
    if (getTempoRestanteCpu() <= 0){
        return true;
    }
    return false;
}

void Processo::executarCpu() {
    
    tempoRestanteCpu--;
    //Se terminou uma fase de CPU, altera estado p/ ir pra fila correta
    if (checarTerminoDaFase()) {
        if(fezIo){
            alterarEstado(Estado::TERMINADO);
        }
        else{
            alterarEstado(Estado::BLOQUEADO);
        }
    } else if (estado == Estado::PRONTO) alterarEstado(Estado::EXECUTANDO);
}

void Processo::setTempoChegada(int tempo){
    tempoChegada = tempoAtual;
};

int Processo::getTempoChegada() { return tempoChegada; }
int Processo::getTempoRestanteCpu() { return tempoRestanteCpu; }
int Processo::getRam() const {return ram;}
int Processo::getId() const { return id; }
int Processo::getDuracaoCpu1() const { return duracaoCpu1; }
int Processo::getDuracaoCpu2() const { return duracaoCpu2; }
int Processo::getDuracaoIo()  { return duracaoIo; }

void Processo::alterarEstado(Estado novoEstado) {
    estado = novoEstado;
}

void Processo::setTempoRestanteCpu(int tempo) {
    tempoRestanteCpu = tempo;
}

void Processo::decrementaTempoRestanteIo(){
    duracaoIo--;
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

bool Processo::getFezIo(){ return fezIo; }

void Processo::setFezIo(){
    fezIo = true;
}

bool Processo::checarTerminoDaFase(){
    if (getTempoRestanteCpu() <= 0){
        return true;
    }
    return false;
}
