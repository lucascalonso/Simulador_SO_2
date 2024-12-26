#include "../include/Despachante.h"
#include "../include/Processo.h"
#include "../include/GerenciadorMemoria.h"
#include "../include/globals.h"
#include <iostream>



Despachante::Despachante(int quantum,int numCpus) : quantum(quantum), numCpus(numCpus){}


void Despachante::tentaAlocarProcesso(Processo* processo) {

    //Prontos suspensos vão pra fila auxiliar
    if (gerenciadorMemoria->alocarMemoria(processo->getId(), processo->getRam())) {
        if(processo->getEstadoString() == "PRONTO_SUSPENSO"){
            std::cout << "Processo#" << processo->getId() << " inserido na fila de auxiliar."<< std::endl;
            filaAuxiliar.push(processo);
            return;
        }
        
        std::cout << "Processo#" << processo->getId() << " alocado na fila de prontos" << std::endl;
        processo->alterarEstado(Estado::PRONTO);
        filaProntos.push(processo);


    //Se não tem memória, vai ou é mantido em fila prontos suspensos
    } else {
        if(processo->getEstadoString() == "PRONTO_SUSPENSO"){
            std::cout << "Processo#" << processo->getId() << " mantido na fila de prontos suspensos. Memória insuficiente" << std::endl;
            filaProntosSuspensos.push(processo);
            return;
        }

        processo->alterarEstado(Estado::PRONTO_SUSPENSO);
        filaProntosSuspensos.push(processo);
        std::cout << "Processo#" << processo->getId() << " alocado na fila de prontos suspensos. Memória insuficiente" << std::endl;
    }
}

