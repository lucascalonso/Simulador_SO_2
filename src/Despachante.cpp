#include "../include/Despachante.h"
#include "../include/Processo.h"
#include "../include/GerenciadorMemoria.h"
#include "../include/GeradorDeProcessos.h"
#include "../include/globals.h"
#include <iostream>

Despachante::Despachante(int quantum,int numCpus) : quantum(quantum), numCpus(numCpus){}

//Chamado na chegada de um processo e ao tentar alocar processos suspensos
void Despachante::tentaAlocarProcesso(Processo* processo) {
    processosAtuais.insert(processo);

    //Caso tenha memória disponível para alocar, vai pra fila auxiliar ou pronto
    if (gerenciadorMemoria->alocarMemoria(processo->getId(), processo->getRam())) {
        if(processo->getEstadoString() == "PRONTO_SUSPENSO"){
            std::cout << "Processo#" << processo->getId() << " inserido na fila de auxiliar."<< std::endl;
            filaAuxiliar.push(processo);
            return;
        }
        
        std::cout << "Processo#" << processo->getId() << " inserido na fila de prontos" << std::endl;
        processo->alterarEstado(Estado::PRONTO);
        adicionarProcessoNaFilaProntos(processo);
        

    //Se não tem memória, vai ou é mantido em fila prontos suspensos
    } else {
        if(processo->getEstadoString() == "PRONTO_SUSPENSO"){
            std::cout << "Processo#" << processo->getId() << " mantido na fila de prontos suspensos. Memória insuficiente" << std::endl;
            filaProntosSuspensos.push(processo);
            return;
        }

        std::cout << "Processo#" << processo->getId() << " inserido na fila de prontos suspensos. Memória insuficiente" << std::endl;
        processo->alterarEstado(Estado::PRONTO_SUSPENSO);
        filaProntosSuspensos.push(processo);
        
    }
}

//Tenta alocar processos suspensos que ainda estão na fila
void Despachante::tentarAlocarProcessosSuspensos() {
    
    size_t tamanhoFila = filaProntosSuspensos.size();
    for (size_t i = 0; i < tamanhoFila; ++i) {
        Processo* processo = filaProntosSuspensos.front();
        filaProntosSuspensos.pop();  

        //Já insere de novo em filaProntosSuspensos caso não tenha memória (final do método tentaAlocarProcesso)
        tentaAlocarProcesso(processo);
    }
}


