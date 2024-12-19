#ifndef GERENCIADOR_MEMORIA_H
#define GERENCIADOR_MEMORIA_H
#include <vector>
#include <utility>
class Despachante;
#include "../include/Processo.h"
#include "../include/Despachante.h"


class GerenciadorMemoria{
    private:
        int memoria[8192];
        int tamanhoTotal;
        int tamanhoLivre;
        int tamPaginas;
        int numPaginas;                     
        Despachante *despachante;

    public:
        GerenciadorMemoria(int tamanhoTotal, Despachante* despachantePtr,int tamPaginas);
        int getNumPaginas();
        void visualizarMemoria();
        bool alocarMemoria(int processoId, int tamanho);
        bool liberaMemoria(Processo* processo);

};
#endif