#ifndef GERENCIADOR_MEMORIA_H
#define GERENCIADOR_MEMORIA_H
#include <vector>
#include <utility>
#include <bitset>
class Despachante;
#include "../include/Processo.h"
#include "../include/Despachante.h"

typedef struct Bloco {
    int inicio;
    int tamanho;
    Bloco* prox;

    Bloco(int inicio, int tamanho, Bloco* prox = nullptr)
        : inicio(inicio), tamanho(tamanho), prox(prox) {}
} Bloco;


class GerenciadorMemoria{
    private:
        int tamanhoTotal;
        int tamanhoLivre;
        int tamPaginas;
        int numPaginas;
        Bloco *listaBlocosLivres;
        std::bitset<8192> memoria;
        std::vector<int> mapaProcessos;
        Despachante *despachante;

    public:
        GerenciadorMemoria(int tamanhoTotal, Despachante* despachantePtr,int tamPaginas);
        int getNumPaginas();
        void visualizarMemoria();
        int getTamPaginas();
        void mesclarBlocos();
        void adicionarBlocoLivre(int inicio, int tamanho); 
        bool alocarMemoria(int processoId, int tamanho);
        bool liberaMemoria(Processo* processo);
};
#endif