void Despachante::tentarAlocarProcessosSuspensos() {
    // Tenta alocar processos suspensos que ainda estão na fila
    size_t tamanhoFila = filaProntosSuspensos.size();
    for (size_t i = 0; i < tamanhoFila; ++i) {
        Processo* processo = filaProntosSuspensos.front();  // Obtém o primeiro processo da fila
        filaProntosSuspensos.pop();  // Remove da fila

        // Tenta alocar o processo. Já insere de novo em filaProntosSuspensos caso não tenha memória!
        tentaAlocarProcesso(processo);
    }
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

    //Interrompe loop 
    bool flag = false;

    Processo* processoAtual;

    //Prioriza inserir prontos Suspensos na fila auxiliar
    
    
    //Set de processos alocados para cada quantum (evitando que o mesmo Processo seja executado em CPUs diferentes durante u.t)
    std::unordered_set<Processo*> processosAlocadosNoQuantum;

    std::cout << "---------------------------------------------" << tempoAtual << " u.t decorridos------------------------------------------------------------\n\n";
    
    tentarAlocarProcessosSuspensos();

    //Para cada CPU, executa processo nele ou precisa buscar nas filas
    for (int i = 0; i < numCpus ; i++) {
        
        processoAtual = cpusDisponiveis[i].P;
        
        //Se tem processo no CPU e não executou nessa u.t (pode ter sofrido timeout no CPU#1 e executar na CPU#2 caso não tenha o check do set )
        if(processoAtual && (processosAlocadosNoQuantum.find(processoAtual) == processosAlocadosNoQuantum.end())){
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
                
                //Se o processo está no set, já está alocado a uma CPU nesse quantum.
                while ((processosAlocadosNoQuantum.find(processoAtual) != processosAlocadosNoQuantum.end()) && contaFila < tamanhoInicial) {
                    filaAuxiliar.push(processoAtual);
                    processoAtual = filaAuxiliar.front();
                    filaAuxiliar.pop();
                    contaFila++;
                    if(contaFila >= tamanhoInicial) break;
                    //Busca até encontrar processo que não esteja alocado ou percorra toda a fila
                }
                
                //Se encontrou processo
                if(contaFila <= tamanhoInicial && (processosAlocadosNoQuantum.find(processoAtual) == processosAlocadosNoQuantum.end())){
                    cpusDisponiveis[i].P = processoAtual;
                    processosAlocadosNoQuantum.insert(processoAtual);
                    processoAtual->executarCpu();
                    cpusDisponiveis[i].tempo_executando_processo = 1;
                    std::cout << "Processo #" << processoAtual->getId() <<  " executado na CPU #" << i+1 << " Count: "<< cpusDisponiveis[i].tempo_executando_processo << "\n";
                    goto checkaTermino;

                } else {
                    std::cout << "Todos os processos possíveis da fila auxiliar já foram alocados nesse quantum." << std::endl;
                    filaAuxiliar.push(processoAtual);
                    processoAtual = nullptr;
                }    
            }

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
                    std::cout << "Processo #" << processoAtual->getId() <<  " executado na CPU #" << i+1 << " Count: "<< cpusDisponiveis[i].tempo_executando_processo << "\n";
                    goto checkaTermino;

                } else {
                    std::cout << "Todos os processos possíveis da fila Prontos já foram alocados nesse quantum." << std::endl;
                    filaProntos.push(processoAtual);
                    processoAtual = nullptr;
                    if(flag){
                        std::cout << "CPU#" << i+1 << " rodou flag e não conseguiu alocar nenhum processo " << std::endl;
                        continue;
                    } 
                    else goto checkFilasSuspensas;
                }
            }
            checkFilasSuspensas:
            // Tenta dealocar bloqueados para prontos suspensos poderem ser escalonados
            if (!filaBloqueados.empty() && !filaProntosSuspensos.empty()) {
                std::cout << "CPU #" << i + 1 
                        << " buscando dealocar processos bloqueados para prontos suspensos.\n";

                // Obtém o primeiro processo da fila de prontos suspensos
                Processo* processoProntoSuspenso = filaProntosSuspensos.front();
                int memoriaNecessaria = processoProntoSuspenso->getRam();
                size_t countProntosSuspensos = filaProntosSuspensos.size();
                int contaFila = 0;

                // Percorre a fila de prontos suspensos verificando condições
                while (contaFila < countProntosSuspensos) {
                    // Checa se o processo já foi processado no quantum atual
                    if (processosAlocadosNoQuantum.count(processoProntoSuspenso) == 0) {
                        // Tenta desalocar os bloqueados para liberar memória suficiente
                        if (desalocarBloqueadosParaProntosSuspensos(memoriaNecessaria)) {
                            break; // Sai do while se a desalocação foi bem-sucedida
                        }
                    }

                    // Reinsere o processo no final da fila e tenta o pro próximo da fila
                    filaProntosSuspensos.pop();
                    filaProntosSuspensos.push(processoProntoSuspenso);
                    processoProntoSuspenso = filaProntosSuspensos.front();
                    memoriaNecessaria = processoProntoSuspenso->getRam();
                    contaFila++;
                }

                // Caso consiga desalocar, realoca o processo de prontos suspensos para prontos
                if (contaFila < countProntosSuspensos) {
                    realocarProntosSuspensos();
                    processosAlocadosNoQuantum.insert(processoProntoSuspenso); // Marca como processado no quantum atual
                    std::cout << "Processo #" << processoProntoSuspenso->getId() 
                            << " movido de prontos suspensos para prontos.\n";

                    flag = true;
                    goto tentaExecutarPronto;

                } else {
                    // Caso não consiga liberar memória suficiente, informa que a CPU ficará ociosa
                    std::cout << "Não foi possível liberar memória suficiente dos processos bloqueados para processos prontos suspensos.\n";
                }
            }
            
            if(!processoAtual){
                std::cout << "CPU#" << i+1 << " ocioso. Aguardando... " << std::endl;
                continue;
            } 
        }
        
        
        checkaTermino:
        // Checa se processo terminou a fase CPU
        if (processoAtual->checarTerminoDaFase()) {
            
            //Reseta informações no CPU
            cpusDisponiveis[i].P = nullptr;
            cpusDisponiveis[i].tempo_executando_processo = 0;
            
            //Se o processo já passou por fase de I/O
            if(processoAtual->getFezIo()){    
                
                processoAtual->tempoTermino = tempoAtual;
                processoAtual->turnAroundTime = processoAtual->tempoTermino - processoAtual->tempoChegada;
                processoAtual->alterarEstado(Estado::TERMINADO);

                std::cout << "Processo #" << processoAtual->getId() 
                          << " finalizado e " <<processoAtual->getRam() << " MB liberados. Turnaround Time: "<< processoAtual->turnAroundTime 
                          << "\n";
                
                //Como Estado do Processo é TERMINADO, será deletado abaixo
                gerenciadorMemoria->liberaMemoria(processoAtual);
            
            //Caso não tenha feito I/O, tempoRestante = duracaoCpu2, adiciona na fila de bloqueados                   
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
    decrementaBloqueados(processosAlocadosNoQuantum);
    decrementaBloqueadosSuspensos(processosAlocadosNoQuantum);

    //Visualização
    imprimirFila(filaProntos, "Fila de Prontos");
    imprimirFila(filaBloqueados, "Fila de Bloqueados");
    imprimirFila(filaProntosSuspensos, "Fila de Prontos Suspensos");
    imprimirFila(filaBloqueadosSuspensos, "Fila de Bloqueados Suspensos");
    imprimirFila(filaAuxiliar, "Fila de Prontos Auxiliar");
    gerenciadorMemoria->visualizarMemoria();
}

void Despachante::setGerenciadorMemoria(GerenciadorMemoria* gm) {
    gerenciadorMemoria = gm;
}

//Simplesmente decrementa tempo dos processos bloqueados suspensos, mandando pra fila prontos suspensos caso termine I/O.
//Tem que executar esse método no final do quantum, após execução dos CPUs e pular iteração caso o processo esteja no set.
//Passar o set como parâmetro
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

void Despachante::verificaRealocacao() {
    // Vai tentar desalocar bloqueados para liberar memória para os prontos suspensos
    Processo* processoProntoSuspenso = filaProntosSuspensos.front();
    int memoriaNecessaria = processoProntoSuspenso->getRam();

    // Tenta desalocar processos da fila de Bloqueados
    if (desalocarBloqueadosParaProntosSuspensos(memoriaNecessaria)) {
        // Após desalocar, tenta realocar os prontos suspensos
        realocarProntosSuspensos();
    } else {
        std::cout << "Memória insuficiente para realocar processos da fila de Prontos Suspensos.\n";
    }
}



int Despachante::desalocarAteNecessario(int memoriaNecessaria, std::vector<Processo*>& processosParaDesalocar) {
    int memoriaLiberada = 0;

    // Ordena os processos pelo tempo restante de I/O
    std::sort(processosParaDesalocar.begin(), processosParaDesalocar.end(),
              [](Processo* a, Processo* b) {
                  return a->getDuracaoIo() > b->getDuracaoIo();
              });

    // Desaloca processos até atingir a memória necessária
    for (Processo* processo : processosParaDesalocar) {
        if (memoriaNecessaria <= 0) break;

        gerenciadorMemoria->liberaMemoria(processo);
        processo->alterarEstado(Estado::BLOQUEADO_SUSPENSO);
        filaBloqueadosSuspensos.push(processo);

        memoriaLiberada += processo->getRam();
        memoriaNecessaria -= processo->getRam();

        std::cout << "Processo #" << processo->getId()
                  << " movido para bloqueados suspensos. "
                  << "Memória liberada: " << processo->getRam() << " MB.\n";
    }
    
    //Se sobraram processos bloqueados que não foram necessários, reinserir na filaBloqueados
    if (!processosParaDesalocar.empty()) {
    
    // Reinserir os processos que não foram desalocados de volta na fila de bloqueados
    for (Processo* processo : processosParaDesalocar) {
        
        // Verifica se o processo ainda está no vetor após a desalocação
        if (processo->getEstadoString() != "BLOQUEADO_SUSPENSO") {
            filaBloqueados.push(processo);
            std::cout << "Processo #" << processo->getId()
                      << " reinserido na fila de bloqueados. \n";
        }
    }
}

    return memoriaLiberada;
}

void Despachante::realocarProntosSuspensos() {
    
    while (!filaProntosSuspensos.empty()) {
        Processo* processo = filaProntosSuspensos.front();

        // Aloca a memória necessária para o processo
        if(gerenciadorMemoria->alocarMemoria(processo->getId(), processo->getRam())){
            processo->alterarEstado(Estado::PRONTO);
            filaProntosSuspensos.pop();
            filaProntos.push(processo);

            std::cout << "Processo #" << processo->getId()
                    << " realocado para prontos. "
                    << "Memória utilizada: " << processo->getRam() << " MB.\n";
        } else break;
    }
}


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


/*
//Método principal do Simulador
void Despachante::escalonar(){

    //Passou uma unidade de tempo
    tempoAtual++;

    //Interrompe loop 
    bool flag = false;

    Processo* processoAtual;

    //Prioriza inserir prontos Suspensos na fila auxiliar
    
    
    //Set de processos alocados para cada quantum (evitando que o mesmo Processo seja executado em CPUs diferentes durante u.t)
    std::unordered_set<Processo*> processosAlocadosNoQuantum;

    std::cout << "---------------------------------------------" << tempoAtual << " u.t decorridos------------------------------------------------------------\n\n";
    
    tentarAlocarProcessosSuspensos();

    //Para cada CPU, executa processo nele ou precisa buscar nas filas
    for (int i = 0; i < numCpus ; i++) {
        
        processoAtual = cpusDisponiveis[i].P;
        
        //Se tem processo no CPU e não executou nessa u.t (pode ter sofrido timeout no CPU#1 e executar na CPU#2 caso não tenha o check do set )
        if(processoAtual && (processosAlocadosNoQuantum.find(processoAtual) == processosAlocadosNoQuantum.end())){
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
                
                //Se o processo está no set, já está alocado a uma CPU nesse quantum.
                while ((processosAlocadosNoQuantum.find(processoAtual) != processosAlocadosNoQuantum.end()) && contaFila < tamanhoInicial) {
                    filaAuxiliar.push(processoAtual);
                    processoAtual = filaAuxiliar.front();
                    filaAuxiliar.pop();
                    contaFila++;
                    if(contaFila >= tamanhoInicial) break;
                    //Busca até encontrar processo que não esteja alocado ou percorra toda a fila
                }
                
                //Se encontrou processo
                if(contaFila <= tamanhoInicial && (processosAlocadosNoQuantum.find(processoAtual) == processosAlocadosNoQuantum.end())){
                    cpusDisponiveis[i].P = processoAtual;
                    processosAlocadosNoQuantum.insert(processoAtual);
                    processoAtual->executarCpu();
                    cpusDisponiveis[i].tempo_executando_processo = 1;
                    std::cout << "Processo #" << processoAtual->getId() <<  " executado na CPU #" << i+1 << " Count: "<< cpusDisponiveis[i].tempo_executando_processo << "\n";
                    goto checkaTermino;

                } else {
                    std::cout << "Todos os processos possíveis da fila auxiliar já foram alocados nesse quantum." << std::endl;
                    filaAuxiliar.push(processoAtual);
                    processoAtual = nullptr;
                }    
            }

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
                    std::cout << "Processo #" << processoAtual->getId() <<  " executado na CPU #" << i+1 << " Count: "<< cpusDisponiveis[i].tempo_executando_processo << "\n";
                    goto checkaTermino;

                } else {
                    std::cout << "Todos os processos possíveis da fila Prontos já foram alocados nesse quantum." << std::endl;
                    filaProntos.push(processoAtual);
                    processoAtual = nullptr;
                    if(flag){
                        std::cout << "CPU#" << i+1 << " rodou flag e não conseguiu alocar nenhum processo " << std::endl;
                        continue;
                    } 
                    else goto checkFilasSuspensas;
                }
            }

            checkFilasSuspensas:
            //Vai tentar dealocar bloqueados para os prontos suspensos poderem ser escalonados    
            if(!filaBloqueados.empty() && !filaProntosSuspensos.empty()){
                std::cout << "CPU #" << i+1 
                                << " buscando dealocar processos bloqueados para prontos suspensos.\n";
                
                // Obtém o primeiro processo da fila de prontos suspensos
                Processo* processoProntoSuspenso = filaProntosSuspensos.front();
                int memoriaNecessaria = processoProntoSuspenso->getRam();

                // Tenta desalocar processos da fila de bloqueados para liberar memória suficiente
                if (desalocarBloqueadosParaProntosSuspensos(memoriaNecessaria)) {
                    // Caso consiga desalocar, realoca o processo de prontos suspensos para prontos
                    realocarProntosSuspensos();
                    std::cout << "Processo #" << processoProntoSuspenso->getId() 
                                << " movido de prontos suspensos para prontos.\n";
                    
                    flag = true;
                    goto tentaExecutarPronto;

                } else {
                // Caso não consiga liberar memória suficiente, informa que a CPU ficará ociosa
                    std::cout << "Não foi possível liberar memória suficiente dos procesoss bloqueados para processos prontos suspensos.\n";
                    std::cout << "CPU#" << i+1 << " ociosa." << std::endl;
                }
            }
            if(!processoAtual){
                std::cout << "CPU#" << i+1 << " ocioso. Aguardando... " << std::endl;
                continue;
            } 
        }
        
        
        checkaTermino:
        // Checa se processo terminou a fase CPU
        if (processoAtual->checarTerminoDaFase()) {
            
            //Reseta informações no CPU
            cpusDisponiveis[i].P = nullptr;
            cpusDisponiveis[i].tempo_executando_processo = 0;
            
            //Se o processo já passou por fase de I/O
            if(processoAtual->getFezIo()){    
                
                processoAtual->tempoTermino = tempoAtual;
                processoAtual->turnAroundTime = processoAtual->tempoTermino - processoAtual->tempoChegada;
                processoAtual->alterarEstado(Estado::TERMINADO);

                std::cout << "Processo #" << processoAtual->getId() 
                          << " finalizado e " <<processoAtual->getRam() << " MB liberados. Turnaround Time: "<< processoAtual->turnAroundTime 
                          << "\n";
                gerenciadorMemoria->liberaMemoria(processoAtual);
            
            //Caso não tenha feito I/O, tempoRestante = duracaoCpu2, adiciona na fila de bloqueados                   
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
    decrementaBloqueados(processosAlocadosNoQuantum);
    decrementaBloqueadosSuspensos(processosAlocadosNoQuantum);

    //Visualização
    imprimirFila(filaProntos, "Fila de Prontos");
    imprimirFila(filaBloqueados, "Fila de Bloqueados");
    imprimirFila(filaProntosSuspensos, "Fila de Prontos Suspensos");
    imprimirFila(filaBloqueadosSuspensos, "Fila de Bloqueados Suspensos");
    imprimirFila(filaAuxiliar, "Fila de Prontos Auxiliar");
    gerenciadorMemoria->visualizarMemoria();
}
*/