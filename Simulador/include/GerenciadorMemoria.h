#ifndef GERENCIADOR_MEMORIA_H
#define GERENCIADOR_MEMORIA_H
#include <vector>
#include <utility>
#include <sstream>
#include "../include/Despachante.h"
#include <wx/wx.h>
#include <map>
class Despachante;
#include "../include/Processo.h"
#include "../include/globals.h"


class GerenciadorMemoria{
    private:
        int memoria[8192];
        int tamanhoTotal;
        int tamanhoLivre;
        int tamPaginas;
        int numPaginas;
        int quadrosLivres;           
        Despachante *despachante;
        std::set<Processo*> processosADeletar;

    public:
        GerenciadorMemoria(int tamanhoTotal, Despachante* despachantePtr,int tamPaginas);
        int getNumPaginas() const;
        int getMemoriaTotal() const;
        int getMemoriaDisponivel();
        int* getMemoria();
        bool alocarMemoria(int processoId, int tamanho);
        void liberaMemoria(Processo* processo,std::set<Processo*,ProcessoComparator>& processosAtuais);
        Despachante* getDespachante() {return despachante;};
        void deletarProcessos();
        std::set<Processo*> getProcessosADeletar();
        Processo *recuperarProcessoPorId(int id);
};
#endif
