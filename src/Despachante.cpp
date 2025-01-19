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
        if(processo->getEstadoString() == "PRONTO_SUSPENSO" && processo->getFezIo()){
            processo->alterarEstado(Estado::PRONTO);
            std::cout << "Processo#" << processo->getId() << " inserido na fila de auxiliar."<< std::endl;
            filaAuxiliar.push(processo);
            return;
        }
        
        std::cout << "Processo#" << processo->getId() << " inserido na fila de prontos" << std::endl;
        processo->alterarEstado(Estado::PRONTO);
        filaProntos.push(processo);
        

    //Se não tem memória, vai ou é mantido em fila prontos suspensos
    } else {
        if(processo->getEstadoString() == "PRONTO_SUSPENSO"){
            //Printar,na presença de muitos processos prontos suspensos, o cout abaixo flooda o buffer, logo escolhi ignorar
            //std::cout << "Processo#" << processo->getId() << " mantido na fila de prontos suspensos. Memória insuficiente" << std::endl;
            filaProntosSuspensos.push(processo);
            return;
        }

        std::cout << "Processo#" << processo->getId() << " inserido na fila de prontos suspensos. Memória insuficiente" << std::endl;
        processo->alterarEstado(Estado::PRONTO_SUSPENSO);
        filaProntosSuspensos.push(processo);
        
    }
}

//Tenta alocar processos suspensos para fila de prontos
void Despachante::tentarAlocarProcessosSuspensos() {
    
    size_t tamanhoFila = filaProntosSuspensos.size();
    for (size_t i = 0; i < tamanhoFila; ++i) {
        Processo* processo = filaProntosSuspensos.front();
        filaProntosSuspensos.pop();  

        //tentarAlocarProcesos já insere novamente em filaProntosSuspensos caso não tenha memória suficiente
        tentaAlocarProcesso(processo);
    }
}


