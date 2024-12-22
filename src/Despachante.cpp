#include "../include/Despachante.h"
#include "../include/Processo.h"
#include "../include/GerenciadorMemoria.h"
#include "../include/globals.h"
#include <iostream>
#include <unordered_set>


Despachante::Despachante(int quantum,int numCpus) : quantum(quantum), numCpus(numCpus){}


void Despachante::adicionarPronto(Processo* processo) {
    std::cout << "Adicionando Processo# " << processo->getId() << " na fila " << std::endl;
    if (gerenciadorMemoria->alocarMemoria(processo->getId(), processo->getRam())) {
        filaProntos.push(processo);
    } else {
        adicionarBloqueado(processo);
    }
}

void Despachante::adicionarBloqueado(Processo* processo) {
    filaBloqueados.push(processo);
    processo->alterarEstado(Estado::BLOQUEADO);
    std::cout << "Processo #" << processo->getId() << " movido para a fila de bloqueados.\n";
}

void Despachante::imprimirFila(const std::queue<Processo*>& fila, const std::string& nomeFila) {
    std::cout << nomeFila << ":\n";
    if (fila.empty()) return;

    //Copia a fila para iterar
    std::queue<Processo*> copiaFila = fila; 
    while (!copiaFila.empty()) {
        Processo* processo = copiaFila.front();
        copiaFila.pop();
        if (processo) {
            std::cout << processo->getId() << " ";
        } else {
            std::cerr << "Erro: Processo nulo encontrado na fila.\n";
        }
    }
    std::cout << std::endl;
}


//Método principal do Simulador
void Despachante::escalonar() {

    //Passou uma unidade de tempo
    tempoAtual++;

    Processo* processoAtual;
    
    //Set de processos alocados para cada quantum (evitando mesmo Processo ser executado em CPUs diferentes durante quantum)
    std::unordered_set<Processo*> processosAlocados;

    std::cout << "---------------------------------------------" << tempoAtual << " u.t decorridos------------------------------------------------------------\n\n";

    //Tenta desbloquear processos primeiro na fila de bloqueados
    desbloquear();
    

    //Para cada CPU, executa processo nele OU busca na fila o processo para executar
    for (int i = 0; i < numCpus ; i++) {
        
        processoAtual = cpusDisponiveis[i].P;
        
        //Se tem processo no CPU executando por menos de 3 u.t
        if(processoAtual && cpusDisponiveis[i].count < quantum){
            processoAtual->executarCpu();
            processosAlocados.insert(processoAtual);
            cpusDisponiveis[i].count++;
        }

        //Caso não tenha processo na CPU OU já tenha executado por 4 u.t
        else if(!processoAtual || cpusDisponiveis[i].count > 3){
            std::cout << "Buscando Processo nas filas..." << std::endl;
            if(cpusDisponiveis[i].count > 3  && processosAlocados.find(processoAtual) != processosAlocados.end()){
                filaProntos.push(processoAtual);
                cpusDisponiveis[i].P = nullptr;
                cpusDisponiveis[i].count = 0;
            }
        
            if(!filaAuxiliar.empty()){
    
                processoAtual = filaAuxiliar.front();
                filaAuxiliar.pop();
                //Se o processo já está no set, já está alocado nesse quantum
                if (processosAlocados.find(processoAtual) != processosAlocados.end()) {
                    filaProntos.push(processoAtual);
                    continue;
                }
                cpusDisponiveis[i].P = processoAtual;
                processoAtual->executarCpu();
                processosAlocados.insert(processoAtual);
            }

            else if(!filaProntos.empty()){
                
                processoAtual = filaProntos.front();
                filaProntos.pop();
                //Se o processo já está no set, já está alocado nesse quantum
                if (processosAlocados.find(processoAtual) != processosAlocados.end()) {
                    filaProntos.push(processoAtual);
                    continue;
                }
                cpusDisponiveis[i].P = processoAtual;
                processoAtual->executarCpu();
                processosAlocados.insert(processoAtual);
            }

            else{
                std::cout << "Nenhum processo para alocar ao CPU# " << i+1 << ". Aguardando..."<< std::endl;
                    continue;
            } 
        }
        std::cout << "Processo #" << processoAtual->getId() 
                          <<  " executado na CPU #" << i+1
                         << "\n";

        // Checa se processo terminou
        if (processoAtual->checarTermino()) {
            
            //Reseta informações no CPU
            cpusDisponiveis[i].P = nullptr;
            cpusDisponiveis[i].count = 0;
            
            //Se o processo já passou por fase de I/O
            if(processoAtual->getFezIo()){    
                
                processoAtual->tempoTermino = tempoAtual;
                processoAtual->turnAroundTime = processoAtual->tempoTermino - processoAtual->tempoChegada;

                gerenciadorMemoria->liberaMemoria(processoAtual);
                std::cout << "Processo #" << processoAtual->getId() 
                          << " finalizado e memória liberada. Turnaround Time: "<< processoAtual->turnAroundTime 
                          << "\n";     
            }
            //Caso não tenha feito I/O, tempoRestante = duracaoCpu2, adiciona na fila de bloqueados
            else{
                processoAtual->setTempoRestanteCpu();
                filaBloqueados.push(processoAtual);
                processoAtual->setFezIo();
            }
        }
    }
    
    //Visualização
    imprimirFila(filaProntos, "Fila de Prontos");
    imprimirFila(filaBloqueados, "Fila de Bloqueados");
    imprimirFila(filaAuxiliar, "Fila de Prontos Auxiliar");
    gerenciadorMemoria->visualizarMemoria();
}

void Despachante::setGerenciadorMemoria(GerenciadorMemoria* gm) {
    gerenciadorMemoria = gm;
}

//Politica de Reordenamento Dinâmico
void Despachante::desbloquear() {
    if (filaBloqueados.empty()) return;

    size_t bloqueadosCount = filaBloqueados.size();

    for (size_t i = 0; i < bloqueadosCount; ++i) {
        
        Processo* bloqueado = filaBloqueados.front();
        filaBloqueados.pop();

        if (!bloqueado) {
            std::cerr << "Erro: Processo inválido encontrado na fila de bloqueados.\n";
            continue;
        }

        // Tentando alocar memória para o processo bloqueado
        if(bloqueado->getEstadoString() == "BLOQUEADO"){
            
            //Se tem memória disponível para alocar
            if (gerenciadorMemoria->alocarMemoria(bloqueado->getId(), bloqueado->getRam())) {
                bloqueado->alterarEstado(Estado::PRONTO);
                filaProntos.push(bloqueado);
                std::cout << "Processo #" << bloqueado->getId() << " desbloqueado e movido para fila de prontos.\n";
            
            // Caso não consiga alocar, push de volta para filaBloqueados
            } else {
                std::cout << "Memória insuficiente para desbloquear Processo #" << bloqueado->getId()
                          << ". Mantido na fila de bloqueados.\n";
                filaBloqueados.push(bloqueado);
            }
        }
        else if(bloqueado->getEstadoString() == "EM ESPERA"){
            
            //Decrementa 1 de duracaoIo
            bloqueado->decrementaTempoRestanteIo();

            //Se passou por todo o tempo de I/O, push pra filaAuxiliar
            if(bloqueado->getDuracaoIo() == 0){
                bloqueado->alterarEstado(Estado::PRONTO);
                filaAuxiliar.push(bloqueado);
                bloqueado->setFezIo();
                std::cout << "Processo #" << bloqueado->getId() << " terminou I/O. Movido para fila de prontos.\n";

            //Caso ainda não tenha terminado I/O
            } else {
                filaBloqueados.push(bloqueado);
            }
        }
    }
}