//Método principal do Simulador
void Despachante::escalonar(){


    //jaCheckouSuspensos para controlar se já tentou suspender bloqueados para alocar prontos suspensos
    //evita loop infinito 
    bool jaCheckouSuspensos = false;

    Processo* processoAtual;
    
    //Set de processos alocados nas CPUs
    //Evita que o mesmo Processo seja executado em CPUs diferentes durante u.t
    //Também controla quais processos terão seu tempo de I/O decrementado
    //Se um processo executou em uma u.t e foi bloqueado, não pode ter seu duracaoIo decrementado nesse ciclo
    processosAlocadosNoQuantum.clear();

    gerenciadorMemoria->deletarProcessos();

    std::cout << "-------------------------------------------------------------------" << tempoAtual << " u.t ------------------------------------------------------------------\n";
    
    tentarAlocarProcessosSuspensos();

    //Para cada CPU, executa processo nele ou busca nas filas caso !processoAtual
    for (int i = 0; i < numCpus ; i++) {
        
        cpusDisponiveis[i].ultimoProcesso = nullptr;
        processoAtual = cpusDisponiveis[i].P;
        
        //Se tem processo no CPU e não executou nessa u.t (pode ter sofrido timeout no CPU#1 e executar na CPU#2 caso não tenha o check do set)
        if(processoAtual && (processosAlocadosNoQuantum.find(processoAtual) == processosAlocadosNoQuantum.end())){
            processoAtual->executarCpu();
            cpusDisponiveis[i].tempo_executando_processo++;
            processosAlocadosNoQuantum.insert(processoAtual);
            
            //Checa se o processo já executou por 1 quantum e não terminou
            if(cpusDisponiveis[i].tempo_executando_processo == quantum  && !processoAtual->checarTerminoDaFase()){
                filaProntos.push(processoAtual);
                processoAtual->alterarEstado(Estado::PRONTO);
                cpusDisponiveis[i].ultimoProcesso = cpusDisponiveis[i].P;
                cpusDisponiveis[i].P = nullptr;
                cpusDisponiveis[i].tempo_executando_processo = 0;
            }
        }

        //Caso não tenha processo na CPU, precisa buscar em uma das filas
        else{
            
            //Prioriza buscar processo na fila auxiliar
            if(!filaAuxiliar.empty()){
                std::cout << "CPU#" << i+1 << " Buscando Processo na fila Auxiliar " << std::endl;
                
                //contador para controlar se checou todos os processos da fila
                int contaFila = 0;
                processoAtual = filaAuxiliar.front();
                size_t tamanhoInicial = filaProntos.size();
                filaAuxiliar.pop();
                
                //Se o processo está no set, já está alocado a uma CPU nesse quantum.
                while ((processosAlocadosNoQuantum.find(processoAtual) != processosAlocadosNoQuantum.end()) && contaFila < tamanhoInicial) {
                    filaAuxiliar.push(processoAtual);
                    processoAtual = filaAuxiliar.front();
                    filaAuxiliar.pop();
                    contaFila++;
                    if(contaFila >= tamanhoInicial) break;
                    //Busca até encontrar processo que não esteja alocado ou percorrar toda a fila
                }
                
                //Se encontrou processo ainda não presente no set, vai executa-lo 
                if(contaFila <= tamanhoInicial && (processosAlocadosNoQuantum.find(processoAtual) == processosAlocadosNoQuantum.end())){
                    cpusDisponiveis[i].P = processoAtual;
                    processosAlocadosNoQuantum.insert(processoAtual);
                    processoAtual->executarCpu();
                    cpusDisponiveis[i].tempo_executando_processo = 1;
                    goto checkaTermino;

                } else {
                    std::cout << "Todos os processos possíveis da fila auxiliar já foram executados nesse quantum." << std::endl;
                    filaAuxiliar.push(processoAtual);
                    processoAtual = nullptr;
                }    
            }

            //Como não achou processo possível na fila auxiliar, busca na fila de prontos
            if(!filaProntos.empty()){
                std::cout << "CPU#" << i+1 << " Buscando Processo na fila Prontos " << std::endl;
                
                tentaExecutarPronto:
                int contaFila = 0;
                processoAtual = filaProntos.front();
                size_t tamanhoInicial = filaProntos.size();
                filaProntos.pop();

                //Se o processo está no set, já está alocado a uma CPU nesse quantum.
                while ((processosAlocadosNoQuantum.find(processoAtual) != processosAlocadosNoQuantum.end()) && contaFila < tamanhoInicial) {
                    filaProntos.push(processoAtual);
                    processoAtual = filaProntos.front();
                    filaProntos.pop();
                    contaFila++;
                    if(contaFila>=tamanhoInicial) break;
                    //Busca até encontrar processo que não esteja alocado ou percorra toda a fila
                }
                //Se encontrou processo
                if(contaFila <= tamanhoInicial && (processosAlocadosNoQuantum.find(processoAtual) == processosAlocadosNoQuantum.end())){
                    cpusDisponiveis[i].P = processoAtual;
                    processosAlocadosNoQuantum.insert(processoAtual);
                    processoAtual->executarCpu();
                    cpusDisponiveis[i].tempo_executando_processo = 1;
                    goto checkaTermino;

                } else {
                    std::cout << "Todos os processos possíveis da fila Prontos já foram executados nesse quantum." << std::endl;
                    filaProntos.push(processoAtual);
                    processoAtual = nullptr;
                    
                    //jaCheckouSuspensos evita loop infinito. só tenta checar filas de processos suspensos uma vez
                    if(jaCheckouSuspensos) continue;
                    else goto checkFilasSuspensas;
                }
            }
            
            checkFilasSuspensas:
            // Tenta desalocar bloqueados para prontos suspensos poderem ir p/ prontos e escalonados
            if (!filaBloqueados.empty() && !filaProntosSuspensos.empty()) {
                std::cout << "CPU #" << i + 1 
                        << " buscando desalocar processos bloqueados para prontos suspensos.\n";

                Processo* processoProntoSuspenso = filaProntosSuspensos.front();
                int memoriaNecessaria = processoProntoSuspenso->getRam();
                size_t countProntosSuspensos = filaProntosSuspensos.size();
                int contaFila = 0;

                // Percorre a fila de prontos suspensos verificando condições
                while (contaFila < countProntosSuspensos) {
                    //Checa se o processo já foi processado no quantum atual
                    if (processosAlocadosNoQuantum.find(processoProntoSuspenso) == processosAlocadosNoQuantum.end()) {
                        //Tenta desalocar os bloqueados para liberar memória suficiente
                        if (desalocarBloqueadosParaProntosSuspensos(memoriaNecessaria)) {
                            break;
                        }
                    }

                    // Reinsere o processo no final da fila e tenta o próximo da fila suspensos
                    filaProntosSuspensos.pop();
                    filaProntosSuspensos.push(processoProntoSuspenso);
                    processoProntoSuspenso = filaProntosSuspensos.front();
                    memoriaNecessaria = processoProntoSuspenso->getRam();
                    contaFila++;
                }

                // Caso consiga desalocar, realoca o processo de prontos suspensos para prontos
                if (contaFila < countProntosSuspensos) {
                    realocarProntosSuspensos();

                    jaCheckouSuspensos = true;
                    goto tentaExecutarPronto;

                } else {
                    //Caso não consiga liberar memória suficiente, informa que a CPU ficará ociosa
                    std::cout << "Não foi possível liberar memória suficiente dos processos bloqueados para processos prontos suspensos.\n";
                }
            }
            //CPU não conseguiu escalonar nenhum processo
            if(!processoAtual){
                std::cout << "CPU#" << i+1 << " ocioso. Aguardando... " << std::endl;
                continue;
            } 
        }
        
        
        checkaTermino:
        //A cada processo executado, precisa tratar caso sua fase de CPU tenha terminado
        //Se ainda não fez I/O, vai para bloqueados
        //Se fez I/O, terminado
        if (processoAtual->checarTerminoDaFase()) {
            
            //Reseta informações no CPU
            cpusDisponiveis[i].ultimoProcesso = cpusDisponiveis[i].P;
            cpusDisponiveis[i].P = nullptr;
            cpusDisponiveis[i].tempo_executando_processo = 0;
            
            //Se o processo já passou por fase de I/O
            //Deve ser terminado
            if(processoAtual->getFezIo()){    
                
                processoAtual->tempoTermino = tempoAtual;
                processoAtual->turnAroundTime = processoAtual->tempoTermino - processoAtual->tempoChegada;
                processoAtual->alterarEstado(Estado::TERMINADO);

                std::cout << "Processo #" << processoAtual->getId() 
                          << " finalizado e " <<processoAtual->getRam() << " MB liberados. Turnaround Time: "<< processoAtual->turnAroundTime <<
                          std::endl;
                          
                
                //Como Estado do Processo é TERMINADO, será deletado no método abaixo
                gerenciadorMemoria->liberaMemoria(processoAtual,processosAtuais);
            
            //Caso não tenha feito I/O, tempoRestanteCpu = duracaoCpu2 e adiciona na fila de bloqueados                   
            } else {
                std::cout << "Processo #" << processoAtual->getId() 
                          << " foi para I/O "  
                          << "\n";
                
                processoAtual->alterarEstado(Estado::BLOQUEADO);
                processoAtual->setTempoRestanteCpu(processoAtual->getDuracaoCpu2());
                filaBloqueados.push(processoAtual);
                processoAtual->setFezIo();
            }
        }
    }
    //Decrementa 1 u.t de processos em I/O que não foram executados nessa u.t
    //Set é passado para controlar quais processos serão decrementados
    decrementaBloqueados(processosAlocadosNoQuantum);
    decrementaBloqueadosSuspensos(processosAlocadosNoQuantum);
}

