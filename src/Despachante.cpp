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
    std::cout << std::endl;
}

void Despachante::escalonar() {
    
    //Caso não tenha nenhum processo em ambas as filas, não há processos para executar
    if (filaProntos.empty() && filaBloqueados.empty()) {
        std::cout << "Nenhum processo pronto e a fila de bloqueados está vazia. Aguardando chegada de processos..\n";
        return;
    }
    //Tenta desbloquear processos
    desbloquear();
    std::unordered_set<Processo*> processosAlocados;


    for (size_t i = 0; i < cpusDisponiveis.size(); ++i) {
            if (!cpusDisponiveis[i] && !filaProntos.empty()) {
                Processo* processoAtual = filaProntos.front();
                filaProntos.pop();

                //Tratar nullptr
                if (!processoAtual) {
                    std::cerr << "Erro: Processo nulo na fila de prontos.\n";
                    continue;
                }

                //Se o processo já está no set, já está alocado nesse quantum
                if (processosAlocados.find(processoAtual) != processosAlocados.end()) {
                    filaProntos.push(processoAtual);
                    continue;
                }

                //Marca processo como alocado nesse quantum
                processosAlocados.insert(processoAtual);

                //Marca CPU como ocupado
                cpusDisponiveis[i] = true;

                int tempoExecutado = std::min(quantum, processoAtual->getTempoRestante());
                processoAtual->executarCpu(tempoExecutado);
                std::cout << "Processo #" << processoAtual->getId() 
                          <<  " executado por " << tempoExecutado << " u.t" 
                          << ". Tempo restante: " << processoAtual->getTempoRestante() << "\n";

                //Tratar casos onde processo termina no meio de um quantum
                // Checa se processo terminou
                if (processoAtual->checarTermino()) {
                    gerenciadorMemoria->liberaMemoria(processoAtual);
                    std::cout << "Processo #" << processoAtual->getId() 
                              << " finalizado e memória liberada.\n";
                    liberarCPU(i);
                } else {
                    //Caso ainda reste tempo, move processo para fila de prontos
                    filaProntos.push(processoAtual);
                    liberarCPU(i);
                }
            }
    }


    //Visualização
    imprimirFila(filaProntos, "Fila de Prontos");
    imprimirFila(filaBloqueados, "Fila de Bloqueados");
    gerenciadorMemoria->visualizarMemoria();
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
