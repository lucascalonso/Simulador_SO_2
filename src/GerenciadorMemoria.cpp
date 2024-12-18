#include "../include/GerenciadorMemoria.h"
#include "../include/Despachante.h"
#include "../include/Processo.h"
#include <iostream>

GerenciadorMemoria::GerenciadorMemoria(int tamanhoTotal, Despachante* despachantePtr,int tamPaginas)
    : tamanhoTotal(tamanhoTotal), tamanhoLivre(tamanhoTotal), despachante(despachantePtr), tamPaginas(tamPaginas) 
    {
        numPaginas = tamanhoTotal / tamPaginas;
        memoria.reset();
        mapaProcessos.resize(numPaginas, 0);
        listaBlocosLivres = new Bloco(0,numPaginas,nullptr);
    }

//Aloca memória de acordo com o tamanho solicitado
bool GerenciadorMemoria::alocarMemoria(int processoId, int tamanhoBloco) {
    int paginasNecessarias = tamanhoBloco / tamPaginas;

    Bloco* blocoAtual = listaBlocosLivres;
    Bloco* blocoAnterior = nullptr;

    //Busca por um bloco livre com tamanho suficiente
    while (blocoAtual != nullptr && blocoAtual->tamanho < paginasNecessarias) {
        blocoAnterior = blocoAtual;
        blocoAtual = blocoAtual->prox;
    }

    //Se não há bloco livre suficiente
    if (blocoAtual == nullptr) {
        std::cout << "Não há blocos livres suficientes para alocar!" << std::endl;
        return false;
    }

    //Marca os quadros no bitmap como ocupados
    for (int i = blocoAtual->inicio; i < blocoAtual->inicio + paginasNecessarias; ++i) {
        memoria.set(i); //Marca o bit como ocupado
        mapaProcessos[i] = processoId; //Associa o quadro ao processo
    }

    //Atualiza o bloco livre
    if (blocoAtual->tamanho == paginasNecessarias) {
        //Remove o bloco da lista, pois foi totalmente utilizado
        if (blocoAnterior == nullptr) {
            listaBlocosLivres = blocoAtual->prox;
        } else {
            blocoAnterior->prox = blocoAtual->prox;
        }
        delete blocoAtual;
    } else {
        //Reduz o tamanho do bloco e ajusta o índice inicial
        blocoAtual->inicio += paginasNecessarias;
        blocoAtual->tamanho -= paginasNecessarias;
    }

    tamanhoLivre -= tamanhoBloco;

    std::cout << "Memória alocada: " << tamanhoBloco << " MB para o Processo #" << processoId << "\n";
    return true;
}


bool GerenciadorMemoria::liberaMemoria(Processo* processo) {
    int processoId = processo->getId();
    int tamanhoBloco = processo->getRam();
    int paginasLiberadas = tamanhoBloco / tamPaginas;

    std::cout << "Memória de " << tamanhoBloco << " MB liberada pelo Processo #" << processoId << "\n";

    //Identifica e libera os quadros
    int inicioBloco = -1;   //Marca o início de um bloco de quadros livres
    for (int i = 0; i < numPaginas; ++i) {
        if (mapaProcessos[i] == processoId) {
            mapaProcessos[i] = 0; // Remove a associação ao processo
            memoria.reset(i);     //Marca o quadro como livre

            if (inicioBloco == -1) {
                inicioBloco = i; // Marca o início do bloco livre
            }
        } else if (inicioBloco != -1) {
            // Finaliza o bloco livre quando encontra um quadro ocupado
            adicionarBlocoLivre(inicioBloco, i - inicioBloco);
            inicioBloco = -1;
        }
    }

    //Caso o último bloco esseja livre, adiciona
    if (inicioBloco != -1) {
        adicionarBlocoLivre(inicioBloco, numPaginas - inicioBloco);
    }

    tamanhoLivre += tamanhoBloco;

    return true;
}

// Método auxiliar p/ adicoina um novo bloco à lista de blocos livres
void GerenciadorMemoria::adicionarBlocoLivre(int inicio, int tamanho) {
    Bloco* novoBloco = new Bloco{inicio, tamanho, nullptr};

    //Insere o bloco na posição correta na lista (ordenado por inicio)
    if (listaBlocosLivres == nullptr || listaBlocosLivres->inicio > inicio) {
        novoBloco->prox = listaBlocosLivres;
        listaBlocosLivres = novoBloco;
    } else {
        Bloco* blocoAtual = listaBlocosLivres;
        while (blocoAtual->prox != nullptr && blocoAtual->prox->inicio < inicio) {
            blocoAtual = blocoAtual->prox;
        }
        novoBloco->prox = blocoAtual->prox;
        blocoAtual->prox = novoBloco;
    }

    //Mescla blocos adjacentes
    mesclarBlocos();
}

// Método auxiliar para mesclar blocos adjacnetes na lista
void GerenciadorMemoria::mesclarBlocos() {
    Bloco* blocoAtual = listaBlocosLivres;

    while (blocoAtual != nullptr && blocoAtual->prox != nullptr) {
        if (blocoAtual->inicio + blocoAtual->tamanho == blocoAtual->prox->inicio) {
            // Mescla os dois blocos
            blocoAtual->tamanho += blocoAtual->prox->tamanho;
            Bloco* blocoRemovido = blocoAtual->prox;
            blocoAtual->prox = blocoAtual->prox->prox;
            delete blocoRemovido;
        } else {
            blocoAtual = blocoAtual->prox;
        }
    }
}

void GerenciadorMemoria::visualizarMemoria() {
    std::cout << std::endl;
    
    const int paginasPorLinha = 128;  // Número de páginas por linha
    
    for (int i = 0; i < numPaginas; i++) {
        // Exibir status da página (IdProcesso ou L)
        std::cout << (mapaProcessos[i] == 0 ? "L" : std::to_string(mapaProcessos[i]));
        
        // Adicionar quebra de linha a cada paginasPorLinha páginas
        if ((i + 1) % paginasPorLinha == 0) {
            std::cout << std::endl;
        }
    }
    
    std::cout << std::endl;
}