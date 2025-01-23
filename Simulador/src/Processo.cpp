#include "../include/Processo.h"
#include "../include/GerenciadorMemoria.h"
#include "../include/globals.h"
#include <iostream>
#include <thread>
#include <random>

Processo::Processo(int id, int duracaoCpu1, int duracaoIo, int duracaoCpu2, int ram)
    : id(id), duracaoCpu1(duracaoCpu1), duracaoIo(duracaoIo), duracaoCpu2(duracaoCpu2), ram(ram), estado(Estado::PRONTO), fezIo(false),duracaoIoTotal(duracaoIo),
    cor(gerarCorUnica(id,duracaoCpu1,duracaoIo,duracaoCpu2,ram))
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

//Chamado para cada processo executado em uma das CPUs
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
    } else if (estado == Estado::PRONTO || estado == Estado::PRONTO_SUSPENSO) alterarEstado(Estado::EXECUTANDO);
}

int Processo::getTempoChegada() { return tempoChegada; }
int Processo::getTempoRestanteCpu() { return tempoRestanteCpu; }
int Processo::getRam() const {return ram;}
int Processo::getId() const { return id; }
int Processo::getDuracaoCpu1() const { return duracaoCpu1; }
int Processo::getDuracaoCpu2() const { return duracaoCpu2; }
int Processo::getDuracaoIo()  { return duracaoIo; }
int Processo::getDuracaoIoTotal()  { return duracaoIoTotal; }

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

wxColor Processo::getCor() const { return cor;}

void Processo::atualizarTempoIo(int tempo) {
    duracaoIo = std::max(0, duracaoIo - tempo);
}

bool Processo::getFezIo(){ return fezIo; }

void Processo::setFezIo(){
    fezIo = true;
}

std::queue<Processo*> getFila(std::queue<Processo*> fila) {return fila;}

//Método gera cores randômicamente, facilitando visualização nao frame da GUI
wxColour Processo::gerarCorUnica(int id, int duracaoCpu1, int duracaoIo, int duracaoCpu2, int ram) {
    std::mt19937 rng(id + ram + duracaoCpu1);
    std::uniform_int_distribution<int> dist(0, 255);

    int r = (id * 50 + dist(rng)) % 256;
    int g = (duracaoCpu1 * 30 + dist(rng)) % 256;
    int b = (ram * 10 + dist(rng)) % 256;

    wxColour cor(r, g, b);
    if (!cor.IsOk()) {
        std::cerr << "Cor inválida gerada para Processo ID: " << id
                  << " R=" << r << ", G=" << g << ", B=" << b << std::endl;
        return wxColour(0, 0, 0);
    }
    return cor;
}