void Despachante::setGerenciadorMemoria(GerenciadorMemoria* gm) {
    gerenciadorMemoria = gm;
}

//Decrementa tempo dos processos bloqueados suspensos, mandando pra fila prontos suspensos caso termine I/O.
//Tem que executar esse método no final do quantum, após execução dos CPUs e pular iteração caso o processo esteja no set.
void Despachante::decrementaBloqueadosSuspensos(std::unordered_set<Processo*>& processosAlocadosNoQuantum) {
    if (filaBloqueadosSuspensos.empty()) return;

    size_t bloqueadosSuspensosCount = filaBloqueadosSuspensos.size();

    for (size_t i = 0; i < bloqueadosSuspensosCount; ++i) {
        
        Processo* processoAtual = filaBloqueadosSuspensos.front();
        filaBloqueadosSuspensos.pop();

        //Se processo nullptr ou processo já foi processado durante quantum (pois esse método é chamado no final de escalonar() )
        if (!processoAtual || processosAlocadosNoQuantum.find(processoAtual) != processosAlocadosNoQuantum.end()){
            filaBloqueadosSuspensos.push(processoAtual);
            continue;
        } 
            
        processoAtual->decrementaTempoRestanteIo();
            
        //Se terminou I/O, vai pra fila de Prontos Suspensos
        if(processoAtual->getDuracaoIo() <= 0){
            processoAtual->alterarEstado(Estado::PRONTO_SUSPENSO);
            filaProntosSuspensos.push(processoAtual);
            processoAtual->setFezIo();
            std::cout << "Processo #" << processoAtual->getId() << " terminou I/O. Movido para fila prontos suspensos.\n";
        
        //Insere na fila de novo
        } else {
            filaBloqueadosSuspensos.push(processoAtual);
        }
    }
}