//Método principal do Simulador
void Despachante::escalonar(){


    //jaCheckouSuspensos para controlar se já tentou suspender bloqueados para alocar prontos suspensos
    //evita loop infinito (vide linha 191 para mais detalhes)
    bool jaCheckouSuspensos = false;

    Processo* processoAtual;
    
    //Set de processos alocados nas CPUs durante u.t
    //Evita que o mesmo Processo seja executado em CPUs diferentes durante u.t
    //Controla quais processos terão seu tempo de I/O decrementado
    //Se um processo executou em uma u.t e foi bloqueado, não pode ter seu duracaoIo decrementado nesse ciclo e 
    //também não pode ser suspenso para alocar prontos suspensos 
    processosAlocados.clear();

    gerenciadorMemoria->deletarProcessos();

    std::cout << "------------------------------------------------------------------" << tempoAtual << " u.t -----------------------------------------------------------------\n";
    
    tentarAlocarProcessosSuspensos();

    //Para cada CPU, executa processo nele ou busca nas filas caso !processoAtual
    for (int i = 0; i < numCpus ; i++) {
        
        cpusDisponiveis[i].ultimoProcesso = nullptr;
        processoAtual = cpusDisponiveis[i].P;
        
        //Se tem processo no CPU e não executou nessa u.t (pode ter terminado fatia de tempo no CPU#1 e executar na CPU#2 caso não tenha o check do set)
        if(processoAtual && (processosAlocados.find(processoAtual) == processosAlocados.end())){
            processoAtual->executarCpu();
            cpusDisponiveis[i].tempo_executando_processo++;
            processosAlocados.insert(processoAtual);
            
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
                processoAtual = filaAuxiliar.front();
                filaAuxiliar.pop();
                cpusDisponiveis[i].P = processoAtual;
                processosAlocados.insert(processoAtual);
                processoAtual->executarCpu();
                cpusDisponiveis[i].tempo_executando_processo = 1;
                goto checkaTermino;   
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
                while ((processosAlocados.find(processoAtual) != processosAlocados.end()) && contaFila < tamanhoInicial) {
                    filaProntos.push(processoAtual);
                    processoAtual = filaProntos.front();
                    filaProntos.pop();
                    contaFila++;
                    if(contaFila>=tamanhoInicial) break;
                    //Busca até encontrar processo que não esteja alocado ou percorra toda a fila
                }
                //Se encontrou processo
                if(contaFila <= tamanhoInicial && (processosAlocados.find(processoAtual) == processosAlocados.end())){
                    cpusDisponiveis[i].P = processoAtual;
                    processosAlocados.insert(processoAtual);
                    processoAtual->executarCpu();
                    cpusDisponiveis[i].tempo_executando_processo = 1;
                    goto checkaTermino;

                } else {
                    std::cout << "Todos os processos possíveis da fila Prontos já foram executados nesse quantum." << std::endl;
                    filaProntos.push(processoAtual);
                    processoAtual = nullptr;
                    
                    //jaCheckouSuspensos evita loop infinito. só tenta checar filas de processos suspensos uma vez
                    if(jaCheckouSuspensos) goto cpuOcioso;
                    else goto checkFilasSuspensas;
                }
            }
            
            checkFilasSuspensas:
            // Tenta suspender bloqueados para prontos suspensos poderem ir p/ prontos e escalonados
            if (!filaBloqueados.empty() && !filaProntosSuspensos.empty()) {
                std::cout << "CPU #" << i + 1 
                          << " buscando suspender processos bloqueados para alocar prontos suspensos.\n";

                Processo* processoProntoSuspenso = filaProntosSuspensos.front();
                int memoriaNecessaria = processoProntoSuspenso->getRam();
                size_t countProntosSuspensos = filaProntosSuspensos.size();
                int contaFila = 0;

                // Percorre a fila de prontos suspensos verificando condições
                while (contaFila < countProntosSuspensos) {
                    //Checa se o processo já foi processado no quantum atual
                    if (processosAlocados.find(processoProntoSuspenso) == processosAlocados.end()) {
                        //Tenta suspender os bloqueados para liberar memória suficiente
                        if (tentarSuspenderBloqueados(memoriaNecessaria)) {
                            break;
                            //conseguiu suspender memóriaNecessária, logo sai do loop
                        }
                    }

                    // Reinsere o processo no final da fila e tenta o próximo da fila suspensos
                    filaProntosSuspensos.pop();
                    filaProntosSuspensos.push(processoProntoSuspenso);
                    processoProntoSuspenso = filaProntosSuspensos.front();
                    memoriaNecessaria = processoProntoSuspenso->getRam();
                    contaFila++;
                }

                // Caso consiga suspender, aloca o processo de prontos suspensos para prontos
                //e tenta buscar prontos para executar
                if (contaFila < countProntosSuspensos) {
                    tentarAlocarProcessosProntosSuspensos();
                    goto tentaExecutarPronto;

                } else {
                    //Caso não consiga liberar memória suficiente, informa que a CPU ficará ociosa
                    //Por que esse jaCheckouSuspensos?
                    //Suponha um situação onde a CPU# 1 executou um processo e count = 4
                    //o processo vai para filaProntos e estará em processosAlocados. 
                    //É possível que entremos na situação da linha 145, onde novamente 
                    //o simulador tentará suspender bloqueados entrará em loop infinito.
                    //o jaCheckouSuspensos impossibilita essa situação. A CPU ficará ociosa.
                    jaCheckouSuspensos = true;
                    std::cout << "Não foi possível liberar memória suficiente dos processos bloqueados para alocar processos prontos suspensos.\n";
                }
            }
            
            //CPU não conseguiu escalonar nenhum processo
            cpuOcioso:
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
                          " - Normalizado: " << (float)processoAtual->turnAroundTime / (processoAtual->getDuracaoCpu1() + processoAtual->getDuracaoCpu2()
                          + processoAtual->getDuracaoIoTotal()) << std::endl;
                          
                
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
    //Set é passado como parâmetro para controlar quais processos serão decrementados
    decrementaBloqueados(processosAlocados);
    decrementaBloqueadosSuspensos(processosAlocados);
}

void Despachante::setGerenciadorMemoria(GerenciadorMemoria* gm) {
    gerenciadorMemoria = gm;
}

//Decrementa tempo dos processos bloqueados suspensos, mandando pra fila prontos suspensos caso termine I/O.
//Método chamado no final do quantum, após execução dos CPUs e não decremeta os procesoss no set, pois já foram executados durante u.t.
void Despachante::decrementaBloqueadosSuspensos(std::unordered_set<Processo*>& processosAlocados) {
    if (filaBloqueadosSuspensos.empty()) return;

    size_t bloqueadosSuspensosCount = filaBloqueadosSuspensos.size();

    for (size_t i = 0; i < bloqueadosSuspensosCount; ++i) {
        
        Processo* processoAtual = filaBloqueadosSuspensos.front();
        filaBloqueadosSuspensos.pop();

        //Se processo nullptr ou processo já foi processado durante quantum (pois esse método é chamado no final de escalonar() )
        if (!processoAtual || processosAlocados.find(processoAtual) != processosAlocados.end()){
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
        
        //Caso contrário, insere na fila novamente
        } else {
            filaBloqueadosSuspensos.push(processoAtual);
        }
    }
}

//Análogo ao método anterior, mas para filas de bloqueados 
void Despachante::decrementaBloqueados(std::unordered_set<Processo*>& processosAlocados) {
    if (filaBloqueados.empty()) return;

    size_t bloqueadosCount = filaBloqueados.size();

    for (size_t i = 0; i < bloqueadosCount; ++i) {
        
        Processo* processoAtual = filaBloqueados.front();
        filaBloqueados.pop();

        //Se processo nullptr ou processo já foi processado durante quantum (esse método é chamado no final de escalonar() )
        if (!processoAtual || processosAlocados.find(processoAtual) != processosAlocados.end()){
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
        
        //Insere na fila novamente
        } else {
            filaBloqueados.push(processoAtual);
        }
    }
}

//Método ordena array de processos bloqueados por duracaoIo, dando preferência para os com maior duração
//Suspende esses processos até atingir a memória necessária para alocar os processos prontos suspensos
int Despachante::suspenderAteNecessario(int memoriaNecessaria, std::vector<Processo*>& processosParaDesalocar) {
    int memoriaLiberada = 0;

    //Ordena os processos pelo tempo restante de I/O
    std::sort(processosParaDesalocar.begin(), processosParaDesalocar.end(),
              [](Processo* a, Processo* b) {
                  return a->getDuracaoIo() > b->getDuracaoIo();
              });

    //Suspende processos até atingir a memória necessária
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
            }
        }
    }

    return memoriaLiberada;
}

//Método percorre fila prontosuspensos e aloca quantos conseguir para prontos
void Despachante::tentarAlocarProcessosProntosSuspensos() {
    
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
bool Despachante::tentarSuspenderBloqueados(int memoriaNecessaria) {
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
    int memoriaLiberada = suspenderAteNecessario(memoriaNecessaria, processosParaDesalocar);
    return memoriaLiberada >= memoriaNecessaria;
}

std::set<Processo*,ProcessoComparator> Despachante::getProcessosAtuais(){return processosAtuais;}
std::unordered_set<Processo*> Despachante::getprocessosAlocados() {return processosAlocados;}

std::queue<Processo*> Despachante::getFilaProntos() { return filaProntos;}
std::queue<Processo*> Despachante::getFilaProntosSuspensos() { return filaProntosSuspensos;}
std::queue<Processo*> Despachante::getFilaBloqueados() {return filaBloqueados;}
std::queue<Processo*> Despachante::getFilaBloqueadosSuspensos() { return filaBloqueadosSuspensos;}
std::queue<Processo*> Despachante::getFilaAuxiliar() { return filaAuxiliar;}