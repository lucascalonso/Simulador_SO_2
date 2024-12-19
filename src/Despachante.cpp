#include "../include/Despachante.h"
#include "../include/Processo.h"
#include "../include/GerenciadorMemoria.h"
#include <iostream>
#include <thread>
#include <unordered_set>
#include <chrono>


Despachante::Despachante(int quantum,int numCpus) : quantum(quantum){
    cpusDisponiveis.resize(numCpus,false);
}

Processo* Despachante::recuperarProcessoPorId(int processoId) {
    auto it = processosPorId.find(processoId);
    if (it != processosPorId.end()) {
        return it->second;
    }
    return nullptr;
}

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

    std::queue<Processo*> copiaFila = fila; //Copia a fila para iterar
    while (!copiaFila.empty()) {
        Processo* processo = copiaFila.front();
        copiaFila.pop();
        if (processo) {
            std::cout << processo->getId() << " ";
        } else {
            std::cerr << "Erro: Processo nulo encontrado na fila.\n";
        }
    }
}

void Despachante::escalonar(int delay) {
    int num_quantum = 0;
    while (!filaProntos.empty() || !filaBloqueados.empty()) {
        std::cout << std::endl;
        desbloquear();
        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        //Set para não executar o mesmo processo em mais de um CPU um quantum
        std::unordered_set<Processo*> processosAlocados; 

        if (!filaProntos.empty()) {
            num_quantum++;
            std::cout << "----------------------------------------------Quantum # "  << num_quantum << "----------------------------------------------" << std::endl;
            for (int i = 0; i < cpusDisponiveis.size(); i++) {
                if (!cpusDisponiveis[i]) {
                    //Necessário para evitar bad_alloc (nullptr)
                    if (filaProntos.empty()) {                                            
                        break;
                    }

                    Processo* processoAtual = filaProntos.front();
                    filaProntos.pop();

                    if (!processoAtual) {
                        std::cerr << "Erro: Processo nulo na fila de prontos.\n";
                        continue;
                    }

                    //Se processo já está no Set processosAlocados, skipa loop e enqueue
                    if (processosAlocados.find(processoAtual) != processosAlocados.end()) {
                        filaProntos.push(processoAtual);
                        continue;
                    }
                    //Seta CPU ocupado
                    cpusDisponiveis[i] = true;

                    int tempoExecutado = std::min(quantum, processoAtual->getTempoRestante());
                    processoAtual->executarCpu(tempoExecutado);
                    std::cout << "Processo #" << processoAtual->getId() << " executado na CPU " << i + 1 << ". Tempo restante: " << processoAtual->getTempoRestante() << "\n";

                    //Adiciona processo ao Set
                    processosAlocados.insert(processoAtual);

                    //Se processo terminou execução, libera memória
                    if (processoAtual->getTempoRestante() <= 0) {
                        processosAlocados.erase(processoAtual);
                        std::cout << "Processo #" << processoAtual->getId() << " finalizado.\n";
                        if (gerenciadorMemoria) {
                            gerenciadorMemoria->liberaMemoria(processoAtual);
                        }
                        delete processoAtual;
                    } else {
                        //Reinsere na fila se ainda tem tempo de execução
                        filaProntos.push(processoAtual); 
                    }
                    //Limpa iésimo CPU
                    liberarCPU(i);  
                }
            }
            imprimirFila(filaProntos, "Fila de Prontos");
            std::cout << std::endl;
            imprimirFila(filaBloqueados, "Fila de Bloqueados");
            std::cout << std::endl;
            gerenciadorMemoria->visualizarMemoria();
            std::cout << std::endl;
            //Limpa o Set de processos que rodaram nesse quantum
            processosAlocados.clear();

        } else {
            if (filaBloqueados.empty()) {
                std::cout << "Nenhum processo pronto e a fila de bloqueados está vazia. Aguardando execução dos processos..\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            } else {
                std::cout << "Nenhum processo pronto. Aguardando desbloqueio..\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
        }
    }

    std::cout << "Todos os processos foram finalizados.\n";
}

void Despachante::setGerenciadorMemoria(GerenciadorMemoria* gm) {
    gerenciadorMemoria = gm;
}

void Despachante::liberarCPU(int cpuIndex) {
    if (cpuIndex >= 0 && cpuIndex < cpusDisponiveis.size()) {
        cpusDisponiveis[cpuIndex] = false;
    }
}

void Despachante::desbloquear() {
    if (filaBloqueados.empty()) return;

    size_t bloqueadosCount = filaBloqueados.size();

    for (size_t i = 0; i < bloqueadosCount; ++i) {
        Processo* bloqueado = filaBloqueados.front();
        filaBloqueados.pop();

        if (!bloqueado) {
            std::cout << "Erro: Processo inválido encontrado na fila de bloqueados.\n";
            continue;
        }

        // Tentando alocar memória para o processo desbloqueado
        if (gerenciadorMemoria->alocarMemoria(bloqueado->getId(), bloqueado->getRam())) {
            bloqueado->alterarEstado(Estado::PRONTO);
            filaProntos.push(bloqueado);
            std::cout << "Processo #" << bloqueado->getId() << " desbloqueado e movido para fila de prontos.\n";
        } else {
            // Caso não consiga alocar, push de volta para filaBloqueados
            std::cout << "Memória insuficiente para desbloquear Processo #" << bloqueado->getId()
                      << ". Mantido na fila de bloqueados.\n";
            filaBloqueados.push(bloqueado);
        }
    }
}

std::queue<Processo*>& Despachante::getFilaProntos() {
    return filaProntos;
}

std::queue<Processo*>& Despachante::getFilaBloqueados() {
    return filaBloqueados;
}
