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
    int paginasNecessarias = tamanhoBloco / tamPaginas;
    //Se há memória disponível, percorre array memoria buscando quadros livres
    if(tamanhoBloco <= tamanhoLivre){   
        for(int i = 0; i < 8192 && paginasNecessarias >0;i++){
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
    int paginasLiberadas = tamanhoBloco / tamPaginas;

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
        //delete processo;
    }
}

void GerenciadorMemoria::deletarProcessos() {
    for (auto it = processosADeletar.begin(); it != processosADeletar.end(); ) {
        delete *it;
        it = processosADeletar.erase(it);
    }
}

//Filtro para colorir MP (Cinza p/ memória livre)
std::string GerenciadorMemoria::getCor(int idProcesso) {
    if (idProcesso == 0) return "\033[90m";
    
    int cor = 31 + (idProcesso % 6);
    return "\033[" + std::to_string(cor) + "m";
}

// Retorna um vetor de pares (ID, cor)
std::vector<std::pair<int, wxColour>> GerenciadorMemoria::visualizarMemoriaComCores() {
    std::vector<std::pair<int, wxColour>> memoriaColorida;

    // Mapeia cada ID de processo para uma cor fixa
    std::map<int, wxColour> mapaDeCores;
    mapaDeCores[0] = wxColour(128, 128, 128); // Cinza para memória livre

    for (int i = 0; i < numPaginas; i++) {
        int idProcesso = memoria[i];

        // Se o ID ainda não tiver uma cor atribuída, gere uma nova cor
        if (mapaDeCores.find(idProcesso) == mapaDeCores.end()) {
            wxColour novaCor(rand() % 256, rand() % 256, rand() % 256); // Cor aleatória
            mapaDeCores[idProcesso] = novaCor;
        }

        // Adiciona o ID e sua cor ao vetor
        memoriaColorida.emplace_back(idProcesso, mapaDeCores[idProcesso]);
    }

    return memoriaColorida;
}

int GerenciadorMemoria::getNumPaginas() const {return numPaginas;}
int* GerenciadorMemoria::getMemoria() {return memoria;}