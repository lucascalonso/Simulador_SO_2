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
    processo->alterarEstado(Estado::EM_ESPERA);
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
void Despachante::escalonar(){

    //Passou uma unidade de tempo
    tempoAtual++;

    Processo* processoAtual;
    
    //Set de processos alocados para cada quantum (evitando mesmo Processo ser executado em CPUs diferentes durante quantum)
    std::unordered_set<Processo*> processosAlocadosNoQuantum;

    std::cout << "---------------------------------------------" << tempoAtual << " u.t decorridos------------------------------------------------------------\n\n";

    //Tenta desbloquear processos primeiro na fila de bloqueados
    desbloquear();
    

    //Para cada CPU, executa processo nele OU busca na fila o processo para executar
    for (int i = 0; i < numCpus ; i++) {
        
        processoAtual = cpusDisponiveis[i].P;
        
        //Se tem processo no CPU, ele certamente ainda não executou por 1 quantum inteiro, ou seria nullptr (linha 75)
        if(processoAtual){
            processoAtual->executarCpu();
            cpusDisponiveis[i].tempo_executando_processo++;
            processosAlocadosNoQuantum.insert(processoAtual);
            std::cout << "Processo #" << processoAtual->getId() <<  " executado na CPU #" << i+1 << " Count: "<< cpusDisponiveis[i].tempo_executando_processo << "\n";
            
            //Checa se o processo já executou por 1 quantum e não terminou
            if(cpusDisponiveis[i].tempo_executando_processo == quantum  && !processoAtual->checarTerminoDaFase()){
                filaProntos.push(processoAtual);
                cpusDisponiveis[i].P = nullptr;
                cpusDisponiveis[i].tempo_executando_processo = 0;
            }
        }

        //Caso não tenha processo na CPU, precisa buscar em uma das filas
        else{
            
            //Se tem processo na fila auxiliar
            if(!filaAuxiliar.empty()){
                std::cout << "CPU#" << i+1 << " Buscando Processo na fila Auxiliar " << std::endl;
                
                int contaFila = 0;
                processoAtual = filaAuxiliar.front();
                size_t tamanhoInicial = filaProntos.size();
                filaAuxiliar.pop();
                
                //Se o processo já está no set, já está alocado nesse quantum.
                while ((processosAlocadosNoQuantum.find(processoAtual) != processosAlocadosNoQuantum.end()) && contaFila < tamanhoInicial) {
                    filaAuxiliar.push(processoAtual);
                    processoAtual = filaAuxiliar.front();
                    filaAuxiliar.pop();
                    contaFila++;
                    if(contaFila >= tamanhoInicial) break;
                    //Busca até encontrar processo que não esteja alocado ou percorra
                }
                
                //Se encontrou processo
                if(contaFila <= tamanhoInicial){
                    cpusDisponiveis[i].P = processoAtual;
                    processosAlocadosNoQuantum.insert(processoAtual);
                    processoAtual->executarCpu();
                    cpusDisponiveis[i].tempo_executando_processo = 1;
                    std::cout << "Processo #" << processoAtual->getId() <<  " executado na CPU #" << i+1 << " Count: "<< cpusDisponiveis[i].tempo_executando_processo << "\n";
                    continue;
    
                } else {
                    std::cout << "Todos os processos possíveis da fila Auxiliar já foram alocados nesse quantum." << std::endl;
                    processoAtual = nullptr;
                }    
            }

            if(!filaProntos.empty()){
                std::cout << "CPU#" << i+1 << " Buscando Processo na fila Prontos " << std::endl;
                
                int contaFila = 0;
                processoAtual = filaProntos.front();
                size_t tamanhoInicial = filaProntos.size();
                filaProntos.pop();

                //Se o processo já está no set, já está alocado nesse quantum.
                while ((processosAlocadosNoQuantum.find(processoAtual) != processosAlocadosNoQuantum.end()) && contaFila < tamanhoInicial) {
                    filaProntos.push(processoAtual);
                    processoAtual = filaProntos.front();
                    filaProntos.pop();
                    contaFila++;
                    if(contaFila>=tamanhoInicial) break;
                    //Busca até encontrar processo que não esteja alocado ou percorra
                }
                //Se encontrou processo
                if(contaFila <= tamanhoInicial){
                    cpusDisponiveis[i].P = processoAtual;
                    processosAlocadosNoQuantum.insert(processoAtual);
                    processoAtual->executarCpu();
                    cpusDisponiveis[i].tempo_executando_processo = 1;
                    std::cout << "Processo #" << processoAtual->getId() <<  " executado na CPU #" << i+1 << " Count: "<< cpusDisponiveis[i].tempo_executando_processo << "\n";
                    continue;
        
                } else {
                    std::cout << "Todos os processos possíveis da fila Prontos já foram alocados nesse quantum." << std::endl;
                    continue;
                }
            
            }  
            else if(filaAuxiliar.empty() && filaProntos.empty()){
                std::cout << "Nenhum processo para alocar ao CPU# " << i+1 << ". Ambas as filas auxiliar e prontos estão vazias. Aguardando..."<< std::endl;
                processoAtual = nullptr;
                continue; 
            }
        }
        // Checa se processo terminou
        if (processoAtual->checarTerminoDaFase()) {
            
            //Reseta informações no CPU
            cpusDisponiveis[i].P = nullptr;
            cpusDisponiveis[i].tempo_executando_processo = 0;
            
            //Se o processo já passou por fase de I/O
            if(processoAtual->getFezIo()){    
                
                processoAtual->tempoTermino = tempoAtual;
                processoAtual->turnAroundTime = processoAtual->tempoTermino - processoAtual->tempoChegada;

                gerenciadorMemoria->liberaMemoria(processoAtual);
                std::cout << "Processo #" << processoAtual->getId() 
                          << " finalizado e " <<processoAtual->getRam() << " MB liberados. Turnaround Time: "<< processoAtual->turnAroundTime 
                          << "\n";
            
            //Caso não tenha feito I/O, tempoRestante = duracaoCpu2, adiciona na fila de bloqueados                   
            } else{
                std::cout << "Processo #" << processoAtual->getId() 
                          << " foi para I/O "  
                          << "\n";

                processoAtual->setTempoRestanteCpu(processoAtual->getDuracaoIo());
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
        
        Processo* processoAtual = filaBloqueados.front();
        filaBloqueados.pop();

        if (!processoAtual) {
            std::cerr << "Erro: Processo inválido encontrado na fila de bloqueados.\n";
            continue;
        }

        // Tentando alocar memória para o processo bloqueado
        if(processoAtual->getEstadoString() == "EM ESPERA"){
            
            //Se tem memória disponível para alocar
            if (gerenciadorMemoria->alocarMemoria(processoAtual->getId(), processoAtual->getRam())) {
                processoAtual->alterarEstado(Estado::PRONTO);
                filaProntos.push(processoAtual);
                std::cout << "Processo #" << processoAtual->getId() << " desbloqueado e movido para fila de prontos.\n";
            
            // Caso não consiga alocar, push de volta para filaBloqueados
            } else {
                std::cout << "Memória insuficiente para desbloquear Processo #" << processoAtual->getId()
                          << ". Mantido na fila de bloqueados.\n";
                filaBloqueados.push(processoAtual);
            }
        }
        else if(processoAtual->getEstadoString() == "BLOQUEADO"){
            
            //Decrementa 1 de duracaoIo
            processoAtual->decrementaTempoRestanteIo();

            //Se passou por todo o tempo de I/O, push pra filaAuxiliar
            if(processoAtual->getDuracaoIo() == 0){
                processoAtual->alterarEstado(Estado::PRONTO);
                filaAuxiliar.push(processoAtual);
                processoAtual->setFezIo();
                std::cout << "Processo #" << processoAtual->getId() << " terminou I/O. Movido para fila Auxiliar.\n";

            //Caso ainda não tenha terminado I/O
            } else {
                filaBloqueados.push(processoAtual);
            }
        }
    }
}



