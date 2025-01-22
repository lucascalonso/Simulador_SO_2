#include "../include/Despachante.h"
#include "../include/GerenciadorMemoria.h"
#include "../include/GeradorDeProcessos.h"
#include "../include/globals.h"
#include "../include/Processo.h"
#include <iostream>

GerenciadorMemoria::GerenciadorMemoria(int tamanhoTotal, Despachante* despachantePtr,int tamPaginas)
    : tamanhoTotal(tamanhoTotal), tamanhoLivre(tamanhoTotal), despachante(despachantePtr), tamPaginas(tamPaginas) 
    {
        //8192 quadros na MP
        numPaginas = tamanhoTotal / tamPaginas;       
        
        //Inicializar quadros MP como 0
        for (int i = 0; i < numPaginas; ++i) {
            memoria[i] = 0;
        }
    }


int GerenciadorMemoria::getMemoriaTotal() const { return tamanhoTotal; }

int GerenciadorMemoria::getMemoriaDisponivel() { return tamanhoLivre; }

//Aloca memória de acordo com o tamanho solicitado
bool GerenciadorMemoria::alocarMemoria(int processoId, int tamanhoBloco) {
    int paginasNecessarias = (tamanhoBloco + tamPaginas - 1) / tamPaginas;

    //Se há memória disponível, percorre array memoria buscando quadros livres
    if(tamanhoBloco < tamanhoLivre){   
        for(int i = 0; i < 8192 && paginasNecessarias > 0;i++){
            if(memoria[i]==0){
                memoria[i] = processoId;
                paginasNecessarias--;
            }
        }
    }
    //Se não há bloco livre suficiente
    else{
        return false;
    }
    tamanhoLivre -= tamanhoBloco;
    std::cout << "Memória alocada: " << tamanhoBloco << " MB para o Processo #" << processoId << "\n";
    return true;
}

//Trata processos suspensos e terminados
void GerenciadorMemoria::liberaMemoria(Processo* processo, std::set<Processo*,ProcessoComparator>& processosAtuais) {
    int processoId = processo->getId();
    int tamanhoBloco = processo->getRam();
    int paginasLiberadas = (tamanhoBloco + tamPaginas - 1) / tamPaginas;

    for (int i = 0; i < 8192 && paginasLiberadas > 0; ++i) {
        if (memoria[i] == processoId) {
            memoria[i] = 0;
            --paginasLiberadas;
        }
    }
    tamanhoLivre += tamanhoBloco;

    //Se processo terminou, precisa limpar do vetor de processosAtuais 
    if (processo->getEstadoString() == "TERMINADO") {
        
        //Encontrar o processo no vetor
        auto it = std::find(processosAtuais.begin(), processosAtuais.end(), processo);
        if (it != processosAtuais.end()) {
            processosAtuais.erase(it);
        }
        processosADeletar.insert(processo);
    }
}

void GerenciadorMemoria::deletarProcessos() {
    for (auto it = processosADeletar.begin(); it != processosADeletar.end(); ) {
        delete *it;
        it = processosADeletar.erase(it);
    }
}

Processo* GerenciadorMemoria::recuperarProcessoPorId(int id) {
    for (Processo* processo : despachante->getProcessosAtuais()) { 
        if (processo->getId() == id) {
            return processo;
        }
    }
    return nullptr;
}

int GerenciadorMemoria::getNumPaginas() const {return numPaginas;}
int* GerenciadorMemoria::getMemoria() {return memoria;}
std::set<Processo*> GerenciadorMemoria::getProcessosADeletar() {return processosADeletar;}