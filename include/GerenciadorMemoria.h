#ifndef GERENCIADOR_MEMORIA_H
#define GERENCIADOR_MEMORIA_H
#include <vector>
#include <utility>
class Despachante;
#include "../include/Processo.h"
#include "../include/Despachante.h"
#include "../include/globals.h"



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
        int getNumPaginas() const;
        int getMemoriaTotal() const;
        int getMemoriaDisponivel();
        std::string getCor(int idProcesso);
        void visualizarMemoria();
        bool alocarMemoria(int processoId, int tamanho);
        void liberaMemoria(Processo* processo);

};
#endif
