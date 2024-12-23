#include "../include/GerenciadorMemoria.h"
#include "../include/Despachante.h"
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

void GerenciadorMemoria::liberaMemoria(Processo* processo) {
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
}


void GerenciadorMemoria::visualizarMemoria() {
    std::cout << std::endl;
    
    const int paginasPorLinha = 128;  // Número de páginas por linha
    
    for (int i = 0; i < numPaginas; i++) {
        // Exibir status da página (IdProcesso ou L)
        std::cout << (memoria[i] == 0 ? "0" : std::to_string(memoria[i]));
        
        // Adicionar quebra de linha a cada paginasPorLinha páginas
        if ((i + 1) % paginasPorLinha == 0) {
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}