//Similar ao método anterior, mas para filas diferentes 
void Despachante::decrementaBloqueados(std::unordered_set<Processo*>& processosAlocadosNoQuantum) {
    if (filaBloqueados.empty()) return;

    size_t bloqueadosCount = filaBloqueados.size();

    for (size_t i = 0; i < bloqueadosCount; ++i) {
        
        Processo* processoAtual = filaBloqueados.front();
        filaBloqueados.pop();

        //Se processo nullptr ou processo já foi processado durante quantum (esse método é chamado no final de escalonar() )
        if (!processoAtual || processosAlocadosNoQuantum.find(processoAtual) != processosAlocadosNoQuantum.end()){
            filaBloqueados.push(processoAtual);
            continue;
        } 
            
        processoAtual->decrementaTempoRestanteIo();
            
        //Se terminou I/O, vai pra fila de auxiliar
        if(processoAtual->getDuracaoIo() <= 0){
            processoAtual->alterarEstado(Estado::PRONTO);
            filaAuxiliar.push(processoAtual);
            processoAtual->setFezIo();
            std::cout << "Processo #" << processoAtual->getId() << " terminou I/O. Movido para fila auxiliar.\n";
        
        //Insere na fila de novo
        } else {
            filaBloqueados.push(processoAtual);
        }
    }
}

