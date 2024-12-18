#ifndef GERENCIADOR_MEMORIA_H
#define GERENCIADOR_MEMORIA_H
#include <vector>
#include <utility>
#include <bitset>
class Despachante;
#include "../include/Processo.h"
#include "../include/Despachante.h"

//Estrutura que organoza os blocos de memória livres. Cada bloco representa um conjunto
//contínuo de índices (início até fim) disponíveis para alocação.
//Aloca mais eficientemente pois otiza a bsca por um bloco suficientemente grande pra alocar 
//o processo e facilita liberação, pois facilita o agrupamento de quadros adjacentes.
typedef struct Bloco {
    int inicio;
    int tamanho;
    Bloco* prox;

    Bloco(int inicio, int tamanho, Bloco* prox = nullptr)
        : inicio(inicio), tamanho(tamanho), prox(prox) {}
} Bloco;


/*bitmap memoria:       Representa quadros da MP livres (0) ou ocupados (1). Otimiza busca por página livre e facilita marcação
                        das páginas (set e reset).
*/

/*vetor mapaProcessos:  Armazena Id do processo associado a cada quadro da MP. Facilita acesso a qual processo pertence a uma
                        determinada página, ou seja, uma relação direta entre os processos e suas alocações.
                        Quando liberamos um processo, o mapa é usado para encontrar todos os quadros pertencentes ao processo.
*/

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