//Método ordena array de processos bloqueados por duracaoIo, dando preferência para os com maior duração
//Suspende esses processos até atingir a memória necessária para alocar os processos prontos suspensos
int Despachante::desalocarAteNecessario(int memoriaNecessaria, std::vector<Processo*>& processosParaDesalocar) {
    int memoriaLiberada = 0;

    //Ordena os processos pelo tempo restante de I/O
    std::sort(processosParaDesalocar.begin(), processosParaDesalocar.end(),
              [](Processo* a, Processo* b) {
                  return a->getDuracaoIo() > b->getDuracaoIo();
              });

    // Desaloca processos até atingir a memória necessária
    for (Processo* processo : processosParaDesalocar) {
        if (memoriaNecessaria <= 0) break;

        gerenciadorMemoria->liberaMemoria(processo,processosAtuais);
        processo->alterarEstado(Estado::BLOQUEADO_SUSPENSO);
        filaBloqueadosSuspensos.push(processo);

        memoriaLiberada += processo->getRam();
        memoriaNecessaria -= processo->getRam();

        std::cout << "Processo #" << processo->getId()
                  << " movido para bloqueados suspensos. "
                  << "Memória liberada: " << processo->getRam() << " MB.\n";
    }
    
    //Se sobraram processos bloqueados que não foram necessários, reinserir na filaBloqueados
    if (!processosParaDesalocar.empty()){
    
        //Reinserir os processos que não foram desalocados de volta na fila de bloqueados
        for (Processo* processo : processosParaDesalocar) {
            
            //Verifica se o processo ainda está no vetor após a desalocação
            if (processo->getEstadoString() != "BLOQUEADO_SUSPENSO") {
                filaBloqueados.push(processo);
                std::cout << "Processo #" << processo->getId()
                        << " reinserido na fila de bloqueados. \n";
            }
        }
    }

    return memoriaLiberada;
}

//Método percorre fila prontosuspensos e aloca quantos conseguir para prontos
void Despachante::realocarProntosSuspensos() {
    
    while (!filaProntosSuspensos.empty()) {
        Processo* processo = filaProntosSuspensos.front();

        if(gerenciadorMemoria->alocarMemoria(processo->getId(), processo->getRam())){
            processo->alterarEstado(Estado::PRONTO);
            filaProntosSuspensos.pop();
            filaProntos.push(processo);

            std::cout << "Processo #" << processo->getId()
                    << " movido para fila de prontos. "
                    << "Memória utilizada: " << processo->getRam() << " MB.\n";
        } else break;
    }
}

//Método verifica se é possível desalocar memoriaNecessaria dos processos bloqueados para alocar prontos suspensos
bool Despachante::desalocarBloqueadosParaProntosSuspensos(int memoriaNecessaria) {
    int memoriaTotalDesalocavel = 0;
    std::vector<Processo*> processosParaDesalocar;

    //Calcular se a memória total desalocável é suficiente 
    size_t tamanhoOriginal = filaBloqueados.size();
    for (size_t i = 0; i < tamanhoOriginal; ++i) {
        
        Processo* processo = filaBloqueados.front();
        filaBloqueados.pop();
        memoriaTotalDesalocavel += processo->getRam();
        processosParaDesalocar.push_back(processo);

        //Previne iterações desnecessárias
        if(memoriaTotalDesalocavel >= memoriaNecessaria) break;
    }

    //Se a memória total desalocável não for suficiente, insere os procesoss de volta
    //na filaBloqueados 
    if (memoriaTotalDesalocavel < memoriaNecessaria) {

        for (Processo* processo : processosParaDesalocar) {
            filaBloqueados.push(processo);
        }
        return false; 
    }

    // Realiza a desalocação até atingir a memória necessária
    int memoriaLiberada = desalocarAteNecessario(memoriaNecessaria, processosParaDesalocar);
    return memoriaLiberada >= memoriaNecessaria;
}

void Despachante::adicionarProcessoNaFilaProntos(Processo* processo) {
        filaProntos.push(processo);
    }

void Despachante::adicionarProcessoNaFilaProntosSuspensos(Processo* processo) {
        filaProntosSuspensos.push(processo);
    }

std::set<Processo*,ProcessoComparator> Despachante::getProcessosAtuais(){return processosAtuais;}

std::unordered_set<Processo*> Despachante::getProcessosAlocadosNoQuantum() {return processosAlocadosNoQuantum;}

std::queue<Processo*> Despachante::getFilaProntos() { return filaProntos;}
std::queue<Processo*> Despachante::getFilaProntosSuspensos() { return filaProntosSuspensos;}
std::queue<Processo*> Despachante::getFilaBloqueados() {return filaBloqueados;}
std::queue<Processo*> Despachante::getFilaBloqueadosSuspensos() { return filaBloqueadosSuspensos;}
std::queue<Processo*> Despachante::getFilaAuxiliar() { return filaAuxiliar;}